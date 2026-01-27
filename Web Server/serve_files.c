#include<stdio.h>
#include<string.h>
#include<sys/socket.h>
#include<sys/types.h>
#include "my_functions.h"

const char* getMimeType(const char* filename) {
    const char* dot = strrchr(filename,'.');
    if(strstr(dot,".html") != NULL) return "text/html";
    else if(strstr(dot,".css") != NULL) return "text/css";
    else if(strstr(dot,".jpeg") != NULL) return "image/jpeg";
    else if(strstr(dot,".png") != NULL) return "image/png";
    return "text/plain";
}


void serve_file(int newfd,const char* path) {
    char file_path[256];
    snprintf(file_path,sizeof(file_path),"./public/%s",path);

    FILE* file = fopen(file_path,"rb");

    if(!file) {
        const char* response = "HTTP/1.1 404 Not Found\r\n"
                               "\r\n"
                               "Resource Not Found";
        send(newfd,response,strlen(response),0);
        return;
    }
    
    fseek(file,0,SEEK_END);
    long fsize = ftell(file);
    fseek(file,0,SEEK_SET);
    char headers[512];
    snprintf(headers,sizeof(headers),
             "HTTP/1.1 200 OK\r\n"
              "Content-Type: %s\r\n"
              "Content-Length: %ld\r\n"
               "\r\n"
            ,getMimeType(file_path),fsize);
    
   send(newfd,headers,strlen(headers),0);

   char buffer[1024];
   size_t bytes_read;
   while((bytes_read = fread(buffer,1,sizeof(buffer),file)) > 0) {
       send(newfd,buffer,sizeof(buffer),0);
   }

   fclose(file);
   printf("Served file %s %ld bytes",file_path,fsize);
}