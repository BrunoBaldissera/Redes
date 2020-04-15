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


    char* buffer = malloc(sizeof(char)*1024);
    char* mensagem = malloc(sizeof(char)*1024);
    while(1){
        fscanf(stdin,"%[^\n]%*c",mensagem);
        mensagem[strlen(mensagem)] = 0; // põe o \0
        if(strlen(mensagem)!= 0){
            send(sock , mensagem , strlen(mensagem)+1 , 0);
            printf("cliente: %s\n",mensagem);
        }
        valread = recv(sock , buffer, 1024, 0);
        if(valread != 0){
            printf("server: %s\n",buffer);
        }
    }

    return 0; 
} 