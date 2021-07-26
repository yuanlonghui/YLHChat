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
#include "datastruct.h"

#include "service.h"
#include "msgQueue.h"

void serve(void* _socket)
{
	//服务函数，通过接受到的服务请求类型，调用一系列的服务函数
	int socket = *(int*)_socket;
	char buffer[MAXLINE] ;
	int n ;
	n = recv(socket, buffer, MAXLINE, 0) ;
	if (n <= 0)
	{
		exit(0);
	}
//	printf("receive: %s\n", buffer);
	buffer[n] = '\0' ;
	int start = 0 ;
	int end ;
	end = parseMsgType(buffer) ;
	char type[20] ;
	char info[MAXLINE - 20] ;
	//接收到的服务请求都是(type info)的格式
	if(substring(type, buffer, start, start + end) == NULL)
	{
		printf("service type parsing error!\n") ;
		char * reply = "ERROR_REQUEST" ;
		send(socket, reply, 14, 0) ;
		return ;
	}
	substring(info, buffer, end + 1, MAXLINE - 1) ;
//	printf("type: %s, info: %s\n", type, info);
	//根据不同的type调用相对应的服务函数
	if(strcmp(type, "login")  == 0)
	{
		login(socket, info) ;
	}
	else if(strcmp(type, "register") == 0)
	{
		regis(socket, info) ;
	}
	else if(strcmp(type, "keepalive") == 0)
	{
		keepalive(socket, info) ;
	}
	else if (strcmp(type, "singlechat") == 0)
	{
		singleChat(socket, info);
	}
	else if (strcmp(type, "filetrans") == 0)
	{
		filetrans(socket, info);
	}
	else if (strcmp(type, "downloadfile") == 0)
	{
		sendfile(socket, info);
	}
	else if (strcmp(type, "buildgroup") == 0)
	{
		buildgroup(socket, info);
	}
	else if (strcmp(type, "addmember") == 0)
	{
		addmember(socket, info);
	}
	else if (strcmp(type, "groupchat") == 0)
	{
		groupchat(socket, info);
	}
	exit(0);
}
void login(int socket, char* logInfo)
{
	//登录服务
	char id[IDLEN], password[PWLEN];
	int id_int;
	int start = 0;
	int end;
	end = parseMsgType(logInfo);
	substring(id, logInfo, start, end);
	substring(password, logInfo, end + 1, MAXLINE - 20);
	id_int = atoi(id);
	int sem_id;
	//申请进入临界区
	getServerSem(&sem_id);
	P(sem_id, 0);
	serverShm * addr;
	addr = &servershm;
	if (addr->user_bitmap[id_int] == 0)
	{
		//不存在的id
		printf("not exist user id try to login!\n");
		const char* reply = "USER_NOT_EXISTS";
		send(socket, reply, 16, 0);
	}
	else if (strcmp(password, addr->users[id_int].password) != 0)
	{
		//密码错误
		printf("error password of user: %s!\n", id);
		const char* reply = "ERROR_PASSWORD";
		send(socket, reply, 15, 0);
	}
	else if (addr->users[id_int].alive == 1)
	{
		//用户已经在线，不允许同时两个客户端登录同一个用户id
		printf("duplicated login!\n");
		const char* reply = "DUPLICATED_LOGIN";
		send(socket, reply, 17, 0);
	}
	else
	{
		//登录成功
		char reply[MAXLINE];
		addr->users[id_int].alive = 1;
		addr->keepalive[id_int] = 1;
		sprintf(reply, "loginreply ");
		//将用户信息写入字符串，发送给客户端
		storeUserToStr(&(addr->users[id_int]), reply + strlen(reply));
		send(socket, reply, strlen(reply), 0);
		printf("login successfully! id: %s, name: %s\n", addr->users[id_int].id, addr->users[id_int].name);
	}
	addr = NULL;
	V(sem_id, 0);
}

void regis(int socket, char* regisInfo)
{
	//注册函数
	char name[NAMELEN], password[PWLEN];
	int id_int;
	int sem_id;
	getServerSem(&sem_id);
	P(sem_id, 0);
	serverShm* addr;
	addr = &servershm;
	id_int = getUserNewId(addr->user_bitmap);
	if (id_int == -1)
	{
		//无剩余id
		printf("no free user id!\n");
		char* reply = "NOFREEUSERID";
		send(socket, reply, 13, 0);
	}
	else
	{
		//分析出密码和用户名
		int start = 0, end;
		end = parseMsgType(regisInfo + start);
		substring(name, regisInfo, start, start + end);
		start += end + 1;
		end = parseMsgType(regisInfo + start);
		substring(password, regisInfo, start, start + end);

		//创建新的用户
		addr->user_bitmap[id_int] = 1;
		storeUserInfoBitmap(addr->user_bitmap);
		addr->keepalive[id_int] = 1;
		char id[IDLEN];
		sprintf(id, "%.4d", id_int);
		memset(&(addr->users[id_int]), 0, sizeof(user));
		strcpy(addr->users[id_int].id, id);
		strcpy(addr->users[id_int].name, name);
		strcpy(addr->users[id_int].password, password);

		char reply[MAXLINE];
		sprintf(reply, "registerreply ");
		//将用户信息写入字符串，发送给客户端
		storeUserToStr(&(addr->users[id_int]), reply + strlen(reply));
		send(socket, reply, strlen(reply), 0);
		printf("register successfully! id: %s, name: %s\n", addr->users[id_int].id, addr->users[id_int].name);
		storeUser(&(addr->users[id_int]));
		addr->users[id_int].msgQ = CreateQueue();
		addr->users[id_int].fileQ = CreateQueue();
	}

	addr = NULL;
	V(sem_id, 0);
}

void singleChat(int socket, char* singleChatInfo)
{
	//私聊
	printf("singlechat\n");
	printf("singlechat info: %s\n", singleChatInfo);
	char id[IDLEN], msg[MAXLINE];
	int start = 0, end;
	//解析出目的id
	end = parseMsgType(singleChatInfo + start);
	substring(id, singleChatInfo, start, start + end);
	start += end + 1;
	substring(msg, singleChatInfo, start, strlen(singleChatInfo));
	int id_int = atoi(id);
	int sem_id;
	getServerSem(&sem_id);
	P(sem_id, 0);
	serverShm* addr;
	addr = &servershm ;
	if (addr->user_bitmap[id_int] == 0)
	{
		//目的不存在
		printf("singlechat error:id not exists!\n");
		const char* reply = "ERRORDSTID";
		send(socket, reply, 11, 0);
	}
	else
	{
		if (Full(addr->users[id_int].msgQ) == 1)
		{
			//目的消息队列满
			printf("full msg queue of id: %s", id);
			const char* reply = "FULLMSGQ";
			send(socket, reply, 9, 0);
		}
		else
		{
			//挂在消息
			Push(addr->users[id_int].msgQ, msg);
			printf("successfully send to %s: %s\n", id, Rear(addr->users[id_int].msgQ));
			const char* reply = "singlechatreply";
			send(socket, reply, 16, 0);
		}
	}
	addr = NULL;
	V(sem_id, 0);
}

void filetrans(int socket, char * info)
{
	//文件传输
	printf("filetrans info: %s\n", info);
	char id[IDLEN], from[IDLEN], path[64];
	//解析必要信息
	int start = 0, end;
	end = parseMsgType(info + start);
	substring(id, info, start, start + end);
	start += end + 1;
	end = parseMsgType(info + start);
	substring(from, info, start, start + end);
	start += end + 1;
	substring(path, info, start, strlen(info));

	int id_int = atoi(id);
	int sem_id;
	getServerSem(&sem_id);
	P(sem_id, 0);
	serverShm* addr;
	addr = &servershm;
	if (addr->user_bitmap[id_int] == 0)
	{
		//目的不存在
		printf("filetrans error:id not exists!\n");
		const char* reply = "ERRORDSTID";
		send(socket, reply, 11, 0);
		addr = NULL;
		V(sem_id, 0);
	}
	else
	{
		if (Full(addr->users[id_int].fileQ) == 1)
		{
			//文件队列满
			printf("full file queue of id: %s", id);
			const char* reply = "FULLFILEQ";
			send(socket, reply, 10, 0);
			addr = NULL;
			V(sem_id, 0);
		}
		else
		{
			char newpath[60];
			sprintf(newpath, "%s%s_%d_%s",tempfile, from, nextIndex(addr->users[id_int].fileQ), path);
			//这里的path必须是文件名，如果带有/那么会出错
			Push(addr->users[id_int].fileQ, newpath);
			//printf("after push:%d, %s\n",QueueLenth(addr->users[id_int].fileQ), Rear(addr->users[id_int].fileQ));
			addr = NULL;
			V(sem_id, 0);
			char recv_buffer[MAXLINE];
			int n;
			FILE* f = fopen(newpath, "w");
			if (f == NULL)
			{
				//回复缓冲失败
				const char* reply = "BUFFERERROR";
				send(socket, reply, 11, 0);
			}
			else
			{
				//接收文件内容
				const char* reply = "filetransready";
				send(socket, reply, 15, 0);
				//printf("start to trans file:%s\n", Rear(addr->users[id_int].fileQ));
				printf("want to store in to: %s\n", newpath);
				n = 1;
				while (n > 0)
				{
					n = recv(socket, recv_buffer, MAXLINE, 0);
					recv_buffer[n] = '\0';
					//回复ack
					send(socket, "fileack", 8, 0);
					printf("fileack\n");
					if (strcmp(recv_buffer, "EOF") == 0 || n <= 0)
					{
						break;
					}
					fwrite(recv_buffer, 1, n, f);
				}
				fclose(f);
				send(socket, "filetransend", 13, 0);
				printf("filetransend\n");
			}
		}
	}
}

void keepalive(int socket, char* infor)
{
	char id[IDLEN];
	strcpy(id, infor);
	char send_buffer[MAXLINE];
	int id_int;
	id_int = atoi(id);
	int sem_id;
	getServerSem(&sem_id);
	P(sem_id, 0);
	serverShm* addr;
	addr = &servershm;
	//keepalive
	addr->keepalive[id_int] = 1;
//	printf("msg: %d, file: %d\n", addr->users[id_int].msgQ->len, addr->users[id_int].fileQ->len);
	//printf("msgQlen: %d\n", QueueLenth(addr->users[id_int].msgQ));
	if (addr->info_change[id_int] == 1)
	{
		//信息同步
		char reply[MAXLINE];
		sprintf(reply, "infosync ");
		storeUserToStr(&(addr->users[id_int]), reply + strlen(reply));
		send(socket, reply, strlen(reply), 0);
		addr->info_change[id_int] = 0;
	}
	while (!Empty(addr->users[id_int].msgQ))
	{
		char* msg = Front(addr->users[id_int].msgQ);
		sprintf(send_buffer, "msg %s", msg);
		//注意到普通消息和群消息是不同的
		// 群消息的发送格式为：msg gmsg gid fromid msgcontent
		// 普通消息发送格式为：msg fromid msgcontent
		//printf("send %s", send_buffer);
		send(socket, send_buffer, strlen(send_buffer), 0);
		Pop(addr->users[id_int].msgQ);
		printf("keepalive send: %s\n", send_buffer);
	}
	while (!Empty(addr->users[id_int].fileQ))
	{
		char* file = Front(addr->users[id_int].fileQ);
		sprintf(send_buffer, "file %s", file);
//		printf("send: %s\n", send_buffer);
		send(socket, send_buffer, strlen(send_buffer), 0);
		Pop(addr->users[id_int].fileQ);
		printf("keepalive send: %s\n", send_buffer);
	}
	send(socket, "\0", 1, 0);
	send(socket, "endkeepalive", 13, 0);
	//printf("endkeepalive\n");

	addr = NULL;
	V(sem_id, 0);
}

void checkalive()
{
	while (1)
	{
		//sleep3秒
		sleep(3);
		int i;
		int sem_id;
		getServerSem(&sem_id);
		P(sem_id, 0);
		serverShm* addr;
		addr = &servershm;
		//三秒内没有收到keepalive认为该用户不在线
		for (i = 0; i < MAX_USER; i++)
		{
			if (addr->user_bitmap[i] == 1)
			{
				if (addr->keepalive[i] == 0)
				{
					addr->users[i].alive = 0;
				}
				else
				{
					addr->users[i].alive = 1;
				}
			}
		}
		//清楚keepalive等待下一轮检测
		memset(addr->keepalive, 0, sizeof(addr->keepalive));
		addr = NULL;
		V(sem_id, 0);
	}
	exit(0);
}

void sendfile(int socket, char* path)
{
	//发送文件，当客户请求下载文件时调用
	printf("sendfile");
	FILE* f = fopen(path, "r");
	if (f == NULL)
	{
		printf("file open error: %s\n", path);
		return;
	}
	
	char send_buffer[MAXLINE];
	char recv_buffer[MAXLINE];
	char from[IDLEN];
	//准备发送
	sprintf(send_buffer, "sendready");
	if (send(socket, send_buffer, strlen(send_buffer), 0) < 0)
	{
		printf("send file error: %s!\n", send_buffer);
		close(socket);
		return;
	}
	int n;
	n = recv(socket, recv_buffer, MAXLINE, 0);
	recv_buffer[n] = '\0';
	printf("receive: %s\n", recv_buffer);
	int start = 0, end;
	end = parseMsgType(recv_buffer + start);
	char type[20];
	substring(type, recv_buffer, start, start + end);
	printf("start to send file: %s", path);
	if (strcmp(type, "receiveready") == 0)
	{
		while (n > 0)
		{
			int m;
			n = 0;
			m = fread(send_buffer, 1, MAXLINE - 1, f);
			if (m != 0)
			{
				//发送内容
				send(socket, send_buffer, m, 0);
			}
			else
			{
				//发送eof
				send(socket, "EOF", 4, 0);
			}
			n = recv(socket, recv_buffer, MAXLINE, 0);
			recv_buffer[n] = '\0';
			printf("receive: %d, %s\n", n, recv_buffer);
			if (strcmp(recv_buffer, "fileack") == 0)
			{
				//接受到ack
				continue;
			}
			else if (strcmp(recv_buffer, "downloadend") == 0)
			{
				//结束
				break;
			}
		}
		printf("send file successfully!\n");
	}
	else
	{
		printf("filetrans error: %s\n", type);
	}
	fclose(f);
	close(socket);
	return;
}

void buildgroup(int socket, char* info)
{
	//创建群聊
	char send_buffer[MAXLINE];
	char recv_buffer[MAXLINE];
	int n;
	char ownerid[IDLEN], gname[NAMELEN];

	int gid_int;
	int sem_id;
	getServerSem(&sem_id);
	P(sem_id, 0);
	serverShm* addr;
	addr = &servershm;
	gid_int = getGroupNewId(addr->group_bitmap);
	//printf("point1\n");
	if (gid_int == -1)
	{
		//无可用id
		printf("no free group id!\n");
		char* reply = "NOFREEGROUOPID";
		send(socket, reply, 15, 0);
	}
	else
	{
		//创建群聊
		int start = 0, end;
		end = parseMsgType(info + start);
		substring(ownerid, info, start, start + end);
		start += end + 1;

		end = parseMsgType(info + start);
		substring(gname, info, start, start + end);
		addr->group_bitmap[gid_int] = 1;
		storeGroupInfoBitmap(addr->group_bitmap);

		char gid[IDLEN];
		sprintf(gid, "%.4d", gid_int);
		memset(&(addr->groups[gid_int]), 0, sizeof(group));
		strcpy(addr->groups[gid_int].id, gid);
		strcpy(addr->groups[gid_int].onwerid, ownerid);
		strcpy(addr->groups[gid_int].name, gname);
		addUser(&(addr->groups[gid_int]), ownerid);
		storeGroup(&(addr->groups[gid_int]));
		int oid_int = atoi(ownerid);
		//保存群聊和用户
		addIntoGroup(&(addr->users[oid_int]), gid);
		//printf("point2\n");
		storeUser(&(addr->users[oid_int]));
		//printf("point3\n");
		char reply[MAXLINE];
		//回复
		sprintf(reply, "createsuccessfully %s", gid);
		send(socket, reply, strlen(reply), 0);
		printf("create group successfully! owner: %s id: %s, name: %s\n", addr->groups[gid_int].onwerid, addr->groups[gid_int].id, addr->groups[gid_int].name);

	}
	addr = NULL;
	V(sem_id, 0);
}

void addmember(int socket, char* info)
{
	//添加成员
	char gid[IDLEN], uid[IDLEN];
	int gid_int, uid_int;
	int start = 0, end;

	//分析信息
	end = parseMsgType(info + start);
	substring(gid, info, start, start + end);
	start += end + 1;

	end = parseMsgType(info + start);
	substring(uid, info, start, start + end);

	gid_int = atoi(gid);
	uid_int = atoi(uid);


	int sem_id;
	getServerSem(&sem_id);
	P(sem_id, 0);
	serverShm* addr;
	addr = &servershm;
	if (addr->user_bitmap[uid_int] == 0 || addr->group_bitmap[gid_int] == 0)
	{
		//不存在的群id或者用户id 
		printf("try to add not exist id or into not exist gid!\n");
		const char* reply = "ID_NOT_EXISTS";
		send(socket, reply, 14, 0);
	}
	else
	{
		//添加用户到群聊，添加群聊到用户，保存两者
		addIntoGroup(&(addr->users[uid_int]), gid);
		storeUser(&(addr->users[uid_int]));
		addr->info_change[uid_int] = 1;
		addUser(&(addr->groups[gid_int]), uid);
		storeGroup(&(addr->groups[gid_int]));
		const char* reply = "addsuccessfully";
		send(socket, reply, 16, 0);
		printf("add member in to group successfully! gid: %s, uid: %s\n", gid, uid);
	}
	addr = NULL;
	V(sem_id, 0);
}

void groupchat(int socket, char* groupInfo)
{
	//群聊消息
	//和私聊差别不大，通过群聊的信息查到每个用户，除了发送者之外，
	//将消息挂载到他们的的消息队列上，当然这个消息带上了群聊标记
	printf("groupchat\n");
	printf("groupchat info: %s\n", groupInfo);
	char gid[IDLEN], msg[MAXLINE], from[IDLEN];
	int fromid_int;
	int start = 0, end;
	end = parseMsgType(groupInfo + start);
	substring(gid, groupInfo, start, start + end);
	start += end + 1;
	printf("start: %d\n", start);
	substring(msg, groupInfo, start, strlen(groupInfo));
	//printf("gid: %s, msg: %s\n", gid, msg);
	substring(from, msg, 0, IDLEN - 1);
	//printf("gid: %s, msg: %s\n", gid, msg);
	fromid_int = atoi(from);
	int gid_int = atoi(gid);
	int sem_id;
	getServerSem(&sem_id);
	P(sem_id, 0);
	serverShm* addr;
	addr = &servershm;

	if (addr->group_bitmap[gid_int] == 0)
	{
		printf("groupchat error:id not exists!\n");
		const char* reply = "ERRORDSTID";
		send(socket, reply, 11, 0);
	}
	else
	{
		//对于在群里的除了发送者之外的每一个人，
		//消息都会放到它们的消息对列里,并且不向客户端发送处理结果
		int i;
		printf("groupnum: %d\n", addr->groups[gid_int].user_num);
		for (i = 0; i < addr->groups[gid_int].user_num; i++)
		{
			int id_int = atoi(addr->groups[gid_int].users[i]);
			if (id_int == fromid_int)
			{
				continue;
			}
			if (Full(addr->users[id_int].msgQ) == 1)
			{
				printf("full msg queue of id: %s", gid);
				char reply[MAXLINE];
				printf("someone missing the massage from group: %s", gid);
			}
			else
			{
				//为了区别普通消息，挂载的格式为："gmsg %s %s", gid, msg
				char gmsg[MAXLINE];
				sprintf(gmsg, "gmsg %s %s", gid, msg);
				Push(addr->users[id_int].msgQ, gmsg);
				printf("successfully send group massage to %s: %s\n", addr->users[id_int].id, Rear(addr->users[id_int].msgQ));
			}
		}
		send(socket, "groupchatreply", 15, 0);
	}
	addr = NULL;
	V(sem_id, 0);
}
