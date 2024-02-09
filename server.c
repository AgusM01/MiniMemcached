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
#include <errno.h>
#include "server.h"

#define PORT_NUM_TEXT   8888
#define PORT_NUM_BIN    8889
#define MAX_EVENTS 1
#define MAX_CHAR 50 //Testing
#define N_COMMANDS 10
#define CAST_DATA_PTR ((struct data_ptr*)evlist->data.ptr)



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
    d_ptr->cant_comm_ptr = 0;
    d_ptr->delimit_pos = malloc(4*N_COMMANDS); /*Liberar esto*/
    d_ptr->actual_pos_arr = 0;
    ev.events = EPOLLIN | EPOLLONESHOT | EPOLLRDHUP; //NO USAR EDGE TRIGGERED YA QUE SI UN HILO MANDA MUCHISIMO NO VA A AVISAR PENSAR CHARLADO CON NERI ESCUCHAR AUDIO ZOE
    //printf("FLAGS: %d\n", (int)ev.events);
    ev.data.ptr = d_ptr;
    
    int fcntl_ret;
    fcntl_ret = fcntl(sockfd, F_SETFL, (fcntl(sockfd, F_GETFL, 0) | O_NONBLOCK));
    assert(fcntl_ret != -1);

    epoll_ctl(epollfd, EPOLL_CTL_ADD, sockfd, &ev);
    return;
}

/*Conecta a un nuevo cliente.*/
void new_client(struct args_epoll_monitor* e_m_struct, struct epoll_event* evlist){

    
    int client_sockfd;
    struct epoll_event ev;
    ev.events = EPOLLIN | EPOLLONESHOT | EPOLLRDHUP; 
    ev.data.ptr = evlist->data.ptr;

    /*Aceptamos al nuevo cliente*/
    client_sockfd = accept(e_m_struct->sockfd_text, NULL, 0);
    printf("Acepto a: %d\n", client_sockfd);
    
    if (client_sockfd == -1){
        if ((errno == EAGAIN) || (errno == EWOULDBLOCK)){            
            epoll_ctl(e_m_struct->epollfd, EPOLL_CTL_MOD, CAST_DATA_PTR->fd, &ev);
            return;
        }
        else{
            perror("Error al aceptar nuevo cliente\n");
            exit(1);
        } 
    }

    epoll_ctl(e_m_struct->epollfd, EPOLL_CTL_MOD, CAST_DATA_PTR->fd, &ev);
    epoll_add(client_sockfd, e_m_struct->epollfd, CAST_DATA_PTR->text_or_binary);

    
    
    return;
}

/*Consume el texto del fd de un cliente.*/
void text_consume(struct args_epoll_monitor* e_m_struct, struct epoll_event* evlist){
    /*Usar la funcion recv para leer del buffer de lectura. Usar ntohl para pasar de network-byte-order (big endian) a host-byte-order <- binario*/ 
    /*Funcion readline() lee hasta un '\n' <- posible*/
    char temp[MAX_CHAR]; /*Buffer temporal para ir desglosando los comandos*/
    //char* buf = malloc(MAX_CHAR);
    //char* resp = malloc(MAX_CHAR);
    char* err = "EINVAL";
    ssize_t fst_read, err_read;
    int pos_err_n = -1; /*Representa donde esta el \n en un mensaje que ya previamente es de error.*/
    int valid = 0; /*Bandera para indicar si el pedido es válido (<= 2048 caracteres)*/
    int cant_commands = N_COMMANDS;
    int atk = 0; //Si el usuario mandó un pedido muy largo incoherente, el hilo podria quedarse varado. Como una proteccion leo 3 veces y si no encontró desconecto. 

    printf("Leo de fd: %d\n", CAST_DATA_PTR->fd); 

    /*Checkeamos si no hay lugares de \n ya leidos, de esta manera si es que hay el hilo simplemente puede leer lo que necesita*/
    if (CAST_DATA_PTR->cant_comm_ptr == 0){

        /*Primer recv para ubicar la posicion del \n*/
        fst_read = recv(CAST_DATA_PTR->fd, temp, MAX_CHAR, MSG_PEEK); 
        assert(fst_read != -1);
        /* 3 casos:
        * fst_read = -1 -> Error de lectura: Cubierto con assert.
        * fst_read = 0 -> EOF/Shutdown Peer socket. 
        * fst_read = n -> numero de bytes leidos. */

        if(fst_read != 0){ /*Leyó algo.*/
            //printf("leí: %s\n", temp);
            /*Recorro lo leido para guardar los \n*/
            /*Si entró aca significa que no hay \n guardados, por lo que reiniciamos la pos del array.*/

            CAST_DATA_PTR->actual_pos_arr = 0;
            for (int i = 0; i < fst_read; i++){
                if(temp[i] == '\n'){
                    //printf("como encontre esto? i= %d\n", i);
                    valid = 1;
                    CAST_DATA_PTR->cant_comm_ptr++;

                    /*Posiblemente nunca deba ejecutarse pero por las dudas.*/
                    if(CAST_DATA_PTR->cant_comm_ptr == cant_commands){ 
                        cant_commands *= 2;
                        CAST_DATA_PTR->delimit_pos = realloc(CAST_DATA_PTR->delimit_pos, cant_commands);
                    }

                    CAST_DATA_PTR->delimit_pos[CAST_DATA_PTR->actual_pos_arr] = i; /*Guardo la pos de los \n*/
                    CAST_DATA_PTR->actual_pos_arr++;
                }
            }

            /*Reiniciamos la posicion del array*/
            CAST_DATA_PTR->actual_pos_arr = 0; 

            /*Que hacer si de base es un error <- La idea es que el hilo consuma todo el error asi deja a los demas hilos con futuros comandos válidos.*/
            if (!valid){ /*No hay ningun \n en 2048 bytes. Debo leer más para encontrarlo y descartar ese comando*/
                printf("Leí error, trataré de consumirlo.\n");
                err_read = recv(CAST_DATA_PTR->fd, temp, MAX_CHAR, 0); /*Consumo los 2048 primeros caracteres donde no hay \n*/
                assert(err_read != -1);

                /*Notar que no tiene sentido que de EOF, si dio 0 es porque se desconectó.*/
                if(err_read == 0){
                    epoll_ctl(e_m_struct->epollfd, EPOLL_CTL_DEL, CAST_DATA_PTR->fd, evlist); 
                    close(CAST_DATA_PTR->fd);
                    free(CAST_DATA_PTR->delimit_pos);
                    free(CAST_DATA_PTR);
                    return;
                }

                /*Leo 2048 caracteres sin consumir para tratar de buscar el \n*/
                err_read = recv(CAST_DATA_PTR->fd, temp, MAX_CHAR, MSG_PEEK);
                assert(err_read != -1);

                if (err_read == 0){
                
                    /*Error, o se desconectó o es EOF, ver el error mediante la bandera.*/
                    /*Este es el caso que mandó 2048 caracteres distintos de \n*/
                    /*Por ahora lo saco del epoll pero revisar*/
                    epoll_ctl(e_m_struct->epollfd, EPOLL_CTL_DEL, CAST_DATA_PTR->fd, evlist); 
                    close(CAST_DATA_PTR->fd);
                    free(CAST_DATA_PTR->delimit_pos);
                    free(CAST_DATA_PTR);
                    return;

                }
                else{ /*Empiezo una búsqueda del \n*/
                    while ((pos_err_n == -1) && (atk < 3)){
                        for(int i = 0; i < err_read; i++){
                            if (temp[i] == '\n'){ 
                                pos_err_n = i;
                            }
                        }
                        if (pos_err_n != -1){
                            err_read = recv(CAST_DATA_PTR->fd, temp, pos_err_n, 0); /*Leo los caracteres que se que pertenecen al mensaje erróneo,*/
                        }
                        else{
                            err_read = recv(CAST_DATA_PTR->fd, temp, err_read, 0); /*Leo los caracteres que se que pertenecen al mensaje erróneo,*/
                            err_read = recv(CAST_DATA_PTR->fd, temp, MAX_CHAR, MSG_PEEK); /*Leo 2048 más para ver si esta el \n, sin consumir.*/
                            assert(err_read != -1);
                            if (!err_read){
                                printf("Entre\n");
                                pos_err_n = 0; /*Leyó EOF por lo que termino el while y hago que se le mande el msg de error*/
                                atk = 0; /*Reinicio atk por las dudas*/
                            }
                            atk++;
                        }
                        printf("atk: %d\n",atk);
                    }

                    int snd = 0;
                    /*Aqui o ya consumimos todo el mal pedido o es un ataque. Lo cual enviamos el mensaje de error y decidimos que hacer.*/
                    //printf("Le mandoooo\n");
                    snd = send(CAST_DATA_PTR->fd, err, strlen(err), MSG_NOSIGNAL); /*La bandera ignora la señal SIGPIPE, de lo contrario hay que hacer un handler.*/
                    assert(snd != -1); //VER QUE A VECES ANTE UN ATK NO MANDA EL EINVAL.
                    //sleep(0.1);
                    if (atk == 3){ /*Lo desconecto, posiblemente ataque*/
                            printf("ATK elimino del epoll a: %d\n", CAST_DATA_PTR->fd);
                            epoll_ctl(e_m_struct->epollfd, EPOLL_CTL_DEL, CAST_DATA_PTR->fd, evlist); 
                            close(CAST_DATA_PTR->fd);
                            free(CAST_DATA_PTR->delimit_pos);
                            free(CAST_DATA_PTR);
                            return;
                    }

                    /*No fue ataque, lo vuelvo a monitorear*/
                    /*Reactivamos el fd para monitoreo (EPOLLONESHOT activado)*/
                    printf("se la perdono...\n");
                    struct epoll_event ev;
                    ev.events = EPOLLIN | EPOLLONESHOT | EPOLLRDHUP; 
                    ev.data.ptr = evlist->data.ptr;
                    epoll_ctl(e_m_struct->epollfd, EPOLL_CTL_MOD, CAST_DATA_PTR->fd, &ev);
                    return;
                }
            }
        }
        else{ /*No leyó nada, de base, lo que significa que se desconectó.*/

            /*Libero todo lo que tenia, cierro el fd y lo saco del epoll*/
            epoll_ctl(e_m_struct->epollfd, EPOLL_CTL_DEL, CAST_DATA_PTR->fd, evlist); 
            close(CAST_DATA_PTR->fd);
            free(CAST_DATA_PTR->delimit_pos);
            free(CAST_DATA_PTR);
            return;
        }
    }
    
    /*Los hilos que ya encuentren posiciones de \n solo haran esta parte. Los que no, deberán hacer todo lo anterior.*/
    /*Reiniciamos la posicion actual del array*/
    //CAST_DATA_PTR->actual_pos_arr = 0; 
    char comm[CAST_DATA_PTR->delimit_pos[CAST_DATA_PTR->actual_pos_arr]];
    int read_comm;
    
    /*Le sumo 1 ya que quiero leer hasta ese \n inclusive.*/
    read_comm = recv(CAST_DATA_PTR->fd, comm, CAST_DATA_PTR->delimit_pos[CAST_DATA_PTR->actual_pos_arr] + 1, 0);
    assert(read_comm != -1);
    
    /*Aumentamos la posicion del array*/
    CAST_DATA_PTR->actual_pos_arr += 1;
    /*Se decrementa la cantidad de comandos*/
    CAST_DATA_PTR->cant_comm_ptr -= 1; 
    
    /*Se llama al parser y luego a manage_client para responder el comando*/            
    printf("Leí: %s, ahora esto al parser.\n", comm);
    /*En teoria a este punto CAST_DATA_PTR->delimit_pos[0] contiene en que numero de byte esta el primer \n*/    
    /*La idea es que cada hilo ayude al siguiente completando el array con los proximos \n*/
    
    
    
    struct epoll_event ev;
    ev.events = EPOLLIN | EPOLLONESHOT | EPOLLRDHUP; 
    ev.data.ptr = evlist->data.ptr;
    epoll_ctl(e_m_struct->epollfd, EPOLL_CTL_MOD, CAST_DATA_PTR->fd, &ev);
    //printf("comm: %s\n", comm);
    
    return;


}

/*Le da un fd listo a cada thread*/
void* epoll_monitor(void* args){
    

    struct args_epoll_monitor* e_m_struct;
    e_m_struct = (struct args_epoll_monitor*) args;
    
    /*Inicializa la lista de los fd listos*/
    
    struct epoll_event* evlist = malloc(sizeof(struct epoll_event) * MAX_EVENTS); //Liberar esto al final
    assert(evlist != 0);
    //e_m_struct->evlist = evlist;
    int a = 0;

    while(1){

        /*Retorna los fd listos para intercambio de datos*/
        /*La idea es que los hilos checkeen si hay disponibles fd, si hay un hilo irá a 
            atender a ese cliente y las siguientes llamadas a epoll_wait no avisaran de este fd.
            La bandera EPOLLONESHOT ayuda a esto haciendo que no aparezca en la lista de ready 
            devuelta por epoll_wait en el caso de tener nuevo input que ya está siendo 
            atendida por un hilo. Ese hilo debe volver a activar las notificaciones de
            ese cliente. El hilo responde una consulta sola y lo vuelve a meter al epoll asi puede ir a atender a mas hilos*/
        a = epoll_wait(e_m_struct->epollfd, evlist, MAX_EVENTS, -1);
        assert(a != -1);

        printf("Atiendo a fd: %d, soy hilo: %ld\n", CAST_DATA_PTR->fd, pthread_self());
        //printf("FLAGS: %d\n", (int)e_m_struct->evlist->events);

        /*Tenemos un fd disponible para lectura.*/
        //printf("Voy a atender a fd: %d\n", CAST_DATA_PTR->fd);
        /*Verificar la presencia de EPOLLHUP o EPOLLER en cuyo caso hay que cerrar el fd*/
        
        if ((evlist->events & EPOLLRDHUP) 
            || (evlist->events & EPOLLERR)
            || (evlist->events & EPOLLHUP)){
            epoll_ctl(e_m_struct->epollfd, EPOLL_CTL_DEL, CAST_DATA_PTR->fd, evlist); /*En lugar de null puede ser e_m_struct->evlist para portabilidad pero ver bien*/
            close(CAST_DATA_PTR->fd);
            printf("fd cerrado: %d\n", CAST_DATA_PTR->fd);
            free(CAST_DATA_PTR->delimit_pos);
            free(CAST_DATA_PTR);
        }
        else{
            //printf("No es un cliente desconectado\n");

            /*Si es el fd del socket del server debemos atender a un nuevo cliente.*/
            if (CAST_DATA_PTR->fd == e_m_struct->sockfd_text ||
                CAST_DATA_PTR->fd == e_m_struct->sockfd_binary){
                
                printf("Hay que aceptar a alguien\n");
                /*Aceptamos al nuevo cliente*/
                new_client(e_m_struct, evlist);
                /*Lo añadimos a la instancia de epoll para monitorearlo.*/
            }
            else{
                printf("Es un cliente de pedidos\n");
                /*Este cliente no es nuevo por lo que nos hará consultas.*/
                /*Hacer un if para diferenciar entre cliente de texto y binario mediante el data.u32*/  
                text_consume(e_m_struct, evlist);  
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

    //printf("EPOLLONESHOT: %d\n", (int)EPOLLONESHOT);
    //printf("EPOLLIN: %d\n", (int)EPOLLIN);
    //printf("EPOLLRDHUP: %d\n", (int)EPOLLRDHUP);
    
    int cores = sysconf(_SC_NPROCESSORS_CONF);

    pthread_t t[cores];

    struct args_epoll_monitor args;
    args.epollfd = epollfd;
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

/*

/////aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\n 



bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb\n 

ar[0] = 200
ar[1] = 300

recv(buf, fd, 201);
recv(buf, fd, 300-200 +1 )


*/