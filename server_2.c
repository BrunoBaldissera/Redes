// Server side C/C++ program to demonstrate Socket programming

//TRABALHO DE REDES
//MATHEUS STEIGENBERG POPULIM - 10734710
//BRUNO 1
//BRUNO 2

#include <unistd.h> 
#include <stdio.h> 
#include <sys/socket.h> 
#include <stdlib.h> 
#include <netinet/in.h> 
#include <string.h> 
#include <stdlib.h>

//vendo se essa função funciona
#include <sys/select.h>
//

#define PORT 1337

int main(int argc, char const *argv[]) 
{ 
    int server_fd, sock, valread; 
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
    if ((sock = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen))<0) { 
        perror("accept"); 
        exit(EXIT_FAILURE); 
    }

    //Comunicação:


    char* buffer = malloc(sizeof(char)*1024);
    char* mensagem = malloc(sizeof(char)*1024);
    while(1){
    	fscanf(stdin,"%[^\n]%*c",mensagem);
	    mensagem[strlen(mensagem)] = 0; // põe o \0
    	if(strlen(mensagem)!= 0){
    		send(sock , mensagem , strlen(mensagem)+1 , 0);
    		printf("server: %s\n",mensagem);
    	}
    	valread = recv(sock , buffer, 1024, 0);
    	if(valread != 0){
    		printf("cliente: %s\n",buffer);
    	}
   	}
    return 0; 
} 