#ifndef __TEXT_H__
#define __TEXT_H__

#include "../Structures/memc.h"
#include "epoll.h"


/*  
    * Toma un string, un delimitador y un puntero a int
    * Devuelve un char** que contiene la cadena tokenizada separada por el delimitador
    * y llena el puntero a int con la cantidad de tokens.*/
char** text_parser(char* text, int* cant_comm, char* delimiter, memc_t mem);

int text_consume(struct args_epoll_monitor* e_m_struct, struct epoll_event* evlist);

int text2(struct args_epoll_monitor* e_m_struct, struct epoll_event* evlist);

#endif /*__TEXT_H__*/