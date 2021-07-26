#ifndef _DS_H_
#define _DS_H_ 1

#include<stdio.h>  
#include<stdlib.h>  
#include<string.h>  
#include<sys/types.h>  
#include<sys/socket.h>  
#include<netinet/in.h>

#include "msgQueue.h"

//定义了服务器，客户端会使用到的数据结构和一些方法，具体实现不做注释
//只对功能进行描述

#define IDLEN 4 + 1
#define NAMELEN 20 + 1
#define PWLEN 6 + 1

#define MAX_USER 10000
#define MAX_GROUP 10000
#define SEVER_SHM_KEY 100
#define SEVER_SEM_KEY 200
#define CLIENT_SHM_KEY 300
#define CLIENT_SEM_KEY 400
#define DEFAULT_PORT 8888  
#define MAXLINE 4096  

extern const char * userinfo ;
extern const char* tempfile ;
extern const char* history;
extern const char* download;
extern const char* groupinfo;

typedef enum {OK, ERROR} retState ;

//获得src的从下标begin开始，到end结束的左闭右开字串，保存到dst中
char* substring(char* dst, const char* src, int begin, int end);
//从msg开始处开始，找到第一个空格或者'\0'的偏移量
int parseMsgType(const char* msg);

//用户结构体，看名字识别属性意义
typedef struct {
	char id[IDLEN] ;
	char name[NAMELEN] ;
	char password[PWLEN] ;
	int group_num ;
	char ** gids ;
	int friends_num ;
	char ** friends ;
	struct sockaddr_in addr ;
	queue* msgQ;
	queue* fileQ;
	int alive ;
} user ;

//从文件中获取已经赋予id的用户信息
retState getUser(user* u) ;
//保存用户信息到文件
retState storeUser(user *u) ;
//释放用户的动态内存
retState freeuser(user *u) ;

//群聊结构体
typedef struct {
	char onwerid[IDLEN];
	char id[IDLEN];
	char name[NAMELEN];
	int user_num;
	char ** users;
} group ;
//为了使同一个机器上的不同客户端的信号量不同
extern int client_sem_key;
//两个文件夹路劲
extern const char * usermsg ;
extern const char * userfile ;

//服务器临界区
typedef struct {
	user users[MAX_USER] ;
	group groups[MAX_GROUP] ;
	int user_bitmap[MAX_USER] ;
	int group_bitmap[MAX_GROUP] ;
	int keepalive[MAX_USER] ;
	int info_change[MAX_USER];
} serverShm ;

//服务器初始化函数
//主要包括从文件中读出用户和群组的信息
retState initial(serverShm * shmptr) ;

//客户端临界区
typedef struct {
	user selfinfo ;
	int legal ;
	int exit ;
} clientShm ;

//两个全局变量
extern serverShm servershm;
extern clientShm clientshm;

//x信号量服务联合类型
union semun
{  //用于信号量的赋值
   int val;
   //用于返回信号量集信息
   struct semid_ds *buf;
   //用于设置或者获取信号量集成员的取值
   unsigned short *array;
} ;
//申请信号量
void P(int sem_id, int sem_num) ;
//释放信号量
void V(int sem_id, int sem_num) ;

//两个路径
extern const char * userbitmap;
extern const char* groupbitmap;
//从文件中读出用户id使用情况映射
retState getUserInforBitmap(int * bitmap) ;
//将整个用户id使用情况映射文件内容都变为零
retState initialBitmap() ;
//从文件中读出群聊id使用情况映射
retState getGroupInforBitmap(int* bitmap);
//将整个群聊id使用情况映射文件内容都变为零
retState initialGroupBitmap();
//保存id使用情况映射到文件中
retState storeUserInfoBitmap(int* bitmap);
retState storeGroupInfoBitmap(int* bitmap);
//得到一个新的有用户id
int getUserNewId(int * bitmap) ;
//初始化超级用户
retState initialRootUser(user * u) ;
//或者服务器，客户端信号量
void getServerSem(int * sem_id) ;
void getClientSem(int * sem_id) ;
//从字符串中解析用户信息
retState parseUserFromStr(user* u, char* info);
//将用户信息写入字符串
retState storeUserToStr(user* u, char* info);
//服务器地址
extern struct sockaddr_in servaddr;
//初始化客户端设置，就是从文件中读出服务器地址
void initClientConfig();
//建立连接，返回状态，socket保存套接字
retState getConnect(int* socket);
//从字符串中解析群聊信息，没有使用到
retState parseGroupFromStr(group* g, char* info);
//保存群聊信息到字符串，没有使用到
retState storeGroupToStr(group* g, char* info);
//从文件中读出已经赋予id的群聊信息
retState getGroup(group* g);
//保存群聊信息到文件中
retState storeGroup(group* g);
//释放群聊动态内存
retState freeGroup(group* g);
//分配新的群聊id
int getGroupNewId(int * bitmap) ;

#define threadStackSize 8 * 1024 * 1024
//将用户id添加到群聊中
retState addUser(group* g, char* uid);
//将群聊id添加到用户信息中
retState addIntoGroup(user* u, char * gid);

#endif