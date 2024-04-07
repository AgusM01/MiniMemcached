#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <assert.h>
#include <errno.h>
#include "text.h"
#include "../Structures/utils.h"
#include "binary.h"
#include "epoll.h"
#include "sock.h"
#include "text.h"


#define MAX_EVENTS 1
#define MAX_CHAR 2048
#define N_COMMANDS 10

void epoll_initiate(int* epollfd){

    *epollfd = epoll_create(1);
    if (!*epollfd)
        perror("epoll_create : epoll.c 26");
    
    return;
}

void epoll_add(int sockfd, int epollfd, int mode, memc_t mem){

    /*Ajustamos la configuracion de cada fd a agregar a la instancia de epoll*/
    /*Como banderas:*/
    /*EPOLLIN: Avisa cuando hay datos para lectura disponibles*/
    /*EPOLLONESHOT: Una vez que nos avisa de un acontencimiento, 
        no nos avisa más hasta que volvamos a activarlo.
        Acordarse de volver a activarlo con epoll_ctl(epollfd, EPOLL_CTL_MOD. sockfd, &ev).*/
    /*EPOLLRDHUP: Se activa cuando un cliente corta la conexión*/

    struct epoll_event ev;
    
    if (!mode){
        struct data_ptr_text* d_ptr = memc_alloc(mem,sizeof(struct data_ptr_text), MALLOC, NULL);
        assert(d_ptr != NULL);
        d_ptr->fd = sockfd;
        d_ptr->text_or_binary = mode;
        d_ptr->actual_pos_arr = 0;
        d_ptr->command = memc_alloc(mem, MAX_CHAR + 1, MALLOC, NULL);
        d_ptr->missing = 0;
        d_ptr->to_complete = memc_alloc(mem, MAX_CHAR + 1, MALLOC, NULL);
        d_ptr->pos_to_complete = 0;
        d_ptr->is_command = 0;
        d_ptr->prev_pos_arr = 0;
        ev.data.ptr = d_ptr;
    }
    else{
        struct data_ptr_binary* binary = memc_alloc(mem, sizeof(struct data_ptr_binary), MALLOC, NULL);
        binary->fd = sockfd;
        binary->text_or_binary = mode;
        binary->binary_to_read_commands = 5;
        binary->length_key = 0;
        binary->comandos_leidos = 0;
        binary->length_dato = 0;
        binary->to_consumed = 0;
        binary->data_or_key = 0;
        binary->dato = NULL;
        binary->key = NULL;
        binary->commands = memc_alloc(mem, 5, MALLOC, NULL);
        ev.data.ptr = binary;
    }
    

    ev.events = EPOLLIN | EPOLLONESHOT | EPOLLRDHUP;     
    if (fcntl(sockfd, F_SETFL, (fcntl(sockfd, F_GETFL, 0) | O_NONBLOCK) == -1))
        perror("fcntl_ret epoll.c");

    epoll_ctl(epollfd, EPOLL_CTL_ADD, sockfd, &ev);
    return;
}

void quit_epoll(struct args_epoll_monitor* e_m_struct, struct epoll_event* evlist){

    

    int* pointer;
    pointer = (int*)evlist->data.ptr;
    int mode = *(pointer + 1);

    if (!mode){
        struct data_ptr_text* ptr;
        ptr = CAST_DATA_PTR_TEXT;
        epoll_ctl(e_m_struct->epollfd, EPOLL_CTL_DEL, ptr->fd, evlist);
        close(ptr->fd);
        free(ptr->command);
        free(ptr->to_complete);
        free(ptr);
    }
    else{
        struct data_ptr_binary* ptr;
        ptr = CAST_DATA_PTR_BINARY;
        close(ptr->fd);
        free(ptr->commands);
        free(ptr->dato);
        free(ptr->key);
        free(ptr);
    }
    return;
}


void* epoll_monitor(void* args){
    

    int a = 0;
    struct args_epoll_monitor* e_m_struct;
    e_m_struct = (struct args_epoll_monitor*) args;
    
    /*Inicializa la lista de los fd listos*/
    
    struct epoll_event* evlist = memc_alloc(e_m_struct->mem,sizeof(struct epoll_event) * MAX_EVENTS, MALLOC, NULL); //Liberar esto al final
    
    while(1){

        /*Retorna los fd listos para intercambio de datos*/
        /*La idea es que los hilos checkeen si hay fd disponibles, si hay un hilo, irá a 
            atender a ese cliente y las siguientes llamadas a epoll_wait no avisaran de este fd.
            La bandera EPOLLONESHOT ayuda a esto haciendo que no aparezca en la lista de ready 
            devuelta por epoll_wait en el caso de tener nuevo input que ya está siendo 
            atendida por un hilo. Ese hilo debe volver a activar las notificaciones de
            ese cliente.*/
        do{
            a = epoll_wait(e_m_struct->epollfd, evlist, MAX_EVENTS, -1);
        }while(a < 0 && errno == EINTR);
        
        int* pointer;
        pointer = (int*)evlist->data.ptr;
        
        int who_fd = *pointer;
        int mode_fd = *(pointer + 1); 

        /*Tenemos un fd disponible para lectura.*/
      
        /*Verificar la presencia de EPOLLHUP o EPOLLER en cuyo caso hay que cerrar el fd*/
        if ((evlist->events & EPOLLRDHUP) 
            || (evlist->events & EPOLLERR)
            || (evlist->events & EPOLLHUP)){
            quit_epoll(e_m_struct, evlist);
        }
        else{
            /*Si es el fd de algún socket del server debemos atender a un nuevo cliente.*/
            if (who_fd == e_m_struct->sockfd_text){
                struct data_ptr_text* ptr;
                ptr = CAST_DATA_PTR_TEXT;

                /*Aceptamos al nuevo cliente*/                
                new_client(e_m_struct, evlist, ptr->text_or_binary);
            }
            else if (who_fd == e_m_struct->sockfd_binary){
                struct data_ptr_binary* ptr;
                ptr = CAST_DATA_PTR_BINARY;

                /*Aceptamos al nuevo cliente*/
                new_client(e_m_struct, evlist, ptr->text_or_binary);
            }
            else{
                int resp = 0;
                if (!mode_fd){
                    /*Es un cliente en modo texto*/
                    /*Este cliente no es nuevo por lo que nos hará consultas.*/
                    resp = text_consume(e_m_struct, evlist);

                }
                else{
                    /*Es un cliente en modo binario*/
                    resp = binary_consume(e_m_struct, evlist);
                }
                
                /*Si las funciones de consumir texto/binario no dieron error, lo vuelvo a monitorear*/
                if (resp != -1){
                    struct epoll_event ev;
                    ev.events = EPOLLIN | EPOLLONESHOT | EPOLLRDHUP; 
                    ev.data.ptr = evlist->data.ptr;
                    epoll_ctl(e_m_struct->epollfd, EPOLL_CTL_MOD, CAST_DATA_PTR_TEXT->fd, &ev);
                }
            }   
        }
    }    
    return NULL; 
}
