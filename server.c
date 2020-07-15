//TRABALHO DE REDES - LADO DO SERVER
//MATHEUS STEIGENBERG POPULIM -  10734710
//BRUNO GAZONI - 7585037
//BRUNO BALDISSERA - 10724351

//TO DO: CRIAR LOGINS PARA USUARIOS, GUARDADOS EM ARQUIVO (USAR HASH PARA SENHAS)
//	DAR NOMES A CANAIS, CRIAR UM CANAL NOVO QUANDO UM CLIENTE CONECTA A ELE (UM MESMO SERVIDOR TERA VARIOS CANAIS RECONHECIDOS PELO NOME)
//	DAR NICKNAMES AOS CLIENTES 
//	IDENTIFICAR UM USUARIO MODERADOR
//	IMPLEMENTAR: KICK
//		     MUTE/UNMUTE
//		     WHOIS

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include <sys/types.h>
#include <signal.h>


#define MAX_CLIENTS 100
#define BUFFER_SZ 2048

#define DEBUG 1

static _Atomic unsigned int cli_count = 0;
//static int uid = 10;

/* Estrutura do cliente */
typedef struct{
	struct sockaddr_in address;
	int sockfd;
	int uid;
	char name[40];
	char canal[50];
	short int mod;
	short int muted;
} client_t;

//Dados dos clientes são armazenados nesse vetor
client_t *clients[MAX_CLIENTS];

//mutex das threads
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;

//imprime endereço de ip do cliente
void print_adress(struct sockaddr_in addr){
    printf("%d.%d.%d.%d",
        addr.sin_addr.s_addr & 0xff,
        (addr.sin_addr.s_addr & 0xff00) >> 8,
        (addr.sin_addr.s_addr & 0xff0000) >> 16,
        (addr.sin_addr.s_addr & 0xff000000) >> 24);
}

//substitui \n por \0
void str_trim_lf (char* arr, int length) {
	int i;
  	for (i = 0; i < length; i++) { // trim \n
    	if (arr[i] == '\n') {
      		arr[i] = '\0';
      		break;
    	}
  	}
}


/* Adiciona clientes à fila de clientes */
void queue_add(client_t *cl){
	pthread_mutex_lock(&clients_mutex);

	for(int i=0; i < MAX_CLIENTS; ++i){
		if(clients[i] == NULL){
			clients[i] = cl;
			break;
		}
	}

	pthread_mutex_unlock(&clients_mutex);
}

/* Remove clientes da fila de clientes */
void queue_remove(int uid){
	pthread_mutex_lock(&clients_mutex);

	for(int i=0; i < MAX_CLIENTS; ++i){
		if(clients[i] != NULL){
			if(clients[i]->uid == uid){
				clients[i] = NULL;
				break;
			}
		}
	}
	pthread_mutex_unlock(&clients_mutex);
}

/* Envia mensagem para todos os clientes em um canal(incluindo o remetente, como dito na especificação) */
int send_message(char *s, int uid, char* canal){
	int send_ret = 0;
	int trials;
	pthread_mutex_lock(&clients_mutex); //trava as threads para acessar as variáveis globais sem problemas

	for(int i=0; i<MAX_CLIENTS; ++i){
		if(clients[i] != NULL && (strcmp(clients[i]->canal, canal) == 0)){
			while(send(clients[i]->sockfd, s, strlen(s),0) < 0 && trials < 5){ // caso o cliente não receba, são feitas 5 tentativas
				trials++;
			}
			if(trials == 5){
				send_ret = 1;
			}
			//recv(clients[i]->sockfd, ack_recv, 4,0);
		}
	}
	//destrava as threads
	pthread_mutex_unlock(&clients_mutex);
	return send_ret;
}

/* Envia mensagem para um cliente especificado pelo nickname e canal*/
int send_message_un(char *s, int uid, char* nickname, char* canal){
	int send_ret = 0;
	int trials;
	pthread_mutex_lock(&clients_mutex); //trava as threads para acessar as variáveis globais sem problemas

	for(int i=0; i<MAX_CLIENTS; ++i){
		if(clients[i] != NULL && (strcmp(clients[i]->name, nickname) == 0)){
			if(strcmp(canal, clients[i]->canal) == 0){
				while(send(clients[i]->sockfd, s, strlen(s),0) < 0 && trials < 5){ // caso o cliente não receba, são feitas 5 tentativas
					trials++;
				}
				if(trials == 5){
					send_ret = 1;
				}
			//recv(clients[i]->sockfd, ack_recv, 4,0);
			}
		}
	}
	//destrava as threads
	pthread_mutex_unlock(&clients_mutex);
	return send_ret;
}

// Trata o login com os clientes 
// LOGIN = admin
// SENHA = admin
int login(void *arg){
	client_t *cli = (client_t *)arg;
	
	char buffer[50];
	char message[50];

	strcpy(message, "Digite o login.");

	char LOGIN_SERVER[50] = "admin";
	char SENHA_SERVER[50] = "admin";

	send(cli->sockfd,message,strlen(message),0);
	recv(cli->sockfd, buffer, 50, 0);

	if(strcmp(buffer,LOGIN_SERVER) != 0){
		strcpy(message,"Login incorreto.");
		send(cli->sockfd,message,strlen(message),0);
		return -1;
	}
	else{
		strcpy(message,"Digite a senha.");
		send(cli->sockfd,message,strlen(message),0);
		recv(cli->sockfd, buffer, 40, 0);

		if(strcmp(buffer,SENHA_SERVER) != 0){
			strcpy(message,"Senha incorreta.");
			send(cli->sockfd,message,strlen(message),0);
			return -1;
		}
		else{
			return 1;
		}
	}
}

/* Lida com toda a comunicação com o cliente */
void *handle_client(void *arg){
	char buff_in[BUFFER_SZ];
	char buff_out[BUFFER_SZ];
	char name[40];
	char nome_canal[50];
	int leave_flag = 0;
	int aut = 1;


	cli_count++;
	client_t *cli = (client_t *)arg;
	
	// Name
	if(recv(cli->sockfd, name, 40, 0) <= 0 || strlen(name) <  1 || strlen(name) >= 40-1){
		printf("Não inseriu o nome.\n");
		leave_flag = 1;
	}
	else{
		strcpy(cli->name, name);

		//----------------------Este trecho lida com o input de canal do cliente---------------
		bzero(buff_out, BUFFER_SZ);
		bzero(buff_in, BUFFER_SZ);

		recv(cli->sockfd, buff_in, 50, 0);
		printf("Nome de canal recebido: %s\n", buff_in);

		//Verificamos se o canal requisitado já existe, e caso contrário, criamos ele e tornamos o cliente que o requisitou moderador.
 		short int canal_ex = 0;

 		printf("cli_count: %d\n", cli_count);

 		for (int i = 0; i < cli_count; i++){
 			if (strncmp(clients[i]->canal, buff_in, strlen(buff_in)) == 0){
 				canal_ex = 1;
 			}
 		}

 		strncpy(cli->canal, buff_in, strlen(buff_in));

 		if (canal_ex == 0){
 			printf("Novo canal %s criado.\n", cli->canal);
 			strcpy(buff_out, "Esse canal nao existia anteriormente a acabou de ser criado.\n\n Voce agora e o moderador deste canal.\n");
 			leave_flag = send_message(buff_out, cli->uid, cli->canal);
 			cli->mod = 1;
 		}

		bzero(buff_out, BUFFER_SZ);
		bzero(buff_in, BUFFER_SZ);
		//--------------------------------------------------------------------------------------

		// se o cliente entrar no chat
		//aut = login(arg);
		//if(aut == 1){
			sprintf(buff_out, "%s conectou-se", cli->name);
			printf("%s\n", buff_out);
			fflush(stdout);
			leave_flag = send_message(buff_out, cli->uid, cli->canal);
		//}
		//else{
		//	leave_flag = 1;
		//}
	}

	bzero(buff_out, BUFFER_SZ);
	bzero(buff_in, BUFFER_SZ);


	while(1){
		if (leave_flag) {
			break;
		}
		int receive = recv(cli->sockfd, buff_in, BUFFER_SZ, 0);
		if (DEBUG) printf("buff_in: %s\n", buff_in);
		if (receive > 0){
			if(strlen(buff_in) > 0){
				if(buff_in[0] == '/'){
					if(strcmp(buff_in,"/ping") == 0){
						send(cli->sockfd,"SERVER: pong",strlen("SERVER: pong\0"),0);
					}
					else if (strncmp(buff_in, "/join", 5) == 0) { //comandos de saída
						char novo_canal[50];
						int j = 0;
						for(int i = 6; i < strlen(buff_in); i++){
							novo_canal[j] = buff_in[i];
							j++;
						}
						novo_canal[j] = '\0';
						strcpy(cli->canal, novo_canal);
						short int flag_canal = 0;
						for (int i = 0; i < cli_count; i++){
 							if (strcmp(clients[i]->canal, novo_canal) == 0){
 								flag_canal = 1;
 							}
 						}
				 		if (flag_canal == 0){
				 			printf("Novo canal %s criado.\n", cli->canal);
				 			strcpy(buff_out, "Esse canal nao existia anteriormente a acabou de ser criado.\n\n Voce agora e o moderador deste canal.\n");
				 			leave_flag = send(cli->sockfd, buff_out, strlen(buff_out), 0);
				 			cli->mod = 1;
				 		}
				 		strcpy(buff_out, "\n=== BEM-VINDO(A) AO CANAL ");
				 		send(cli->sockfd, buff_out, strlen(buff_out), 0);
				 		strcpy(buff_out, cli->canal);
				 		send(cli->sockfd, buff_out, strlen(buff_out), 0);
				 		strcpy(buff_out, "===\n\n");
				 		send(cli->sockfd, buff_out, strlen(buff_out), 0);
		    		}
		    		else if (strncmp(buff_in, "/nickname", 9) == 0) { //comandos de saída
		    			if (DEBUG) printf("o comando foi %s\n strlen(buff_in) = %d\n", buff_in, strlen(buff_in));
		    			int j = 0;
		    			char novo_apelido[50];
						for(int i = 10; i < strlen(buff_in); i++){
							novo_apelido[j] = buff_in[i];
							j++;
						}
						novo_apelido[j] = '\0';
						if (DEBUG) printf("novo apelido: %s\n", novo_apelido);
		    			strcpy(cli->name, novo_apelido);
						send(cli->sockfd,"Pronto, seu apelido foi reconfigurado para ", strlen("Pronto, seu apelido foi reconfigurado para \0"),0);
						send(cli->sockfd, novo_apelido, strlen(novo_apelido), 0);
						send(cli->sockfd, ".\n", strlen(".\n"), 0);

		    		}
					else if (strncmp(buff_in, "/whois", 6) == 0) { //comandos de saída
						if(cli->mod == 0){
							send(cli->sockfd,"Você precisa ser moderador para usar este comando.",strlen("Você precisa ser moderador para usar este comando.\0"),0);
						}
						else{
							printf("comando whois acionado\n");
						}
		    		}
		    		else if (strncmp(buff_in, "/kick", 5) == 0) { //comandos de saída
		    			if(cli->mod == 0){
							send(cli->sockfd,"Você precisa ser moderador para usar este comando.",strlen("Você precisa ser moderador para usar este comando.\0"),0);
						}
						else{
							char kicked_user[50];
							int j = 0;
							for(int i = 6; i < strlen(buff_in); i++){
								kicked_user[j] = buff_in[i];
								j++;
							}
							if (DEBUG) printf("O usuario a ser kickado e: %s\n", kicked_user);

							short int flag_kick = 0;

							if (strcmp(cli->name, kicked_user) == 0) {
								send(cli->sockfd,"Voce nao pode kickar a si proprio.",strlen("Voce nao pode kickar a si proprio.\0"),0);
								flag_kick = 2;
							}

							for(int i = 0; i < cli_count; i++){
								if (flag_kick != 0) break;
								if (strcmp(clients[i]->name, kicked_user) == 0){
									if(strcmp(clients[i]->canal, cli->canal) == 0){
										if (DEBUG) printf("canal mutuo: %s\n", cli->canal);

										strcpy(buff_out, kicked_user);
										send_message(buff_out, cli->uid, cli->canal);
										strcpy(buff_out, " foi kickado do grupo por um moderador e entrará no limbo. Até a próxima!\n");
										send_message(buff_out, cli->uid, cli->canal);

										strcpy(clients[i]->canal, "limbo");
										flag_kick = 1;
									}
								}
							}
							if (flag_kick == 0){
								send(cli->sockfd,"Usuario nao existe ou não esta no canal.",strlen("Usuario nao existe ou não esta no canal.\0"),0);
							}
						}
		    		}
		    		else if (strncmp(buff_in, "/mute", 5) == 0) { //comandos de saída
		    			if(cli->mod == 0){
							send(cli->sockfd,"Voce precisa ser moderador para usar este comando.",strlen("Voce precisa ser moderador para usar este comando.\0"),0);
						}
						else{
							if (DEBUG) printf("comando mute acionado\n");
							char muted_user[50];
							int j = 0;
							for(int i = 6; i < strlen(buff_in); i++){
								muted_user[j] = buff_in[i];
								j++;
							}
							if (DEBUG) printf("muted user: %s\n", muted_user);
							short int flag_mute = 0;
							for(int i = 0; i < cli_count; i++){
								if (flag_mute != 0) break;
								if (strcmp(clients[i]->name, muted_user) == 0){
									if(strcmp(clients[i]->canal, cli->canal) == 0){
										if (DEBUG) printf("canal mutuo: %s\n", cli->canal);

										strcpy(buff_out, muted_user);
										send_message(buff_out, cli->uid, cli->canal);
										strcpy(buff_out, " foi silenciado do grupo por um moderador e não poderá mais ser ouvido até que seja dessilenciado.\n");
										send_message(buff_out, cli->uid, cli->canal);

										flag_mute = 1;
										clients[i]->muted = 1;
									}
								}
							}
							if (flag_mute == 0){
								send(cli->sockfd,"Usuario nao existe ou não esta no canal.",strlen("Usuario nao existe ou não esta no canal.\0"),0);
							}
						}
		    		}
		    		else if (strncmp(buff_in, "/unmute", 7) == 0) { //comandos de saída
		    			if(cli->mod == 0){
							send(cli->sockfd,"Voce precisa ser moderador para usar este comando.",strlen("Voce precisa ser moderador para usar este comando.\0"),0);
						}
						else{
							if (DEBUG) printf("comando unmute acionado\n");
						}
		    		}
		    	}
				else{
					sprintf(buff_out,"%s: %s\n", cli->name, buff_in);
					str_trim_lf(buff_out, strlen(buff_out));
					
					printf("%s\n", buff_out);

					if(cli->muted != 1){
						leave_flag = send_message(buff_out, cli->uid, cli->canal);
					}
				}
			}
		} else if (receive == 0 || strcmp(buff_out, "/exit") == 0){
			sprintf(buff_out, "%s has left\n", cli->name);
			printf("%s", buff_out);
			send_message(buff_out, cli->uid, cli->canal);
			leave_flag = 1;
		} else {
			printf("ERROR: -1\n");
			leave_flag = 1;
		}

		bzero(buff_out, BUFFER_SZ);
		bzero(buff_in, BUFFER_SZ);
	}

  /* Remove o cliente da fila, e libera a thread */
	close(cli->sockfd);
  	queue_remove(cli->uid);
  	free(cli);
  	cli_count--;
  	pthread_detach(pthread_self());

	return NULL;
}

int main(int argc, char *argv[]){

	//configuração
	//char *ip = "127.0.0.1";
	int port = 3000;
	int option = 1;
	int uid = 0;
	int listenfd = 0, connfd = 0;
  	struct sockaddr_in serv_addr;
  	struct sockaddr_in cli_addr;
  	pthread_t tid;
  	char canais[100][50];


	//Configurações do Socket
	listenfd = socket(AF_INET, SOCK_STREAM, 0);
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr.sin_port = htons(port);


	if(setsockopt(listenfd, SOL_SOCKET,(SO_REUSEPORT | SO_REUSEADDR),(char*)&option,sizeof(option)) < 0){
		perror("ERRO: setsockopt");
		return EXIT_FAILURE;
	}

	/* Bind */
	if(bind(listenfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
		perror("ERRO: bind");
		return EXIT_FAILURE;
	}

	/* Listen */
	if (listen(listenfd, 10) < 0) {
		perror("ERRO: listen");
		return EXIT_FAILURE;
	}

	printf("\nServer aberto\n\n");

	while(1){
		socklen_t clilen = sizeof(cli_addr);
		connfd = accept(listenfd, (struct sockaddr*)&cli_addr, &clilen);

		char serv_disp[50];
		/* Checa se o máximo de clientes foi atingido*/
		if((cli_count + 1) == MAX_CLIENTS){
			printf("O server está cheio,conexão rejeitada com ");
			print_adress(cli_addr);
			printf("id: (%d)\n", uid + 1);

			strcpy(serv_disp, "O server está cheio, tente novamente mais tarde...\n");
			send(connfd, serv_disp, strlen(serv_disp),0);
			close(connfd);
			continue;
		}

		strcpy(serv_disp, "Conexao com o servidor bem-sucedida!");
		send(connfd, serv_disp, strlen(serv_disp),0);

		/* Configurações do cliente */
		client_t *cli = (client_t *)malloc(sizeof(client_t));
		cli->address = cli_addr;
		cli->sockfd = connfd;
		cli->uid = uid++;

		/* Adiciona cliente à fila e cria uma thread */
		queue_add(cli);
		pthread_create(&tid, NULL, &handle_client, (void*)cli);

		/* Reduz o uso da CPU*/
		sleep(0.1);
	}

	return EXIT_SUCCESS;
}
