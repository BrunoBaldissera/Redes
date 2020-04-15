// Client side C/C++ program to demonstrate Socket programming 

//TRABALHO DE REDES
//MATHEUS STEIGENBERG POPULIM - 10734710
//BRUNO 1
//BRUNO 2

#include <stdio.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h> 
#define PORT 1337 
   
int main(int argc, char const *argv[]) 
{ 
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


    char* msg_recv = malloc(sizeof(char)*1024);
    char* msg_send = malloc(sizeof(char)*1024);

    fd_set read_fds;

	while(1){

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
	        valread = recv(sock , msg_recv, 1024, 0);
	        printf("cliente: %s\n", msg_recv);
	    }

	    if( FD_ISSET(STDIN_FILENO, &read_fds )){
	        /* The user typed something.  Read it fgets or something.
	           Then send the data to the server. */
	    	fscanf(stdin,"%[^\n]%*c",msg_send);
	    	valread = send(sock , msg_send, 1024, 0);
	    }
	}

    return 0; 
} 