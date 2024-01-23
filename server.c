#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <unistd.h>


#define PORT_NUM_TEXT   8888
#define PORT_NUM_BIN    8889
#define MAX_EVENTS 1

/*Crea un socket TCP en dominio IPv4*/
/*Retorna un fd que representa nuestro socket y al cual se conectarán los clientes.*/
void sock_creation(int* sockfd){
    
    *sockfd = socket(AF_INET, SOCK_STREAM, 0);
    assert(*sockfd != -1);

    /*Instancia la estructura*/
    struct sockaddr_in sa;
    sa.sin_family = AF_INET;
    sa.sin_port = htons(PORT_NUM_TEXT);
    sa.sin_addr.s_addr = htonl(INADDR_ANY);

    /*Bindea el socket al puerto a utilizar y les asigna las características requeridas*/
    bind(*sockfd, (struct sockaddr*) &sa, sizeof(sa));

    // Ponemos el socket en escucha.
    listen(*sockfd, SOMAXCONN);
    assert(*sockfd != -1);
    return;
}

/*Crea una instancia de epoll*/
void epoll_initiate(int* epollfd){

    *epollfd = epoll_create(1);
    assert(*epollfd != -1);
    
    return;
}

/*Añade los fd a la instancia de epoll para monitorearlos*/
void epoll_add(int sockfd, int epollfd){
    
    struct epoll_event ev;
    ev.data.fd = sockfd;
    ev.events = EPOLLIN;

    epoll_ctl(epollfd, EPOLL_CTL_ADD, sockfd, &ev);
    return;
}

//void epoll_monitor(int epollfd, struct epoll_event* evlist)
void* epoll_monitor(void* args){

    while(1){
        /*Retorna los fd listos para intercambio de datos*/
        epoll_wait(epollfd, evlist, MAX_EVENTS, -1);

        /*Hay que diferenciar entre nuevas conexiones y pedidos de clientes ya aceptados*/

    }
    return; 

}
int main (int argc, char* argv[]){

    //fd de socket
    int sockfd; 
    int sockconnect;
    int epollfd;

    /*Crea el socket y devuelve su fd ya bindeado y en escucha*/
    sock_creation(&sockfd);

    /*Inicia la instancia de epoll y devuelve el fd que hace referencia a la misma*/
    epoll_initiate(&epollfd);

    /*Inicializa la lista de los fd listos*/
    struct epoll_event* evlist = malloc(sizeof(struct epoll_event) * MAX_EVENTS);
    int cores = sysconf(_SC_NPROCESSORS_CONF);

    pthread_t t[cores];

    /*Ver como pasar argumentos a epoll_monitor*/
    void* args = malloc(sizeof(struct epoll_event)*2);
    

    for (int i = 0; i < cores; i++)
        pthread_create(t[i], NULL, epoll_monitor, NULL);

    
    return 0;
}
