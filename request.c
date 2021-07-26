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
#include "msgQueue.h"

#include "request.h"


void getFileName(char* dst, char* src)
{
	//只能被本文件中的函数调用的一个服务函数，从文件路径中获得文件名
	int i;
	int len = strlen(src);
	//向前搜索第一个分隔符
	for (i = len - 1; i >= 0; i--)
	{
		if (src[i] == '/' || src[i] == '\\')
		{
			break;
		}
	}
	i++;
	//要么i = -1 , 要么处于分隔符上
	substring(dst, src, i, len);
}

void login(char* info)
{
	//登录请求
	//连接服务器
	int socket ;
	if (getConnect(&socket) == ERROR)
	{
		return;
	}
	char send_buffer[MAXLINE];
	char recv_buffer[MAXLINE];
	//发送登录请求
	sprintf(send_buffer, "login %s", info);
	if (send(socket, send_buffer, strlen(send_buffer), 0) < 0)
	{
		printf("send login massage error: %s!\n", send_buffer);
		close(socket);
		return;
	}
	//接收回应
	int n;
	n = recv(socket, recv_buffer, MAXLINE, 0);
	recv_buffer[n] = '\0';
	//printf("receive: %s\n", recv_buffer);
	int start = 0, end;
	//分析回应
	end = parseMsgType(recv_buffer);
	char type[20], reply[MAXLINE - 20];
	substring(type, recv_buffer, start, end);
	substring(reply, recv_buffer, end + 1, MAXLINE - 1);
	if (strcmp(type, "loginreply") == 0)
	{
		//登录成功
		//从reply中解析出整个user信息
		int sem_id;
		getClientSem(&sem_id);
		P(sem_id, 0);
		clientShm* addr;
		addr = &clientshm;
		memset(&(addr->selfinfo), 0, sizeof(user));
		parseUserFromStr(&(addr->selfinfo), reply);
		//printf("%s", reply + 3);
		addr->legal = 1;
		//printf("login successfully! name: %s, id: %s\n", addr->selfinfo.name, addr->selfinfo.id);
		addr->selfinfo.msgQ = CreateQueue();
		addr->selfinfo.fileQ = CreateQueue();
		addr = NULL;
		V(sem_id, 0);
		printf("login sucessfully!\n");
		
	}
	else
	{
		//登录失败
		printf("login error!: %s\n", type);
		
	}
	//关闭socket
	close(socket);
	return;
}

void regis(char* info)
{
	//注册请求
	//连接服务器
	int socket;
	if (getConnect(&socket) == ERROR)
	{
		return;
	}
	char send_buffer[MAXLINE];
	char recv_buffer[MAXLINE];
	sprintf(send_buffer, "register %s", info);
	//发送注册请求
	if (send(socket, send_buffer, strlen(send_buffer), 0) < 0)
	{
		printf("send register massage error: %s!\n", send_buffer);
		close(socket);
		return;
	}
	int n;
	//接收回应
	n = recv(socket, recv_buffer, MAXLINE, 0);
	recv_buffer[n] = '\0';
//	printf("receive: %s\n", recv_buffer);
	int start = 0, end;
	//解析回应
	end = parseMsgType(recv_buffer);
	char type[20], reply[MAXLINE - 20];
	substring(type, recv_buffer, start, end);
	substring(reply, recv_buffer, end + 1, MAXLINE - 1);

	if (strcmp(type, "registerreply") == 0)
	{
		//注册成功
		//从reply中解析出整个user信息
		int sem_id;
		getClientSem(&sem_id);
		P(sem_id, 0);
		clientShm* addr;
		addr = &clientshm;
		parseUserFromStr(&(addr->selfinfo), reply);
		printf("register sucessfully! your id is: %s\n", addr->selfinfo.id);
		addr->legal = 1;
		addr->selfinfo.msgQ = CreateQueue();
		addr->selfinfo.fileQ = CreateQueue();
		addr = NULL;
		V(sem_id, 0);
		
		
	} 
	else
	{
		//注册失败
		printf("register error!: %s\n", type);
		
	}
	close(socket);
	return;
}

void keepalive()
{
	//keepalive请求，发送keepalive，同时通过服务器发送来的信息改变自己用户的信息
	// 	   借此机会完成消息传递
	//建立连接
	int socket;
	if (getConnect(&socket) == ERROR)
	{
		return;
	}
	char recv_buffer[MAXLINE];
	char send_buffer[MAXLINE];
	int sem_id;
	getClientSem(&sem_id);
	P(sem_id, 0);
	clientShm* addr;
	addr = &clientshm;
	//发送keepalive请求
	sprintf(send_buffer, "keepalive %s", addr->selfinfo.id);
	//printf("\n%s\n", send_buffer);
	if (send(socket, send_buffer, strlen(send_buffer), 0) < 0)
	{
		//printf("keepalive error!\n");
		V(sem_id, 0);
		close(socket);
		return;
	}
	else
	{
		//接收回应，直到收到keepaliveend
		char path_file[64];
		sprintf(path_file, "%sfilepath.txt", history);
		FILE* f_file = fopen(path_file, "a");
		int n = 1;
		while (n > 0)
		{
			n = recv(socket, recv_buffer, MAXLINE, 0);
			recv_buffer[n] = '\0';
//			printf("receive: %s\n", recv_buffer);
			char type[20], msg[MAXLINE];
			int start = 0, end;
			//解析回应
			end = parseMsgType(recv_buffer + start);
			substring(type, recv_buffer, start, start + end);
			substring(msg, recv_buffer, start + end + 1, strlen(recv_buffer));
			if (strcmp(type, "msg") == 0)
			{
				//消息传递回应
				//需要分析是普通消息还是群聊消息
				//每个人的历史聊天文件分为两种
				//个人聊天文件名为 "%s_%s.msg",id,id第一个id是和谁的聊天记录，第二个id是自己
				//群聊文件为 "group%s_%s.msg",id,id第一个id是群id，第二个id是自己
				//聊天记录文件中的格式都为："%s %s",id,msg，id是发言人，msg是内容
				char path_msg[64];
				char type[IDLEN];
				int start = 0, end;
				end = parseMsgType(msg + start);
				substring(type, msg, start, start + end);
				start += end + 1;

				if (strcmp(type, "gmsg") == 0)
				{
					//群聊消息
					char gid[IDLEN], msgcontent[MAXLINE];
					end = parseMsgType(msg + start);
					substring(gid, msg, start, start + end);
					start += end + 1;
					substring(msgcontent, msg, start, strlen(msg));

					char path[64];
					sprintf(path, "%sgroup%s_%s.msg", history, gid, addr->selfinfo.id);
					FILE* f = fopen(path, "a");
					//保存群聊消息
					fprintf(f, "%s\n", msgcontent);
					fclose(f);
					//加入消息队列
					Push(addr->selfinfo.msgQ, msg);
				}
				else
				{
					//私聊消息
					char from[IDLEN];
					substring(from, msg, 0, IDLEN);
					sprintf(path_msg, "%s%s_%s.msg", history, from, addr->selfinfo.id);
					FILE* f = fopen(path_msg, "a");
					//保存
					fprintf(f, "%s\n", msg);
					fclose(f);
					//加入消息队列
					Push(addr->selfinfo.msgQ, msg);
				}
			}
			else if (strcmp(type, "file") == 0)
			{
				//文件消息，加入文件队列等待处理
				Push(addr->selfinfo.fileQ, msg);
				fprintf(f_file, "%s\n", msg);
			}
			else if (strcmp(type, "infosync") == 0)
			{
				//用户信息变化，重新解析
				queue* fileQ, * msgQ;
				fileQ = addr->selfinfo.fileQ;
				msgQ = addr->selfinfo.msgQ;
				memset(&(addr->selfinfo), 0, sizeof(user));
				parseUserFromStr(&(addr->selfinfo), msg);
				//printf("%s", reply + 3);
				addr->legal = 1;
				//printf("login successfully! name: %s, id: %s\n", addr->selfinfo.name, addr->selfinfo.id);
				addr->selfinfo.fileQ = fileQ;
				addr->selfinfo.msgQ = msgQ;
			}
			else if(strcmp(type, "endkeepalive") == 0)
			{
				//结束
				break;
			}
		}
		fclose(f_file);
	}
	addr = NULL;
	V(sem_id, 0);
	close(socket);
	return;
}

void singlechat(char * to, char* info)
{
	//私聊
	//建立连接
	int socket;
	if (getConnect(&socket) == ERROR)
	{
		return;
	}
	char send_buffer[MAXLINE];
	char recv_buffer[MAXLINE];
	char from[IDLEN];
	//获取本地用户消息
	int sem_id;
	getClientSem(&sem_id);
	P(sem_id, 0);
	clientShm* addr;
	addr = &clientshm;
	strcpy(from, addr->selfinfo.id);
	addr = NULL;
	V(sem_id, 0);
	sprintf(send_buffer, "singlechat %s %s %s",to, from, info);
	//发送私聊消息
	printf("\nsend: %s\n", send_buffer);
	if (send(socket, send_buffer, strlen(send_buffer), 0) < 0)
	{
		printf("send massage error: %s!\n", send_buffer);
		close(socket);
		return;
	}
	//保存消息
	char path[64];
	sprintf(path, "%s%s_%s.msg", history, to, from);
	FILE* f = fopen(path, "a");
	fprintf(f, "%s %s\n", from, info);
	fclose(f);
	//接受回应
	int n;
	n = recv(socket, recv_buffer, MAXLINE, 0);
	recv_buffer[n] = '\0';
	printf("receive: %s\n", recv_buffer);
	int start = 0, end;
	//分析回应
	end = parseMsgType(recv_buffer + start);
	char type[20];
	substring(type, recv_buffer, start, start + end);
	if (strcmp(type, "singlechatreply") == 0)
	{
		//成功
		printf("send to mailbox successfully!\n");
	}
	else
	{
		//失败
		printf("single chat error: %s\n", type);
	}
	close(socket);
	return;
}

void filetrans(char* to, char* path)
{
	//文件传输
	//检查文件是否可以打开
	FILE* f = fopen(path, "r");
	if (f == NULL)
	{
		printf("file open error: %s\n", path);
		return;
	}
	int socket;
	if (getConnect(&socket) == ERROR)
	{
		return;
	}
	char send_buffer[MAXLINE];
	char recv_buffer[MAXLINE];
	char from[IDLEN];
	int sem_id;
	getClientSem(&sem_id);
	P(sem_id, 0);
	clientShm* addr;
	addr = &clientshm;
	strcpy(from, addr->selfinfo.id);
	addr = NULL;
	V(sem_id, 0);
	char filename[64];
	getFileName(filename, path);
	sprintf(send_buffer, "filetrans %s %s %s", to, from, filename);
	printf("\nsend: %s\n", send_buffer);
	//发送文件传输请求
	if (send(socket, send_buffer, strlen(send_buffer), 0) < 0)
	{
		printf("send massage error: %s!\n", send_buffer);
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
	if (strcmp(type, "filetransready") == 0)
	{
		//接到文件可以开始传输命令
		while (n > 0)
		{
			int m;
			n = 0;
			m = fread(send_buffer, 1, MAXLINE - 1, f);
			if (m != 0)
			{
				//发送文件内容
				send(socket, send_buffer, m, 0);
			}
			else
			{
				//文件结束
				//一旦文件中出现单独的EOF，那么可能终止文件传输
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
			else if(strcmp(recv_buffer, "filetransend") == 0)
			{
				//发送结束确认
				break;
			}
		}
	}
	else
	{
		//文件传输错误
		printf("filetrans error: %s\n", type);
	}
	fclose(f);
	close(socket);
	return;
}

void downloadfile(char* info)
{
	//下载文件
	int socket;
	if (getConnect(&socket) == ERROR)
	{
		return;
	}
	char filename[32];
	char newpath[64];
	getFileName(filename, info);
	sprintf(newpath, "%s%s", download, filename);
	char send_buffer[MAXLINE];
	sprintf(send_buffer, "downloadfile %s", info);
	//发送请求
	send(socket, send_buffer, strlen(send_buffer), 0);

	char recv_buffer[MAXLINE];
	char type[20];
	int n;
	n = recv(socket, recv_buffer, MAXLINE, 0);
	recv_buffer[n] = '\0';
	int start = 0, end;
	end = parseMsgType(recv_buffer + start);
	substring(type, recv_buffer, start, start + end);
	if (strcmp(type, "sendready") == 0)
	{
		//收到准备就绪回应
		printf("want to store in to: %s\n", newpath);
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
			const char* reply = "receiveready";
			send(socket, reply, 13, 0);
			n = 1;
			while (n > 0)
			{
				n = recv(socket, recv_buffer, MAXLINE, 0);
				recv_buffer[n] = '\0';
				//发送ack
				send(socket, "fileack", 8, 0);
				//printf("fileack\n");
				if (strcmp(recv_buffer, "EOF") == 0 || n <= 0)
				{
					break;
				}
				//写文件
				fwrite(recv_buffer, 1, n, f);
			}
			fclose(f);
			send(socket, "downloadend", 12, 0);
			printf("downloadend: %s\n", newpath);
		}
	}
	else
	{
		//错误
		printf("file missed during downloading!\n");
	}
}

void buildgroup(char* gname)
{
	//创建群聊
	int socket;
	if (getConnect(&socket) == ERROR)
	{
		return;
	}
	char ownerid[IDLEN];
	int sem_id;
	P(sem_id, 0);
	clientShm* addr;
	addr = &clientshm;
	strcpy(ownerid, addr->selfinfo.id);
	addr = NULL;
	V(sem_id, 0);
	char send_buffer[MAXLINE], recv_buffer[MAXLINE];
	int n;
	sprintf(send_buffer, "buildgroup %s %s", ownerid, gname);
	//发送请求
	send(socket, send_buffer, strlen(send_buffer), 0);
	//接收回应
	n = recv(socket, recv_buffer, MAXLINE, 0);
	recv_buffer[n] = '\0';
//	printf("receive: %s\n", recv_buffer);
	int start = 0, end;
	//分析回应
	end = parseMsgType(recv_buffer);
	char type[20], reply[MAXLINE - 20];
	substring(type, recv_buffer, start, end);
	substring(reply, recv_buffer, end + 1, MAXLINE - 1);

	if (strcmp(type, "createsuccessfully") == 0)
	{
		//创建成功，写入gid信息
		int sem_id;
		getClientSem(&sem_id);
		P(sem_id, 0);
		clientShm* addr;
		addr = &clientshm;
		addIntoGroup(&(addr->selfinfo), reply);
		addr = NULL;
		V(sem_id, 0);
		printf("creat group sucessfully! gid is: %s\n", reply);

	}
	else
	{
		//创建失败
		printf("creat group error!: %s\n", type);
	}
	close(socket);
	return;

}

void addmember(char* gid, char* uid)
{
	//添加群聊成员
	//建立连接
	int socket;
	if (getConnect(&socket) == ERROR)
	{
		return;
	}

	char send_buffer[MAXLINE], recv_buffer[MAXLINE];
	int n;
	sprintf(send_buffer, "addmember %s %s", gid, uid);
	//发送请求
	send(socket, send_buffer, strlen(send_buffer), 0);

	n = recv(socket, recv_buffer, MAXLINE, 0);
	recv_buffer[n] = '\0';
	//分析回应
	int start = 0, end;
	end = parseMsgType(recv_buffer);
	char type[20];
	substring(type, recv_buffer, start, end);

	if (strcmp(type, "addsuccessfully") == 0)
	{
		//加入成功
		printf("add member into group sucessfully! gid: %s, uid: %s\n", gid, uid);
	}
	else
	{
		printf("add member into group error!: %s\n", type);
	}
	close(socket);
	return;
}

void groupchat(char* gid, char* msg)
{
	//群聊
	int socket;
	if (getConnect(&socket) == ERROR)
	{
		return;
	}
	char send_buffer[MAXLINE];
	char recv_buffer[MAXLINE];
	char from[IDLEN];
	int sem_id;
	getClientSem(&sem_id);
	P(sem_id, 0);
	clientShm* addr;
	addr = &clientshm;
	strcpy(from, addr->selfinfo.id);
	addr = NULL;
	V(sem_id, 0);
	//发送请求
	sprintf(send_buffer, "groupchat %s %s %s", gid, from, msg);
	printf("\nsend: %s\n", send_buffer);
	if (send(socket, send_buffer, strlen(send_buffer), 0) < 0)
	{
		printf("send massage error: %s!\n", send_buffer);
		close(socket);
		return;
	}
	char path[64];
	sprintf(path, "%sgroup%s_%s.msg", history, gid, from);
	FILE* f = fopen(path, "a");
	fprintf(f, "%s %s\n", from, msg);
	fclose(f);
	int n;
	//接收回应
	n = recv(socket, recv_buffer, MAXLINE, 0);
	recv_buffer[n] = '\0';
	printf("receive: %s\n", recv_buffer);
	int start = 0, end;
	end = parseMsgType(recv_buffer + start);
	char type[20];
	substring(type, recv_buffer, start, start + end);
	if (strcmp(type, "groupchatreply") == 0)
	{
		//成功发送
		printf("send to group successfully!\n");
	}
	else
	{
		printf("group chat error: %s\n", type);
	}
	close(socket);
	return;
}