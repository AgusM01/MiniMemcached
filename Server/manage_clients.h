#ifndef __MANAGE_CLIENTS_H__
#define __MANAGE_CLIENTS_H__

#include "epoll.h"

#define MAX_CHAR 2048

/*Funcion que se encarga de responder pedidos del modo texto*/
int manage_txt_client   (struct args_epoll_monitor* e_m_struct, 
                        struct epoll_event* evlist, 
                        char** token_comands, 
                        int cant_comm);

/*Funcion que se encarga de responder pedidos del modo binario*/
int manage_bin_client   (struct args_epoll_monitor* e_m_struct, 
                        struct epoll_event* evlist);

#endif // !__MANAGE_CLIENTS_H__
