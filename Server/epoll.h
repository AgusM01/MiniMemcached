#ifndef __EPOLL_H__
#define __EPOLL_H__

#include "../Structures/memc.h"
#include <sys/epoll.h>

/*Estructura utilizada para controlar los fd conectados al protocolo binario*/
struct data_ptr_binary{
    int binary_to_read_commands; /*Comandos que quedan para leer*/
    int comandos_leidos;
    unsigned char* commands; /*Aca esta raro*/
    char* key;
    char* dato;
    int length_key;
    int length_dato;
    int to_consumed; /*Bytes de clave/dato que quedan por consumir*/
    int data_or_key;
};

/*Estructura "padre" utilizada para controlar los fd del modo texto.
 * También guardan la estructura binaria solo que no la utilizan. 
 * Esto es ya que de lo contrario no puedo saber de que protocolo provienen
 * los fd que devuelve el epoll_wait*/
struct data_ptr {
    int fd;
    int text_or_binary; /*0 para text, 1 para binary*/
    int* delimit_pos; /*Array de posiciones de \n*/
    int cant_comm_ptr; /*Cantidad de \n*/
    int actual_pos_arr; /*Posicion actual del array delimit_pos*/
    char* command;
    int readed;
    int missing;
    char* to_complete;
    int pos_to_complete;
    int is_command;
    int prev_pos_arr;
    struct data_ptr_binary* binary;
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
