#ifndef _RQ_H_
#define _RQ_H_
//������һϵ�е����󣬺ͷ��������н���
//��¼
void login(char * info) ;
//ע��
void regis(char * info) ;
//keepalive
void keepalive() ;
//˽��
void singlechat(char* to, char* info);
//�ļ�����
void filetrans(char* to, char* path);
//�ļ�����
void downloadfile(char* info);
//����Ⱥ��
void buildgroup(char* gname);
//���Ⱥ�ĳ�Ա
void addmember(char* gid, char* uid);
//����Ⱥ����Ϣ
void groupchat(char* gid, char* msg);




#endif
