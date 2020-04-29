// Server side C/C++ program to demonstrate Socket programming

//TRABALHO DE REDES
//MATHEUS STEIGENBERG POPULIM 	- 10734710
//BRUNO GAZONI 			- 7585037
//BRUNO BALDISSERA CARLOTTO 	- 10724351

#include <unistd.h> 
#include <stdio.h> 
#include <sys/socket.h> 
#include <stdlib.h> 
#include <netinet/in.h> 
#include <string.h> 
#include <stdlib.h>
#include <signal.h>
#define PORT 1337

//Aqui declaramos função chamada quando recebe-se um sinal de interrupção.
void sighandler(int);

//Declaramos globalmente o socket
int sock;

int main(int argc, char const *argv[]){
	//Tratamos aqui os sinais de interrupção (SIGINT ou SIGPIPE), para os quais é chamada a função que avisa que o usuário está saindo do programa.
	signal(SIGINT, sighandler);
    	signal(SIGPIPE, sighandler);

    	int server_fd, valread; 
    	struct sockaddr_in address; 
    	int opt = 1; 
    	int addrlen = sizeof(address);
       
   	// Declaramos aqui o descritor de arquivo do socket 
   	if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0){ 
   		perror("socket failed"); 
   		exit(EXIT_FAILURE); 
   	} 
       
	//Configuramos as opções do socket
  	if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) { 
  		perror("setsockopt"); 
  	     	exit(EXIT_FAILURE); 
  	}
    	address.sin_family = AF_INET; 
    	address.sin_addr.s_addr = INADDR_ANY; 
	// Ligando o socket à porta 1337 
    	address.sin_port = htons( PORT ); 
    	
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

    	//A comunicação: aqui estão as variáveis que representam cada mensagem enviada e recebida.
    	char* msg_recv = malloc(sizeof(char)*1024);
    	char* msg_send = malloc(sizeof(char)*1024);
	
	fd_set read_fds;

	/*Enquanto o programa está ativo, este laço é executado,
	  onde são executadas as ações ecessárias de interação entre cliente e servidor.*/
	while(1){
		int fd_max = STDIN_FILENO;

	    	/* Configura os buts para os descritores de arquivos*/
	    	FD_ZERO(&read_fds);
	    	FD_SET(STDIN_FILENO, &read_fds);
	    	FD_SET(sock, &read_fds);

	    	/* Selecionamos o maior bit configurado para ser usado na função select abaixo.*/    
	    	if( sock > fd_max ){
	    		fd_max = sock; 
	 	}

	    	/* Aqui se espera até que um dos descritores de arquivo tenham dados (não sejam nulos), para depois continuar a comunicação */
	    	if (select(fd_max + 1, &read_fds, NULL, NULL, NULL) == -1){
	      		perror("select: ");
	      		exit(1);
	    	}

	    	/* After select, if an fd's bit is set, then there is data to read. */      
	    	if( FD_ISSET(sock, &read_fds)){
	        	/* There is data waiting on your socket.  Read it with recv(). */
	        	valread = recv(sock , msg_recv, 1024, 0);
	        	if(valread == 0){
	        		continue;
	        	}
	        	printf("Cliente: %s\n", msg_recv);
		}

		if( FD_ISSET(STDIN_FILENO, &read_fds )){
		/* The user typed something.  Read it fgets or something.
		   Then send the data to the server. */
			fscanf(stdin,"%[^\n]%*c", msg_send);
		    	msg_send[strlen(msg_send)] = 0;
		    	valread = send(sock , msg_send, strlen(msg_send)+1, 0);
		}
	}
	// Fechamos o socket e liberamos os espaços
	close(sock);
	free(msg_recv);
	free(msg_send);

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
