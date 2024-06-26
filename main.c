#include <linux/capability.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/signal.h>
#include "Structures/utils.h"
#include "Server/epoll.h"
#include "Server/sock.h"
#include "Structures/memc.h"
#include <sys/epoll.h>

#define PORT_NUM_TEXT   888
#define PORT_NUM_BIN    889

#define MEM_LIMIT 50000000   // Un aproximado de la memoria que ocupa un thread.
/*Funcion encargada de ejecutar el server*/
int main(int argc, char* argv[]){

    if (argc != 1) {
        fprintf(stderr, "Bad args\n");
        exit(1);
    }

    int sockfd_text; /*Este seria el socket de texto*/
    int sockfd_binary;; /*Este seria el socket binario -> 9999 testing*/

    sock_creation(&sockfd_text, PORT_NUM_TEXT);
    sock_creation(&sockfd_binary, PORT_NUM_BIN);

    drop_privileges();

    /*Calcula la cantidad de procesadores de la máquina actual*/
    int cores = sysconf(_SC_NPROCESSORS_CONF);

    limit_mem(MEM_LIMIT);

    int epollfd; /*fd referente a la instancia de epoll*/

    memc_t mem = memc_init((HasFunc)murmur3_32, 1000000, 500);

    /*Inicia la instancia de epoll y devuelve el fd que hace referencia a la misma*/
    epoll_initiate(&epollfd);

    /*Añade el fd del socket de texto creado a la instancia de epoll para monitorearlo.*/
    epoll_add(sockfd_text, epollfd, 0, mem);

    /*Añade el fd del socket binario creado a la instancia de epoll para monitorearlo.*/
    epoll_add(sockfd_binary, epollfd, 1, mem);

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

    return 0;
}