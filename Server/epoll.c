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
#define CAST_DATA_PTR ((struct data_ptr*)evlist->data.ptr)
#define CAST_DATA_PTR_BINARY CAST_DATA_PTR->binary

/*Crea una instancia de epoll*/
void epoll_initiate(int* epollfd){

    *epollfd = epoll_create(1);
    if (!*epollfd)
        perror("epoll_create : epoll.c 26");
    
    return;
}

/*Añade los fd a la instancia de epoll para monitorearlos*/
void epoll_add(int sockfd, int epollfd, int mode, memc_t mem){
    /*Ajustamos la configuracion de cada fd a agregar a la instancia de epoll*/
    /*Como banderas:*/
    /*EPOLLIN: Avisa cuando hay datos para lectura disponibles*/
    /*EPOLLONESHOT: Una vez que nos avisa de un acontencimiento, 
        no nos avisa más hasta que volvamos a activarlo.
        Acordarse de volver a activarlo con epoll_ctl(epollfd, EPOLL_CTL_MOD. sockfd, &ev).*/
    /*EPOLLRDHUP: Se activa cuando un cliente corta la conexión*/
    struct epoll_event ev;
    struct data_ptr* d_ptr = memc_alloc(mem,sizeof(struct data_ptr), MALLOC, NULL); //Liberar esto al final.

    assert(d_ptr != NULL);
    d_ptr->fd = sockfd;
    d_ptr->text_or_binary = mode;
    if (mode == 0){
        d_ptr->cant_comm_ptr = 0;
        d_ptr->actual_pos_arr = 0;
        d_ptr->delimit_pos = memc_alloc(mem, 4*N_COMMANDS, MALLOC, NULL); /*Liberar esto*/
        d_ptr->binary = NULL;
        d_ptr->command = memc_alloc(mem, MAX_CHAR + 1, MALLOC, NULL);
        d_ptr->missing = 0;
        d_ptr->readed = 0;
        d_ptr->to_complete = memc_alloc(mem, MAX_CHAR + 1, MALLOC, NULL);
        d_ptr->pos_to_complete = 0;
        d_ptr->is_command = 0;
        d_ptr->prev_pos_arr = 0;
    }
    if (mode == 1){
        struct data_ptr_binary* binary = memc_alloc(mem, sizeof(struct data_ptr_binary), MALLOC, NULL);
        d_ptr->binary = binary;
        d_ptr->binary->binary_to_read_commands = 5;
        d_ptr->binary->length_key = 0;
        d_ptr->binary->comandos_leidos = 0;
        d_ptr->binary->length_dato = 0;
        d_ptr->binary->to_consumed = 0;
        d_ptr->binary->data_or_key = 0;
        d_ptr->binary->dato = NULL;
        d_ptr->binary->key = NULL;
        d_ptr->binary->commands = memc_alloc(mem, 6, MALLOC, NULL);
        d_ptr->delimit_pos = NULL;        
    }
    ev.data.ptr = d_ptr;
    

    ev.events = EPOLLIN | EPOLLONESHOT | EPOLLRDHUP; //NO USAR EDGE TRIGGERED YA QUE SI UN HILO MANDA MUCHISIMO NO VA A AVISAR PENSAR CHARLADO CON NERI ESCUCHAR AUDIO ZOE
    
    if ( -1 == fcntl(sockfd, F_SETFL, (fcntl(sockfd, F_GETFL, 0) | O_NONBLOCK)))
        perror("fcntl_ret epoll.c");

    epoll_ctl(epollfd, EPOLL_CTL_ADD, sockfd, &ev);
    return;
}

void quit_epoll(struct args_epoll_monitor* e_m_struct, struct epoll_event* evlist){

    struct data_ptr* ptr;
    ptr = CAST_DATA_PTR;

    struct data_ptr_binary* ptr_bin;
    ptr_bin = CAST_DATA_PTR_BINARY;

    //printf("Lo saque a: %d\n", ptr->fd);
    epoll_ctl(e_m_struct->epollfd, EPOLL_CTL_DEL, ptr->fd, evlist); /*En lugar de null puede ser e_m_struct->evlist para portabilidad pero ver bien*/
    
    if(!ptr->text_or_binary){
        close(ptr->fd);
        free(ptr->command);
        free(ptr->delimit_pos);
        free(ptr->to_complete);
    }
    else{
        close(ptr->fd);
        free(ptr_bin->commands);
        free(ptr_bin->dato);
        free(ptr_bin->key);
        free(ptr_bin);
    }
    free(ptr);
    return;
}


/*Le da un fd listo a cada thread*/
void* epoll_monitor(void* args){
    

    int a = 0;
    struct args_epoll_monitor* e_m_struct;
    e_m_struct = (struct args_epoll_monitor*) args;
    
    /*Inicializa la lista de los fd listos*/
    
    struct epoll_event* evlist = memc_alloc(e_m_struct->mem,sizeof(struct epoll_event) * MAX_EVENTS, MALLOC, NULL); //Liberar esto al final
    
    //e_m_struct->evlist = evlist;
    //printf("Entro al while soy %ld\n", pthread_self());
    while(1){

        /*Retorna los fd listos para intercambio de datos*/
        /*La idea es que los hilos checkeen si hay disponibles fd, si hay un hilo irá a 
            atender a ese cliente y las siguientes llamadas a epoll_wait no avisaran de este fd.
            La bandera EPOLLONESHOT ayuda a esto haciendo que no aparezca en la lista de ready 
            devuelta por epoll_wait en el caso de tener nuevo input que ya está siendo 
            atendida por un hilo. Ese hilo debe volver a activar las notificaciones de
            ese cliente. El hilo responde una consulta sola y lo vuelve a meter al epoll asi puede ir a atender a mas hilos*/
        do{
            a = epoll_wait(e_m_struct->epollfd, evlist, MAX_EVENTS, -1);
        }while(a < 0 && errno == EINTR);
        //perror("epoll_wait");
        //assert(a != -1);
        struct data_ptr* ptr;
        ptr = CAST_DATA_PTR;

        //printf("Atiendo a fd: %d, soy hilo: %ld\n", ptr->fd, pthread_self());
        //printf("FLAGS: %d\n", (int)e_m_struct->evlist->events);

        /*Tenemos un fd disponible para lectura.*/
        //printf("Voy a atender a fd: %d\n", CAST_DATA_PTR->fd);
        /*Verificar la presencia de EPOLLHUP o EPOLLER en cuyo caso hay que cerrar el fd*/
        
        if ((evlist->events & EPOLLRDHUP) 
            || (evlist->events & EPOLLERR)
            || (evlist->events & EPOLLHUP)){
            quit_epoll(e_m_struct, evlist);
        }
        else{
            //printf("No es un cliente desconectado\n");

            /*Si es el fd del socket del server debemos atender a un nuevo cliente.*/
            if ((ptr->fd == e_m_struct->sockfd_text) ||
                (ptr->fd == e_m_struct->sockfd_binary)){
                
                //printf("Hay que aceptar a alguien.\n");
                /*Aceptamos al nuevo cliente*/
                new_client(e_m_struct, evlist, ptr->text_or_binary);
                /*Lo añadimos a la instancia de epoll para monitorearlo.*/
            }
            else{
                int resp = 0;
                if (!ptr->text_or_binary){

                    //printf("Es un cliente de pedidos\n");
                    /*Este cliente no es nuevo por lo que nos hará consultas.*/
                    /*Hacer un if para diferenciar entre cliente de texto y binario mediante el data.u32*/  

                    //resp = text_consume(e_m_struct, evlist);
                    resp = text2(e_m_struct, evlist);

                }
                else{
                    resp = binary_consume(e_m_struct, evlist);
                }
                
                if (resp != -1){
                    struct epoll_event ev;
                    ev.events = EPOLLIN | EPOLLONESHOT | EPOLLRDHUP; 
                    ev.data.ptr = evlist->data.ptr;
                    epoll_ctl(e_m_struct->epollfd, EPOLL_CTL_MOD, CAST_DATA_PTR->fd, &ev);
                }
            }   
        }
    }    
    return NULL; 
}