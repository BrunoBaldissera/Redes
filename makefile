compila:
	gcc client.c -o client -lpthread
	gcc server.c -o server -lpthread

roda_servidor:
	./server

roda_cliente:
	./client 3.23.201.37 17353

