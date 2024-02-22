#ifndef __EPOLL_H__
#define __EPOLL_H__

#include "../structures/memc.h"
#include <sys/epoll.h>

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

struct data_ptr {
    int fd;
    int text_or_binary; /*0 para text, 1 para binary*/
    int* delimit_pos; /*Array de posiciones de \n*/
    int cant_comm_ptr; /*Cantidad de \n*/
    int actual_pos_arr; /*Posicion actual del array delimit_pos*/
    char* command;
    int readed;
    int missing;
    struct data_ptr_binary* binary;
};



struct args_epoll_monitor {
    int epollfd;
    int sockfd_text;
    int sockfd_binary;
    memc_t mem;
};

void epoll_initiate(int* epollfd);

void epoll_add(int sockfd, int epollfd, int mode, memc_t mem);

void quit_epoll(struct args_epoll_monitor* e_m_struct, struct epoll_event* evlist);

void* epoll_monitor(void* args);



#endif /*__EPOLL_H__*/