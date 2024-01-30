#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <unistd.h>


#define PORT_NUM_TEXT   8888
#define PORT_NUM_BIN    8889
#define MAX_EVENTS 1
#define CAST_DATA_PTR ((struct data_ptr*)e_m_struct->evlist->data.ptr)

/*Esto en el .h*/
struct args_epoll_monitor {
    int epollfd;
    int sockfd_text;
    int sockfd_binary;
    int fdready; //Retorno de epoll_wait.
    pthread_mutex_t mutex;
    struct epoll_event* evlist;
};

struct data_ptr {
    int fd;
    int text_or_binary; /*0 para text, 1 para binary*/
};


/*Crea un socket TCP en dominio IPv4*/
/*Retorna un fd que representa nuestro socket y al cual se conectarán los clientes.*/
void sock_text_creation(int* sockfd){
    
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
    /*EPOLLET: Activa las notificaciones Edge-Triggered las cuales solo nos dicen si hubo actividad 
        en un fd a monitorear DESDE la llamada anterior a epoll_wait. 
        Si desde que llamamos a epoll_wait() por primera vez no pasó nada más. 
        Cuando llamemos a epoll_wait() por segunda vez no nos devolverá nada y actua en base a la bandera de timeout*/
    /*EPOLLRDHUP: Se activa cuando un cliente corta la conexión*/
    struct epoll_event ev;
    struct data_ptr* d_ptr = malloc(sizeof(struct data_ptr)); //Liberar esto al final.
    d_ptr->fd = sockfd;
    d_ptr->text_or_binary = mode; /*Mode es 0 si es texto y 1 si es binario*/ 
    ev.events = EPOLLIN | EPOLLONESHOT | EPOLLET | EPOLLRDHUP;
    ev.data.ptr = d_ptr;

    epoll_ctl(epollfd, EPOLL_CTL_ADD, sockfd, &ev);
    return;
}

void new_client(struct args_epoll_monitor* e_m_struct, int* client_sockfd){
    /*Aceptamos al nuevo cliente*/
    *client_sockfd = accept(e_m_struct->sockfd_text, NULL, 0);
    assert(*client_sockfd != -1);

    /*Reactivamos el fd para monitoreo (EPOLLONESHOT activado)*/
    struct epoll_event ev;
    ev.events = EPOLLIN | EPOLLONESHOT | EPOLLET | EPOLLRDHUP;
    ev.data.ptr = e_m_struct->evlist->data.ptr;

    epoll_ctl(e_m_struct->epollfd, EPOLL_CTL_MOD, e_m_struct->sockfd_text, &ev);
    
    printf("Acepto a: %d\n", *client_sockfd);

    return;
}

void manage_client(struct args_epoll_monitor* e_m_struct){
    /*Cantidad de comandos que tendrá el pedido*/
    //int cant_com; 
    /*text_parser(client_fd, buf)*/
    /*En teoria ahora buf tiene el texto a parsear*/
}

/*void* epoll_monitor(struct args_epoll_monitor* args)*/
void* epoll_monitor(void* args){
    
    struct args_epoll_monitor* e_m_struct;
    e_m_struct = (struct args_epoll_monitor*) args;

    while(1){

        /*Retorna los fd listos para intercambio de datos*/
        /*No se bloquea ya que utilizamos notificaciones Edge-Triggered*/
        /*La idea es que los hilos checkeen si hay disponibles fd, si hay un hilo irá a 
            atender a ese cliente y las siguientes llamadas a epoll_wait no avisaran de este fd.
            La bandera EPOLLONESHOT ayuda a esto haciendo que no aparezca en la lista de ready 
            devuelta por epoll_wait en el caso de tener nuevo input que ya está siendo 
            atendida por un hilo. Ese hilo debe volver a activar las notificaciones de
            ese cliente.*/
        pthread_mutex_lock(&e_m_struct->mutex);
        e_m_struct->fdready = epoll_wait(e_m_struct->epollfd, e_m_struct->evlist, MAX_EVENTS, 0);

        /*Tenemos un fd disponible para lectura.*/
        /*Notar que esta es una región crítica, debemos asegurarla así un solo hilo atiende a un cliente.*/
        /*La podemos asegurar con un mutex*/
        if (e_m_struct->fdready){
             e_m_struct->fdready--; /*Atenderemos al cliente*/
             pthread_mutex_unlock(&e_m_struct->mutex);
        
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
                    printf("Es un nuevo cliente\n");
                    int client_sockfd;

                    /*Aceptamos al nuevo cliente*/
                    new_client(e_m_struct, &client_sockfd);

                    /*Lo añadimos a la instancia de epoll para monitorearlo.*/
                    epoll_add(client_sockfd, e_m_struct->epollfd, CAST_DATA_PTR->text_or_binary);
                }
                else{
                    /*Este cliente no es nuevo por lo que nos hará consultas.*/
                    /*Hacer un if para diferenciar entre cliente de texto y binario mediante el data.u32*/  

                    manage_client(e_m_struct);  
                }   
            }
        }
        else{pthread_mutex_unlock(&e_m_struct->mutex);}
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
    args.fdready = 0; //Retorno de epoll_wait
    for (int i = 0; i < cores; i++){
        pthread_create(&t[i], NULL, epoll_monitor, &args);
    }
    for(int i = 0; i < cores; i++)
        pthread_join(t[i], NULL);

    /*Capturar señal SIGPIPE*/

    
    return 0;
}
