// Server side C/C++ program to demonstrate Socket programming

//TRABALHO DE REDES
//MATHEUS STEIGENBERG POPULIM 	- 10734710
//BRUNO GAZONI 			- 7585037
//BRUNO BALDISSERA		- 10724351	

//Esse código é potencialmente periogoso e deve ser manipulado com cuidado, altos níveis de ceticismo e ironia.
// Licença para redistribuição: NDCEM 4.1 -> Ninguém Deveria Copiar Essa Merda

#include <unistd.h> 
#include <stdio.h> 
#include <sys/socket.h> 
#include <stdlib.h> 
#include <netinet/in.h> 
#include <string.h> 
#include <stdlib.h>
#include <signal.h>
#include <pthread.h>
#define PORT 1337

//Aqui declaramos função chamada quando recebe-se um sinal de interrupção.
void sighandler(int);

//Declaramos globalmente o socket
int sock;

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
	//Tratamos aqui os sinais de interrupção (SIGINT ou SIGPIPE), para os quais é chamada a função que atribui a flag de interrupção.
	signal(SIGINT, sighandler);
	signal(SIGPIPE, sighandler);

	int server_fd, sock, valread; 
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
	// Ligamos o socket à porta 1337 
	address.sin_port = htons( PORT ); 
   
	// Fazemos o bind do socket ao endereço correto
	if (bind(server_fd, (struct sockaddr *)&address, sizeof(address))<0){ 
	   	perror("bind failed"); 
    	exit(EXIT_FAILURE); 
	}

	// Marcamos o server_fd como o socket passivo para receber a conexão
	if (listen(server_fd, 3) < 0){ 
    	perror("listen"); 
    	exit(EXIT_FAILURE); 
	}

	/*Extraimos aqui a primeira conexão válida dentre a lista de pendentes, criamos um novo socket
	  com o mesmo protocolo e família de endereço do server_fd criado e alocamos um novo descritor de arquivo do
	  socket, via a função accpet()*/
	if(((sock = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0 )) { 
		perror("accept"); 
		exit(EXIT_FAILURE); 
	}

	// A comunicação: aqui estão as variáveis que representam cada mensagem enviada e recebida, e o buffer que tratará o tamanho delas, caso necessário.
	char* msg_recv = malloc(sizeof(char)*4096);
	char* msg_send = malloc(sizeof(char)*8192);
	char* buffer = malloc(sizeof(char)*4096);
	char nome[60];

	// Tamanho máximo da mensagem
	int msg_max_size = 10;
	
	// Representa um conjunto de descritores de arquivo (struct vem da biblioteca select.c)
	fd_set read_fds;

	int msg_count = 0;

	printf("Conectado!\n");
	
	printf("A primeira mensagem que você enviar será o como o outro usuário visualizará seu nome.\n");
	printf("Limite do tamanho da mensagem: %d. Ou seja, mensagens maiores que %d serão truncadas\n", msg_max_size, msg_max_size-1);

	sleep(0.01);

	/*Enquanto o programa está ativo, este laço é executado,
	  onde são executadas as ações ecessárias de interação entre cliente e servidor.*/
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
	    	if (select(fd_max + 1, &read_fds, NULL, NULL, NULL) == -1){
	      		perror("select: ");
	      		exit(1);
	    	}

		if( FD_ISSET(STDIN_FILENO, &read_fds )){
	    	fscanf(stdin,"%[^\n]%*c",msg_send);
	    	//printf("%d\n", (int)strlen(msg_send));
	    	msg_send[strlen(msg_send)] = 0;
	    	int offset = 0;
			int msg_size;
	    	int flag;
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


	    /* After select, if an fd's bit is set, then there is data to read. */      
	    if( FD_ISSET(sock, &read_fds)){
	        /* There is data waiting on your socket.  Read it with recv(). */
	        valread = recv(sock , msg_recv, 4096, 0);
	        if(valread == 0){
	        	printf("%s desconectou, triste né meu filho?\n", nome);
	        	break;
	        }
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

	// Fechamos o socket e liberamos a memória alocada para as variáveis
	close(sock);
	free(msg_recv);
	free(msg_send);
	free(buffer);

    	return 0;
}

/* Essa função é chamada pela função signal (que por sua vez é chamada quando o programa recebe um sinal de interrupção)
	e imprime tanto no terminal cliente quanto no servidor uma mensagem de saída antes de fechar o programa.*/ 
void sighandler(int signum){
	printf("Programa interrompido, saindo do programa... (%d)", signum);
	send(sock, "SAINDO DO PROGRAMA....", strlen("SAINDO DO PROGRAMA....")+1, 0);
	close(sock);
	exit(1);
}
