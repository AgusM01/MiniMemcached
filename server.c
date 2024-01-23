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

/*Esto en el .h*/
struct args_epoll_monitor {
        int epollfd;
        int sockfd;
        struct epoll_event* evlist;
    };


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
    ev.events = EPOLLIN || EPOLLONESHOT; /*Acordarse de volver a activar el fd luego de atenderlo*/

    epoll_ctl(epollfd, EPOLL_CTL_ADD, sockfd, &ev);
    return;
}

/*void* epoll_monitor(struct args_epoll_monitor* args)*/
void* epoll_monitor(void* args){
    
    struct args_epoll_monitor* e_m_struct;
    e_m_struct = (struct args_epoll_monitor*) args;
    int fdready;



    while(1){
        /*Retorna los fd listos para intercambio de datos*/
        fdready = epoll_wait(e_m_struct->epollfd, e_m_struct->evlist, MAX_EVENTS, -1);

        ///*Hay que diferenciar entre nuevas conexiones y pedidos de clientes ya aceptados*/
        //if (e_m_struct->evlist->data.fd == e_m_struct->sockfd){
        //    new_client(e_m_struct->evlist->data.fd);
        //}
        //else{
        //    ;
        //}
    }
    return NULL; 
}

new_client(int sockfd){

}
/*Esta función pasa a llamarse serv_init() y es la encargada de prender/manejar el server*/
int main (int argc, char* argv[]){

    int sockfd; /*Este seria el socket de texto actualmente*/
    int epollfd;

    /*Crea el socket y devuelve su fd ya bindeado y en escucha*/
    sock_creation(&sockfd);

    /*Inicia la instancia de epoll y devuelve el fd que hace referencia a la misma*/
    epoll_initiate(&epollfd);

    /*Añade el fd del socket creado a la instancia de epoll para monitorearlo.*/
    epoll_add(sockfd, epollfd);

    /*Inicializa la lista de los fd listos*/
    struct epoll_event* evlist = malloc(sizeof(struct epoll_event) * MAX_EVENTS);
    int cores = sysconf(_SC_NPROCESSORS_CONF);

    pthread_t t[cores];

    struct args_epoll_monitor args;
    args.epollfd = epollfd;
    args.evlist = evlist;
    args.sockfd = sockfd;

    for (int i = 0; i < cores; i++)
        pthread_create(&t[i], NULL, epoll_monitor, &args);

    
    return 0;
}
