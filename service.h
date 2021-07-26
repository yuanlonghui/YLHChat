#ifndef _SER_H_
#define _SER_H_ 1

//定义类一系列的服务函数
//看名字识功能
void serve(void* _socket) ;
void login(int socket, char * loginfo) ;
void regis(int socket, char * regisInfo) ;
void keepalive(int socket, char * infor) ;
void singleChat(int socket, char * sigleChatInfo) ;
void filetrans(int socket, char * info) ;
//检查客户在线信息
void checkalive();
void sendfile(int socket, char* path);
void buildgroup(int socket, char* info);
void addmember(int socket, char* info);
void groupchat(int socket, char* info);








#endif
