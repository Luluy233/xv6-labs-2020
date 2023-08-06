// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"

//lab6
//cow物理页面引用计数
struct ref_stru{
  struct spinlock lock;//自旋锁
  int cnt[PHYSTOP/PGSIZE];//引用计数
}ref;

void freerange(void *pa_start, void *pa_end);

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.

struct run {
  struct run *next;
};

struct {
  struct spinlock lock;
  struct run *freelist;
} kmem;

void
kinit()
{
  initlock(&kmem.lock, "kmem");
  initlock(&ref.lock, "ref");//lab6：初始化自旋锁
  freerange(end, (void*)PHYSTOP);
}

void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE){
    // 在kfree中将会对cnt[]减1，这里要先设为1，否则就会减成负数
    ref.cnt[(uint64)p/PGSIZE]=1;
    kfree(p);
  }
    
}

// Free the page of physical memory pointed at by v,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(void *pa)
{
  struct run *r;

  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");
  
  acquire(&ref.lock);
  //引用计数=0释放内存
  if(--ref.cnt[(uint64)pa/PGSIZE]==0){
    release(&ref.lock);

    r = (struct run*)pa;
    // Fill with junk to catch dangling refs.
    memset(pa, 1, PGSIZE);


    acquire(&kmem.lock);
    r->next = kmem.freelist;
    kmem.freelist = r;
    release(&kmem.lock);
  }
  else{
    release(&ref.lock);
  }
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  struct run *r;

  acquire(&kmem.lock);
  r = kmem.freelist;
  if(r){
    kmem.freelist = r->next;
    //lab6：初始化内存引用计数=1
    acquire(&ref.lock);
    ref.cnt[(uint64)r/PGSIZE]=1;
    release(&ref.lock);
  }
    
  release(&kmem.lock);

  if(r)
    memset((char*)r, 5, PGSIZE); // fill with junk
  return (void*)r;
}


//lab6
/**
 * @brief cowpage 判断一个页面是否为COW页面
 * @param pagetable 指定查询的页表
 * @param va 虚拟地址
 * @return 0 是 -1 不是
 */
int cowpage(pagetable_t pagetable,uint64 va)
{
  if(va>=MAXVA)//虚拟地址不合法
    return -1;
  pte_t *pte=walk(pagetable,va,0);//找到虚拟地址va对应的页表项pte
  if(pte==0)//pte不存在
    return -1;
  if((*pte&PTE_V)==0)//pte存在但无效
    return -1;
  if((*pte&PTE_F)==0)//pte存在、有效且是cow页面
    return -1;
  else
    return 0;
}

/**
 * @brief cowalloc copy-on-write分配器
 * @param pagetable 指定页表
 * @param va 指定的虚拟地址,必须页面对齐
 * @return 分配后va对应的物理地址，如果返回0则分配失败
 */
void* cowalloc(pagetable_t pagetable, uint64 va)
{
  if(va % PGSIZE !=0)//没有页对齐
    return 0;

  //寻找pagetable中va对应的物理地址
  uint64 pa=walkaddr(pagetable,va);
  if(pa==0)//找不到
    return 0;
  
  //寻找pagetable中va对应的页表项
  pte_t *pte=walk(pagetable,va,0);

  if(krefcnt((char*)pa)==1){
    //只剩一个进程对此物理地址引用，直接修改pte
    *pte = *pte | PTE_W;
    *pte = *pte & ~PTE_F;
    return (void*)pa;
  }
  else{
    //多个进程对物理内存存在引用
    //需要分配新的页面，并拷贝旧页面的内容
    char *mem=kalloc();
    if(mem==0) //没有物理内存
      return 0;

    //复制旧页面内容到新页
    memmove(mem,(char*)pa,PGSIZE);

    //清除PTE_w，否则在mappagges会判定为remap
    *pte = *pte & ~PTE_V;

    //为新页面添加映射
    if(mappages(pagetable,va,PGSIZE,(uint64)mem,(PTE_FLAGS(*pte) | PTE_W) & ~PTE_F) !=0){
      kfree(mem);//释放物理内存
      *pte = * pte | PTE_V;
      return 0;
    }

    //原来的物理内存引用计数-1
    kfree((char*)PGROUNDDOWN(pa));
    return mem;
  }
}

/**
 * @brief krefcnt 获取内存的引用计数
 * @param pa 指定的内存地址
 * @return 引用计数
 */
int krefcnt(void* pa)
{
  return ref.cnt[(uint64)pa/PGSIZE];
}

/**
 * @brief kaddrefcnt 增加内存的引用计数
 * @param pa 指定的内存地址
 * @return 0:成功 -1:失败
 */
int kaddrefcnt(void* pa)
{
  //页不对齐
  if(((uint64)pa%PGSIZE)!=0)
    return -1;
  //物理地址超过范围
  if((char *)pa < end || (uint64)pa >= PHYSTOP)
    return -1;
  
  acquire(&ref.lock);
  ref.cnt[(uint64)pa/PGSIZE]++;
  release(&ref.lock);
  return 0;
}
