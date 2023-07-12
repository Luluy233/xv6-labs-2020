#include "kernel/types.h"
#include "user.h"
/*
编写一个使用UNIX系统调用的程序来在两个进程之间“ping-pong”一个字节，请使用两个管道，每个方向一个。
父进程应该向子进程发送一个字节;
子进程应该打印“<pid>: received ping”，其中<pid>是进程ID，
并在管道中写入字节发送给父进程，然后退出;
父级应该从读取从子进程而来的字节，打印“<pid>: received pong”，然后退出。
*/

int main(int argc, char const * argv[])
{
    int p_c2f[2];//子进程写->父进程读(子进程是写端)
    int p_f2c[2];//父进程写->子进程读（父进程是写端）
    pipe(p_c2f);
    pipe(p_f2c);

    char buf='p';//要传递的数据

    //创建子进程
    int pid=fork();
    int exit_status=0;//状态

    //错误
    if(pid<0){
        //释放所有文件描述符
        close(p_f2c[0]);
        close(p_f2c[1]);
        close(p_c2f[0]);
        close(p_c2f[1]);
        fprintf(2,"error: fork() error!");
        exit(1);
    }
    //子进程：读f2c，写c2f
    else if(pid==0){
        close(p_c2f[0]); //关闭：读c2f
        close(p_f2c[1]); //关闭：写f2c
        //读错误
        if(read(p_f2c[0],&buf,sizeof(char))!=sizeof(char)){
            fprintf(2,"child read error\n");
            exit_status=1;
        }
        else{
            fprintf(1,"%d: received ping\n",getpid());

        }
        //写错误
        if(write(p_c2f[1],&buf,sizeof(char))!=sizeof(char)){
            fprintf(2,"child write error\n");
            exit_status=1;
        }

        close(p_f2c[0]);
        close(p_c2f[1]);
        exit(exit_status);

    }
    //父进程：写f2c
    else{
        close(p_f2c[0]);
        close(p_c2f[1]);

        if(write(p_f2c[1],&buf,sizeof(char))!=sizeof(char)){
            fprintf(2,"father write error\n");
            exit_status=1;
        }

        if(read(p_c2f[0],&buf,sizeof(char))!=sizeof(char)){
            fprintf(2,"father read error\n");
            exit_status=1;
        }
        else{
            fprintf(1,"%d: received pong\n",getpid());
        }

        close(p_c2f[0]); //关闭：读c2f
        close(p_f2c[1]); //关闭：写f2c
        exit(exit_status);

    }

}