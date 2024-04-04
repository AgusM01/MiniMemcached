#ifndef __SOCK_H__
#define __SOCK_H__

/*Creo sockets*/
void sock_creation(int* sockfd, int port_num);

/*Agrego un nuevo cliente*/
void new_client(struct args_epoll_monitor* e_m_struct, struct epoll_event* evlist, int mode);

#endif /*__SOCK_H__*/
