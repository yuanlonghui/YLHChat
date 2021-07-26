#include<stdio.h>  
#include<stdlib.h>  
#include<string.h>   
#include<sys/types.h>  
#include<sys/socket.h> 
#include<netinet/in.h>  
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <unistd.h>
#include <time.h>
#include "datastruct.h"
#include "commandline.h"

extern in_addr_t inet_addr(const char* cp);

int main(int argc, char** argv)  
{  
	srand((unsigned int)time(NULL));
	client_sem_key += rand() % 100;
	//先创建客户进程的通信信号量和共享内存区域
	int sem_id ;
	//可以调用一个自定义的函数，这里显式的创建是为了更好的展现
	//共享主存区域
	sem_id = semget(client_sem_key, 1, 0666|IPC_CREAT) ;
	
	//初始化信号量和共享主存区域
	union semun sem_val ;
	sem_val.val = 1 ;
	semctl(sem_id, 0, SETVAL, sem_val) ;
	clientShm * addr = NULL ;
	addr = &clientshm ;
	memset(addr, 0, sizeof(clientShm)) ;
	addr = NULL;

	initClientConfig();

    printf("======welcom to YLHChar!======\n") ;
    commandline() ;
	semctl(sem_id, 0, IPC_RMID);
    exit(0);  
}  
