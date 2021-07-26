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
#include <wait.h>


#include <signal.h>


#include "datastruct.h" 
#include "commandline.h"
#include "request.h"


//是否停止
int ifexit=0;  

int paserCommandType(char *command)
{
	int i ;
	for(i = 0 ; i < MAXLINE ; i++)
	{
		if (command[i] == ' ' || command[i] == '\0')
		{
			return i ;
		}
	}
}

void processOneCommand()
{

    //输出提示符
    //
	//	printf("processOneCommand\n");
    char name[NAMELEN] ;
    char id[IDLEN] ;
	int legal;
	int msgleft = 0;
	int fileleft = 0;
    sprintf(name, "NOLOGIN") ;
    sprintf(id, "NULL") ;
	//当前命令行内容同用户信息有关，需要进入临界区
	//P信号量
    int sem_id ;
	getClientSem(&sem_id) ;
    P(sem_id, 0) ;
    clientShm * addr ;
    addr = &clientshm ;
	legal = addr->legal;
    if(legal == 1)
    {
    	strcpy(name, (addr->selfinfo).name) ;
    	strcpy(id, (addr->selfinfo).id) ;
		msgleft = addr->selfinfo.msgQ->len;
		fileleft = addr->selfinfo.fileQ->len;
    }
    addr = NULL ;
	//退出临界区释放信号量
	V(sem_id, 0);
    
	//输出命令行
	printf("[YLHChat@%s@%s@msg(%d)@file(%d)]$", name, id, msgleft, fileleft);
    
	//过长处理
    char buffer[MAXLINE] ;
	int n ;
	fgets(buffer, MAXLINE, stdin) ; 
	n = strlen(buffer) ;
	if(n == MAXLINE - 1)
	{
		printf("command too long!\n") ;
		return ;
	}
	if(buffer[0] == '\n')
    	return ;
    //去除'\n'
    buffer[n - 1] = '\0' ;
    
    //分析命令
	int start = 0 ;
	int end ;
	//命令的形式一般是以type开头空格分割
	end = paserCommandType(buffer) ;
	//	printf("end: %d\n", end);
	char type[20] ;
	char info[MAXLINE - 20] ;
	if(substring(type, buffer, start, end) == NULL || end - start >= 20)
	{
		printf("cmd type parsing error!\n") ;
		return ;
	}
	//	printf("type: %s\n", type);
	substring(info, buffer, end + 1, MAXLINE - 1) ;
	if (strcmp(type, "login") == 0)
	{
		//login 客户端动作
		//
		if (legal == 1)
		{
			printf("irlegal login operation\n");
				return;
		}
		//输出提示同时输入必要信息
		char tmpid[2 * IDLEN], ch, password[2 * PWLEN];
		printf("id: ");
		scanf("%s", tmpid);
		scanf("%c", &ch);
		printf("password: ");
		scanf("%s", password);
		scanf("%c", &ch);
		//长度错误处理
		if (strlen(tmpid) != IDLEN - 1 || strlen(password) >= PWLEN)
		{
			printf("error length of id or password\n");
			return;
		}
		//为创建请求线程login准备
		tmpid[IDLEN - 1] = '\0';
		password[PWLEN - 1] = '\0';
		sprintf(info, "%s %s", tmpid, password);
		int pid;
		//由于登录过程涉及修改用户信息，所以必须共享内存空间采用vfork
		if((pid = vfork()) == 0)
		{
			login(info) ;
			
			exit(0) ;
		}
		int status;
		waitpid(pid, &status, 0);
		return;
	}
	else if(strcmp(type, "register") == 0)
	{
		//注册命令
		//当前不在线输出错误
		if (legal == 1)
		{
			printf("irlegal register operation\n");
			return;
		}
		//输出提示，同时输入必要信息
		char name[2 * NAMELEN], ch, password1[2 * PWLEN], password2[2 * PWLEN];
		printf("name: ");
		scanf("%s", name);
		scanf("%c", &ch);
		printf("password: ");
		scanf("%s", password1);
		scanf("%c", &ch);
		printf(" confirm: ");
		scanf("%s", password2);
		scanf("%c", &ch);
		//长度错误
		if (strlen(name) >= NAMELEN || strlen(password1) >= PWLEN || strlen(password2) >= PWLEN)
		{
			printf("error length of id or password\n");
			return;
		}
		//密码设置错误
		if (strcmp(password1, password2) != 0)
		{
			printf("the first password isn't same as the first one!\n");
			return;
		}
		name[NAMELEN - 1] = '\0';
		password1[PWLEN - 1] = '\0';
		sprintf(info, "%s %s", name, password1);
		int pid;
		//注册成功之后自动登录，
		//同样需要修改个人信息，使用vfork，共享内存空间
		if((pid = vfork()) == 0)
		{
			regis(info) ;
			exit(0) ;
		}
		int status;
		waitpid(pid, &status, 0);
		return;
	}
	else if(strcmp(type, "singlechat") == 0)
	{
		//私聊
		//不在线错误
		if (legal == 0)
		{
			printf("please login\n");
			return;
		}
		//输出提示，同时输入必要信息
		int start = 0, end;
		char id[IDLEN  * 2], msg[MAXLINE], ch;
		printf("to: ");
		scanf("%s", id);
		scanf("%c", &ch);
		printf("massage: ");
		fgets(msg, MAXLINE, stdin);
		int len = strlen(msg);
		msg[len - 1] = '\0'; //去除'\n'
		//目标id长度错误
		if (strlen(id) != IDLEN - 1)
		{
			printf("error dst id length!\n");
		}
		else
		{
			//必要信息都已准备好，不需要记录修改的内存空间，直接使用fork
			if (fork() == 0)
			{
				singlechat(id, msg);
				exit(0);
			}
		}
		return;
	}
	else if (strcmp(type, "logout") == 0)
	{
		//登出操作
		//不在线，本来就不在线
		if (legal == 0)
		{
			printf("you are already logout\n");
			return;
		}
		//直接修改当前在线状态
		//同时释放记录用户信息的内存
		int sem_id;
		getClientSem(&sem_id);
		P(sem_id, 0);
		clientShm* addr;
		addr = &clientshm;
		addr->legal = 0;
		freeuser(&(addr->selfinfo));
		addr = NULL;
		V(sem_id, 0);
		return;
	}
	else if (strcmp(type, "filetrans") == 0)
	{
		//文件传输
		//不在线错误
		if (legal == 0)
		{
			printf("please login\n");
			return;
		}
		//输入必要信息
		char tmpid[2 * IDLEN], ch, filepath[64];
		printf("to: ");
		scanf("%s", tmpid);
		scanf("%c", &ch);
		printf("filepath: ");
		scanf("%s", filepath);
		scanf("%c", &ch);
		//目标ID长度错误
		if (strlen(tmpid) != IDLEN - 1)
		{
			printf("error length of id\n");
			return;
		}
		else
		{
			//检测文件是否可以打开
			FILE* f = fopen(filepath, "r");
			if (f == NULL)
			{
				printf("fileopen error: %s\n", filepath);
				return;
			}
			fclose(f);
			//调用fork创建进程发送文件
			if (fork() == 0)
			{
				filetrans(tmpid, filepath);
				exit(0);
			}
			return;
		}
	}
	else if (strcmp(type, "friends") == 0)
	{
		//未实现好友功能
	}
	else if (strcmp(type, "buildgroup") == 0)
	{
		//创建群聊
		//不在线错误
		if (legal == 0)
		{
			printf("please login\n");
			return;
		}
		//输入必要信息
		char tmpname[2 * NAMELEN], ch;
		printf("name: ");
		scanf("%s", tmpname);
		scanf("%c", &ch);
		//长度错误处理
		if (strlen(tmpname) >= NAMELEN)
		{
			printf("too long length of name\n");
			return;
		}
		//涉及到临界资源修改，使用vfork
		if (vfork() == 0)
		{
			buildgroup(tmpname);
			exit(0);
		}
		return;
	}
	else if (strcmp(type, "addmember") == 0)
	{
		//添加群聊成员
		if (legal == 0)
		{
			printf("please login\n");
			return;
		}
		//输入必要信息
		char gid[2 * IDLEN], ch, uid[2 * IDLEN];
		printf("group: ");
		scanf("%s", gid);
		scanf("%c", &ch);
		printf("id: ");
		scanf("%s", uid);
		scanf("%c", &ch);
		//id长度错误
		if (strlen(gid) != IDLEN - 1 || strlen(uid) != IDLEN - 1)
		{
			printf("error length of gid or uid\n");
			return;
		}
		//查看是否具有权限
		int sem_id;
		getClientSem(&sem_id);
		P(sem_id, 0);
		clientShm* addr;
		addr = &clientshm;
		int i;
		int flag = 0;
		for (i = 0; i < addr->selfinfo.group_num; i++)
		{
			if (strcmp(gid, addr->selfinfo.gids[i]) == 0)
			{
				flag = 1;
				break;
			}
		}
		addr = NULL;
		V(sem_id, 0);
		if (flag == 0)
		{
			//无权限
			printf("you are not in the group: %s, so you can't add member in to it!\n", gid);
		}
		else
		{
			//有权限，不需要记录修改，调用fork
			if (fork() == 0)
			{
				addmember(gid, uid);
				exit(0);
			}
		}
		return;
	}
	else if (strcmp(type, "groups") == 0)
	{
		//查看自己所在群
		//不在线错误
		if (legal == 0)
		{
			printf("please login\n");
			return;
		}
		//查看自己所在的所有群id
		int sem_id;
		getClientSem(&sem_id);
		P(sem_id, 0);
		clientShm* addr;
		addr = &clientshm;
		int i;
		printf("the groups you have beeb in:\n");
		for (i = 0; i < addr->selfinfo.group_num; i++)
		{
			//输出群id
			printf("%s\n", addr->selfinfo.gids[i]);
		}
		addr = NULL;
		V(sem_id, 0);
	}
	else if (strcmp(type, "groupchat") == 0)
	{
		//群聊功能
		//不在线错误
		if (legal == 0)
		{
			printf("please login\n");
			return;
		}
		//输入必要信息
		int start = 0, end;
		char gid[IDLEN * 2], msg[MAXLINE], ch;
		printf("gid: ");
		scanf("%s", gid);
		scanf("%c", &ch);
		printf("massage: ");
		fgets(msg, MAXLINE, stdin);
		int len = strlen(msg);
		msg[len - 1] = '\0'; //去除'\n'
		if (strlen(gid) != IDLEN - 1)
		{
			printf("error dst gid length!\n");
			return;
		}
		//查看是否具有权限
		int sem_id;
		getClientSem(&sem_id);
		P(sem_id, 0);
		clientShm* addr;
		addr = &clientshm;
		int i;
		int flag = 0;
		for (i = 0; i < addr->selfinfo.group_num; i++)
		{
			if (strcmp(gid, addr->selfinfo.gids[i]) == 0)
			{
				flag = 1;
				break;
			}
		}
		addr = NULL;
		V(sem_id, 0);
		if (flag == 0)
		{
			//无权限
			printf("you are not in the group: %s, so you can't send message to it!\n", gid);
		}
		else  
		{
			//有权限
			if (fork() == 0)
			{
				groupchat(gid, msg);
				exit(0);
			}
		}
		return;
	}
	else if (strcmp(type, "getmsg") == 0)
	{
		//处理本地消息队列里最久的一条消息，front
		if (legal == 0)
		{
			printf("please login\n");
			return;
		}
		//申请进入临界区
		int sem_id;
		getClientSem(&sem_id);
		P(sem_id, 0);
		clientShm* addr;
		addr = &clientshm;
		//消息队列不空
		if (!Empty(addr->selfinfo.msgQ))
		{
			char* msg = Front(addr->selfinfo.msgQ);
			//需要分析是普通消息还是群聊消息
			//每个人的历史聊天文件分为两种
			//个人聊天文件名为 "%s_%s.msg",id,id第一个id是和谁的聊天记录，第二个id是自己
			//群聊文件为 "group%s_%s.msg",id,id第一个id是群id，第二个id是自己
			//聊天记录文件中的格式都为："%s %s",id,msg，id是发言人，msg是内容
			char path_msg[64];
			char type[IDLEN];
			char from[IDLEN], info[MAXLINE];
			int start = 0, end;
			//分析消息类型，群聊或私聊
			end = parseMsgType(msg + start);
			substring(type, msg, start, start + end);
			start += end + 1;
			if (strcmp(type, "gmsg") == 0)
			{
				//群聊
				//分析群聊消息的来源
				char gid[IDLEN];
				end = parseMsgType(msg + start);
				substring(gid, msg, start, start + end);
				start += end + 1;
				end = parseMsgType(msg + start);
				substring(from, msg, start, start + end);
				start += end + 1;
				substring(info, msg, start, strlen(msg));
				//输出
				printf("group: %s massage, from: %s, %s\n", gid, from, info);
			}
			else
			{
				//私聊
				substring(from, msg, 0, IDLEN - 1);
				substring(info, msg, IDLEN, strlen(msg));
				//输出
				printf("massage from: %s, %s\n", from, info);
			}

			Pop(addr->selfinfo.msgQ);
		}
		else
		{
			printf("no new massage!\n");
		}
		addr = NULL;
		V(sem_id, 0);
	}
	else if (strcmp(type, "getfile") == 0)
	{
		//处理文件队列里的最久远文件
		if (legal == 0)
		{
			printf("please login\n");
			return;
		}
		int sem_id;
		getClientSem(&sem_id);
		P(sem_id, 0);
		clientShm* addr;
		addr = &clientshm;
		int empty = Empty(addr->selfinfo.fileQ);
		addr = NULL;
		V(sem_id, 0);
		//文件队列不空
		if (!empty)
		{
			char filename[64];
			P(sem_id, 0);
			clientShm* addr;
			addr = &clientshm;
			strcpy(filename, Front(addr->selfinfo.fileQ));
			addr = NULL;
			V(sem_id, 0);
			//分析文件名，文件名的格式为./tempfile/from_to_num_filename
			printf("the file is: %s\n", filename);
			printf("Download or drop?[D/d]:");
			char choose = '\0';
			choose = getchar();
			//这是只有两种选择，下载或丢弃，只有做出选择才能退出该状态
			while (choose != 'D' && choose != 'd')
			{
				printf("Download or drop?[D/d]:");
				choose = getchar();
			}
			char ch_1 = getchar();
			while (ch_1 != '\n')
			{
				ch_1 = getchar();
			}
			P(sem_id, 0);
			addr = &clientshm;
			Pop(addr->selfinfo.fileQ);
			addr = NULL;
			V(sem_id, 0);
			if (choose == 'D')
			{
				//下载
				
				void* stack = malloc(threadStackSize);
				clone(&downloadfile, (char*)stack + threadStackSize, CLONE_VM, filename);
				//if (fork == 0)
				//{
				//	downloadfile(filename);
				//	exit(0);
				//}
			}
		}
		else
		{
			//文件队列为空
			printf("no new file!\n");
		}
		
	}
	else if (strcmp(type, "quit") == 0 || strcmp(type, "exit") == 0)
	{
		ifexit = 1;
	}
	return ;
}

void mytimer()
{
	//隔一段时间进行keepalive操作
	while(1)
	{
		//睡眠一秒
		sleep(1);
		int sem_id;
		getClientSem(&sem_id);
		P(sem_id, 0);
		clientShm* addr;
		addr = &clientshm;
		if (addr->legal == 0)
		{
			//如果不在线
			V(sem_id, 0);
			continue;
		}
		if (addr->exit == 1)
		{
			//退出
			V(sem_id, 0);
			break;
		}
		V(sem_id, 0);
		//如果在线，调用keepalive
		keepalive();
	}
	exit(0) ;
}

int commandline()
{
	//创建mytimer线程
	void* stack = malloc(threadStackSize);
	clone(&mytimer, (char*)stack + threadStackSize, CLONE_VM, 0);
	
	//处理命令
	while(!ifexit)
	{
		processOneCommand() ;
	}
	//通知mytimer退出
	int sem_id;
	getClientSem(&sem_id);
	P(sem_id, 0);
	clientShm* addr;
	addr = &clientshm;
	addr->exit = 1;
	V(sem_id, 0);
	return 0 ;
}
