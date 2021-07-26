#ifndef _MSG_Q_
#define _MSG_Q_ 1

//����ļ���������Ϣ���к�һЩ�Զ��в����ķ���
//�Ƚϼ�

#define MAX 1000
typedef struct {
	char* data[MAX];
	int len;
	int front;
	int rear;
} queue;
//�������У����ص�ַ����̬�����
queue* CreateQueue();
//���ٶ��У��ͷŶ�̬�ռ�
void DestoryQueue(queue* obj);
//�����ڴ棬����data�е�����д��
void Push(queue* obj, char* data);
//�ͷ�frontָ��Ķ�̬�ڴ�
void Pop(queue* obj);
//��������е���Ϣָ��
char* Front(const queue* obj);
//��������е���Ϣָ��
char* Rear(const queue* obj);
//���г���
int QueueLenth(const queue* obj);
//�Ƿ�Ϊ��
int Empty(const queue* obj);
//�Ƿ�Ϊ��
int Full(const queue* obj);
//ǰ������
int frontIndex(const queue* obj);
//��һ������
int nextIndex(const queue* obj);
//��index��Ӧ���������Ӱ
int QueueMod(int index);


#endif