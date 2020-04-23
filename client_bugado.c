// Client side C/C++ program to demonstrate Socket programming 

//TRABALHO DE REDES
//MATHEUS STEIGENBERG POPULIM - 10734710
//BRUNO GAZONI - 7585037
//BRUNO 2

#include <stdio.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <signal.h>
#define PORT 1337 

volatile sig_atomic_t flag = 0;
void my_function(int sig){ // can be called asynchronously
  	flag = 1; // set flag
}
   
int main(int argc, char const *argv[]){
	signal(SIGINT, my_function);
	signal(SIGPIPE, my_function);

    int sock = 0, valread; 
    struct sockaddr_in serv_addr; 

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) 
    { 
        printf("\n Socket creation error \n"); 
        return -1; 
    } 
   
    serv_addr.sin_family = AF_INET; 
    serv_addr.sin_port = htons(PORT); 
       
    // Convert IPv4 and IPv6 addresses from text to binary form 
    if(inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr)<=0)  
    { 
        printf("\nInvalid address/ Address not supported \n"); 
        return -1; 
    }

    //printf("%ld\n", sizeof(serv_addr));

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        printf("\nConnection Failed \n"); 
        return -1; 
    }


    char* msg_recv = malloc(sizeof(char)*4096);
    char* msg_send = malloc(sizeof(char)*8192);
	char* buffer = malloc(sizeof(char)*4096);
    fd_set read_fds;

	while(1){

		if(flag == 1){
			break;
		}

		// CLIENTE SIDE CLIENTE SIDE CLIENTE SIDE CLIENTE SIDE CLIENTE SIDE CLIENTE SIDE CLIENTE SIDE CLIENTE SIDE CLIENTE SIDE
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
	      	perror("select:");
	      	exit(1);
	    }

	    /* After select, if an fd's bit is set, then there is data to read. */      
	    if( FD_ISSET(sock, &read_fds)){
	        /* There is data waiting on your socket.  Read it with recv(). */
	        valread = recv(sock , msg_recv, 4096, 0);
	        if(valread == 0){
	        	continue;
	        }
	        printf("Servidor: %s\n", msg_recv);
	    }


	    if( FD_ISSET(STDIN_FILENO, &read_fds )){
	    	fscanf(stdin,"%[^\n]%*c",msg_send);
	    	printf("%d\n", (int)strlen(msg_send));
	    	msg_send[strlen(msg_send)] = 0;
	    	int offset = 0;
		    while(strlen(msg_send+offset) >= 99){
		    	memcpy(buffer,msg_send+offset,99);
		    	buffer[99] = '\0';
		    	valread = send(sock , buffer, 100, 0);
		    	offset += 99;
		    }
		    printf("aa %d\n", (int)strlen(msg_send+offset));
	    	valread = send(sock , msg_send+offset, strlen(msg_send+offset)+1, 0);
	    }
	}
	close(sock);

	free(msg_recv);
	free(msg_send);

    return 0; 
} 
