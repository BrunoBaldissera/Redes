//TRABALHO DE REDES - LADO DO SERVER
//MATHEUS STEIGENBERG POPULIM -  10734710
//BRUNO GAZONI - 7585037
//BRUNO BALDISSERA - 10724351

/*AVISO IMPORTANTE:
		A comunicação feita entre redes foi testada utilizando a ferramenta ngrok, 
	em que a máquina que executa o programa servidor deve utilizar para fazer um tunelamento
	com um servidor ngrok externo, para que os clientes por sua vez se conectem ao ip e porta
	providenciados por este serviço no momento em que o servidor o aciona.
		Esta configũração é feita no makefile, na opção roda_cliente, onde o ip
	do servidor ngrok é passado como argumento do programa, bem como a porta necessária.
		Por exemplo, no último teste feito entre os membros do grupo. A máquina que roda
	o servidor localmente usou o comando: ./ngrok tcp 3000
	já que 3000 é a porta onde o servidor escuta. Então o serviço ngrok
	ofereceu um domínio da seguinte forma:
	Forwarding                    tcp://2.tcp.ngrok.io:17353 -> localhost:3000  
		Então foi atualizado no github o makefile para rodar o programa cliente fazendo com que
	o socket escute na porta 17353 do domínio 2.tcp.ngrok.io,
	que por sua vez é identificado pelo ip 3.23.201.37.
		Finalmente, o cliente efetua um pull no github para a versão mais recente, roda o cliente pela
	diretriz do makefile atualizada e a comunicação é estabelecida com servidor entre redes.
		OBS: Por causa dessa dinâmica, o comando /whois acaba por sempre retornar o ip
	do alvo como sendo do localhost, mesmo estando com uma lógica correta de implementação, uma vez
	que a comunicação é interpretada como ocorrendo em uma mesma máquina local.*/

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

/* Estrutura do cliente, com todas as suas informações necessárias de seu socket, direitos de moderador,
	canal em que se encontra, número do usuário, nome (único) e se está silenciado ou não */
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

/* Envia mensagem para todos os clientes em um canal (incluindo o remetente, como dito na especificação) */
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
			}
		}
	}
	//destrava as threads
	pthread_mutex_unlock(&clients_mutex);
	return send_ret;
}

/* Lida com toda a comunicação com o cliente, recebendo suas mensagens, redirecionando para outros clientes
	quando necessário, executando operações, interpretando comandos.*/
void *handle_client(void *arg){
	char buff_in[BUFFER_SZ];
	char buff_out[BUFFER_SZ];
	char name[40];
	int leave_flag = 0;

	//incrementamos a contagem de clientes, visto que a cada vez que esta função é chamada temos um novo cliente.
	cli_count++;
	client_t *cli = (client_t *)arg;
	
	// Tratamos o nome enviado pelo usuário
	if(recv(cli->sockfd, name, 40, 0) <= 0 || strlen(name) <  1 || strlen(name) >= 40-1){
		printf("Não inseriu o nome.\n");
		leave_flag = 1;
	}
	//caso já exista um usuário com o nome, um nome provisório userX, sendo X o número do cliente, é dado para o usuário.
	else{
		short int flag_user = 0;
		for(int i = 0; i < cli_count; i++){
			if (strcmp(clients[i]->name, name) == 0){
				strcpy(buff_out, "Já existe um usuário com esse nome no IRC, te daremos um apelido provisório, você pode mudá-lo com o comando /nickname"
					", seguido do apelido novo.\n");
				send(cli->sockfd, buff_out, strlen(buff_out), 0);
				char new_name[50];
				sprintf(new_name, "user%d", cli_count);
				sprintf(buff_out, "Seu novo apelido é %s\n", new_name);
				send(cli->sockfd, buff_out, strlen(buff_out), 0);
				strcpy(cli->name, new_name);
			    flag_user = 1;
				break;
			}
		}
		if (flag_user == 0) strcpy(cli->name, name);

		//----------------------Este trecho lida com o input de canal do cliente---------------
		bzero(buff_out, BUFFER_SZ);
		bzero(buff_in, BUFFER_SZ);

		//recebemos a string com o nome do canal que o usuário enviou
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

 		//caso o canal seja novo, damos direitos de moderador ao usuário neste canal
 		if (canal_ex == 0){
 			printf("Novo canal %s criado.\n", cli->canal);
 			strcpy(buff_out, "Esse canal nao existia anteriormente a acabou de ser criado.\n\n Voce agora e o moderador deste canal.\n\n");
 			leave_flag = send_message(buff_out, cli->uid, cli->canal);
 			cli->mod = 1;
 		}

		bzero(buff_out, BUFFER_SZ);
		bzero(buff_in, BUFFER_SZ);

		//enviamos uma mensagem para todos no canal comunicando a entrada do usuário neste
		sprintf(buff_out, "%s conectou-se", cli->name);
		printf("%s\n", buff_out);
		fflush(stdout);
		leave_flag = send_message(buff_out, cli->uid, cli->canal);
	}

	bzero(buff_out, BUFFER_SZ);
	bzero(buff_in, BUFFER_SZ);


	/*Este é o loop principal da função, responsável por receber as mensagens do usuário,
		identificar se é um comando ou não (caso seja ele faz as operações necessárias), e
		se não, manda como uma mensagem comum aos outros usuários do canal, sempre verificando
		se o usuário saiu do programa como condição de parada.*/
	while(1){
		if (leave_flag) {
			break;
		}
		int receive = recv(cli->sockfd, buff_in, BUFFER_SZ, 0);
		if (DEBUG) printf("buff_in: %s\n", buff_in);
		if (receive > 0){
			if(strlen(buff_in) > 0){
				//verificanmos se o primeiro caractere do input do usuário é uma barra, o que caracteriza um comando.
				if(buff_in[0] == '/'){
					//Comando de /ping, o servidor apenas responde com a mensagem "pong"
					if(strcmp(buff_in,"/ping") == 0){
						send(cli->sockfd,"SERVER: pong",strlen("SERVER: pong\0"),0);
					}
					//esse comando faz com que o servidor explique todos os demais comandos para o usuário.
					else if (strncmp(buff_in, "/help", 5) == 0) {
						strcpy(buff_out, "\n Comandos disponiveis: \n"
							"\t/ping : O servidor responde com \"pong\".\n"
							"\t/join nomeCanal : Você entrará no canal nomeCanal, e caso ele já não exista, será criado e você será um moderador lá. "
							"(Um canal deixa de existir se não houverem mais usuários nele)\n"
							"\t/nickname Apelido : Você mudará o nome pelo qual é reconhecido no chat para Apelido.\n"
							"Comandos para moderadores: \n\n\t"
							"\t/whois Usuario : O servidor responde com o ip do usuario identificado por Usuario.\n"
							"\t/kick Usuario : Caso ele exista no canal, Usuario é expulso do canal onde o moderador e ele estão.\n"
							"\t/mute Usuario : As mensagens de Usuario não são mais vistas pelos outros usuários, até que ele seja dessilenciado.\n"
							"\t/unmute Usuario : Reverte a condição de silenciamento do Usuario.\n"
							"\t/exit : Sai do IRC e finaliza o programa.\n"
							"Divirta-se!\n\n");
				 		send(cli->sockfd, buff_out, strlen(buff_out), 0);
					}
					//No /join verificamos o canal para o qual o usuário quer se conectar,
					// e caso ele não exista ainda o criamos, oferencendo direitos de moderador ao usuário.
					// Caso o usuário esteja saindo de um canal no qual possui direitos de moderador, esses direitos são passados para outro
					// usuário que permaneceu no canal, se este existir. Se não, o canal deixa de existir.
					else if (strncmp(buff_in, "/join", 5) == 0) {
						char novo_canal[50];
						int j = 0;
						for(int i = 6; i < strlen(buff_in); i++){
							novo_canal[j] = buff_in[i];
							j++;
						}
						novo_canal[j] = '\0';
						if (novo_canal[0] != '#'){
							strcpy(buff_out, "O nome do canal deve começar com o caractere '#', tente novamente.\n\n");
				 			send(cli->sockfd, buff_out, strlen(buff_out), 0);
							bzero(buff_out, BUFFER_SZ);
							bzero(buff_in, BUFFER_SZ);
							continue;
						}

						if (strcmp(cli->canal, novo_canal) == 0){
							strcpy(buff_out, "Você já está neste canal!\n\n");
				 			send(cli->sockfd, buff_out, strlen(buff_out), 0);
				 			bzero(buff_out, BUFFER_SZ);
							bzero(buff_in, BUFFER_SZ);
				 			continue;
						}

						short int flag_canal = 0;
						for (int i = 0; i < cli_count; i++){
 							if (strcmp(clients[i]->canal, novo_canal) == 0){
 								flag_canal = 1;
 								cli->mod = 0;	//retira privilegios de moderador quando o usuario entra num canal ja ocupado
 							}
 						}
 						//Avisa aos usuarios do canal que o usuario esta saindo
 						strcpy(buff_out, cli->name);
						send_message(buff_out, cli->uid, cli->canal);
						strcpy(buff_out, " saiu do canal.\n");
						send_message(buff_out, cli->uid, cli->canal);

						if (cli->mod == 1){
							cli->mod = 0;

							for(int i = 0; i < cli_count; i++){
								if (strcmp(cli->canal, clients[i]->canal) == 0 && strcmp(cli->name, clients[i]->name) != 0){
									clients[i]->mod = 1;
									sprintf(buff_out, "%s é o novo moderador deste canal!\n", clients[i]->name);
									send_message(buff_out, cli->uid, cli->canal);
									break;
								}
							}
						}

						//Muda o canal
 						strcpy(cli->canal, novo_canal);

				 		if (flag_canal == 0){
				 			printf("Novo canal %s criado.\n", cli->canal);
				 			strcpy(buff_out, "Esse canal nao existia anteriormente a acabou de ser criado.\n\n Voce agora e o moderador deste canal.\n\n");
				 			send(cli->sockfd, buff_out, strlen(buff_out), 0);
				 			cli->mod = 1;	//da privilegios de moderador
				 		}
				 		sprintf(buff_out, "\n=== Bem-vindo(a) ao canal %s ===\n\n", cli->canal);
				 		send(cli->sockfd, buff_out, strlen(buff_out), 0);

				 		//Avisa aos usuarios do canal que o usuario entrou
				 		strcpy(buff_out, cli->name);
						send_message(buff_out, cli->uid, cli->canal);
						strcpy(buff_out, " entrou ao canal.\n");
						send_message(buff_out, cli->uid, cli->canal);

				 		flag_canal = 0;
		    		}
		    		// /nickname verifica o apelido que o usuário quer obter, verifica se alguém já possui-o, 
		    		// e caso não, muda o nome do usuário, avisando os demais usuários do canal.
		    		else if (strncmp(buff_in, "/nickname", 9) == 0) { //comandos de saída
		    			if (DEBUG) printf("o comando foi %s\n strlen(buff_in) = %lu\n", buff_in, strlen(buff_in));
		    			int j = 0;
		    			char novo_apelido[50];
						for(int i = 10; i < strlen(buff_in); i++){
							novo_apelido[j] = buff_in[i];
							j++;
						}
						novo_apelido[j] = '\0';
						short int flag_nick = 0;
						if (DEBUG) printf("novo apelido: %s\n", novo_apelido);
						for(int i = 0; i < cli_count; i++){
							if (strcmp(clients[i]->name, novo_apelido) == 0){
								strcpy(buff_out, "Já existe um usuário com esse nome no IRC, tente outro apelido.\n\n");
								send(cli->sockfd, buff_out, strlen(buff_out), 0);
								flag_nick = 1;
								break;
							}
						}

						if(flag_nick == 0){
							sprintf(buff_out, "%s mudou seu nome para %s!\n\n", cli->name, novo_apelido);
							send_message(buff_out, cli->uid, cli->canal);

			    			strcpy(cli->name, novo_apelido);
							send(cli->sockfd,"Pronto, seu apelido foi reconfigurado para ", strlen("Pronto, seu apelido foi reconfigurado para \0"),0);
							send(cli->sockfd, novo_apelido, strlen(novo_apelido), 0);
							send(cli->sockfd, ".\n", strlen(".\n"), 0);
						}

		    		}
		    		//Caso o usuário seja moderador, verifica se o usuário definido no comando existe, está no canal, e,
		    		//se afirmativo, retorna o ip do usuário. 
					else if (strncmp(buff_in, "/whois", 6) == 0) { //comandos de saída
						if(cli->mod == 0){
							send(cli->sockfd,"Você precisa ser moderador para usar este comando.\n",strlen("Você precisa ser moderador para usar este comando.\n\0"),0);
						}
						else{
							if (DEBUG) printf("comando whois acionado\n");

							char searched_user[50];
							int j = 0;
							for(int i = 7; i < strlen(buff_in); i++){
								searched_user[j] = buff_in[i];
								j++;
							}
							searched_user[j] = '\0';
							if (DEBUG) printf("searched_user: %s\n", searched_user);

							short int flag_search = 0;
							for(int i = 0; i < cli_count; i++){
								if (flag_search != 0) break;
								if (strcmp(clients[i]->name, searched_user) == 0){
									if(strcmp(clients[i]->canal, cli->canal) == 0){
										if (DEBUG) printf("canal mutuo: %s\n", cli->canal);

										sprintf(buff_out, "O ip de %s é: ", searched_user);
				 						send(cli->sockfd, buff_out, strlen(buff_out), 0);

										sprintf(buff_out, "%d.%d.%d.%d",
        									clients[i]->address.sin_addr.s_addr & 0xff,
       										(clients[i]->address.sin_addr.s_addr & 0xff00) >> 8,
        									(clients[i]->address.sin_addr.s_addr & 0xff0000) >> 16,
        									(clients[i]->address.sin_addr.s_addr & 0xff000000) >> 24);
										send(cli->sockfd, buff_out, strlen(buff_out), 0);
										flag_search = 1;
									}
								}
							}
							if (flag_search == 0){
								send(cli->sockfd,"Usuario nao existe ou não esta no canal.\n",strlen("Usuario nao existe ou não esta no canal.\n\0"),0);
							}
						}
		    		}
		    		//Caso o usuário seja moderador, verifica se o usuário definido no comando existe, está no canal, e,
		    		//se sim, muda o canal do alvo para o canal "limbo", impossível de ser conecado por outra forma,
		    		//ja que não possui um caracter '#'.
		    		else if (strncmp(buff_in, "/kick", 5) == 0) { //comandos de saída
		    			if(cli->mod == 0){
							send(cli->sockfd,"Você precisa ser moderador para usar este comando.\n",strlen("Você precisa ser moderador para usar este comando.\n\0"),0);
						}
						else{
							char kicked_user[50];
							int j = 0;
							for(int i = 6; i < strlen(buff_in); i++){
								kicked_user[j] = buff_in[i];
								j++;
							}
							kicked_user[j] = '\0';
							if (DEBUG) printf("O usuario a ser kickado e: %s\n", kicked_user);

							short int flag_kick = 0;

							if (strcmp(cli->name, kicked_user) == 0) {
								send(cli->sockfd,"Voce nao pode kickar a si proprio.\n",strlen("Voce nao pode kickar a si proprio.\n\0"),0);
								flag_kick = 2;
							}

							for(int i = 0; i < cli_count; i++){
								if (flag_kick != 0) break;
								if (strcmp(clients[i]->name, kicked_user) == 0){
									if(strcmp(clients[i]->canal, cli->canal) == 0){
										if (DEBUG) printf("canal mutuo: %s\n", cli->canal);

										sprintf(buff_out, "%s foi kickado do grupo por um moderador e entrará no limbo. Até a próxima!\n\n", kicked_user);
										send_message(buff_out, cli->uid, cli->canal);

										strcpy(clients[i]->canal, "limbo");
										flag_kick = 1;
									}
								}
							}
							if (flag_kick == 0){
								send(cli->sockfd,"Usuario nao existe ou não esta no canal.\n",strlen("Usuario nao existe ou não esta no canal.\n\0"),0);
							}
						}
		    		}
		    		//Caso o usuário seja moderador, verifica se o usuário definido no comando existe, está no canal, e,
		    		//se sim, silencia o alvo, fazendo com que suas mensagens não sejam ouvidas pelos demais usuários do canal,
		    		//graças a uma flag de silenciado, que é checada sempre que um usuário manda uma mensagem.
		    		else if (strncmp(buff_in, "/mute", 5) == 0) { //comandos de saída
		    			if(cli->mod == 0){
							send(cli->sockfd,"Voce precisa ser moderador para usar este comando.\n",strlen("Voce precisa ser moderador para usar este comando.\n\0"),0);
						}
						else{
							if (DEBUG) printf("comando mute acionado\n");
							char muted_user[50];
							int j = 0;
							for(int i = 6; i < strlen(buff_in); i++){
								muted_user[j] = buff_in[i];
								j++;
							}
							muted_user[j] = '\0';
							if (DEBUG) printf("muted user: %s\n", muted_user);
							short int flag_mute = 0;

							for(int i = 0; i < cli_count; i++){
								if (flag_mute != 0) break;
								if (strcmp(clients[i]->name, muted_user) == 0){
									if(strcmp(clients[i]->canal, cli->canal) == 0){
										if (DEBUG) printf("canal mutuo: %s\n", cli->canal);

										sprintf(buff_out, "%s foi silenciado do grupo por um moderador e não poderá mais ser ouvido até que seja dessilenciado.\n\n", muted_user);
										send_message(buff_out, cli->uid, cli->canal);

										flag_mute = 1;
										clients[i]->muted = 1;
									}
								}
							}
							if (flag_mute == 0){
								send(cli->sockfd,"Usuario nao existe ou não esta no canal.\n",strlen("Usuario nao existe ou não esta no canal.\n\0"),0);
							}
						}
		    		}
		    		//faz o mesmo que a função /mute, mas revertendo o valor da flag de silenciamento do alvo.
		    		else if (strncmp(buff_in, "/unmute", 7) == 0) { //comandos de saída
		    			if(cli->mod == 0){
							send(cli->sockfd,"Voce precisa ser moderador para usar este comando.\n",strlen("Voce precisa ser moderador para usar este comando.\n\0"),0);
						}
						else{
							if (DEBUG) printf("comando unmute acionado\n");
							char unmuted_user[50];
							int j = 0;
							for(int i = 8	; i < strlen(buff_in); i++){
								unmuted_user[j] = buff_in[i];
								j++;
							}
							unmuted_user[j] = '\0';
							if (DEBUG) printf("unmuted user: %s\n", unmuted_user);
							short int flag_unmute = 0;
							for(int i = 0; i < cli_count; i++){
								if (flag_unmute != 0) break;
								if (strcmp(clients[i]->name, unmuted_user) == 0){
									if(strcmp(clients[i]->canal, cli->canal) == 0){
										if (DEBUG) printf("canal mutuo: %s\n", cli->canal);

										sprintf(buff_out, "%s não está mais sileciado e agora pode ser ouvido por todos!\n\n", unmuted_user);
										send_message(buff_out, cli->uid, cli->canal);

										flag_unmute = 1;
										clients[i]->muted = 0;
									}
								}
							}
							if (flag_unmute == 0){
								send(cli->sockfd,"Usuario nao existe ou não esta no canal.\n",strlen("Usuario nao existe ou não esta no canal.\n\0"),0);
							}
						}
		    		}
		    		//Sai do programa, fazendo as devidas notificações, mudanças de direitos de moderador, decrementando o número
		    		// de usuários do IRC, e fechando o socket.
		    		else if (strcmp(buff_in, "/exit") == 0 || strcmp(buff_in, "/quit") == 0){
		    			if (DEBUG) printf("%s saindo...\n", cli->name);
						if (cli->mod == 1){
						cli->mod = 0;

						sprintf(buff_out, "%s has left\n", cli->name);
						printf("%s", buff_out);
						send_message(buff_out, cli->uid, cli->canal);

						for(int i = 0; i < cli_count; i++){
							if (strcmp(cli->canal, clients[i]->canal) == 0 && strcmp(cli->name, clients[i]->name) != 0){
								clients[i]->mod = 1;
								if (DEBUG) printf("%s é o novo moderador\n", clients[i]->name);
								sprintf(buff_out, "%s é o novo moderador deste canal!\n", clients[i]->name);
								send_message(buff_out, cli->uid, cli->canal);
								break;
							}
						}
					}

					}
					//Caso o caractere '/' for usado mas sem um comando implementado, avisamos o usuário
					// que o comando não existe e recomendamos o uso do comando /help.
		    		else {
		    			strcpy(buff_out, "O comando requisitado não existe. Caso precise, use o comando /help\n\n");
						send_message(buff_out, cli->uid, cli->canal);
		    		}	
		    	}
		    	//Caso seja uma mensagem comum, envia ela a todos os membros do canal, com a identificação de seu nome anteriormente à mensagem em si.
				else{
					sprintf(buff_out,"%s: %s\n", cli->name, buff_in);
					str_trim_lf(buff_out, strlen(buff_out));
					
					printf("%s\n", buff_out);

					if(cli->muted != 1){
						leave_flag = send_message(buff_out, cli->uid, cli->canal);
					}
				}
			}
		}
		else if (receive == 0){
			leave_flag = 1;
		} 
		else {
			printf("ERROR: -1\n");
			leave_flag = 1;
		}

		//são limpados os buffers de input e output.
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

	//configuração para a comunicação via sockets.
	//char *ip = "127.0.0.1";
	int port = 3000;
	int option = 1;
	int uid = 0;
	int listenfd = 0, connfd = 0;
  	struct sockaddr_in serv_addr;
  	struct sockaddr_in cli_addr;
  	pthread_t tid;

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

	//Este loop aceita conexões de qualquer usuário que tente se conectar com o servidor, criando threads distintas para cada cliente,
	// usando a função principal 'handle_client()'
	while(1){
		socklen_t clilen = sizeof(cli_addr);
		connfd = accept(listenfd, (struct sockaddr*)&cli_addr, &clilen);
		print_adress(cli_addr);
		
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
