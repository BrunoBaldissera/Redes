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
#define DEBUG 1

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
	    //Caso a mensagem seja maior que 4096 bytes, ela é enviada em pedaços
	    for(int offset = 0; strlen(message+offset) > 0; offset += 4095){
			int msg_size = strlen(message+offset);
			if(msg_size < 4096){
				memcpy(buffer,message+offset,msg_size);
			    buffer[msg_size] = '\0';
			    send(sockfd, buffer, strlen(buffer), 0);

			    if(strcmp(message, "/exit") == 0 || strcmp(message, "/quit") == 0){
	    			printf("saindo.\n");
	    			sleep(1);
	    			printf("saindo....\n");
	    			sleep(1);
	    			printf("saindo.........\n");
	    			sleep(1);
	    			flag = 1;
	    		}

			    break;
			}
			else{
			    memcpy(buffer,message+offset,4095);
			    buffer[4095] = '\0';
			    send(sockfd, buffer, strlen(buffer), 0);
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

int main(int argc, char *argv[]){
	char name[40];
	int porta = atoi(argv[2]);

	printf("%s é o ip e %d é a porta\n", argv[1], porta);

	//ignora CTRL C
	if (signal(SIGINT, SIG_IGN) != SIG_IGN){
    	signal(SIGINT, ignore);
	}

	printf("\n\nAntes de qualquer coisa, por favor digite o seu nome: ");
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
  	server_addr.sin_addr.s_addr = inet_addr(argv[1]);
  	server_addr.sin_port = htons(porta);

  	char comando[100];
  	char nome_canal[50];
  	fflush(stdin);

  	printf("\n\nOlá %s! Bem vindo ao IRC. Para conectar ao servidor e começar a conversar digite /connect seguido do nome do canal"
  	       "(o nome de um canal deve começar com o caractere '#')\n\n", name);
  	while(1){
  		if(feof(stdin)){
  			return EXIT_FAILURE;
  		}
  		fgets(comando, 100, stdin);
  		str_trim_lf(comando,strlen(comando));
  		if(strncmp(comando,"/connect", 8) == 0){
  			int j = 0;
  			for(int i = 9; comando[i] != '\0'; i++){
  				nome_canal[j] = comando[i];
  				j++;
  			}
  			nome_canal[j] = '\0';
  			if (DEBUG) printf("o nome do canal é %s\n", nome_canal);
  			if (nome_canal[0] != '#'){
				printf("O nome do canal deve começar com o caractere '#', tente novamente.\n\n");
				strcpy(nome_canal, "");
				continue;
			}
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

	char serv_disp[50];
	if (recv(sockfd, serv_disp, 50, 0) > 0){
		printf("%s\n\n", serv_disp);
		//aqui verificamos se a mensagem recebuda é a de servidor disponível ou não, e como sabemos que a de indisponibilidade começa com o caractere 'O', usamos isso
		//para uma facil distinção.
		if (serv_disp[0] == 'O') return 0;
	}

	printf("\n=== Bem-vindo(a) ao canal: %s===\n Para saber mais, digite /help ===\n\n", nome_canal);

	// Enviando o nome para o servidor
	send(sockfd, name, 40, 0);
	//sleep(0.3);
	// Enviando ao servidor o nome do canal a ser conectado
	str_trim_lf(nome_canal, strlen(nome_canal));
	send(sockfd, nome_canal, 50, 0);

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
			printf("\nAte a proxima!\n");
			break;
    	}
    	sleep(0.5);
	}

	close(sockfd);

	return EXIT_SUCCESS;
}
