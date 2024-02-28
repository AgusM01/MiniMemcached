#ifndef __COMUNICATE_H__
#define __COMUNICATE_H__

#include "manage_clients.h"
#include "epoll.h"
#include <pthread.h>

int writen  (int fd, 
            const void *buf, 
            int len);

int recv_mem    (struct args_epoll_monitor* e_m_struct, 
                struct epoll_event* evlist, 
                void* buf, 
                size_t len, 
                int flags);

int my_length(char* buf);

#endif // __COMUNICATE_H__