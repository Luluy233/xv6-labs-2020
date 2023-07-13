#include "kernel/types.h"
#include "user/user.h"
#include "kernel/param.h"

int main(int argc, char *argv[]) {
  char buffer[512];//标准输入缓冲区（1次从标准输入读取5）
  char param[MAXARG][MAXARG];//存储argv命令行参数 + 从标准输入读取的参数
  char *pass[MAXARG];//传递给exec的参数，z指向param的指针数组

  //pass指针指向param
  for (int i = 0; i < 32; i++)
    pass[i] = param[i];

  //param初始化：0~argc-2存储命令行参数argv[1]~argv[argc-1]
  for (int i = 1; i < argc; i++)
    strcpy(param[i - 1], argv[i]);

  // n：每次从标准输入读取512个字节到缓冲区的返回值
  int n;
  //从标准输入读取内容到缓冲区buffer
  while ((n = read(0, buffer, sizeof(buffer))) > 0) {
    // 数组param的偏移量（下标）
    int pos = argc - 1;
    //c：指向param[pos]参数的当前字节
    char *c = param[pos];
    for (char *p = buffer; *p; p++) {
      if (*p == ' ' || *p == '\n') { //读取完1个参数
        *c = '\0';  //标记参数结束，eg：param
        pos++;  //总参数量+1
        c = param[pos];  //指向下一个参数
      } else  //读完1个字符，向下移动
        *c++ = *p;
    }
    //读完当前缓冲区
    *c = '\0';
    pos++;
    pass[pos] = 0;

    if (fork()==0) {
      exec(pass[0], pass);
    } else{
      wait(0);
    }
  }
  // 读出错
  if (n < 0) {
    printf("xargs: read error\n");
    exit(1);
  }

  exit(0);
}