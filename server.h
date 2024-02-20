#ifndef __SERVER_H__
#define __SERVER_H__

/*Estructura con la informacion de cada fd guardada en la parte de data de cada estructura epoll_event*/
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

struct args_epoll_monitor; 
/*Crea un socket TCP en dominio IPv4*/
/*Retorna un fd que representa nuestro socket y al cual se conectarán los clientes.*/
void sock_text_creation (int* sockfd);

/*Crea una instancia de epoll*/
void epoll_initiate (int* epollfd);

/*Añade los fd a la instancia de epoll para monitorearlos*/
void epoll_add  (int sockfd, 
                 int epollfd, 
                 int mode);

/*Conecta a un nuevo cliente.*/
void new_client (struct args_epoll_monitor* e_m_struct, 
                 struct epoll_event* evlist,
                 int mode);

/*Consume el texto del fd de un cliente.*/
void text_consume   (struct args_epoll_monitor* e_m_struct, 
                     struct epoll_event* evlist);

/*Le da un fd listo a cada thread*/
void* epoll_monitor (void* args);

void length_binary(unsigned char* commands, int* length);

void int_to_binary(int num, void* len);

void manage_client_binary(struct args_epoll_monitor* e_m_struct, 
                     struct epoll_event* evlist);
                     
void manage_client(struct args_epoll_monitor* e_m_struct, struct epoll_event* evlist, char** token_comands, int cant_comm);

void quit_epoll(struct args_epoll_monitor* e_m_struct, struct epoll_event* evlist);

#endif /*__SERVER_H__*/