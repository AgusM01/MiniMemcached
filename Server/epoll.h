#ifndef __EPOLL_H__
#define __EPOLL_H__

#include "../structures/memc.h"
#include <sys/epoll.h>


void epoll_initiate(int* epollfd);

void epoll_add(int sockfd, int epollfd, int mode, memc_t mem);

void quit_epoll(struct args_epoll_monitor* e_m_struct, struct epoll_event* evlist);

void* epoll_monitor(void* args);



#endif /*__EPOLL_H__*/