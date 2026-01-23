#include<stdio.h>
#include<stdlib.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<cstring>
#include<netdb.h>
#include<arpa/inet.h>
#include<netinet/in.h>

int main(int argc,char* argv[]) {
    struct addrinfo hints,* res;
    int status;
    char ipstr[INET6_ADDRSTRLEN];
    if(argc != 3) {
        fprintf(stderr,"usage: showip hostname");
        return 1;
    }
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    if((status = getaddrinfo(argv[2],NULL,&hints,&res)) != 0) {
        fprintf(stderr,"getaddrinfo: %s\n",gai_strerror(status));
        return 2;
    }
    printf("IP Addresses: \n");
    for(struct addrinfo* p = res;p!=NULL;p=p->ai_next) {
        void* addr;
        char* ipver;
        struct sockaddr_in *ipv4;
        struct sockaddr_in6 *ipv6;

        if(p->ai_family == AF_INET) {
            ipv4 = (struct sockaddr_in*)p->ai_addr;
            addr = &(ipv4->sin_addr);
            ipver = "IPV4";
        } else {
            ipv6 = (struct sockaddr_in6*)p->ai_addr;
            addr = &(ipv6->sin6_addr);
            ipver = "IPV6";
        }

        inet_ntop(p->ai_family,addr,ipstr,sizeof ipstr);
        printf("%s:   %s\n",ipver,ipstr);
    }

    freeaddrinfo(res);
    return 0;
}
