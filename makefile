compila:
	gcc client.c -o client -Wall -lpthread
	gcc server.c -o server -Wall -lpthread

roda_servidor:
	./server

roda_cliente:
	./client 3.23.201.37 17353

