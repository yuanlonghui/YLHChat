#define _GNU_SOURCE
#include <sched.h>
#include<stdio.h>  
#include<stdlib.h>  
#include<string.h>  
#include<errno.h>  
#include<sys/types.h>  
#include<sys/socket.h>  
#include<netinet/in.h> 
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <unistd.h>
#include <signal.h>
#include "datastruct.h"
#include "service.h"

 
int main(int argc, char** argv)  
{  
	
	//初始化bitmap，服务器开始时只有一个注册用户0000，超级用户，不需要初始化文件，初始化密码为 root
	if (argc != 1 && strcmp(argv[1], "new") == 0)
	{
		//如果是不需要新的服务器，那么就会尝试读取已经存在了的信息
		//如果需要一个新的服务器，那么会将两个bitmap文件初始化掉
		retState s = initialBitmap();
		if (s == ERROR)
		{
			exit(0);
		}
		s = initialGroupBitmap();
		if (s == ERROR)
		{
			exit(0);
		}
	}
	//先创建服务器进程的通信信号量和共享内存区域
	int sem_id ;
	//可以调用一个自定义的函数，这里显式的创建是为了更好的展现
	//一个信号量，访问主存区域
	sem_id = semget(SEVER_SEM_KEY, 1, 0666|IPC_CREAT) ;
	
	//初始化信号量和共享主存区域
	union semun sem_val ;
	sem_val.val = 1 ;
	semctl(sem_id, 0, SETVAL, sem_val) ;
	serverShm * addr = NULL ;
	addr = &servershm ;
	memset(addr, 0, sizeof(serverShm)) ;
	
	initial(addr) ;
	addr = NULL;
	
	int socket_listen, socket_serve ;
	struct sockaddr_in servaddr ;
	//初始化socket
	if((socket_listen = socket(AF_INET, SOCK_STREAM, 0)) == -1)
	{
		printf("creat listening socket for server error\n") ;
		exit(-1) ; 
	}
	//设置ip地址
	memset(&servaddr, 0, sizeof(servaddr)) ;
	servaddr.sin_family = AF_INET ;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY) ;
	servaddr.sin_port = htons(DEFAULT_PORT) ;
	//绑定
	if(bind(socket_listen, (struct sockaddr *) &servaddr, sizeof(servaddr)) == -1)
	{
		printf("bind listening socket error!\n") ;
		exit(-1) ; 
	}
    //监听
    if(listen(socket_listen, 10) == -1)
    {
    	printf("listening error!\n") ;
    	exit(-1) ; 
    }
    
    printf("======waiting for client's request======\n");  

	//创建线程检查用户在线情况
	void* stack = malloc(threadStackSize);
	clone(&checkalive, (char*)stack + threadStackSize, CLONE_VM, NULL);

    while(1)
    {
		//如果有新的连接
    	if((socket_serve = accept(socket_listen, NULL, NULL)) == -1)
    	{
    		printf("accept error\n") ;
    		continue ;
    	}
		//创建一个子线程去服务连接
		stack = malloc(threadStackSize);
		clone(&serve, (char*)stack + threadStackSize, CLONE_VM, &socket_serve);
		close(socket_serve) ;
    }
    close(socket_listen) ;
	semctl(sem_id, 0, IPC_RMID);

}  
