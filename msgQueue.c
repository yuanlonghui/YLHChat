#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include "msgQueue.h"

queue* CreateQueue() {
	queue* obj = (queue*)malloc(sizeof(queue));
	if (obj == NULL)
		return NULL;
	obj->front = 0;
	obj->rear = 0;
	obj->len = 0;
	memset(obj->data, 0, sizeof(obj->data));
	return obj;
}
void DestoryQueue(queue* obj) {
	int i;
	int front = obj->front;
	int rear = obj->rear;
	for (i = front; i != rear; i = (i + 1) % MAX)
	{
		if (obj->data[i] != NULL)
		{
			free(obj->data[i]);
			obj->data[i] = NULL;
		}
	}
	free(obj);
}
void Push(queue* obj, char* data) {
	int len = obj->len + 1;
	if (len >= MAX) {
		//这里需要处理，或者留给服务器处理
		return;
	}
	obj->len++;
	int size = strlen(data);
	obj->data[obj->rear] = (char*)malloc(size + 1);
	int i;
	for (i = 0; i <= size; i++)
	{
		obj->data[obj->rear][i] = data[i];
		//'\0'也被复制
	}
	obj->rear++;
	obj->rear %= MAX;
	return;
}
void Pop(queue* obj) {
	obj->len--;
	free(obj->data[obj->front]);
	obj->data[obj->front] = NULL;
	obj->front++;
	obj->front %= MAX;
}
char* Front(const queue* obj) {
	return obj->data[obj->front];
}
char* Rear(const queue* obj) {
	int i = (obj->rear - 1 + MAX) % (MAX);
	return obj->data[i];
}
int QueueLenth(const queue* obj) {
	return obj->len;
}
int Empty(const queue* obj) {
	if (obj->len == 0)
		return 1;
	else
		return 0;
}
int Full(const queue* obj) {
	if (obj->len == MAX - 1) {
		return 1;
	}
	else
	{
		return 0;
	}
}

int frontIndex(const queue* obj)
{
	return obj->front;
}

int nextIndex(const queue* obj)
{
	return obj->rear ;
}

int QueueMod(int index)
{
	return (index + MAX) % MAX;
}