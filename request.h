#ifndef _RQ_H_
#define _RQ_H_
//定义了一系列的请求，和服务器进行交互
//登录
void login(char * info) ;
//注册
void regis(char * info) ;
//keepalive
void keepalive() ;
//私聊
void singlechat(char* to, char* info);
//文件传输
void filetrans(char* to, char* path);
//文件下载
void downloadfile(char* info);
//创建群聊
void buildgroup(char* gname);
//添加群聊成员
void addmember(char* gid, char* uid);
//发送群聊消息
void groupchat(char* gid, char* msg);




#endif
