compila:
	gcc client.c -o client -lpthread
	gcc server.c -o server -lpthread

roda_servidor:
	./server

roda_cliente:
	./client 127.0.0.1 3000

