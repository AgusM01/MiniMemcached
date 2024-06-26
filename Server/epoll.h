#ifndef __EPOLL_H__
#define __EPOLL_H__

#include "../Structures/memc.h"
#include <sys/epoll.h>

#define CAST_DATA_PTR_TEXT ((struct data_ptr_text*)evlist->data.ptr)
#define CAST_DATA_PTR_BINARY ((struct data_ptr_binary*)evlist->data.ptr)

/*Estructura utilizada para controlar los fd conectados al protocolo binario*/
struct data_ptr_binary{
    int fd;
    int text_or_binary; /*0 para text, 1 para binary*/
    int binary_to_read_commands; /*Comandos que quedan para leer*/
    int comandos_leidos;
    unsigned char* commands; 
    char* key;
    char* dato;
    int length_key;
    int length_dato;
    int to_consumed; /*Bytes de clave/dato que quedan por consumir*/
    int data_or_key;
};

/*Estructura utilizada para controlar los fd conectados al protocolo de texto */
struct data_ptr_text {
    int fd;
    int text_or_binary; /*0 para text, 1 para binary*/
    int actual_pos_arr; /*Posicion actual del array delimit_pos*/
    int missing;
    char* command;
    char* to_complete;
    int pos_to_complete;
    int is_command;
    int prev_pos_arr;
};


/*Estructura que se utiliza para guardar los argumentos de la funcion epoll_monitor*/
struct args_epoll_monitor {
    int epollfd;
    int sockfd_text;
    int sockfd_binary;
    memc_t mem;
};

/*Funcion que inicia la instancia de epoll*/
void epoll_initiate(int* epollfd);

/*Funcion que añade clientes nuevos al epoll*/
void epoll_add(int sockfd, int epollfd, int mode, memc_t mem);

/*Funcion que elimina un cliente del epoll*/
void quit_epoll(struct args_epoll_monitor* e_m_struct, struct epoll_event* evlist);

/*Funcion que monitorea los clientes*/
void* epoll_monitor(void* args);

#endif /*__EPOLL_H__*/
