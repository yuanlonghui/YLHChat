#ifndef _SER_H_
#define _SER_H_ 1

//������һϵ�еķ�����
//������ʶ����
void serve(void* _socket) ;
void login(int socket, char * loginfo) ;
void regis(int socket, char * regisInfo) ;
void keepalive(int socket, char * infor) ;
void singleChat(int socket, char * sigleChatInfo) ;
void filetrans(int socket, char * info) ;
//���ͻ�������Ϣ
void checkalive();
void sendfile(int socket, char* path);
void buildgroup(int socket, char* info);
void addmember(int socket, char* info);
void groupchat(int socket, char* info);








#endif
