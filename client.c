
//https://www.youtube.com/watch?v=fNerEo6Lstw


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

#define LENGTH 2048

// Global variables
volatile sig_atomic_t flag = 0;
int sockfd = 0;
char name[40];

void ignore(){
	//ignora
	printf("\nPara sair do programa use o comando /quit ou CTRL + D\n");
}

int commands(char* word){
	if(strcmp(word,"connect") == 0){
		return 1;
	}
	if(strcmp(word,"quit") == 0){
		return 2;
	}
	if(strcmp(word,"ping") == 0){
		return 3;
	}
	return 0;
}

void catch_ctrl_c_and_exit(int sig) {
    flag = 1;
}

void str_overwrite_stdout() {
  	//printf("\r%s", "\r");
  	fflush(stdout);
}



void str_trim_lf (char* arr, int length) {
  	int i;
  	for (i = 0; i < length; i++) { // trim \n
    	if (arr[i] == '\n') {
     		arr[i] = '\0';
      		break;
    	}
  	}
}


void send_msg_handler() {
  	char message[LENGTH] = {};
	char buffer[LENGTH + 32] = {};

  	while(1) {
	  	str_overwrite_stdout();
	    fgets(message, LENGTH, stdin);
	    if(feof(stdin)){
	    	break;
	    }
		str_trim_lf(message, LENGTH);
	    if (strcmp(message, "/exit") == 0) {
			break;
	    } 
	    else{
	    	sprintf(buffer, "%s\n", message);
	    	send(sockfd, buffer, strlen(buffer), 0);
	    }
		bzero(message, LENGTH);
	    bzero(buffer, LENGTH + 32);
  	}
  	catch_ctrl_c_and_exit(2);
}

void recv_msg_handler() {
	//char* acknowledge = "ack\0";
	char message[LENGTH] = {};
  	while (1) {
		int receive = recv(sockfd, message, LENGTH, 0);
    	if (receive > 0) {
    		//send(sockfd, acknowledge, 4, 0);
      		printf("%s\n", message);
      		str_overwrite_stdout();
    	} 	
    	else if (receive == 0) {
			break;
    	}
		memset(message, 0, sizeof(message));
  	}
}

int main(int argc, char **argv){

	char* ip = "127.0.0.1";
	int porta = 1337;

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


  	// Connect to Server
  	int err = connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr));
  	if (err == -1) {
		printf("ERROR: connect\n");
		return EXIT_FAILURE;
	}

	// Send name
	send(sockfd, name, 40, 0);

	printf("=== WELCOME TO THE CHATROOM ===\n");

	pthread_t recv_msg_thread;
  	if(pthread_create(&recv_msg_thread, NULL, (void *) recv_msg_handler, NULL) != 0){
		printf("ERROR: pthread\n");
		return EXIT_FAILURE;
	}

	pthread_t send_msg_thread;
  	if(pthread_create(&send_msg_thread, NULL, (void *) send_msg_handler, NULL) != 0){
		printf("ERROR: pthread\n");
    	return EXIT_FAILURE;
    }

	while (1){
		if(flag){
			printf("\nBye\n");
			break;
    	}
    	sleep(0.5);
	}

	close(sockfd);

	return EXIT_SUCCESS;
}