#ifndef __BINARY_H__
#define __BINARY_H__

int binary_consume(struct args_epoll_monitor* e_m_struct, struct epoll_event* evlist);

int read_length(struct args_epoll_monitor* e_m_struct, struct epoll_event* evlist);

int read_content(struct args_epoll_monitor* e_m_struct, struct epoll_event* evlist, int data_or_key);

void restart_binary(struct epoll_event* evlist);

#endif /*__BINARY_H__*/