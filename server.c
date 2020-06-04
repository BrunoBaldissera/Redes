//https://www.youtube.com/watch?v=fNerEo6Lstw

//TRABALHO DE REDES - LADO DO SERVER
//MATHEUS STEIGENBERG POPULIM -  10734710
//BRUNO GAZONI - 7585037
//BRUNO BALDISSERA - 10724351

// A partir de agora, vamos mexer em cima disso

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

static _Atomic unsigned int cli_count = 0;
static int uid = 10;

/* Client structure */
typedef struct{
	struct sockaddr_in address;
	int sockfd;
	int uid;
	char name[32];
} client_t;

client_t *clients[MAX_CLIENTS];

pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;



void print_client_addr(struct sockaddr_in addr){
    printf("%d.%d.%d.%d",
        addr.sin_addr.s_addr & 0xff,
        (addr.sin_addr.s_addr & 0xff00) >> 8,
        (addr.sin_addr.s_addr & 0xff0000) >> 16,
        (addr.sin_addr.s_addr & 0xff000000) >> 24);
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


/* Add clients to queue */
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

/* Remove clients to queue */
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

/* Send message to all clients except sender */
int send_message(char *s, int uid){
	pthread_mutex_lock(&clients_mutex);
	int send_ret = 0;
	int trials;

	for(int i=0; i<MAX_CLIENTS; ++i){
		if(clients[i] != NULL){
			if(clients[i]->uid != uid){
				trials = 0;
				while(send(clients[i]->sockfd, s, strlen(s),0) < 0 && trials < 5){
					trials++;
				}
				if(trials == 5){
					send_ret = 1;
				}
				//recv(clients[i]->sockfd, ack_recv, 4,0);
			}
		}
	}
	return send_ret;
	pthread_mutex_unlock(&clients_mutex);
}

int login(void *arg){
	client_t *cli = (client_t *)arg;
	char message[50] = "Digite o login.";
	char buffer[50];

	char LOGIN_SERVER[50] = "admin";
	char SENHA_SERVER[50] = "admin";

	send(cli->sockfd,message,strlen(message),0);
	recv(cli->sockfd, buffer, 40, 0);

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

/* Handle all communication with the client */
void *handle_client(void *arg){
	char buff_in[BUFFER_SZ];
	char buff_out[BUFFER_SZ];
	char name[40];
	char senha[40];
	int leave_flag = 0;
	int aut = 1;


	cli_count++;
	client_t *cli = (client_t *)arg;

	// Name
	if(recv(cli->sockfd, name, 40, 0) <= 0 || strlen(name) <  1 || strlen(name) >= 40-1){
		printf("Didn't enter the name.\n");
		leave_flag = 1;
	}
	else{
		strcpy(cli->name, name);
		// se o cara realmente entrar no chat
		aut = login(arg);
		if(aut == 1){
			sprintf(buff_out, "%s entrou no chat", cli->name);
			printf("%s\n", buff_out);
			fflush(stdout);
			leave_flag = send_message(buff_out, cli->uid);
		}
		else{
			leave_flag = 1;
		}
	}

	bzero(buff_out, BUFFER_SZ);

	while(1){
		if (leave_flag) {
			break;
		}
		int receive = recv(cli->sockfd, buff_in, BUFFER_SZ, 0);
		if (receive > 0){
			if(strlen(buff_in) > 0){
				if(strcmp(buff_in,"/ping") == 0){
					send(cli->sockfd,"SERVER: pong",strlen("SERVER: pong\0"),0);
				}
				else{
					sprintf(buff_out,"%s: %s\n", cli->name, buff_in);
					str_trim_lf(buff_out, strlen(buff_out));
					
					printf("%s\n", buff_out);
					leave_flag = send_message(buff_out, cli->uid);
				}
			}
		} else if (receive == 0 || strcmp(buff_out, "/exit") == 0){
			sprintf(buff_out, "%s has left\n", cli->name);
			printf("%s", buff_out);
			send_message(buff_out, cli->uid);
			leave_flag = 1;
		} else {
			printf("ERROR: -1\n");
			leave_flag = 1;
		}

		bzero(buff_out, BUFFER_SZ);
		bzero(buff_in, BUFFER_SZ);
	}

  /* Delete client from queue and yield thread */
	close(cli->sockfd);
  	queue_remove(cli->uid);
  	free(cli);
  	cli_count--;
  	pthread_detach(pthread_self());

	return NULL;
}

int main(int argc, char **argv){

	char *ip = "127.0.0.1";
	int port = 1337;
	int option = 1;
	int listenfd = 0, connfd = 0;
  	struct sockaddr_in serv_addr;
  	struct sockaddr_in cli_addr;
  	pthread_t tid;

	//Configurações do Socket
	listenfd = socket(AF_INET, SOCK_STREAM, 0);
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = inet_addr(ip);
	serv_addr.sin_port = htons(port);

	//Ignorar sinais de parada
	signal(SIGPIPE, SIG_IGN);

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

	printf("Bem vindo ao server\n");

	while(1){
		socklen_t clilen = sizeof(cli_addr);
		connfd = accept(listenfd, (struct sockaddr*)&cli_addr, &clilen);

		/* Checa se o máximo de clientes foi atingido*/
		if((cli_count + 1) == MAX_CLIENTS){
			printf("O server está cheio,conexão rejeitada com");
			print_client_addr(cli_addr);
			printf(":%d\n", cli_addr.sin_port);
			close(connfd);
			continue;
		}

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