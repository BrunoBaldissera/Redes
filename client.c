//TRABALHO DE REDES - LADO DO CLIENTE
//MATHEUS STEIGENBERG POPULIM -  10734710
//BRUNO GAZONI - 7585037
//BRUNO BALDISSERA - 10724351

#include <stdio.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/time.h>
#define PORT 1337 

//Declaração da flag como variável global, para uso na função que pode ser chamada de forma assíncrona
volatile sig_atomic_t flag = 0;

void set_flag(int sig){
  	flag = 1;
}

//Comandos que podem ser enviados via terminal para operar o chat
int commands(char* word){
	if(strcmp(word,"exit") == 0){
		return 1;
	}
	if(strcmp(word,"quit") == 0){
		return 1;
	}
	if(strcmp(word,"sair") == 0){
		return 1;
	}
	if(strcmp(word,"abandonar o navio") == 0){
		return 1;
	}
	if(strcmp(word,"sebo nas canelas") == 0){
		return 1;
	}
	return 0;
}
   
int main(int argc, char const *argv[]){

	//Funções para finalizar o programa via flag setada por 1, redirecionada aos sinais do SO
	signal(SIGINT, set_flag);
	signal(SIGPIPE, set_flag);

    int sock = 0, valread;
	struct sockaddr_in serv_addr;

	//Criação do socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) 
    { 
        printf("\n Socket creation error \n"); 
        return -1; 
    } 
   
    //Configuração do socket
    serv_addr.sin_family = AF_INET; 
    serv_addr.sin_port = htons(PORT); 
       
	//Conversão de endereços IPv4 para binário
    if(inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr)<=0)  
    { 
        printf("\nInvalid address/ Address not supported \n"); 
        return -1; 
    }

	//Conexão com o socket
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        printf("\nConnection Failed \n"); 
        return -1; 
    }

	//==== Comunicação ====//
	
	//Variáveis para armazenar mensagens
	char* msg_recv = malloc(sizeof(char)*4096);
	char* msg_send = malloc(sizeof(char)*8192);
	char* buffer = malloc(sizeof(char)*4096);
	char nome[60];

	//Tamanho máximo da mensagem enviada em um único bloco de texto
	int msg_max_size = 1024;

	//Demux dos descritores de arquivo prontos para serem lidos
	fd_set read_fds;

	int msg_count = 0;

	printf("Conectado!\n");
	printf("A primeira mensagem que você enviar será como o outro usuário visualizará seu nome.\n");
	printf("Limite do tamanho da mensagem: %d. Ou seja, mensagens maiores que %d serão truncadas\n", msg_max_size, msg_max_size-1);

	//Timeouts para tempo de processamento do terminal
	struct timeval timeout_time;
    timeout_time.tv_sec = 3;
    timeout_time.tv_usec = 0;

	/*Enquanto o programa está ativo, este laço é executado,
	onde são executadas as ações ecessárias de interação entre cliente e servidor.
 	A cada iteração verificamos se a flag de interrupção indica interrupção, e caso positivo, 
	enviamos uma mensagem de encerramento do programa e encerramos o laço.*/

	while(1){

		if(flag == 1){
			break;
		}

		//variável que guarda o descritor de artigo do stdin
	    int fd_max = STDIN_FILENO;

		//Configuração dos descritores de arquivo a serem usados para leitura de mensagens
	    FD_ZERO(&read_fds);
	    FD_SET(STDIN_FILENO, &read_fds);
	    FD_SET(sock, &read_fds);

		//O sock não pode exceder o maior bit do stdin
	    if( sock > fd_max ){
	    	fd_max = sock; 
	 	}

		//Select para checar dados em um dos descritores de arquivo
	    if (select(fd_max + 1, &read_fds, NULL, NULL, &timeout_time) == -1){
	      	perror("select:");
	      	exit(1);
	    }

		/* O próximo bloco assinala o socket para o stdin, recebe mensagens através dele e quebra
		a mensagem em mensagens menores caso seja ela ultrapasse o número de caracteres máximo pré-determinado */
		if( FD_ISSET(STDIN_FILENO, &read_fds )){
	    	fscanf(stdin,"%[^\n]%*c",msg_send);
	    	msg_send[strlen(msg_send)] = 0;
	    	int offset = 0;
			int msg_size;
	    	int flag;

			//Tratamento de '\' enviada por terminal
	    	if(msg_send[0] == '\\'){
	    		flag = commands(msg_send+1);
	    		if(flag == 1){
	    			close(sock);
	    			break;
	    		}
	    	}

		    for(int offset = 0;strlen(msg_send+offset) > 0;offset += msg_max_size-1){
		    	msg_size = strlen(msg_send+offset);
		    	if(msg_size < msg_max_size){
					memcpy(buffer,msg_send+offset,msg_size);
		    		buffer[msg_size] = '\0';
		    		sleep(0.01); //dorme por 50 milissegundos
		    		valread = send(sock , buffer, msg_size+1, 0);
		    		break;
		    	}
		    	else{
		    		memcpy(buffer,msg_send+offset,msg_max_size-1);
		    		buffer[msg_max_size-1] = '\0';
		    		sleep(0.01);
		    		valread = send(sock , buffer, msg_max_size, 0);
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

			//exibição da mensagem
	        else{
	        	if(msg_count == 0){
	        		strcpy(nome,msg_recv);
	        	}
	        	else{
	        		printf("%s: %s\n",nome, msg_recv);
	        	}
	        	msg_count++;
	        }
	    }
	}

	close(sock);
	free(msg_recv);
	free(msg_send);

    return 0; 
} 
