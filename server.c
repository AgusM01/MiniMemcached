#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/epoll.h>


#define PORT_NUM_TEXT   8888
#define PORT_NUM_BIN    8889

/*Crea un socket TCP en dominio IPv4*/
void sock_creation(int* sockfd){
    
    *sockfd = socket(AF_INET, SOCK_STREAM, 0);
    assert(*sockfd != -1);

    struct sockaddr_in sa;
    sa.sin_family = AF_INET;
    sa.sin_port = htons(PORT_NUM_TEXT);
    sa.sin_addr.s_addr = htonl(INADDR_ANY);

    bind(*sockfd, (struct sockaddr*) &sa, sizeof(sa));
    return;
}

/*Pone el socket en escucha */
void sock_connection(int* sockfd, int epollfd, struct epoll_event* ev){
    // Ponemos el socket en escucha.
    listen(*sockfd, SOMAXCONN);
    assert(*sockfd != -1);

    int sockconnect;
    while(1){
        //Queda a la escucha de conexiones.
        sockconnect = accept(*sockfd, NULL, NULL); 
        assert(sockconnect != -1);

        new_client(sockconnect, epollfd, ev);
    }

    return; 
}

/*Crea una instancia de epoll*/
void epoll_initiate(int* epollfd){
    
    *epollfd = epoll_create(1);
    assert(*epollfd != -1);
    
    return;
}

/*"Atiende" a un cliente metiendolo en la epoll_interest_list*/
int new_client (int sockconnect, int epollfd, struct epoll_event* ev){

    ev->data.fd = sockconnect; 

    int ret;
    ret = epoll_ctl(epollfd, EPOLL_CTL_ADD, sockconnect, ev);
    assert(ret != -1);
    return; 

}

void handle_conections(int epollfd){
    
    int ret;
    //int epoll_wait(int epfd, struct epoll_event *events,int maxevents, int timeout);

    return;
}

int main (int argc, char* argv[]){

    //fd de socket
    int sockfd; 
    int sockconnect;
    int epollfd;

    struct epoll_event ev;
    ev.events = EPOLLIN;

    sock_creation(&sockfd);

    epoll_initiate(&epollfd);
    ///////////////////////////////////////////////////

    sock_connection(&sockfd, epollfd, &ev);

    return 0;
}
