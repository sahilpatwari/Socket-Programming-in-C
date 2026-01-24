//Purpose: Code for a TCP-based Echo Server

#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<string.h>
#include<netdb.h>
#include<arpa/inet.h>
#include<netinet/in.h>

#define PORT "3490"
#define BACKLOG 10
#define BUFFER 100
int main() {
    struct sockaddr_storage their_addr;
    struct addrinfo hints,*res,*p;
    int status,sockfd,newfd,yes = 1;
    socklen_t sin_size;
    char buffer[BUFFER];
    memset(&hints,0,sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if((status = getaddrinfo(NULL,PORT,&hints,&res)) != 0) {
        fprintf(stderr,"getaddrinfo: %s\n",gai_strerror(status));
        return 1;
    }
   
    for(p = res;p != NULL;p = p->ai_next) {
        if((sockfd = socket(p->ai_family,p->ai_socktype,p->ai_protocol)) == -1) {
            perror("socket");
            continue;
        }

        if(setsockopt(sockfd,SOL_SOCKET, SO_REUSEADDR,&yes,sizeof(int)) == -1) {
            perror("setsocketopt");
            exit(1);
        }
        
        if(bind(sockfd,p->ai_addr,p->ai_addrlen) == -1) {
            close(sockfd);
            perror("bind");
            continue;
        }
        break;
    }
    
    if(p == NULL) {
        fprintf(stderr,"Server couldn't bind to a specific port");
        exit(1);
    }
    freeaddrinfo(res);
    if(listen(sockfd,BACKLOG) == -1) {
        perror("listen");
        exit(1);
    }

    printf("Server is listening...\n");
    
    while(1) {
        sin_size = sizeof their_addr;
        newfd = accept(sockfd,(struct sockaddr*)&their_addr,&sin_size);

        if(newfd == -1) {
            perror("accept");
            continue;
        }
        
        printf("Client Connected\n");
        int numBytes = recv(newfd,buffer,BUFFER - 1,0);

        if(numBytes > 0) {
            buffer[BUFFER] = '\0';
            printf("Received: %s",buffer);
            if(send(newfd,buffer,numBytes,0) == -1) {
                perror("send");
            }
            printf("Sent: %s",buffer);
        } else {
            perror("receive");
        }
        close(newfd);
    }
    return 0;
}
