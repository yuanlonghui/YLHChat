Server: YLHChatServer.o datastruct.o service.o msgQueue.o
	gcc -o Server YLHChatServer.o datastruct.o service.o msgQueue.o

Client: YLHChatClient.o datastruct.o commandline.o request.o msgQueue.o
	gcc -o Client YLHChatClient.o datastruct.o commandline.o request.o msgQueue.o

YLHChatClient.o: YLHChatClient.c datastruct.h commandline.h
	gcc -c YLHChatClient.c
YLHChatServer.o: YLHChatServer.c datastruct.h service.h
	gcc -c YLHChatServer.c
datastruct.o: datastruct.c datastruct.h msgQueue.h
	gcc -c datastruct.c 
service.o: service.c service.h datastruct.h msgQueue.h
	gcc -c service.c
commandline.o: commandline.c commandline.h datastruct.h request.h
	gcc -c commandline.c
request.o: request.c request.h datastruct.h msgQueue.h
	gcc -c request.c 
msgQueue.o: msgQueue.c msgQueue.h
	gcc -c msgQueue.c

.PHONY: clean
clean: 
	-rm -f *.o