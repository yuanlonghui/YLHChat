#ifndef _MSG_Q_
#define _MSG_Q_ 1

//这个文件定义了消息队列和一些对队列操作的方法
//比较简单

#define MAX 1000
typedef struct {
	char* data[MAX];
	int len;
	int front;
	int rear;
} queue;
//创建队列，返回地址，动态申请的
queue* CreateQueue();
//销毁队列，释放动态空间
void DestoryQueue(queue* obj);
//申请内存，并将data中的内容写入
void Push(queue* obj, char* data);
//释放front指向的动态内存
void Pop(queue* obj);
//最早入队列的消息指针
char* Front(const queue* obj);
//最晚入队列的消息指针
char* Rear(const queue* obj);
//队列长度
int QueueLenth(const queue* obj);
//是否为空
int Empty(const queue* obj);
//是否为满
int Full(const queue* obj);
//前端索引
int frontIndex(const queue* obj);
//下一个索引
int nextIndex(const queue* obj);
//求index对应的正真的缩影
int QueueMod(int index);


#endif