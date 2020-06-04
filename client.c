//TRABALHO DE REDES - LADO DO CLIENTE
//MATHEUS STEIGENBERG POPULIM -  10734710
//BRUNO GAZONI - 7585037
//BRUNO BALDISSERA - 10724351

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>

#define LENGTH 20000 //tamanho máximo do buffer de mensagens digitadas pelo usuário.
//Esse buffer será repartido em trechos de 4096 bytes na função send_msg_handler


// Global variables
//flag pra sair do programa
volatile sig_atomic_t flag = 0;
//socket
int sockfd = 0;

void ignore(){
	//ignora
	printf("\nPara sair do programa use o comando /quit ou CTRL + D\n");
}

void flag_sair(int sig) {
    flag = 1;
}

void str_overwrite_stdout() {
  	//printf("\r%s", "\r");
  	fflush(stdout);
}

//elimina o \n da string e o substitui por um \0
void str_trim_lf (char* arr, int length) {
  	int i;
  	for (i = 0; i < length; i++) { // trim \n
    	if (arr[i] == '\n') {
     		arr[i] = '\0';
      		break;
    	}
  	}
}

//responsável por mandar mensagens
void send_msg_handler() {
  	char message[LENGTH];
	char buffer[4096];

  	while(1) {
  		if(flag == 1){
  			break;
  		}
	  	str_overwrite_stdout();
	    fgets(message, LENGTH, stdin);
	    if(feof(stdin)){ //comandos de saída (EOF)
	    	break;
	    }
		str_trim_lf(message, LENGTH);
	    if (strcmp(message, "/exit") == 0) { //comandos de saída
			break;
	    }
	    if (strcmp(message, "/quit") == 0) { //comandos de saída
			break;
	    }
	    else{
	    	//Caso a mensagem seja maior que 4096 bytes, ela é enviada em pedaços
	    	for(int offset = 0; strlen(message+offset) > 0; offset += 4095){
			    int msg_size = strlen(message+offset);
			    if(msg_size < 4096){
					memcpy(buffer,message+offset,msg_size);
			    	buffer[msg_size] = '\0';
			    	send(sockfd, buffer, strlen(buffer), 0);
			    	break;
			    }
			    else{
			    	memcpy(buffer,message+offset,4095);
			    	buffer[4095] = '\0';
			    	send(sockfd, buffer, strlen(buffer), 0);
			    }
			}
	    }
	    //zera os buffers
		bzero(message, LENGTH);
	    bzero(buffer, 4096);
  	}
  	//seta a flag para 1, que finaliza o programa
  	flag_sair(2);
  	pthread_detach(pthread_self());
}

//responsável por receber mensagens
void recv_msg_handler() {
	char message[4096];
  	while (1) {
  		if(flag == 1){
  			break;
  		}
  		//recebe a mensagem e a imprime
		int receive = recv(sockfd, message, 4096, 0);
    	if (receive > 0) {
      		printf("%s\n", message);
      		str_overwrite_stdout();// fflush
    	} 	
    	else if (receive == 0) {// caso o recv seja 0, sai do programa
			break;
    	}
    	//zera o buffer
		memset(message, 0, sizeof(message));
  	}
  	flag_sair(3);
  	pthread_detach(pthread_self());
}

int main(int argc, char **argv){
	char name[40];
	char ip[20] = "127.0.0.1";
	int porta = 1337;

	//ignora CTRL C
	if (signal(SIGINT, SIG_IGN) != SIG_IGN){
    	signal(SIGINT, ignore);
	}

	printf("Digite o seu nome: ");
  	fgets(name, 40, stdin);
  	str_trim_lf(name, strlen(name));


	if (strlen(name) > 39 || strlen(name) < 2){
		printf("O nome deve ter mais que 1 e menos que 40 caracteres\n");
		return EXIT_FAILURE;
	}

	struct sockaddr_in server_addr;

	/* Socket settings */
	sockfd = socket(AF_INET, SOCK_STREAM, 0); // TCP
  	server_addr.sin_family = AF_INET;
  	server_addr.sin_addr.s_addr = inet_addr(ip);
  	server_addr.sin_port = htons(porta);

  	char comando[50];

  	printf("Olá %s! Bem vindo ao IRC. Para conectar ao servidor, digite /connect\n", name);
  	while(1){
  		if(feof(stdin)){
  			return EXIT_FAILURE;
  		}
  		fgets(comando, 40, stdin);
  		str_trim_lf(comando,strlen(comando));
  		if(strcmp(comando,"/connect") == 0){
  			break;
  		}
  		if(strcmp(comando,"/quit") == 0){
  			return EXIT_FAILURE;
  		}
  	}

  	// Conectando ao servidor
  	int err = connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr));
  	if (err == -1) {
		printf("Erro: connect. Inicie o servidor\n");
		return EXIT_FAILURE;
	}

	printf("=== BEM VINDO À SALA DE CHAT, POR QUESTÕES DE SEGURANÇA, FAÇA LOGIN POR FAVOR ===\n");

	// Enviando o nome para o servidor
	send(sockfd, name, 40, 0);

	pthread_t recv_msg_thread;
  	if(pthread_create(&recv_msg_thread, NULL, (void *) recv_msg_handler, NULL) != 0){
		printf("ERRO: pthread\n");
		return EXIT_FAILURE;
	}

	pthread_t send_msg_thread;
  	if(pthread_create(&send_msg_thread, NULL, (void *) send_msg_handler, NULL) != 0){
		printf("ERRO: pthread\n");
    	return EXIT_FAILURE;
    }

	while (1){
		if(flag){
			printf("\nTchau\n");
			break;
    	}
    	sleep(0.5);
	}

	close(sockfd);

	return EXIT_SUCCESS;
}