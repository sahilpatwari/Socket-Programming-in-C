//Purpose : Simple HTTP Web Server which can handle basic GET requests

#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<string.h>
#include<netdb.h>
#include<arpa/inet.h>
#include<netinet/in.h>
#include<pthread.h>

#include "my_functions.h"
#include<stdbool.h>

#define PORT "3490"
#define BACKLOG 10
#define BUFFER 1024

typedef struct {
    int client_fd;
}thread_args_t;


bool is_Path(const char* pathName,int newfd) {
    //Avoid Directory Traversal Attacks
     if(strstr(pathName,"..")) {
        const char* response = "HTTP/1.1 403 Forbidden\r\n"
                               "\r\n"
                               "Access Denied";
        send(newfd,response,strlen(response),0);
        return false;
    }

    if(strlen(pathName) > 1023) {
        const char* response = "HTTP/1.1 414 URI Too Long\r\n"
                               "\r\n"
                               "URL is too long to be supported by the server";
        send(newfd,response,strlen(response),0);
        return false;
    } 
    
    for(int i = 0;i < strlen(pathName);i++) {
        char c = pathName[i];
        if(c >= 'a' && c <= 'z') continue;
        if(c >= 'A' && c <= 'Z') continue;
        if(c >= '0' && c <= '9') continue;
        if(c == '.' || c == '-' || c == '_' ||  c == '/') continue;
        const char* response = "HTTP/1.1 400 Bad Request\r\n"
                               "\r\n"
                               "Invalid URL";
        send(newfd,response,strlen(response),0);
        return false;

        return false;
    }
    return true;
    
}


void* handle_client(void* args) {
    thread_args_t *thread_args = (thread_args_t*)args;
    int newfd = thread_args->client_fd;
    free(thread_args);
    char buffer[BUFFER];
    int bytes_received = recv(newfd,buffer,BUFFER - 1,0);
    buffer[bytes_received] ='\0';
    if(bytes_received < 0) {
        perror("receive");
        exit(1);
    }

    const char* response;
    char method[16],path[1024],protocol[16];
    int count = sscanf(buffer,"%15s %1023s %15s",method,path,protocol);
    printf("%s\n",path);
    if(count != 3) {
        fprintf(stderr,"Unknown Error in Parsing the Request");
        response = "HTTP/1.1 500 Internal Server Error\r\n"
                "\r\n"
                "Internal Server Error";
        
        if(send(newfd,response,strlen(response),0) == -1) {
            perror("send");
            close(newfd);
        }
    } else {
        if(!is_Path(path,newfd)) return 0;
        if(strcmp(method,"GET") == 0) {
            if(strcmp(path,"/") == 0) {
                serve_file(newfd,"/index.html");
            } else {
                serve_file(newfd,path);
            }
        } else if(strcmp(method,"POST") == 0) {
            int content_length = 0;
            char* cl_ptr = strstr(buffer,"Content-Length: ");

            if(cl_ptr) {
                    content_length = strtol(cl_ptr + 16,NULL,10);
            } else {
                    response = "HTTP/1.1 411 Length Required\r\n"
                    "\r\n"
                    "Invalid! POST Request invalid without Content-Length";
                    send(newfd,response,strlen(response),0);
            }
            
                int total_bytes_received = 0;
                char* body_start = strstr(buffer,"\r\n\r\n");

            if(strcmp(path,"/image") == 0) {
                    FILE* fp = fopen("uploaded.png","wb");
                    if(!fp) {
                        perror("File Open Failed");
                        close(newfd);
                        return 0;
                    }
                
                    if(body_start != NULL) {
                        body_start += 4;

                        int header_size = body_start - buffer;
                        int initial_body_bytes = bytes_received - header_size;

                        if(initial_body_bytes > 0 ) {
                                fwrite(body_start,1,initial_body_bytes,fp);
                                total_bytes_received += initial_body_bytes;
                        } 
                    }

                    while(total_bytes_received < content_length) {
                            memset(buffer,0,BUFFER);

                            int n = recv(newfd,buffer,BUFFER - 1,0);

                            if(n < 0) {
                                printf("Partial disconnect during upload\n");
                                break;
                            }

                            fwrite(buffer,1,n,fp);
                            total_bytes_received += n; 
                    }

                    printf("Upload Complete\n");
                    fclose(fp);

                    response = "HTTP/1.1 200  OK\r\n"
                            "\r\n"
                            "Upload Successful";
                    send(newfd,response,strlen(response),0);
            } else if(strcmp(path,"/details") == 0){
                    if(body_start != NULL) {
                        body_start += 4;
                    }
                    int i = 0;
                    char text[512];
                    while(i < strlen(body_start) && i < 511) {
                        text[i] = body_start[i];
                        i++;
                    }
                    text[i] ='\0';
                    i = 0;
                    printf("%s\n",text);
                    char* name_ptr = strstr(text,"Name");
                    char clean_name[100];
                    if(name_ptr) {
                    name_ptr += 5;
                    }
                    while(*name_ptr != '&') {
                    clean_name[i] = *name_ptr;
                    i++;
                    name_ptr++;
                    }
                    char info[512];
                    snprintf(info,sizeof info ,
                            "HTTP/1.1 200 OK \r\n"
                            "Content-Type: text/html \r\n"
                            "\r\n"
                            "<html><body><h1>Hello %s! Your details have been saved"
                            ,clean_name);
                    send(newfd,info,strlen(info),0);
            } else {
                    response = "HTTP/1.1 501 Not Implemented\r\n"
                    "\r\n"
                    "Server does not support this Request";
                    send(newfd,response,strlen(response),0);
            }

        } else {
            response = "HTTP/1.1 501 Not Implemented\r\n"
                    "\r\n"
                    "Server does not support this Request";
            send(newfd,response,strlen(response),0);
        }
    }

    close(newfd);
}


int main() {
    struct sockaddr_storage their_addr;
    struct addrinfo hints, * res, * p;
    socklen_t sin_size;
    int status, sockfd, newfd, yes = 1;
    
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

        if(setsockopt(sockfd,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof(int)) == -1) {
            perror("setsockopt");
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
        fprintf(stderr,"Server couldn't bind to a specific port\n");
        exit(1);
    } 
    freeaddrinfo(res);

    if(listen(sockfd,BACKLOG) == -1) {
        perror("listen");
        exit(1);
    }
    printf("Server is Listening\n");
    while(1) {
        sin_size = sizeof their_addr;
        newfd = accept(sockfd,(struct sockaddr*)&their_addr,&sin_size);

        if(newfd == -1) {
            perror("accept");
            exit(1);
        }

        thread_args_t *args = malloc(sizeof(thread_args_t));
        if(!args) {
            perror("malloc");
            close(newfd);
            return 0;
        }
        
        args->client_fd = newfd;
        pthread_t tid;
        printf("Spawning thread for the client\n");
        printf("\nNumber of Threads: %d\n",numThread);
        if(pthread_create(&tid,NULL,handle_client,(void*)args) != 0) {
            perror("pthread");
            free(args);
            close(newfd);
        } else {
            pthread_detach(tid);
        }

    }
    return 0;
}