server client : server.o client.o 
	gcc -o server server.o -lrt -lpthread
	gcc -o client client.o -lrt -lpthread

server.o : server.c
	gcc -c -o server.o server.c -lrt -lpthread

client.o : client.c
	gcc -c -o client.o client.c -lrt -lpthread

