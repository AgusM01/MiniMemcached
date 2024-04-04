#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include "epoll.h"
#include "sock.h"
#include <assert.h>

#define CAST_DATA_PTR ((struct data_ptr*)evlist->data.ptr)

/*Crea un socket TCP en dominio IPv4*/
/*Retorna un fd que representa nuestro socket y al cual se conectarán los clientes.*/
void sock_creation(int* sockfd, int port_num){
    
    *sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == NULL)
        perror("sock_creation");

    /*Instancia la estructura*/
    struct sockaddr_in sa;
    sa.sin_family = AF_INET;
    sa.sin_port = htons(port_num);
    sa.sin_addr.s_addr = htonl(INADDR_ANY);

    /*Bindea el socket al puerto a utilizar y les asigna las características requeridas*/
    assert(!bind(*sockfd, (struct sockaddr*) &sa, sizeof(sa)));

    // Ponemos el socket en escucha.
    if (listen(*sockfd, SOMAXCONN) == -1)
        perror("sock_listen");
    return;
}

/*Conecta a un nuevo cliente.*/
void new_client(struct args_epoll_monitor* e_m_struct, struct epoll_event* evlist, int mode){

    struct data_ptr* ptr;
    ptr = CAST_DATA_PTR;
    
    int client_sockfd;
    struct epoll_event ev;
    ev.events = EPOLLIN | EPOLLONESHOT | EPOLLRDHUP; 
    ev.data.ptr = evlist->data.ptr;

    if (mode == 0){
        /*Aceptamos al nuevo cliente*/
        client_sockfd = accept(e_m_struct->sockfd_text, NULL, 0);
    }
    else{
        client_sockfd = accept(e_m_struct->sockfd_binary, NULL, 0);
    }
    
    
    if (client_sockfd == -1){
        if ((errno == EAGAIN) || (errno == EWOULDBLOCK)){            
            epoll_ctl(e_m_struct->epollfd, EPOLL_CTL_MOD, ptr->fd, &ev);
            return;
        }
        else{
            perror("Error al aceptar nuevo cliente\n");
            exit(1);
        } 
    }

    /*Vuelve a meter al fd que acepta clientes*/
    epoll_ctl(e_m_struct->epollfd, EPOLL_CTL_MOD, ptr->fd, &ev);
    
    /*Mete al fd nuevo*/
    epoll_add(client_sockfd, e_m_struct->epollfd, ptr->text_or_binary, e_m_struct->mem);

    return;
}

