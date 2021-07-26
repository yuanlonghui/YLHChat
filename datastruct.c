#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<sys/types.h>  
#include<sys/socket.h>  
#include<netinet/in.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>

#include "datastruct.h"
#include "msgQueue.h"


const char * usermsg = "./usermsg/" ;
const char * userfile = "./userfile/" ;
const char * userinfo = "./userinfo/" ;
const char * userbitmap = "./userinfo/bitmap" ;
const char* groupinfo = "./groupinfo/";
const char* groupbitmap = "./groupinfo/bitmap";
const char * tempfile = "./tempfile/";
const char* history = "./history/";
const char* download = "./download/";
struct sockaddr_in servaddr;
serverShm servershm;
clientShm clientshm;
int client_sem_key = CLIENT_SEM_KEY;

retState getUser(user* u)
{
	char filepath[64] ;
	memset(filepath, 0 , 64) ;
	strcat(filepath, userinfo) ;
	strcat(filepath, u->id) ;
	FILE * f ;
	if(!(f = fopen(filepath, "r")))
	{
		printf("load user information from %s error!\n", filepath) ;
		return ERROR ;
	}
	else
	{
		char id[IDLEN];
		fscanf(f, "%s", id);
		if (strcmp(id, u->id) != 0)
		{
			printf("error initial file for: %s\n", u->id);
		}
		fscanf(f, "%s", u->name) ;
		fscanf(f, "%s", u->password);
		fscanf(f, "%d", &(u->group_num));
		int i ;
		u->gids = (char **)malloc(u->group_num * sizeof(char *)) ;
		if(u->gids == NULL)
		{
			printf("malloc memory for %s\'s group error\n", u->id) ;
			fclose(f) ;
			return ERROR ;
		}
		memset(u->gids, 0, u->group_num * sizeof(char *)) ;
		for(i = 0 ; i < u->group_num; i++)
		{
			u->gids[i] = (char *) malloc(IDLEN) ;
			if(u->gids[i] == NULL)
			{
				printf("malloc memory for %s\'s group error\n", u->id) ;
				freeuser(u) ;
				fclose(f) ;
				return ERROR ;
			}
			fscanf(f, "%s", u->gids[i]) ;
		}
		
		fscanf(f, "%d", &(u->friends_num)) ;
		u->friends = (char **)malloc(u->friends_num * sizeof(char *)) ;
		if(u->friends == NULL)
		{
			printf("malloc memory for %s\'s friends error\n", u->id) ;
			fclose(f) ;
			return ERROR ;
		}
		memset(u->friends, 0, u->friends_num * sizeof(char *)) ;
		for(i = 0 ; i < u->friends_num; i++)
		{
			u->friends[i] = (char *) malloc(IDLEN) ;
			if(u->friends[i] == NULL)
			{
				printf("malloc memory for %s\'s friends error\n", u->id) ;
				freeuser(u) ;
				fclose(f) ;
				return ERROR ;
			}
			fscanf(f, "%s", u->friends[i]) ;
		}
		return OK ;
	}
}

retState storeUser(user *u)
{
	char path[20] ;
	memset(path, 0, 20) ;
	strcat(path, userinfo) ;
	strcat(path, u->id) ;
	FILE * f ;
	f = fopen(path, "w") ;
	if(f == NULL)
	{
		printf("open file %s for wirte in user infomation error\n", u->id) ;
		return ERROR ;
	}
	else
	{
		fprintf(f, "%s ", u->id);
		fprintf(f, "%s ", u->name) ;
		fprintf(f, "%s ", u->password) ;
		fprintf(f, "%d ", u->group_num) ;
		int i ; 
		for(i = 0 ; i < u->group_num ; i++)
		{
			fprintf(f, "%s ", u->gids[i]) ;
		}
		fprintf(f, "%d ", u->friends_num) ;
		for(i = 0 ; i < u->friends_num ; i++)
		{
			fprintf(f, "%s ", u->friends[i]) ;
		}
		fclose(f) ;
		return OK ;
	}
}

retState parseUserFromStr(user* u, char* info)
{
	char buffer[32];
	int start = 0;
	int end;
	end = parseMsgType(info + start);
	//printf("start: %d, end: %d\n", start, end);
	substring(u->id, info, start, start + end);
	start += end + 1;
	end  = parseMsgType(info + start);
	//printf("start: %d, end: %d\n", start, end);
	substring(u->name, info, start, start + end);
	start += end + 1;
	end = parseMsgType(info + start);
	//printf("start: %d, end: %d\n", start, end);
	substring(u->password, info, start, start + end);
	start += end + 1;
	end = parseMsgType(info + start);
	//printf("start: %d, end: %d\n", start, end);
	substring(buffer, info, start, start + end);
	start += end + 1;
	int group_num;
	sscanf(buffer, "%d", &group_num);
	int i;
	u->group_num = group_num;
	u->gids = (char**)malloc(u->group_num * sizeof(char*));
	if (u->gids == NULL)
	{
		printf("malloc memory for %s\'s group error\n", u->id);
		return ERROR;
	}
	memset(u->gids, 0, u->group_num * sizeof(char*));
	for (i = 0; i < u->group_num; i++)
	{
		u->gids[i] = (char*)malloc(IDLEN);
		if (u->gids[i] == NULL)
		{
			printf("malloc memory for %s\'s group error\n", u->id);
			freeuser(u);
			return ERROR;
		}
		end = parseMsgType(info + start);
		substring(u->gids[i], info, start, start + end);
		start += end + 1;
	}

	end = parseMsgType(info + start);
	substring(buffer, info, start, start + end);
	start += end + 1;
	int friends_num;
	sscanf(buffer, "%d", &friends_num);
	u->friends_num = friends_num;
	u->friends = (char**)malloc(u->friends_num * sizeof(char*));
	if (u->friends == NULL)
	{
		printf("malloc memory for %s\'s friends error\n", u->id);
		return ERROR;
	}
	memset(u->friends, 0, u->friends_num * sizeof(char*));
	for (i = 0; i < u->friends_num; i++)
	{
		u->friends[i] = (char*)malloc(IDLEN);
		if (u->friends[i] == NULL)
		{
			printf("malloc memory for %s\'s friends error\n", u->id);
			freeuser(u);
			return ERROR;
		}
		end = parseMsgType(info + start);
		substring(u->friends[i], info, start, start + end);
		start += end + 1;
	}
	return OK;

}

retState storeUserToStr(user* u, char* info)
{
	char buffer[32];
	info[0] = '\0';
	strcat(info, u->id);
	strcat(info," ");
	strcat(info, u->name);
	strcat(info, " ");
	strcat(info, u->password);
	strcat(info, " ");
	sprintf(buffer, "%d ", u->group_num);
	strcat(info, buffer);
	int i;
	for (i = 0; i < u->group_num; i++)
	{
		strcat(info, u->gids[i]);
		strcat(info, " ");
	}
	sprintf(buffer, "%d ", u->friends_num);
	strcat(info, buffer);
	for (i = 0; i < u->friends_num; i++)
	{
		strcat(info, u->friends[i]);
		strcat(info, " ");
	}
	return OK;
}

retState freeuser(user *u)
{
	int i ; 
	for(i = 0 ; i < u->group_num && u->gids != NULL; i++)
	{
		if(u->gids[i] != NULL)
		{
			free(u->gids[i]) ;
			u->gids[i] = NULL ;
		}
	}
	if(u->gids != NULL)
	{
		free(u->gids) ;
		u->gids = NULL ;
	}
	
	for(i = 0 ; i < u->friends_num && u->friends != NULL; i++)
	{
		if(u->friends[i] != NULL)
		{
			free(u->friends[i]) ;
			u->friends[i] = NULL ;
		}
	}
	if(u->friends != NULL)
	{
		free(u->friends) ;
		u->friends = NULL ;
	}
	if (u->msgQ != NULL)
	{
		DestoryQueue(u->msgQ);
		u->msgQ = NULL;
	}
	if (u->fileQ != NULL)
	{
		DestoryQueue(u->fileQ);
		u->fileQ = NULL;
	}

	return OK ;
}

void P(int sem_id, int sem_num)
{
	struct sembuf xx ;
	xx.sem_num = sem_num ;
	xx.sem_op = -1 ;
	xx.sem_flg =0 ;
	semop(sem_id, &xx, 1) ;
}

void V(int sem_id, int sem_num)
{
	struct sembuf xx ;
	xx.sem_num = sem_num ;
	xx.sem_op = 1 ;
	xx.sem_flg =0 ;
	semop(sem_id, &xx, 1) ;
}

retState getUserInforBitmap(int * bitmap)
{
	FILE * f ;
	f = fopen(userbitmap, "r") ;
	if(f == NULL)
	{
		printf("open user bitmap error\n") ;
		return ERROR ;
	}
	else
	{
		int i ; 
		for(i = 0 ; i < MAX_USER ; i++)
		{
			fscanf(f, "%d", &(bitmap[i])) ;
		}
		return OK ;
	}
}

retState initialBitmap()
{
	FILE * f ;
	f = fopen(userbitmap, "w") ;
	if(f == NULL)
	{
		printf("initial user bitmap error\n") ;
		return ERROR ;
	}
	else
	{
		int i ; 
//		fprintf(f, "%d ", 1) ;
		for(i = 0 ; i < MAX_USER ; i++)
		{
			fprintf(f, "%d ", 0) ;
		}
		return OK ;
	}
}


int getUserNewId(int * bitmap)
{
	int i ;
	for(i = 1 ; i < MAX_USER ; i++)
	{
		if(bitmap[i] == 0)
		{
			return i ;
		}
	}
	return -1 ;
}

retState initial(serverShm * shmptr)
{
	printf("initial\n");
	retState s;
	s = getUserInforBitmap(shmptr->user_bitmap) ;
	if (s == ERROR)
	{
		exit(0);
	}
	if (shmptr->user_bitmap[0] == 0)
	{
		initialRootUser(&(shmptr->users[0]));
		shmptr->user_bitmap[0] = 1;
		storeUser(&(shmptr->users[0]));
		storeUserInfoBitmap(shmptr->user_bitmap);
	}
	else 
	{
		sprintf((shmptr->users[0]).id, "%.4d", 0);
		getUser(&(shmptr->users[0]));
		shmptr->users[0].msgQ = CreateQueue();
		shmptr->users[0].fileQ = CreateQueue();
	}
	s = getGroupInforBitmap(shmptr->group_bitmap);
	if (s == ERROR)
	{
		exit(0);
	}
	int i ;
	for(i = 1 ; i < MAX_USER ; i++)
	{
		if(shmptr->user_bitmap[i] == 1) 
		{
			sprintf((shmptr->users[i]).id, "%.4d", i) ;
			getUser(&(shmptr->users[i])) ;
			shmptr->users[i].msgQ = CreateQueue();
			shmptr->users[i].fileQ = CreateQueue();
		}
	}
	for (i = 0; i < MAX_GROUP; i++)
	{
		if (shmptr->group_bitmap[i] == 1)
		{
			sprintf((shmptr->groups[i]).id, "%.4d", i);
			getGroup(&(shmptr->groups[i]));
		}
	}
	return OK ;
}

retState initialRootUser(user * u)
{
	printf("initialRootUser\n");
	sprintf(u->id, "0000") ;
	sprintf(u->name, "admin") ;
	sprintf(u->password, "root") ;
	u->msgQ = CreateQueue();
	u->fileQ = CreateQueue();
	return OK;
}

void getServerSem(int * sem_id)
{
	//一个信号量，访问共享主存区域
	*sem_id = semget(SEVER_SEM_KEY, 1, 0666|IPC_CREAT) ;
	return ;
}

void getClientSem(int * sem_id)
{
	//一个信号量，访问共享主存区域
	*sem_id = semget(client_sem_key, 1, 0666|IPC_CREAT) ;
	return ;
}

int parseMsgType(const char* msg)
{
	int i;
	for (i = 0; i < MAXLINE; i++)
	{
		if (msg[i] == ' ' || msg[i] == '\0')
		{
			return i;
		}
	}
	return -1;
}

char* substring(char* dst, const char* src, int begin, int end)
{
	int i;
	if (begin > end || dst == NULL || src == NULL || begin < 0)
	{
		return NULL;
	}
	else
	{
		for (i = begin; i < end && src[i] != '\0'; i++)
		{
			dst[i - begin] = src[i];
		}
		dst[i - begin] = '\0';
		return dst;
	}
}

void initClientConfig()
{
	FILE* f;
	f = fopen("config.txt", "r");
	if (f == NULL)
	{
		printf("read config file error!\n");
		exit(0);
	}

	char strip[16];
	short port;

	fscanf(f, "%s", strip);
	fscanf(f, "%hd", &port);

	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(port);
	servaddr.sin_addr.s_addr = inet_addr(strip);
	return;
}

retState getConnect(int *_socket)
{
	if ((*_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1)
	{
		printf("creat socket for client error!\n");
		return ERROR;
	}
	if (connect(*_socket, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0)
	{
		printf("connect error!");
		return ERROR;
	}
	return OK;
}

retState getGroupInforBitmap(int* bitmap)
{
	FILE* f;
	f = fopen(groupbitmap, "r");
	if (f == NULL)
	{
		printf("open group bitmap error\n");
		return ERROR;
	}
	else
	{
		int i;
		for (i = 0; i < MAX_USER; i++)
		{
			fscanf(f, "%d", &(bitmap[i]));
		}
		return OK;
	}
}

retState initialGroupBitmap()
{
	FILE* f;
	f = fopen(groupbitmap, "w");
	if (f == NULL)
	{
		printf("initial group bitmap error\n");
		return ERROR;
	}
	else
	{
		int i;
		for (i = 0; i < MAX_USER; i++)
		{
			fprintf(f, "%d ", 0);
		}
		return OK;
	}
}

retState parseGroupFromStr(group* g, char* info)
{
	char buffer[32];
	int start = 0;
	int end;
	end = parseMsgType(info + start);
	substring(g->onwerid, info, start, start + end);
	start += end + 1;

	end = parseMsgType(info + start);
	substring(g->id, info, start, start + end);
	start += end + 1;

	end = parseMsgType(info + start);
	substring(g->name, info, start, start + end);
	start += end + 1;

	end = parseMsgType(info + start);
	substring(buffer, info, start, start + end);
	start += end + 1;
	int user_num;
	sscanf(buffer, "%d", &user_num);

	g->user_num = user_num;
	int i;
	g->users = (char**)malloc(g->user_num * sizeof(char*));
	if (g->users == NULL)
	{
		printf("malloc memory for %s\'s users error\n", g->id);
		return ERROR;
	}
	memset(g->users, 0, g->user_num * sizeof(char*));
	for (i = 0; i < g->user_num; i++)
	{
		g->users[i] = (char*)malloc(IDLEN);
		if (g->users[i] == NULL)
		{
			printf("malloc memory for %s\'s group error\n", g->id);
			freeGroup(g);
			return ERROR;
		}
		end = parseMsgType(info + start);
		substring(g->users[i], info, start, start + end);
		start += end + 1;
	}
}

retState storeGroupToStr(group* g, char* info)
{
	char buffer[32];
	info[0] = '\0';
	strcat(info, g->onwerid);
	strcat(info, " ");
	strcat(info, g->id);
	strcat(info, " ");
	strcat(info, g->name);
	strcat(info, " ");
	sprintf(buffer, "%d ", g->user_num);
	strcat(info, buffer);
	int i;
	for (i = 0; i < g->user_num ; i++)
	{
		strcat(info, g->users[i]);
		strcat(info, " ");
	}
	return OK;
}

retState getGroup(group* g)
{
	char filepath[64];
	memset(filepath, 0, 64);
	strcat(filepath, groupinfo);
	strcat(filepath, g->id);
	FILE* f;
	if (!(f = fopen(filepath, "r")))
	{
		printf("load group information from %s error!\n", filepath);
		return ERROR;
	}
	else
	{
		fscanf(f, "%s", g->onwerid);
		char id[IDLEN];
		fscanf(f, "%s", id);
		if (strcmp(id, g->id) != 0)
		{
			printf("error initial file for: %s\n", g->id);
		}
		fscanf(f, "%s", g->name);

		fscanf(f, "%d", &(g->user_num));

		//printf("%d\n", g->user_num);

		int i;
		g->users = (char**)malloc(g->user_num * sizeof(char*));
		if (g->users == NULL)
		{
			printf("malloc memory for %s\'s users error\n", g->id);
			fclose(f);
			return ERROR;
		}
		memset(g->users, 0, g->user_num * sizeof(char*));
		for (i = 0; i < g->user_num; i++)
		{
			g->users[i] = (char*)malloc(IDLEN);
			if (g->users[i] == NULL)
			{
				printf("malloc memory for %s\'s users error\n", g->id);
				freeGroup(g);
				fclose(f);
				return ERROR;
			}
			fscanf(f, "%s", g->users[i]);
		}
	}
}


retState storeGroup(group* g)
{
	char path[20];
	memset(path, 0, 20);
	strcat(path, groupinfo);
	strcat(path, g->id);
	FILE* f;
	f = fopen(path, "w");
	if (f == NULL)
	{
		printf("open file %s for wirte in user infomation error\n", g->id);
		return ERROR;
	}
	else
	{
		fprintf(f, "%s ", g->onwerid);
		fprintf(f, "%s ", g->id);
		fprintf(f, "%s ", g->name);
		fprintf(f, "%d ", g->user_num);
		int i;
		for (i = 0; i < g->user_num; i++)
		{
			fprintf(f, "%s ", g->users[i]);
		}
		fclose(f);
		return OK;
	}
}

retState freeGroup(group* g)
{
	int i;
	for (i = 0; i < g->user_num; i++)
	{
		if (g->users[i] != NULL)
		{
			free(g->users[i]);
			g->users[i] = NULL;
		}
	}
	return OK;
}

int getGroupNewId(int* bitmap)
{
	int i;
	for (i = 0; i < MAX_GROUP; i++)
	{
		if (bitmap[i] == 0)
		{
			return i;
		}
	}
	return -1;
}

retState addIntoGroup(user* u, char* gid)
{
	int i;
	for (i = 0; i < u->group_num; i++)
	{
		if (strcmp(u->gids[i], gid) == 0)
		{
			return OK;
		}
	}
	int size = u->group_num + 1;
	char** temp = (char**)malloc(size * sizeof(char*));
	u->group_num = size;
	for (i = 0; i < size - 1; i++)
	{
		temp[i] = u->gids[i];
	}
	temp[size - 1] = (char*)malloc(IDLEN);
	strcpy(temp[size - 1], gid);
	if (u->gids != NULL)
	{
		free(u->gids);
	}
	u->gids = temp;
	return OK;
}

retState addUser(group* g, char* uid)
{
	int i;
	for (i = 0; i < g->user_num; i++)
	{
		if (strcmp(g->users[i], uid) == 0)
		{
			return OK;
		}
	}
	char size = g->user_num + 1;
	char** temp = (char**)malloc(size * sizeof(char*));
	g->user_num = size;
	for (i = 0; i < size - 1; i++)
	{
		temp[i] = g->users[i];
	}
	temp[size - 1] = (char*)malloc(IDLEN);
	strcpy(temp[size - 1], uid);
	if (g->users != NULL)
	{
		free(g->users);
	}
	g->users = temp;
	return OK;
}


retState storeUserInfoBitmap(int* bitmap)
{
	FILE* f;
	f = fopen(userbitmap, "w");
	if (f == NULL)
	{
		printf("store user bitmap error\n");
		return ERROR;
	}
	else
	{
		int i;
		for (i = 0; i < MAX_USER; i++)
		{
			fprintf(f, "%d ", bitmap[i]);
		}
		return OK;
	}
}

retState storeGroupInfoBitmap(int* bitmap)
{
	FILE* f;
	f = fopen(groupbitmap, "w");
	if (f == NULL)
	{
		printf("store group bitmap error\n");
		return ERROR;
	}
	else
	{
		int i;
		for (i = 0; i < MAX_USER; i++)
		{
			fprintf(f, "%d ", bitmap[i]);
		}
		return OK;
	}
}

