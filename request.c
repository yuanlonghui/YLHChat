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
	//ֻ�ܱ����ļ��еĺ������õ�һ�������������ļ�·���л���ļ���
	int i;
	int len = strlen(src);
	//��ǰ������һ���ָ���
	for (i = len - 1; i >= 0; i--)
	{
		if (src[i] == '/' || src[i] == '\\')
		{
			break;
		}
	}
	i++;
	//Ҫôi = -1 , Ҫô���ڷָ�����
	substring(dst, src, i, len);
}

void login(char* info)
{
	//��¼����
	//���ӷ�����
	int socket ;
	if (getConnect(&socket) == ERROR)
	{
		return;
	}
	char send_buffer[MAXLINE];
	char recv_buffer[MAXLINE];
	//���͵�¼����
	sprintf(send_buffer, "login %s", info);
	if (send(socket, send_buffer, strlen(send_buffer), 0) < 0)
	{
		printf("send login massage error: %s!\n", send_buffer);
		close(socket);
		return;
	}
	//���ջ�Ӧ
	int n;
	n = recv(socket, recv_buffer, MAXLINE, 0);
	recv_buffer[n] = '\0';
	//printf("receive: %s\n", recv_buffer);
	int start = 0, end;
	//������Ӧ
	end = parseMsgType(recv_buffer);
	char type[20], reply[MAXLINE - 20];
	substring(type, recv_buffer, start, end);
	substring(reply, recv_buffer, end + 1, MAXLINE - 1);
	if (strcmp(type, "loginreply") == 0)
	{
		//��¼�ɹ�
		//��reply�н���������user��Ϣ
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
		//��¼ʧ��
		printf("login error!: %s\n", type);
		
	}
	//�ر�socket
	close(socket);
	return;
}

void regis(char* info)
{
	//ע������
	//���ӷ�����
	int socket;
	if (getConnect(&socket) == ERROR)
	{
		return;
	}
	char send_buffer[MAXLINE];
	char recv_buffer[MAXLINE];
	sprintf(send_buffer, "register %s", info);
	//����ע������
	if (send(socket, send_buffer, strlen(send_buffer), 0) < 0)
	{
		printf("send register massage error: %s!\n", send_buffer);
		close(socket);
		return;
	}
	int n;
	//���ջ�Ӧ
	n = recv(socket, recv_buffer, MAXLINE, 0);
	recv_buffer[n] = '\0';
//	printf("receive: %s\n", recv_buffer);
	int start = 0, end;
	//������Ӧ
	end = parseMsgType(recv_buffer);
	char type[20], reply[MAXLINE - 20];
	substring(type, recv_buffer, start, end);
	substring(reply, recv_buffer, end + 1, MAXLINE - 1);

	if (strcmp(type, "registerreply") == 0)
	{
		//ע��ɹ�
		//��reply�н���������user��Ϣ
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
		//ע��ʧ��
		printf("register error!: %s\n", type);
		
	}
	close(socket);
	return;
}

void keepalive()
{
	//keepalive���󣬷���keepalive��ͬʱͨ������������������Ϣ�ı��Լ��û�����Ϣ
	// 	   ��˻��������Ϣ����
	//��������
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
	//����keepalive����
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
		//���ջ�Ӧ��ֱ���յ�keepaliveend
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
			//������Ӧ
			end = parseMsgType(recv_buffer + start);
			substring(type, recv_buffer, start, start + end);
			substring(msg, recv_buffer, start + end + 1, strlen(recv_buffer));
			if (strcmp(type, "msg") == 0)
			{
				//��Ϣ���ݻ�Ӧ
				//��Ҫ��������ͨ��Ϣ����Ⱥ����Ϣ
				//ÿ���˵���ʷ�����ļ���Ϊ����
				//���������ļ���Ϊ "%s_%s.msg",id,id��һ��id�Ǻ�˭�������¼���ڶ���id���Լ�
				//Ⱥ���ļ�Ϊ "group%s_%s.msg",id,id��һ��id��Ⱥid���ڶ���id���Լ�
				//�����¼�ļ��еĸ�ʽ��Ϊ��"%s %s",id,msg��id�Ƿ����ˣ�msg������
				char path_msg[64];
				char type[IDLEN];
				int start = 0, end;
				end = parseMsgType(msg + start);
				substring(type, msg, start, start + end);
				start += end + 1;

				if (strcmp(type, "gmsg") == 0)
				{
					//Ⱥ����Ϣ
					char gid[IDLEN], msgcontent[MAXLINE];
					end = parseMsgType(msg + start);
					substring(gid, msg, start, start + end);
					start += end + 1;
					substring(msgcontent, msg, start, strlen(msg));

					char path[64];
					sprintf(path, "%sgroup%s_%s.msg", history, gid, addr->selfinfo.id);
					FILE* f = fopen(path, "a");
					//����Ⱥ����Ϣ
					fprintf(f, "%s\n", msgcontent);
					fclose(f);
					//������Ϣ����
					Push(addr->selfinfo.msgQ, msg);
				}
				else
				{
					//˽����Ϣ
					char from[IDLEN];
					substring(from, msg, 0, IDLEN);
					sprintf(path_msg, "%s%s_%s.msg", history, from, addr->selfinfo.id);
					FILE* f = fopen(path_msg, "a");
					//����
					fprintf(f, "%s\n", msg);
					fclose(f);
					//������Ϣ����
					Push(addr->selfinfo.msgQ, msg);
				}
			}
			else if (strcmp(type, "file") == 0)
			{
				//�ļ���Ϣ�������ļ����еȴ�����
				Push(addr->selfinfo.fileQ, msg);
				fprintf(f_file, "%s\n", msg);
			}
			else if (strcmp(type, "infosync") == 0)
			{
				//�û���Ϣ�仯�����½���
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
				//����
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
	//˽��
	//��������
	int socket;
	if (getConnect(&socket) == ERROR)
	{
		return;
	}
	char send_buffer[MAXLINE];
	char recv_buffer[MAXLINE];
	char from[IDLEN];
	//��ȡ�����û���Ϣ
	int sem_id;
	getClientSem(&sem_id);
	P(sem_id, 0);
	clientShm* addr;
	addr = &clientshm;
	strcpy(from, addr->selfinfo.id);
	addr = NULL;
	V(sem_id, 0);
	sprintf(send_buffer, "singlechat %s %s %s",to, from, info);
	//����˽����Ϣ
	printf("\nsend: %s\n", send_buffer);
	if (send(socket, send_buffer, strlen(send_buffer), 0) < 0)
	{
		printf("send massage error: %s!\n", send_buffer);
		close(socket);
		return;
	}
	//������Ϣ
	char path[64];
	sprintf(path, "%s%s_%s.msg", history, to, from);
	FILE* f = fopen(path, "a");
	fprintf(f, "%s %s\n", from, info);
	fclose(f);
	//���ܻ�Ӧ
	int n;
	n = recv(socket, recv_buffer, MAXLINE, 0);
	recv_buffer[n] = '\0';
	printf("receive: %s\n", recv_buffer);
	int start = 0, end;
	//������Ӧ
	end = parseMsgType(recv_buffer + start);
	char type[20];
	substring(type, recv_buffer, start, start + end);
	if (strcmp(type, "singlechatreply") == 0)
	{
		//�ɹ�
		printf("send to mailbox successfully!\n");
	}
	else
	{
		//ʧ��
		printf("single chat error: %s\n", type);
	}
	close(socket);
	return;
}

void filetrans(char* to, char* path)
{
	//�ļ�����
	//����ļ��Ƿ���Դ�
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
	//�����ļ���������
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
		//�ӵ��ļ����Կ�ʼ��������
		while (n > 0)
		{
			int m;
			n = 0;
			m = fread(send_buffer, 1, MAXLINE - 1, f);
			if (m != 0)
			{
				//�����ļ�����
				send(socket, send_buffer, m, 0);
			}
			else
			{
				//�ļ�����
				//һ���ļ��г��ֵ�����EOF����ô������ֹ�ļ�����
				send(socket, "EOF", 4, 0);
			}
			n = recv(socket, recv_buffer, MAXLINE, 0);
			recv_buffer[n] = '\0';
			printf("receive: %d, %s\n", n, recv_buffer);
			if (strcmp(recv_buffer, "fileack") == 0)
			{
				//���ܵ�ack
				continue;
			}
			else if(strcmp(recv_buffer, "filetransend") == 0)
			{
				//���ͽ���ȷ��
				break;
			}
		}
	}
	else
	{
		//�ļ��������
		printf("filetrans error: %s\n", type);
	}
	fclose(f);
	close(socket);
	return;
}

void downloadfile(char* info)
{
	//�����ļ�
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
	//��������
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
		//�յ�׼��������Ӧ
		printf("want to store in to: %s\n", newpath);
		FILE* f = fopen(newpath, "w");
		if (f == NULL)
		{
			//�ظ�����ʧ��
			const char* reply = "BUFFERERROR";
			send(socket, reply, 11, 0);
		}
		else
		{
			//�����ļ�����
			const char* reply = "receiveready";
			send(socket, reply, 13, 0);
			n = 1;
			while (n > 0)
			{
				n = recv(socket, recv_buffer, MAXLINE, 0);
				recv_buffer[n] = '\0';
				//����ack
				send(socket, "fileack", 8, 0);
				//printf("fileack\n");
				if (strcmp(recv_buffer, "EOF") == 0 || n <= 0)
				{
					break;
				}
				//д�ļ�
				fwrite(recv_buffer, 1, n, f);
			}
			fclose(f);
			send(socket, "downloadend", 12, 0);
			printf("downloadend: %s\n", newpath);
		}
	}
	else
	{
		//����
		printf("file missed during downloading!\n");
	}
}

void buildgroup(char* gname)
{
	//����Ⱥ��
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
	//��������
	send(socket, send_buffer, strlen(send_buffer), 0);
	//���ջ�Ӧ
	n = recv(socket, recv_buffer, MAXLINE, 0);
	recv_buffer[n] = '\0';
//	printf("receive: %s\n", recv_buffer);
	int start = 0, end;
	//������Ӧ
	end = parseMsgType(recv_buffer);
	char type[20], reply[MAXLINE - 20];
	substring(type, recv_buffer, start, end);
	substring(reply, recv_buffer, end + 1, MAXLINE - 1);

	if (strcmp(type, "createsuccessfully") == 0)
	{
		//�����ɹ���д��gid��Ϣ
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
		//����ʧ��
		printf("creat group error!: %s\n", type);
	}
	close(socket);
	return;

}

void addmember(char* gid, char* uid)
{
	//���Ⱥ�ĳ�Ա
	//��������
	int socket;
	if (getConnect(&socket) == ERROR)
	{
		return;
	}

	char send_buffer[MAXLINE], recv_buffer[MAXLINE];
	int n;
	sprintf(send_buffer, "addmember %s %s", gid, uid);
	//��������
	send(socket, send_buffer, strlen(send_buffer), 0);

	n = recv(socket, recv_buffer, MAXLINE, 0);
	recv_buffer[n] = '\0';
	//������Ӧ
	int start = 0, end;
	end = parseMsgType(recv_buffer);
	char type[20];
	substring(type, recv_buffer, start, end);

	if (strcmp(type, "addsuccessfully") == 0)
	{
		//����ɹ�
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
	//Ⱥ��
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
	//��������
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
	//���ջ�Ӧ
	n = recv(socket, recv_buffer, MAXLINE, 0);
	recv_buffer[n] = '\0';
	printf("receive: %s\n", recv_buffer);
	int start = 0, end;
	end = parseMsgType(recv_buffer + start);
	char type[20];
	substring(type, recv_buffer, start, start + end);
	if (strcmp(type, "groupchatreply") == 0)
	{
		//�ɹ�����
		printf("send to group successfully!\n");
	}
	else
	{
		printf("group chat error: %s\n", type);
	}
	close(socket);
	return;
}