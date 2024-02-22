#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/signal.h>
#include "server.h"
#include "structures/utils.h"
#include "epoll.h"
#include "sock.h"

#define PORT_NUM_TEXT   8888
#define PORT_NUM_BIN    8889


/*Esta función pasa a llamarse serv_init() y es la encargada de prender/manejar el server*/
int main (int argc, char* argv[]){

    size_t byte_limit = 100000000000;
    limit_mem(byte_limit);

    int sockfd_text; /*Este seria el socket de texto*/
    int sockfd_binary;; /*Este seria el socket binario -> 9999 testing*/
    int epollfd; /*fd referente a la instancia de epoll*/

    memc_t mem = memc_init((HasFunc)hash_len, 2, 500, 0);

    /*Crea los sockets y devuelve su fd ya bindeado y en escucha*/
    sock_creation(&sockfd_text, PORT_NUM_TEXT);

    sock_creation(&sockfd_binary, PORT_NUM_BIN);

    /*Inicia la instancia de epoll y devuelve el fd que hace referencia a la misma*/
    epoll_initiate(&epollfd);

    /*Añade el fd del socket de texto creado a la instancia de epoll para monitorearlo.*/
    epoll_add(sockfd_text, epollfd, 0, mem);

    /*Añade el fd del socket binario creado a la instancia de epoll para monitorearlo.*/
    epoll_add(sockfd_binary, epollfd, 1, mem);

    
    int cores = sysconf(_SC_NPROCESSORS_CONF);

    pthread_t t[cores];

    struct args_epoll_monitor args;
    args.epollfd = epollfd;
    args.sockfd_text = sockfd_text;
    args.sockfd_binary = sockfd_binary;
    args.mem = mem;

    signal(SIGPIPE, SIG_IGN);
    
    for (int i = 0; i < cores; i++){
        pthread_create(&t[i], NULL, epoll_monitor, &args);
    }
    for(int i = 0; i < cores; i++)
        pthread_join(t[i], NULL);

    /*Capturar señal SIGPIPE*/

    
    return 0;
}