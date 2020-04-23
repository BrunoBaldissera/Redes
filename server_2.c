// Server side C/C++ program to demonstrate Socket programming

//TRABALHO DE REDES
//MATHEUS STEIGENBERG POPULIM - 10734710
//BRUNO GAZONI - 7585037
//BRUNO 2

#include <unistd.h> 
#include <stdio.h> 
#include <sys/socket.h> 
#include <stdlib.h> 
#include <netinet/in.h> 
#include <string.h> 
#include <stdlib.h>
#include <signal.h>
#define PORT 1337

//Esta é a flag que determina se o processo do servidor sofreu interrupção ou não. É inicializada com 0 (falso).
volatile sig_atomic_t int_flag = 0;

//Aqui temos a função que atribui 1 (verdadeiro) para a flag de interrupção.
void sighandler(int);

int sock;

int main(int argc, char const *argv[]){
  	//Tratamos aqui os sinais de interrupção ou SIGPIPE, para os quais é chamada a função que atribui a flag de interrupção.
	signal(SIGINT, sighandler);
    signal(SIGPIPE, sighandler);

    int server_fd, valread; 
    struct sockaddr_in address; 
    int opt = 1; 
    int addrlen = sizeof(address);
       
    // Creating socket file descriptor 
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0){ 
    	perror("socket failed"); 
        	exit(EXIT_FAILURE); 
    } 
       
    // Forcefully attaching socket to the port 8080 
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) { 
    	perror("setsockopt"); 
       	exit(EXIT_FAILURE); 
    }
 
    address.sin_family = AF_INET; 
    address.sin_addr.s_addr = INADDR_ANY; 
    address.sin_port = htons( PORT ); 
       
    // Forcefully attaching socket to the port 8080 
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address))<0){ 
       	perror("bind failed"); 
       	exit(EXIT_FAILURE); 
    }
 
    if (listen(server_fd, 3) < 0){ 
       	perror("listen"); 
       	exit(EXIT_FAILURE); 
    }
 	if(((sock = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0 )) { 
    	perror("accept"); 
     	exit(EXIT_FAILURE); 
  	}

    //Comunicação:
    char* msg_recv = malloc(sizeof(char)*1024);
    char* msg_send = malloc(sizeof(char)*1024);

    fd_set read_fds;

	/*Enquanto o programa está ativo, este laço é executado,
	  onde são executadas as ações ecessárias de interação entre cliente e servidor.
 	  A cada iteração verificamos se a flag de interrupção indica interrupção, e caso positivo, 
	  enviamos uma mensagem de encerramento do programa e encerramos o laço.*/
	while(1){
		if(int_flag == 1){
			send(sock, "SAINDO DO PROGRAMA....", strlen("SAINDO DO PROGRAMA....")+1, 0);
			break;
		}

		// SERVER SIDE SERVER SIDE SERVER SIDE SERVER SIDE SERVER SIDE SERVER SIDE SERVER SIDE SERVER SIDE SERVER SIDE SERVER SIDE SERVER SIDE SERVER SIDE 
		int fd_max = STDIN_FILENO;

	    	/* Set the bits for the file descriptors you want to wait on. */
	    	FD_ZERO(&read_fds);
	    	FD_SET(STDIN_FILENO, &read_fds);
	    	FD_SET(sock, &read_fds);

	    	/* The select call needs to know the highest bit you set. */    
	    	if( sock > fd_max ){
	    		fd_max = sock; 
	 	}

	    	/* Wait for any of the file descriptors to have data. */
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
			fscanf(stdin,"%[^\n]%*c",msg_send);
		    	msg_send[strlen(msg_send)] = 0;
		    	valread = send(sock , msg_send, strlen(msg_send)+1, 0);
		}
	}
	
	close(sock);
	free(msg_recv);
	free(msg_send);

    return 0;
}

void sighandler(int signum){ //Sua chamada pode ser feita assíncronamente
	printf("Saindo do programa, signum: %d", signum);
	send(sock, "SAINDO DO PROGRAMA....", strlen("SAINDO DO PROGRAMA....")+1, 0);
	exit(1);
}
