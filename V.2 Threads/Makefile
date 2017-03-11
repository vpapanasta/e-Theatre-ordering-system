server client : server_t.o client_t.o 
	gcc -o server_t server_t.o -lrt -lpthread
	gcc -o client_t client_t.o -lrt -lpthread

server.o : server_t.c
	gcc -c -o server_t.o server_t.c -lrt -lpthread

client.o : client_t.c
	gcc -c -o client_t.o client_t.c -lrt -lpthread

