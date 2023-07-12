#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"

char*
fmtname(char *path)  //从路径中提取文件名
{
  static char buf[DIRSIZ+1];  //DIRSIZE=14 defined in fs.h
  char *p;

  // Find first character after last slash.  
  for(p=path+strlen(path); p >= path && *p != '/'; p--) //从路径字符串末尾开始向前找第一个‘/’
    ;
  p++;

  // Return blank-padded name.
  if(strlen(p) >= DIRSIZ)  //文件名过长，不处理
    return p;
  memmove(buf, p, strlen(p));
  memset(buf+strlen(p), ' ', DIRSIZ-strlen(p));
  return buf;  //提取到的文件名
}

void
ls(char *path)
{
  char buf[512], *p;
  int fd; //文件描述符
  struct dirent de;  //保存目录项信息
  struct stat st;  //保存文件信息

  if((fd = open(path, 0)) < 0){  //使用open：是否成功打开文件
    fprintf(2, "ls: cannot open %s\n", path);
    return;
  }

  if(fstat(fd, &st) < 0){ //fstat()：获取文件统计信息，保存在st中；获取失败
    fprintf(2, "ls: cannot stat %s\n", path);
    close(fd);
    return;
  }

  switch(st.type){ //获取成功，查看文件类型
  case T_FILE:  //普通文件，打印文件信息
    printf("%s %d %d %l\n", fmtname(path), st.type, st.ino, st.size);
    break;

  case T_DIR:  //目录，遍历所有文件
    if(strlen(path) + 1 + DIRSIZ + 1 > sizeof buf){  //路径名过长
      printf("ls: path too long\n");
      break;
    }
    strcpy(buf, path); //路径复制到buf中
    p = buf+strlen(buf);
    *p++ = '/';  //末尾添加反斜杠'/'
    while(read(fd, &de, sizeof(de)) == sizeof(de)){  //从文件fd中读目录项信息
      if(de.inum == 0) //目录项无效
        continue;
      memmove(p, de.name, DIRSIZ);
      p[DIRSIZ] = 0;
      if(stat(buf, &st) < 0){
        printf("ls: cannot stat %s\n", buf);
        continue;
      }
      printf("%s %d %d %d\n", fmtname(buf), st.type, st.ino, st.size);
    }
    break;
  }
  close(fd);
}

int
main(int argc, char *argv[])
{
  int i;

  if(argc < 2){  //显示所有文件
    ls(".");
    exit(0);
  }
  for(i=1; i<argc; i++)
    ls(argv[i]);
  exit(0);
}
