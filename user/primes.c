#include "kernel/types.h"
#include "user.h"

#define INT_LEN 4
#define WR 1
#define RD 0

/*
您的目标是使用pipe和fork来设置管道。第一个进程将数字2到35输入管道。
对于每个素数，您将安排创建一个进程，
该进程通过一个管道从其左邻居读取数据，并通过另一个管道向其右邻居写入数据。
由于xv6的文件描述符和进程数量有限，因此第一个进程可以在35处停止。
*/

/*
考虑所有小于1000的素数的生成。Eratosthenes的筛选法可以通过执行以下伪代码的进程管线来模拟：
p = get a number from left neighbor
print p
loop:
    n = get a number from left neighbor
    if (p does not divide n)
        send n to right neighbor
*/
// 当管道write端关闭时，read返回0
// 父进程：子read，父write

// prime可视为1个进程，与调用它的函数（父进程）共享left管道，prime中read（left[0])
// 与其调用的函数（孙子进程）共享right管道，prime中write(right[1])
// 管道方向：父->子->孙（单向传递）
void prime(int left[2])
{
    close(left[WR]);

    // 读取左管道第1个数字（素数）
    int p;
    if(read(left[RD],&p,INT_LEN)==INT_LEN){
        fprintf(1,"prime %d\n",p);
    }
    else{ //都读完了
        close(left[RD]);
        exit(0);
    }

    // 右管道
    int right[2];
    pipe(right);
   
    if(fork()==0){  // 孙子进程
        prime(right);
    }
    else{   //在当前进程（子进程）
        // 子进程不用读，只要写
        close(right[RD]);

        // 逐个读取左管道的数字
        int n;
        while(1){
            if(read(left[RD],&n,INT_LEN)==INT_LEN){
                //若n不能被p整除，则写入右管道
                if(n%p){
                    write(right[WR],&n,INT_LEN);   
                }
            }
            else{ //读完了
                close(right[WR]);
                close(left[RD]);
                break;
            }
        }
        wait(0);//等所有子进程结束
    }
    exit(0);
}

int main(int argc, char const *argv[])
{
    int p[2];
    pipe(p);

    //将初始数据2～35写入管道
    for(int i = 2; i <=35; i++)
        write(p[1],&i,INT_LEN);

    //为当前素数resutl创建1个进程
    if(fork()==0){
        prime(p);
    }
    else{
        close(p[0]);
        close(p[1]);
        wait(0);
    }
    exit(0);
}