#ifndef __SERVER_H__
#define __SERVER_H__

/*Esto en el .h*/
struct args_epoll_monitor {
    int epollfd;
    int sockfd_text;
    int sockfd_binary;
};

struct data_ptr {
    int fd;
    int text_or_binary; /*0 para text, 1 para binary*/
    int* delimit_pos; /*Array de posiciones de \n*/
    int cant_comm_ptr; /*Cantidad de \n*/
    int actual_pos_arr; /*Posicion actual del array delimit_pos*/ 
};

/*Crea un socket TCP en dominio IPv4*/
/*Retorna un fd que representa nuestro socket y al cual se conectarán los clientes.*/
void sock_text_creation(int* sockfd);

/*Crea una instancia de epoll*/
void epoll_initiate(int* epollfd);

/*Añade los fd a la instancia de epoll para monitorearlos*/
void epoll_add(int sockfd, int epollfd, int mode);

/*Conecta a un nuevo cliente.*/
void new_client(struct args_epoll_monitor* e_m_struct, struct epoll_event* evlist);

/*Consume el texto del fd de un cliente.*/
void text_consume(struct args_epoll_monitor* e_m_struct, struct epoll_event* evlist);

/*Le da un fd listo a cada thread*/
void* epoll_monitor(void* args);



#endif /*__SERVER_H__*/