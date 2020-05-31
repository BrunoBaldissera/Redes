/*

#include<stdio.h>
#include<stdlib.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<string.h>
#include <arpa/inet.h>
#include <fcntl.h> // for open
#include <unistd.h> // for close
#include <pthread.h>

char client_message[2000];
char buffer[1024];
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
void * socketThread(void *arg)
{
  int newSocket = *((int *)arg);
  recv(newSocket , client_message , 2000 , 0);
  // Send message to the client socket 
  pthread_mutex_lock(&lock);
  char *message = malloc(sizeof(client_message)+20);
  strcpy(message,"Hello Client : ");
  strcat(message,client_message);
  strcat(message,"\n");
  strcpy(buffer,message);
  free(message);
  pthread_mutex_unlock(&lock);
  sleep(1);
  send(newSocket,buffer,13,0);
  printf("Exit socketThread \n");
  close(newSocket);
  pthread_exit(NULL);
}
int main(){
  int serverSocket, newSocket;
  struct sockaddr_in serverAddr;
  struct sockaddr_storage serverStorage;
  socklen_t addr_size;
  //Create the socket. 
  serverSocket = socket(PF_INET, SOCK_STREAM, 0);
  // Configure settings of the server address struct
  // Address family = Internet 
  serverAddr.sin_family = AF_INET;
  //Set port number, using htons function to use proper byte order 
  serverAddr.sin_port = htons(7799);
  //Set IP address to localhost 
  serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
  //Set all bits of the padding field to 0 
  memset(serverAddr.sin_zero, '\0', sizeof serverAddr.sin_zero);
  //Bind the address struct to the socket 
  bind(serverSocket, (struct sockaddr *) &serverAddr, sizeof(serverAddr));
  //Listen on the socket, with 40 max connection requests queued 
  if(listen(serverSocket,50)==0)
	printf("Listening\n");
  else
	printf("Error\n");
	pthread_t tid[60];
	int i = 0;
	while(1)
	{
		//Accept call creates a new socket for the incoming connection
		addr_size = sizeof serverStorage;
		newSocket = accept(serverSocket, (struct sockaddr *) &serverStorage, &addr_size);
		//for each client request creates a thread and assign the client request to it to process
	   //so the main thread can entertain next request
		if( pthread_create(&tid[i], NULL, socketThread, &newSocket) != 0 )
		   printf("Failed to create thread\n");
		if( i >= 50)
		{
		  i = 0;
		  while(i < 50)
		  {
			pthread_join(tid[i++],NULL);
		  }
		  i = 0;
		}
	}
  return 0;
}

*/

//TRABALHO DE REDES - LADO DO CLIENTE
//MATHEUS STEIGENBERG POPULIM -  10734710
//BRUNO GAZONI - 7585037
//BRUNO BALDISSERA - 10724351

#include <arpa/inet.h> 
#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h> 
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h> 
#include <sys/time.h>
#include <unistd.h>
#define PORT 1337


/* Essa função é chamada pela função signal (que por sua vez é chamada quando o programa recebe um sinal de interrupção)
  e imprime tanto no terminal cliente quanto no servidor uma mensagem de saída antes de fechar o programa.*/ 
void sighandler(int signum){
  printf("Programa interrompido, saindo do programa... (%d)", signum);
  exit(1);
}

//Função que facilita o cálculo do tempo percorrido para executar um ping
double timeInSeconds() {
	struct timeval tv;
	gettimeofday(&tv,NULL);
	return (((double)tv.tv_sec)*1)+((double)tv.tv_usec/1000000.0);
}

int commands(char* word){
  if(strcmp(word,"ping") == 0){
	return 2;
  }
  if(strcmp(word,"quit") == 0){
	return 1;
  }
  return 0;
}

void * socketThread(void *arg);





int main(int argc, char const *argv[]){
  
  	printf("Bem vindo ao server 3000!\n");


  	socklen_t addr_size;
  	struct sockaddr_in serverAddr;
  	struct sockaddr_storage serverStorage;

  	//Tratamos aqui os sinais de interrupção (SIGINT ou SIGPIPE), para os quais é chamada a função que atribui a flag de interrupção.
  	signal(SIGINT, sighandler);
  	signal(SIGPIPE, sighandler);

  	int server_fd, sock; 
  	struct sockaddr_in address;
  	int opt = 1; 
  	int addrlen = sizeof(address);
   
  	// Aqui criamos o descritor de arquivo do socket 
  	if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0){ 
		perror("socket failed"); 
	  	exit(EXIT_FAILURE); 
  	}

  	// Configuramos as opções do socket
  	if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) { 
		perror("setsockopt"); 
		exit(EXIT_FAILURE); 
  	}

  	address.sin_family = AF_INET; 
  	address.sin_addr.s_addr = INADDR_ANY;
  	address.sin_port = htons( PORT ); // Ligamos o socket à porta 1337

  	// Fazemos o bind do socket ao endereço correto
  	if (bind(server_fd, (struct sockaddr *)&address, sizeof(address))<0){ 
		perror("bind failed"); 
		exit(EXIT_FAILURE); 
  	}

  	// Marcamos o server_fd como o socket passivo para receber a conexão
  	if (listen(server_fd, 50) < 0){ 
		perror("listen"); 
		exit(EXIT_FAILURE); 
  	}

  	pthread_t tid[60];

  	printf("Esperando os clientes conectarem...\n");


	int i = 0;
	while(1){
		//Accept call creates a new socket for the incoming connection
		addr_size = sizeof(serverStorage);
		sock = accept(server_fd, (struct sockaddr *) &serverStorage, &addr_size);
		//for each client request creates a thread and assign the client request to it to process
	   //so the main thread can entertain next request
		if( pthread_create(&tid[i], NULL, socketThread, &sock) != 0){
			printf("Failed to create thread\n");
		}
		if( i >= 50){
			i = 0;
			while(i < 50){
				pthread_join(tid[i++],NULL);
			}
			i = 0;
		}

	}

  
	return 0;
}


void * socketThread(void *arg){
	int valread;
	int sock = *((int *)arg);

  	//======================================
	//Variáveis para armazenar mensagens
  	char* msg_recv = malloc(sizeof(char)*4096);
  	char* msg_send = malloc(sizeof(char)*4096);
  	char* buffer = malloc(sizeof(char)*4096);
  	char nome[60] = "desconhecido";
  
  	int msg_max_size = 1024;//Tamanho máximo da mensagem enviada em um único bloco de texto
  
  	//Demux dos descritores de arquivo prontos para serem lidos
  	fd_set read_fds;

  	double start, end;
  	struct timeval timeout_time;
  	timeout_time.tv_sec = 3;
  	timeout_time.tv_usec = 0;

  	int flag, ping_flag = 0, msg_count = 0, nome_def = 0;
  
  printf("Limite do tamanho da mensagem: %d. Mensagens maiores que %d serão truncadas\n\n", msg_max_size, msg_max_size-1);
  printf("Defina aqui o seu nome a ser visto pelo cliente seguido da tecla enter:\n\n");

  /*Enquanto o programa está ativo, este laço é executado,
	onde são executadas as ações necessárias de interação entre cliente e servidor.*/
  while(1){
	int fd_max = STDIN_FILENO;

		/* Configura os bits para os descritores de arquivos. */
		FD_ZERO(&read_fds);
		FD_SET(STDIN_FILENO, &read_fds);
		FD_SET(sock, &read_fds);

		/* Selecionamos o maior bit configurado para ser usado na função select abaixo. */    
		if( sock > fd_max ){
		  fd_max = sock; 
	  }

		/* Aqui se espera até que um dos descritores de arquivo tenham dados (não sejam nulos), para depois continuar a comunicação. */
		if (select(fd_max + 1, &read_fds, NULL, NULL, &timeout_time) == -1){
			perror("select: ");
			exit(1);
		}

	if( FD_ISSET(STDIN_FILENO, &read_fds )){
		fscanf(stdin,"%[^\n]%*c",msg_send);
		//printf("%d\n", (int)strlen(msg_send));
		msg_send[strlen(msg_send)] = 0;
	  int msg_size;
		if(msg_send[0] == '/'){
		  flag = commands(msg_send+1);
		  if(flag == 1){
			break;
		  }
		  else if(flag == 2){
			valread = send(sock , "/ping\0", strlen("/ping\0")+1, 0);
			ping_flag = 1;
			start = timeInSeconds();
		  }
		}
		else{
		  if(nome_def == 0){
		  nome_def = 1;
		  printf("\n\tNome definido!\n\n"); 
		}

		//quebra das mensagens com tamanho maior que o definido pela variável msg_max_size

		printf("%d",)
		for(int offset = 0;strlen(msg_send+offset) > 0;offset += msg_max_size-1){
		  msg_size = strlen(msg_send+offset);
			if(msg_size < msg_max_size){
		  		memcpy(buffer,msg_send+offset,msg_size);
			  	buffer[msg_size] = '\0';
			  	valread = send(sock , buffer, msg_size+1, 0);
			  	break;
			}
			else{
			  	memcpy(buffer,msg_send+offset,msg_max_size-1);
			  	buffer[msg_max_size-1] = '\0';
			  	valread = send(sock , buffer, msg_max_size, 0);
			}
		}
		}
	}


	  //Bloco que checa a presença de mais dados para serem recebidos através do recv()      
		if( FD_ISSET(sock, &read_fds)){
			/* There is data waiting on your socket.  Read it with recv(). */
			valread = recv(sock , msg_recv, 4096, 0);
			//checagem de desconexão
			if(valread == 0){
				printf("%s desconectou, triste né meu filho?\n", nome);
				break;
			}
			else{
				printf("%s: %s\n",nome, msg_recv);
			}
			msg_count++;
		}

	}

  	// Fechamos o socket e liberamos a memória alocada para as variáveis
  	close(sock);
  	free(msg_recv);
  	free(msg_send);
  	free(buffer);


  	//======================================

  	pthread_exit(NULL);
}