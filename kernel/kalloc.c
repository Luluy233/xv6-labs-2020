// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.
// 物理内存分配器，分配给用户进程、内核堆栈、页表页面和管道缓冲区
// 分配这个4096字节的内存

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"

void freerange(void *pa_start, void *pa_end);

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.

struct run {  //空闲内存块节点
  struct run *next;  //指向下一个空闲内存块
};

struct {
  struct spinlock lock;  //自旋锁
  struct run *freelist;  //指向空闲内存块链表的指针
} kmem; //内核空间空闲块

void
kinit() //内核空间初始化
{
  initlock(&kmem.lock, "kmem"); //初始化锁
  freerange(end, (void*)PHYSTOP); //释放end到PHYSTOP的内存空间
}

void
freerange(void *pa_start, void *pa_end)  //释放从pa_start到pa_end的内核空间
{
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE) //每次释放PGSIZE=4096byte的内存空间
    kfree(p);
}

// 获取内存空间空闲字节数
// 内存空间使用链表组织，因此便利链表即可
void
freebytes(uint64 *dst)
{
  *dst = 0;
  struct run *p = kmem.freelist; // 用于遍历

  acquire(&kmem.lock);
  while (p) {
    *dst += PGSIZE; //注意：是字节数！！
    p = p->next;
  }
  release(&kmem.lock);
}



// Free the page of physical memory pointed at by v,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(void *pa)  //释放指针pa指向的物理内存页表
{
  struct run *r;

  // 判断地址pa是否合法
  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");

  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE); //将页面填充特定值1

  r = (struct run*)pa; //pa转为空闲内存指针

  acquire(&kmem.lock);
  r->next = kmem.freelist; //将pa（刚被释放的物理内存页面）加入空闲内存链表头部
  kmem.freelist = r;
  release(&kmem.lock);
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
// 分配物理内存，返回物理内存指针，若返回值wield0说明内存不可被分配
void *
kalloc(void)
{
  struct run *r;

  acquire(&kmem.lock);
  r = kmem.freelist; //去除空闲内存链表的第一个块
  if(r)
    kmem.freelist = r->next; 
  release(&kmem.lock);

  if(r)
    memset((char*)r, 5, PGSIZE); // fill with junk
  return (void*)r;
}
