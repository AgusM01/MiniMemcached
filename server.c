#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

#define PORT_NUM_TEXT   8888
#define PORT_NUM_BIN    8889
#define MAX_EVENTS 1
#define MAX_CHAR 2048
#define CAST_DATA_PTR ((struct data_ptr*)e_m_struct->evlist->data.ptr)

/*Esto en el .h*/
struct args_epoll_monitor {
    int epollfd;
    int sockfd_text;
    int sockfd_binary;
    struct epoll_event* evlist;
};

struct data_ptr {
    int fd;
    int text_or_binary; /*0 para text, 1 para binary*/
};


/*Crea un socket TCP en dominio IPv4*/
/*Retorna un fd que representa nuestro socket y al cual se conectarán los clientes.*/
void sock_text_creation(int* sockfd){
    //Ver lectura no bloqueante 
    *sockfd = socket(AF_INET, SOCK_STREAM, 0);
    assert(*sockfd != -1);

    /*Instancia la estructura*/
    struct sockaddr_in sa;
    sa.sin_family = AF_INET;
    sa.sin_port = htons(PORT_NUM_TEXT);
    sa.sin_addr.s_addr = htonl(INADDR_ANY);

    /*Bindea el socket al puerto a utilizar y les asigna las características requeridas*/
    bind(*sockfd, (struct sockaddr*) &sa, sizeof(sa));

    // Ponemos el socket en escucha.
    listen(*sockfd, SOMAXCONN);
    assert(*sockfd != -1);
    return;
}

/*Crea una instancia de epoll*/
void epoll_initiate(int* epollfd){

    *epollfd = epoll_create(1);
    assert(*epollfd != -1);
    
    return;
}

/*Añade los fd a la instancia de epoll para monitorearlos*/
void epoll_add(int sockfd, int epollfd, int mode){
    /*Ajustamos la configuracion de cada fd a agregar a la instancia de epoll*/
    /*Como banderas:*/
    /*EPOLLIN: Avisa cuando hay datos para lectura disponibles*/
    /*EPOLLONESHOT: Una vez que nos avisa de un acontencimiento, 
        no nos avisa más hasta que volvamos a activarlo.
        Acordarse de volver a activarlo con epoll_ctl(epollfd, EPOLL_CTL_MOD. sockfd, &ev).*/
    /*EPOLLRDHUP: Se activa cuando un cliente corta la conexión*/
    struct epoll_event ev;
    struct data_ptr* d_ptr = malloc(sizeof(struct data_ptr)); //Liberar esto al final.
    d_ptr->fd = sockfd;
    d_ptr->text_or_binary = mode; /*Mode es 0 si es texto y 1 si es binario*/ 
    ev.events = EPOLLIN | EPOLLONESHOT | EPOLLRDHUP; //NO USAR EDGE TRIGGERED YA QUE SI UN HILO MANDA MUCHISIMO NO VA A AVISAR PENSAR CHARLADO CON NERI ESCUCHAR AUDIO ZOE
    ev.data.ptr = d_ptr;

    epoll_ctl(epollfd, EPOLL_CTL_ADD, sockfd, &ev);
    return;
}

void new_client(struct args_epoll_monitor* e_m_struct, int* client_sockfd){

    /*Aceptamos al nuevo cliente*/
    *client_sockfd = accept(e_m_struct->sockfd_text, NULL, 0);
    assert(*client_sockfd != -1);

    /*El fd no se bloquea en lectura. Esta funcion modifica las banderas del estado del archivo agregando la bandera O_NONBLOCK.*/
    int fcntl_ret;
    fcntl_ret = fcntl(*client_sockfd, F_SETFL, (fcntl(*client_sockfd, F_GETFL, 0) | O_NONBLOCK));
    assert(fcntl_ret != -1);

    /*Reactivamos el fd para monitoreo (EPOLLONESHOT activado)*/
    struct epoll_event ev;
    ev.events = EPOLLIN | EPOLLONESHOT | EPOLLRDHUP; 
    ev.data.ptr = e_m_struct->evlist->data.ptr;

    epoll_ctl(e_m_struct->epollfd, EPOLL_CTL_MOD, e_m_struct->sockfd_text, &ev);
    
    printf("Acepto a: %d\n", *client_sockfd);

    return;
}

void manage_client(struct args_epoll_monitor* e_m_struct){
    /*Usar la funcion recv para leer del buffer de lectura. Usar ntohl para pasar de network-byte-order (big endian) a host-byte-order*/
    /*Funcion readline() lee hasta un '\n' <- posible*/
    char* buf = malloc(MAX_CHAR);
    //char* resp = malloc(MAX_CHAR);
    char* err = "EINVAL";
    ssize_t fst_read, snd_read = -1;

    /*Primer recv para ubicar la posicion del \n*/
    fst_read = recv(CAST_DATA_PTR->fd, buf, MAX_CHAR, MSG_PEEK); 
    assert(fst_read != -1);
    /* 3 casos:
    * r = -1 -> Error de lectura: Cubierto con assert.
    * r = 0 -> EOF/Shutdown Peer socket. 
    * r = n -> numero de bytes leidos. */
    if(fst_read != 0){
        
        for (int i = 0; i < fst_read; i++){
            if(buf[i] == '\n')
                snd_read = i; /*Hasta donde leer la proxima.*/
        }
        if (snd_read == -1)
            send(CAST_DATA_PTR->fd, err, strlen(err), MSG_NOSIGNAL); /*La bandera ignora la señal SIGPIPE, de lo contrario hay que hacer un handler.*/
        //else

        //if (snd_read != -1)
            
    }
    else{
        if (fst_read == 0){ /*Acá significaria que se desconectó..*/} 
    }



}

/*void* epoll_monitor(struct args_epoll_monitor* args)*/
void* epoll_monitor(void* args){
    
    struct args_epoll_monitor* e_m_struct;
    e_m_struct = (struct args_epoll_monitor*) args;

    while(1){

        /*Retorna los fd listos para intercambio de datos*/
        /*La idea es que los hilos checkeen si hay disponibles fd, si hay un hilo irá a 
            atender a ese cliente y las siguientes llamadas a epoll_wait no avisaran de este fd.
            La bandera EPOLLONESHOT ayuda a esto haciendo que no aparezca en la lista de ready 
            devuelta por epoll_wait en el caso de tener nuevo input que ya está siendo 
            atendida por un hilo. Ese hilo debe volver a activar las notificaciones de
            ese cliente. El hilo responde una consulta sola y lo vuelve a meter al epoll asi puede ir a atender a mas hilos*/
        epoll_wait(e_m_struct->epollfd, e_m_struct->evlist, MAX_EVENTS, -1);
        /*Tenemos un fd disponible para lectura.*/
        
        /*Verificar la presencia de EPOLLHUP o EPOLLER en cuyo caso hay que cerrar el fd*/
        if ((e_m_struct->evlist->events & EPOLLRDHUP) 
            || (e_m_struct->evlist->events & EPOLLERR)
            || (e_m_struct->evlist->events & EPOLLHUP)){
            close(CAST_DATA_PTR->fd);
            free(CAST_DATA_PTR);
        }
        else{
            printf("No es un cliente desconectado\n");

            /*Si es el fd del socket del server debemos atender a un nuevo cliente.*/
            if (CAST_DATA_PTR->fd == e_m_struct->sockfd_text ||
                CAST_DATA_PTR->fd == e_m_struct->sockfd_binary){
                
                printf("Hay que aceptar a alguien\n");
                int client_sockfd;
                /*Aceptamos al nuevo cliente*/
                new_client(e_m_struct, &client_sockfd);
                /*Lo añadimos a la instancia de epoll para monitorearlo.*/
                epoll_add(client_sockfd, e_m_struct->epollfd, CAST_DATA_PTR->text_or_binary);
            }
            else{
                printf("Es un cliente de pedidos\n");
                /*Este cliente no es nuevo por lo que nos hará consultas.*/
                /*Hacer un if para diferenciar entre cliente de texto y binario mediante el data.u32*/  
                manage_client(e_m_struct);  
            }   
        }
    }
    return NULL; 
}

/*Esta función pasa a llamarse serv_init() y es la encargada de prender/manejar el server*/
int main (int argc, char* argv[]){

    int sockfd_text; /*Este seria el socket de texto*/
    int sockfd_binary = 99999; /*Este seria el socket binario -> 9999 testing*/
    int epollfd; /*fd referente a la instancia de epoll*/

    /*Crea el socket y devuelve su fd ya bindeado y en escucha*/
    sock_text_creation(&sockfd_text);

    /*Inicia la instancia de epoll y devuelve el fd que hace referencia a la misma*/
    epoll_initiate(&epollfd);

    /*Añade el fd del socket de texto creado a la instancia de epoll para monitorearlo.*/
    epoll_add(sockfd_text, epollfd, 0);

    /*Añade el fd del socket binario creado a la instancia de epoll para monitorearlo.*/
    //epoll_add(sockfd_binary, epollfd, 1);

    /*Inicializa la lista de los fd listos*/
    struct epoll_event* evlist = malloc(sizeof(struct epoll_event) * MAX_EVENTS);
    int cores = sysconf(_SC_NPROCESSORS_CONF);

    pthread_t t[cores];

    struct args_epoll_monitor args;
    args.epollfd = epollfd;
    args.evlist = evlist;
    args.sockfd_text = sockfd_text;
    args.sockfd_binary = sockfd_binary;
    for (int i = 0; i < cores; i++){
        pthread_create(&t[i], NULL, epoll_monitor, &args);
    }
    for(int i = 0; i < cores; i++)
        pthread_join(t[i], NULL);

    /*Capturar señal SIGPIPE*/

    
    return 0;
}
