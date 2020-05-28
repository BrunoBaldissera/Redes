//TRABALHO DE REDES - LADO DO CLIENTE
//MATHEUS STEIGENBERG POPULIM -  10734710
//BRUNO GAZONI - 7585037
//BRUNO BALDISSERA - 10724351

#include <arpa/inet.h> 
#include <errno.h>
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
void quit(int signum){
	printf("Programa interrompido, saindo do programa... (%d)", signum);
	exit(1);
}

void ignore(){
	//ignora
	printf("Para sair do programa use o comando /quit ou CTRL + D\n");
}

//Função que facilita o cálculo do tempo percorrido para executar um ping
double timeInSeconds() {
    struct timeval tv;
    gettimeofday(&tv,NULL);
    return (((double)tv.tv_sec))+((double)tv.tv_usec/1000000.0);
}

//Comandos que podem ser enviados via terminal para operar o chat
int commands(char* word){
	if(strcmp(word,"ping") == 0){
		return 2;
	}
	if(strcmp(word,"rping") == 0){
		return 3;
	}
	if(strcmp(word,"quit") == 0){
		return 1;
	}
	return 0;
}
   
int main(int argc, char const *argv[]){
	int flag;
	
	//tratando CTRL + C
	if (signal(SIGINT, SIG_IGN) != SIG_IGN)
    signal(SIGINT, ignore);

	char input[40];
	printf("Bem vindo a casa do caralho. o que deseja fazer?\n");
	while(1){
		scanf("%s",input);
		if(input[0] == '/'){
			flag = commands(input+1);
		}
		if(flag == 4){
			break;
		}
	}

	int valread, sock;
	struct sockaddr_in serv_addr;

	//Criação do socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) { 
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
        printf("\nConexão falhou. Execute o servidor primeiro.\n"); 
        return -1; 
    }
	printf("Conectado!\n\n");

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

	int ping_flag = 0, msg_count = 0, nome_def = 0;
	
	printf("Limite do tamanho da mensagem: %d. Mensagens maiores que %d serão truncadas\n\n", msg_max_size, msg_max_size-1);
	printf("Defina aqui o seu nome a ser visto pelos outros seguido da tecla enter:\n\n");

	/*Enquanto o programa está ativo, este laço é executado,
	  onde são executadas as ações necessárias de interação entre cliente e servidor.*/
	while(1){
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
			int msg_size;


			//Tratamento de '\' enviada por terminal
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
        		if(msg_count == 0){
        			strcpy(nome,msg_recv);
        		}
	        	else{
	        		if(msg_recv[0] == '/'){
	        			//tratamento de ping
	        			flag = commands(msg_recv+1);
	        			if(flag == 2 && ping_flag == 0){
	        				printf("%s pingou você\n", nome);
	        				valread = send(sock ,"/rping\0", strlen("/rping\0")+1, 0);
	        			}
	        			else if(flag == 3 && ping_flag == 1){
	        				end = timeInSeconds();
	        				printf("Ping feito com sucesso, tempo demorado: %lf segundos\n", end-start);
	        				ping_flag = 0;
	        			}
	        		}
	        		//exibição da mensagem
	        		else{
	        			printf("%s: %s\n",nome, msg_recv);
	        		}
	        	}
        		msg_count++;
        	}
        }
	}

	close(sock);
	free(buffer);
	free(msg_recv);
	free(msg_send);

    return 0; 
}
