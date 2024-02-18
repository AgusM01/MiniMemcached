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
#include <math.h>
#include "server.h"
#include "text_parser.h"
#include "structures/memc.h"
#include "structures/utils.h"

#define PORT_NUM_TEXT   8888
#define PORT_NUM_BIN    8889
#define MAX_EVENTS 1
#define MAX_CHAR 50
#define N_COMMANDS 10
#define CAST_DATA_PTR ((struct data_ptr*)evlist->data.ptr)
#define CAST_DATA_PTR_BINARY CAST_DATA_PTR->binary
#define DELIMITER " \n"

//enum commands {
//    PUT,
//    DEL,
//    GET,
//    STATS,
//    
//
//}
/*Argumentos para la funcion epoll_monitor*/
struct args_epoll_monitor {
    int epollfd;
    int sockfd_text;
    int sockfd_binary;
    memc_t mem;
};

/*Crea un socket TCP en dominio IPv4*/
/*Retorna un fd que representa nuestro socket y al cual se conectarán los clientes.*/
void sock_creation(int* sockfd, int port_num){
    //Ver lectura no bloqueante 
    *sockfd = socket(AF_INET, SOCK_STREAM, 0);
    assert(*sockfd != -1);

    /*Instancia la estructura*/
    struct sockaddr_in sa;
    sa.sin_family = AF_INET;
    sa.sin_port = htons(port_num);
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
    d_ptr->text_or_binary = mode;
    if (mode == 0){
        d_ptr->cant_comm_ptr = 0;
        d_ptr->actual_pos_arr = 0;
        d_ptr->delimit_pos = malloc(4*N_COMMANDS); /*Liberar esto*/
        d_ptr->binary = NULL;
        d_ptr->command = malloc(MAX_CHAR + 1);
        d_ptr->missing = 0;
        d_ptr->readed = 0;
    }
    if (mode == 1){
        struct data_ptr_binary* binary = malloc(sizeof(struct data_ptr_binary));
        d_ptr->binary = binary;
        d_ptr->binary->binary_to_read_commands = 5;
        d_ptr->binary->length_key = 0;
        d_ptr->binary->comandos_leidos = 0;
        d_ptr->binary->length_dato = 0;
        d_ptr->binary->to_consumed = 0;
        d_ptr->binary->data_or_key = 0;
        d_ptr->binary->dato = NULL;
        d_ptr->binary->key = NULL;
        d_ptr->binary->commands = malloc(6);
        d_ptr->delimit_pos = NULL;        
    }
    ev.data.ptr = d_ptr;
    

    ev.events = EPOLLIN | EPOLLONESHOT | EPOLLRDHUP; //NO USAR EDGE TRIGGERED YA QUE SI UN HILO MANDA MUCHISIMO NO VA A AVISAR PENSAR CHARLADO CON NERI ESCUCHAR AUDIO ZOE
    
    int fcntl_ret;
    fcntl_ret = fcntl(sockfd, F_SETFL, (fcntl(sockfd, F_GETFL, 0) | O_NONBLOCK));
    assert(fcntl_ret != -1);

    epoll_ctl(epollfd, EPOLL_CTL_ADD, sockfd, &ev);
    return;
}

/*Conecta a un nuevo cliente.*/
void new_client(struct args_epoll_monitor* e_m_struct, struct epoll_event* evlist, int mode){

    
    int client_sockfd;
    struct epoll_event ev;
    ev.events = EPOLLIN | EPOLLONESHOT | EPOLLRDHUP; 
    ev.data.ptr = evlist->data.ptr;

    if (mode == 0){
        /*Aceptamos al nuevo cliente*/
        client_sockfd = accept(e_m_struct->sockfd_text, NULL, 0);
        printf("Acepto a: %d modo texto.\n", client_sockfd);
    }
    else{
        client_sockfd = accept(e_m_struct->sockfd_binary, NULL, 0);
        printf("Acepto a: %d modo binario.\n", client_sockfd);
    }
    
    
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

void read_length(struct args_epoll_monitor* e_m_struct, struct epoll_event* evlist){
    
    int resp = 0;
    int tot_read = 0;
    int pos = 0;


    pos = 5 - CAST_DATA_PTR_BINARY->binary_to_read_commands;
    
    resp = recv(CAST_DATA_PTR->fd, (CAST_DATA_PTR_BINARY->commands + pos), CAST_DATA_PTR_BINARY->binary_to_read_commands, 0);
    perror("error_recv");
    if (resp == 0 || resp == -1){
        quit_epoll(e_m_struct, evlist);
        return;
    }
    
    tot_read = CAST_DATA_PTR_BINARY->binary_to_read_commands - resp;
    //printf("tot_read: %d\n", tot_read);
    
    if (tot_read > 0){ /*Todavia no llegó toda la longitud de la clave.*/
        CAST_DATA_PTR_BINARY->binary_to_read_commands = tot_read;
        CAST_DATA_PTR_BINARY->comandos_leidos = resp;
        return;
    }
    if (tot_read == 0 && CAST_DATA_PTR_BINARY->key == NULL){
        length_binary(CAST_DATA_PTR_BINARY->commands + 1, &CAST_DATA_PTR_BINARY->length_key);
        CAST_DATA_PTR_BINARY->key = malloc(CAST_DATA_PTR_BINARY->length_key + 1);
        CAST_DATA_PTR_BINARY->to_consumed = CAST_DATA_PTR_BINARY->length_key;
        return;
    }
    if (tot_read == 0 && CAST_DATA_PTR_BINARY->dato == NULL){
        //printf("dato null\n");
        length_binary(CAST_DATA_PTR_BINARY->commands + 1, &CAST_DATA_PTR_BINARY->length_dato);
        CAST_DATA_PTR_BINARY->dato = malloc(CAST_DATA_PTR_BINARY->length_dato + 1);
        CAST_DATA_PTR_BINARY->to_consumed = CAST_DATA_PTR_BINARY->length_dato;
        return;
    }
}

void read_content(struct args_epoll_monitor* e_m_struct, struct epoll_event* evlist, int data_or_key){

    int pos = 0;
    int resp = 0;
    int tot_read = 0;
    void* content;
    
    //printf("length key: %d\n", CAST_DATA_PTR_BINARY->length_key);
    //printf("to_consume: %d\n", CAST_DATA_PTR_BINARY->to_consumed);
    if(!data_or_key){
        content = CAST_DATA_PTR_BINARY->key;
        pos = CAST_DATA_PTR_BINARY->length_key - CAST_DATA_PTR_BINARY->to_consumed;
    }
    else{
        //printf("length dato: %d\n", CAST_DATA_PTR_BINARY->length_dato);
        content = CAST_DATA_PTR_BINARY->dato;
        pos = CAST_DATA_PTR_BINARY->length_dato - CAST_DATA_PTR_BINARY->to_consumed;
    }
    
    resp = recv(CAST_DATA_PTR->fd, content + pos, CAST_DATA_PTR_BINARY->to_consumed, 0);
    perror("error_recv");
    if (resp == 0 || resp == -1){
        quit_epoll(e_m_struct, evlist);
        return;
    }
    
    tot_read = CAST_DATA_PTR_BINARY->to_consumed - resp;

    if (tot_read > 0){ 
        CAST_DATA_PTR_BINARY->to_consumed = CAST_DATA_PTR_BINARY->to_consumed - resp;
        return;
    }
    if (tot_read == 0){
        CAST_DATA_PTR_BINARY->to_consumed = 0; /*Ya terminó de consumir todo*/
    }
    return;

}

/*Consume el binario del fd de un cliente.*/
void binary_consume(struct args_epoll_monitor* e_m_struct, struct epoll_event* evlist){
    
    int resp = 0;
    
    /*En la primera vez consume el primer comando (PUT/GET/DEL/...)*/
    if (CAST_DATA_PTR_BINARY->binary_to_read_commands == 5){
        //printf("Leo de fd: %d\n", CAST_DATA_PTR->fd);
        resp = recv(CAST_DATA_PTR->fd, CAST_DATA_PTR_BINARY->commands, 1, 0); 
        perror("error_recv");
        if (resp == 0 || resp == -1){
            quit_epoll(e_m_struct, evlist);
            return;
        }

        CAST_DATA_PTR_BINARY->binary_to_read_commands -= 1; /*Comandos a leer, ya leí el primero*/

    }
    
    /*Si no es ningun comando conocido o que requiera leer valores (STATS) directamente lo maneja*/
    if ((CAST_DATA_PTR_BINARY->commands[0] != 11)
        || (CAST_DATA_PTR_BINARY->commands[0] != 12)
        || (CAST_DATA_PTR_BINARY->commands[0] != 13)){
            manage_client_binary(e_m_struct, evlist);
            return;
        }
    //printf("Lei el comando: %s\n", CAST_DATA_PTR_BINARY->commands[0]);  
    /*Llama a la funcion que lee las longitudes y calcula cuando mide la clave.*/
    if(CAST_DATA_PTR_BINARY->key == NULL)
        read_length(e_m_struct, evlist);

    //printf("Command = %d\n", (int)CAST_DATA_PTR_BINARY->commands[0]);
    //printf("Command = %d\n", (int)CAST_DATA_PTR_BINARY->commands[1]);
    //printf("Command = %d\n", (int)CAST_DATA_PTR_BINARY->commands[2]);
    //printf("Command = %d\n", (int)CAST_DATA_PTR_BINARY->commands[3]);
    //printf("Command = %d\n", (int)CAST_DATA_PTR_BINARY->commands[4]);

    /*Si la clave es NULL, significa que todavia no terminó de leer las longitudes, lo meto al epoll y espero a que mande mas longitudes.*/
    if (CAST_DATA_PTR_BINARY->key == NULL){
        struct epoll_event ev;
        ev.events = EPOLLIN | EPOLLONESHOT | EPOLLRDHUP; 
        ev.data.ptr = evlist->data.ptr;
        epoll_ctl(e_m_struct->epollfd, EPOLL_CTL_MOD, CAST_DATA_PTR_BINARY->fd, &ev);
        return;
    }

    /*Si el dato es NULL deberia leer la clave*/
    if (CAST_DATA_PTR_BINARY->data_or_key == 0)
        read_content(e_m_struct, evlist, CAST_DATA_PTR_BINARY->data_or_key);

    /*Todavía no terminó de leer el contenido*/
    if (CAST_DATA_PTR_BINARY->to_consumed != 0){
        
        struct epoll_event ev;
        ev.events = EPOLLIN | EPOLLONESHOT | EPOLLRDHUP; 
        ev.data.ptr = evlist->data.ptr;
        epoll_ctl(e_m_struct->epollfd, EPOLL_CTL_MOD, CAST_DATA_PTR_BINARY->fd, &ev);
        return;
    }

    /*Si es un put tengo que hacer otra lectura*/
    if ((int)CAST_DATA_PTR_BINARY->commands[0] == 11){

        CAST_DATA_PTR_BINARY->binary_to_read_commands = 4 - CAST_DATA_PTR_BINARY->comandos_leidos;
        CAST_DATA_PTR_BINARY->data_or_key += 1;

        if (CAST_DATA_PTR_BINARY->dato == NULL)
            read_length(e_m_struct, evlist);
        
        if (CAST_DATA_PTR_BINARY->dato == NULL){
            struct epoll_event ev;
            ev.events = EPOLLIN | EPOLLONESHOT | EPOLLRDHUP; 
            ev.data.ptr = evlist->data.ptr;
            epoll_ctl(e_m_struct->epollfd, EPOLL_CTL_MOD, CAST_DATA_PTR_BINARY->fd, &ev);
            return;
        }

        read_content(e_m_struct, evlist, CAST_DATA_PTR_BINARY->data_or_key);

        /*Todavía no terminó de leer el contenido*/
        if (CAST_DATA_PTR_BINARY->to_consumed != 0){

            struct epoll_event ev;
            ev.events = EPOLLIN | EPOLLONESHOT | EPOLLRDHUP; 
            ev.data.ptr = evlist->data.ptr;
            epoll_ctl(e_m_struct->epollfd, EPOLL_CTL_MOD, CAST_DATA_PTR_BINARY->fd, &ev);
            return;
        }
        
    }
    /*En teoria aca ya tengo la key completa y el dato, en caso que haya sido un put*/
    printf("Key: %s\n", CAST_DATA_PTR_BINARY->key);
    if(CAST_DATA_PTR_BINARY->dato != NULL)
        printf("Dato: %s\n", CAST_DATA_PTR_BINARY->dato);
    
    manage_client_binary(e_m_struct, evlist);
    
    return;
}
 
void length_binary(unsigned char* commands, int* length){

    for (int i = 0; i < 4; i++){
        int temp = (int)commands[i];
        *length += temp * pow(256, 3 - i);
    }
    return;    
}

void manage_client_binary(struct args_epoll_monitor* e_m_struct, struct epoll_event* evlist){

    char ok = 101; 
    char einval = 111;
    char enotfound = 112;
    int command = -1;
    int res = 0;
    int snd = 0;
    char* buf;
    unsigned char len[4];
    stats_t stats;
    char stat_buf[150];
    int len_stat_buf = 0;
    /*Primero debo ver que comando me mandó, está en command[0]*/

    if ((int)CAST_DATA_PTR_BINARY->commands[0] == 11){
        command = 0;
        res = memc_put(e_m_struct->mem, 
                        CAST_DATA_PTR_BINARY->key,
                        CAST_DATA_PTR_BINARY->dato,
                        CAST_DATA_PTR_BINARY->length_key,
                        CAST_DATA_PTR_BINARY->length_dato,
                        CAST_DATA_PTR->text_or_binary);
        snd = send(CAST_DATA_PTR->fd, &ok, 1, 0);
        perror("error_send");
        if (snd == 0 || snd == -1){
            quit_epoll(e_m_struct, evlist);
            return;
        }

    }
    if ((int)CAST_DATA_PTR_BINARY->commands[0] == 13){
        command = 1;
        res = memc_get(e_m_struct->mem,
                        CAST_DATA_PTR_BINARY->key,
                        CAST_DATA_PTR_BINARY->length_key,
                        (void**)&buf,
                        CAST_DATA_PTR->text_or_binary);
        if (!res){
            snd = send(CAST_DATA_PTR->fd, &enotfound, 1, 0);
            perror("error_send");
            if (snd == 0 || snd == -1){
                quit_epoll(e_m_struct, evlist);
                return;
            }
        }
        else{
            int_to_binary(res, (void*)&len);
            snd = send(CAST_DATA_PTR->fd, &ok, 1,0);
            perror("error_send");
            if (snd == 0 || snd == -1){
                quit_epoll(e_m_struct, evlist);
                free(buf);
                return;
            }
            snd = send(CAST_DATA_PTR->fd, &len, 4,0);
            perror("error_send");
            if (snd == 0 || snd == -1){
                quit_epoll(e_m_struct, evlist);
                free(buf);
                return;
            }
            snd = send(CAST_DATA_PTR->fd, buf, res,0);
            perror("error_send");
            if (snd == 0 || snd == -1){
                quit_epoll(e_m_struct, evlist);
                free(buf);
                return;
            }
        }

    }
    if ((int)CAST_DATA_PTR_BINARY->commands[0] == 12){
        command = 2;
        res = memc_del(e_m_struct->mem,
                        CAST_DATA_PTR_BINARY->key,
                        CAST_DATA_PTR_BINARY->length_key,
                        CAST_DATA_PTR->text_or_binary);
        if (res == -1){
            snd = send(CAST_DATA_PTR->fd, &enotfound, 1,0);
            perror("error_send");
            if (snd == 0 || snd == -1){
                quit_epoll(e_m_struct, evlist);
                return;
            }
        }
        else{
            snd = send(CAST_DATA_PTR->fd, &ok, 1,0);
            perror("error_send");
            if (snd == 0 || snd == -1){
                quit_epoll(e_m_struct, evlist);
                return;
            }
        }
        
    }
    if ((int)CAST_DATA_PTR_BINARY->commands[0] == 21){
        command = 3;
        stats = memc_stats(e_m_struct->mem);
        len_stat_buf = sprintf(stat_buf,"OK PUTS=%lu DELS=%lu GETS= %lu KEYS=%lu\n", stats->puts, stats->dels, stats->gets, stats->keys);
        int_to_binary(len_stat_buf, (void*)&len);
        
        snd = send(CAST_DATA_PTR->fd, &ok, 1, 0);        
        perror("error_send");
        if (snd == 0 || snd == -1){
            quit_epoll(e_m_struct, evlist);
            return;
        }
        
        snd = send(CAST_DATA_PTR->fd, &len, 4, 0);        
        perror("error_send");
        if (snd == 0 || snd == -1){
            quit_epoll(e_m_struct, evlist);
            return;
        }
        
        snd = send(CAST_DATA_PTR->fd, &stat_buf, len_stat_buf, 0);        
        perror("error_send");
        if (snd == 0 || snd == -1){
            quit_epoll(e_m_struct, evlist);
            return;
        }
    }
    if (command == -1){

        snd = send(CAST_DATA_PTR->fd, &einval, 1, 0);
        perror("error_send");
        if (snd == 0 || snd == -1){
            quit_epoll(e_m_struct, evlist);
            return;
        }

    }
    return;

}

/*Consume el texto del fd de un cliente.*/
void text_consume(struct args_epoll_monitor* e_m_struct, struct epoll_event* evlist){
    /*Usar la funcion recv para leer del buffer de lectura. Usar ntohl para pasar de network-byte-order (big endian) a host-byte-order <- binario*/ 
    /*Funcion readline() lee hasta un '\n' <- posible*/
    /*Falta que termine de llegar un comando*/    

    /*Buffer temporal para procesar el comando.*/
    char temp[MAX_CHAR + 1];
    //char* buf = malloc(MAX_CHAR);
    //char* resp = malloc(MAX_CHAR);
    char* err = "EINVAL\n";
    ssize_t fst_read, err_read;
    int pos_err_n = -1; /*Representa donde esta el \n en un mensaje que ya previamente es de error.*/
    int valid = 0; /*Bandera para indicar si el pedido es válido (<= 2048 caracteres)*/
    int cant_commands = N_COMMANDS;
    int atk = 0; //Si el usuario mandó un pedido muy largo incoherente, el hilo podria quedarse varado. Como una proteccion leo 3 veces y si no encontró desconecto. 
    int snd = 0;
    int rec = 0;
    printf("Leo de fd: %d\n", CAST_DATA_PTR->fd); 

    /*Checkeamos si no hay lugares de \n ya leidos, de esta manera si es que hay el hilo simplemente puede leer lo que necesita*/
    if (CAST_DATA_PTR->cant_comm_ptr == 0){

        /*Primer recv para ubicar la posicion del \n*/
        fst_read = recv(CAST_DATA_PTR->fd, temp, MAX_CHAR, MSG_PEEK); 
        perror("error_send");
        if (fst_read == 0 || fst_read == -1){
            quit_epoll(e_m_struct, evlist);
            return;
        }

        /*Ubicamos terminador de cadena.*/
        temp[fst_read] = '\0';
        /* 3 casos:
        * fst_read = -1 -> Error de lectura: Cubierto con assert.
        * fst_read = 0 -> EOF/Shutdown Peer socket. 
        * fst_read = n -> numero de bytes leidos. */
            
        /*Recorro lo leido para guardar los \n*/
        /*Si entró aca significa que no hay \n guardados, por lo que reiniciamos la pos del array.*/
        CAST_DATA_PTR->actual_pos_arr = 0;
        for (int i = 0; i < fst_read; i++){
            if(temp[i] == '\n'){
                printf("Lo encontré i= %d\n", i);
                valid = 1;
                CAST_DATA_PTR->cant_comm_ptr += 1;
                /*Posiblemente nunca deba ejecutarse pero por las dudas.*/
                if(CAST_DATA_PTR->cant_comm_ptr == cant_commands){ 
                    cant_commands *= 2;
                    CAST_DATA_PTR->delimit_pos = realloc(CAST_DATA_PTR->delimit_pos, cant_commands * 4);
                }
                CAST_DATA_PTR->delimit_pos[CAST_DATA_PTR->actual_pos_arr] = i + 1; /*Guardo la pos de los \n , como empieza del 0 le sumo 1 para que el recv tambien lo consuma*/
                CAST_DATA_PTR->actual_pos_arr += 1;
            }
        }
        /*Reiniciamos la posicion del array*/
        CAST_DATA_PTR->actual_pos_arr = 0; 
        /*Que hacer si de base es un error <- La idea es que el hilo consuma todo el error asi deja a los demas hilos con futuros comandos válidos.*/
        /*Notar que CREO que bash pone justo \n cada 2048 caracteres*/
        if ((!valid && fst_read == MAX_CHAR) || CAST_DATA_PTR->readed >= 2048){ /*No hay ningun \n en 2048 bytes. Debo leer más para encontrarlo y descartar ese comando*/
            printf("Leí error, trataré de consumirlo.\n");

            err_read = recv(CAST_DATA_PTR->fd, temp, MAX_CHAR, 0); /*Consumo los 2048 primeros caracteres donde no hay \n*/
            perror("error_recv_error");
            if (err_read == 0 || err_read == -1){
                quit_epoll(e_m_struct, evlist);
                return;
            }
            
            temp[err_read] = '\0';
            /*Leo 2048 caracteres sin consumir para tratar de buscar el \n*/
            err_read = recv(CAST_DATA_PTR->fd, temp, MAX_CHAR, MSG_PEEK);
            perror("error_recv_error");
            if (err_read == 0 || err_read == -1){
                quit_epoll(e_m_struct, evlist);
                return;
            }
            //printf("err_read: %d\n", err_read);
            temp[err_read] = '\0';
            //printf("temp 2: %s\n", temp);
            
            while ((pos_err_n == -1) && (atk < 3)){
                for(int i = 0; i < err_read; i++){
                    if (temp[i] == '\n'){ 
                        pos_err_n = i + 1;
                        //printf("i: %d\n", i);
                        //printf("temp 3: %s \n", temp);
                    }
                }
                if (pos_err_n != -1){
                    err_read = recv(CAST_DATA_PTR->fd, temp, pos_err_n, 0); /*Leo los caracteres que se que pertenecen al mensaje erróneo, junto con el \n.*/
                    perror("error_recv_error");
                    if (err_read == 0 || err_read == -1){
                        quit_epoll(e_m_struct, evlist);
                        return;
                    }
                    temp[err_read] = '\0';
                }
                else{
                    
                    err_read = recv(CAST_DATA_PTR->fd, NULL, err_read, 0); /*Leo los caracteres que se que pertenecen al mensaje erróneo,*/
                    perror("error_recv_error");
                    if (err_read == 0 || err_read == -1){
                        quit_epoll(e_m_struct, evlist);
                        return;
                    }
                    
                    err_read = recv(CAST_DATA_PTR->fd, temp, MAX_CHAR, MSG_PEEK); /*Leo 2048 más para ver si esta el \n, sin consumir.*/
                    perror("error_recv_error");
                    temp[err_read] = '\0';
                    if (!err_read){
                        pos_err_n = 0; /*Leyó EOF por lo que termino el while y hago que se le mande el msg de error*/
                        atk = 0; /*Reinicio atk por las dudas*/
                    }
                    atk += 1;
                }
                printf("atk: %d\n",atk);
            }
            /*Aqui o ya consumimos todo el mal pedido o es un ataque. Lo cual enviamos el mensaje de error y decidimos que hacer.*/
            //printf("Le mandoooo\n");
            snd = send(CAST_DATA_PTR->fd, err, strlen(err), MSG_NOSIGNAL); /*La bandera ignora la señal SIGPIPE, de lo contrario hay que hacer un handler.*/
            perror("error_send_error");
            if (snd == 0 || snd == -1){
                quit_epoll(e_m_struct, evlist);
                return;
            }

            //sleep(0.1);
            if (atk == 3){ /*Lo desconecto, posiblemente ataque*/
                quit_epoll(e_m_struct, evlist);
                return;
            }
            /*No fue ataque, lo vuelvo a monitorear*/
            /*Reactivamos el fd para monitoreo (EPOLLONESHOT activado)*/
            printf("se la perdono...\n");
            CAST_DATA_PTR->readed = 0;
            return;
            /*Enviar einval y sacarlo, hacerla mas facil.*/
        }
        if(!valid){ /*No leyó ningun \n, debo esperar a que llegue el resto del pedido.*/

            if (CAST_DATA_PTR->readed + fst_read >= MAX_CHAR){
                snd = send(CAST_DATA_PTR->fd, &err, strlen(err), 0);
                perror("error_send");
                if (snd == 0 || snd == -1){
                    quit_epoll(e_m_struct, evlist);
                    return;
                }
                CAST_DATA_PTR->readed = 0;
                return;
            }
            fst_read = recv(CAST_DATA_PTR->fd, CAST_DATA_PTR->command + CAST_DATA_PTR->readed, fst_read, 0); /*Lo consumo*/
            perror("error_recv");
            if (fst_read == 0 || fst_read == -1){
                quit_epoll(e_m_struct, evlist);
                return;
            }
            CAST_DATA_PTR->readed += fst_read; /*Si ya pasaron 2048 sin \n lo vuelo*/
            CAST_DATA_PTR->missing = 1;
            printf("Command: %s\n", CAST_DATA_PTR->command);
            return;
        }
    }
    
    /*Los hilos que ya encuentren posiciones de \n solo haran esta parte. Los que no, deberán hacer todo lo anterior.*/
    /*Reiniciamos la posicion actual del array*/
    //CAST_DATA_PTR->actual_pos_arr = 0; 
    int d = 0;
    int s = CAST_DATA_PTR->delimit_pos[CAST_DATA_PTR->actual_pos_arr];
    int read_comm;
    int cant_comm;
    char** token_commands; /*Liberar esto*/
    
    if (CAST_DATA_PTR->actual_pos_arr > 0)
        d = CAST_DATA_PTR->delimit_pos[CAST_DATA_PTR->actual_pos_arr - 1];
    
    s = s - d;
    if (CAST_DATA_PTR->missing){ //Lo tengo que unir
        
        rec = recv(CAST_DATA_PTR->fd, CAST_DATA_PTR->command + CAST_DATA_PTR->readed, fst_read, 0);
        perror("error_recv");
        if (rec == 0 || rec == -1){
            quit_epoll(e_m_struct, evlist);
            return;
        }
        
        printf("command pre token: %s\n", CAST_DATA_PTR->command);
        CAST_DATA_PTR->command[CAST_DATA_PTR->readed + fst_read] = '\0';
        token_commands = text_parser(CAST_DATA_PTR->command, &cant_comm, DELIMITER);
    }
    else{
        char* comm = malloc(s + 1);

        read_comm = recv(CAST_DATA_PTR->fd, comm, s, 0);
        perror("error_recv");
        if (read_comm == 0 || read_comm == -1){
            quit_epoll(e_m_struct, evlist);
            free(comm);
            return;
        }

        comm[s + 1] = '\0'; /*Le agrego el terminador de cadena*/
        token_commands = text_parser(comm, &cant_comm, DELIMITER);
    }

    CAST_DATA_PTR->actual_pos_arr += 1;
    CAST_DATA_PTR->cant_comm_ptr -= 1; 

    for (int i = 0; i < cant_comm; i++)
        printf("comando %d = %s\n", i, token_commands[i]);

    manage_client(e_m_struct, evlist, token_commands, cant_comm);
    
    free(token_commands);
    
    return; 

}

void quit_epoll(struct args_epoll_monitor* e_m_struct, struct epoll_event* evlist){

    printf("Lo saque a: %d\n", CAST_DATA_PTR->fd);
    epoll_ctl(e_m_struct->epollfd, EPOLL_CTL_DEL, CAST_DATA_PTR->fd, evlist); /*En lugar de null puede ser e_m_struct->evlist para portabilidad pero ver bien*/
    
    if(!CAST_DATA_PTR->text_or_binary){
        close(CAST_DATA_PTR->fd);
        free(CAST_DATA_PTR->command);
        free(CAST_DATA_PTR->delimit_pos);
    }
    else{
        close(CAST_DATA_PTR->fd);
        free(CAST_DATA_PTR_BINARY->commands);
        free(CAST_DATA_PTR_BINARY->dato);
        free(CAST_DATA_PTR_BINARY->key);
        free(CAST_DATA_PTR_BINARY);
    }
    free(CAST_DATA_PTR);
    return;
}

void manage_client(struct args_epoll_monitor* e_m_struct, struct epoll_event* evlist, char** token_comands, int cant_comm){
    
    stats_t stats;

    char* get_value;
    
    char einval[8] = "EINVAL\n";
    char ok[4] = "OK\n";
    char ebig[6] = "EBIG\n";
    char ebinary[9] = "EBINARY\n";
    char enotfound[11] = "ENOTFOUND\n";
    char ok_get[4] = "OK ";
    char stat_buf[MAX_CHAR];

    int snd = 0;
    int res;
    int command = -1;
    int len_stat_buf;

    printf("cant_comm: %d\n", cant_comm);
    if (!strcmp(token_comands[0], "PUT")) 
        command = 0;
    if (!strcmp(token_comands[0], "DEL")) 
        command = 1;
    if (!strcmp(token_comands[0], "GET")) 
        command = 2;
    if (!strcmp(token_comands[0], "STATS")) 
        command = 3;

    if ((cant_comm == 0)
        || (command == -1)
        || ((command == 0) && (cant_comm != 3))
        || ((command == 1) && (cant_comm != 2))
        || ((command == 2) && (cant_comm != 2))
        || ((command == 3) && (cant_comm != 1)))
        {
        snd = send(CAST_DATA_PTR->fd, einval, 7, MSG_NOSIGNAL); /*La bandera ignora la señal SIGPIPE, de lo contrario hay que hacer un handler.*/
        perror("error_send");
        if (snd == 0 || snd == -1){
            quit_epoll(e_m_struct, evlist);
            return;
        }

        struct epoll_event ev;
        ev.events = EPOLLIN | EPOLLONESHOT | EPOLLRDHUP; 
        ev.data.ptr = evlist->data.ptr;
        epoll_ctl(e_m_struct->epollfd, EPOLL_CTL_MOD, CAST_DATA_PTR->fd, &ev);
        return;
    }

    if (command == 0){
        
        //printf("Pongo clave: %s, valor: %s\n Longitudes clave: %ld, valor: %ld\n", token_comands[1], token_comands[2], strlen(token_comands[1]),strlen(token_comands[2]));

        res = memc_put  (e_m_struct->mem, 
                        token_comands[1], 
                        token_comands[2], 
                        strlen(token_comands[1]),
                        strlen(token_comands[2]),
                        CAST_DATA_PTR->text_or_binary); /*TEXTO*/
        
        snd = send(CAST_DATA_PTR->fd, ok, strlen(ok), MSG_NOSIGNAL);
        perror("error_send");
        if (snd == 0 || snd == -1){
            quit_epoll(e_m_struct, evlist);
            return;
        }
    }
    if (command == 1){
        res = memc_del  (e_m_struct->mem,
                        token_comands[1],
                        strlen(token_comands[1]),
                        CAST_DATA_PTR->text_or_binary);
        
        if (res == 0)
            snd = send(CAST_DATA_PTR->fd, ok, strlen(ok), MSG_NOSIGNAL);
        else
            snd = send(CAST_DATA_PTR->fd, einval, strlen(einval), MSG_NOSIGNAL); /*La bandera ignora la señal SIGPIPE, de lo contrario hay que hacer un handler.*/
        
        perror("error_send");
        if (snd == 0 || snd == -1){
            quit_epoll(e_m_struct, evlist);
            return;
        }
    }
    if (command == 2){
        //printf("Busco: %s, longitud: %ld\n", token_comands[1], strlen(token_comands[1]));
        res = memc_get  (e_m_struct->mem,
                        token_comands[1], 
                        strlen(token_comands[1]), 
                        (void**)&get_value, 
                        CAST_DATA_PTR->text_or_binary);

        if (res == -1)
            snd = send(CAST_DATA_PTR->fd, ebinary, strlen(ebinary), MSG_NOSIGNAL);
        if (res == 0)
            snd = send(CAST_DATA_PTR->fd, enotfound, strlen(enotfound), MSG_NOSIGNAL);
        if (res > 0){
            char resp[res + 5];
            sprintf(resp, "OK %s\n", get_value);
            if (strlen(ok_get) <= 2048)
                snd = send(CAST_DATA_PTR->fd, resp, strlen(resp), MSG_NOSIGNAL);
            else    
                snd = send(CAST_DATA_PTR->fd, ebig, strlen(ebig), MSG_NOSIGNAL);
        }

        perror("error_send");
        if (snd == 0 || snd == -1){
            quit_epoll(e_m_struct, evlist);
            if (res > 0)
                free(get_value);
            return;
        }

        if (res > 0)
            free(get_value);
    
    }
    if (command == 3){

        stats = memc_stats(e_m_struct->mem);
        len_stat_buf = sprintf(stat_buf,"OK PUTS=%lu DELS=%lu GETS= %lu KEYS=%lu\n", stats->puts, stats->dels, stats->gets, stats->keys);
        assert(len_stat_buf > 0);
        snd = send(CAST_DATA_PTR->fd, stat_buf, len_stat_buf, MSG_NOSIGNAL);
        perror("error_send");
        if (snd == 0 || snd == -1){
            quit_epoll(e_m_struct, evlist);
            return;
        }
    }
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
    printf("Entro al while soy %ld\n", pthread_self());
    while(1){

        /*Retorna los fd listos para intercambio de datos*/
        /*La idea es que los hilos checkeen si hay disponibles fd, si hay un hilo irá a 
            atender a ese cliente y las siguientes llamadas a epoll_wait no avisaran de este fd.
            La bandera EPOLLONESHOT ayuda a esto haciendo que no aparezca en la lista de ready 
            devuelta por epoll_wait en el caso de tener nuevo input que ya está siendo 
            atendida por un hilo. Ese hilo debe volver a activar las notificaciones de
            ese cliente. El hilo responde una consulta sola y lo vuelve a meter al epoll asi puede ir a atender a mas hilos*/
        a = epoll_wait(e_m_struct->epollfd, evlist, MAX_EVENTS, -1);
        perror("epoll_wait");
        assert(a != -1);

        printf("Atiendo a fd: %d, soy hilo: %ld\n", CAST_DATA_PTR->fd, pthread_self());
        //printf("FLAGS: %d\n", (int)e_m_struct->evlist->events);

        /*Tenemos un fd disponible para lectura.*/
        //printf("Voy a atender a fd: %d\n", CAST_DATA_PTR->fd);
        /*Verificar la presencia de EPOLLHUP o EPOLLER en cuyo caso hay que cerrar el fd*/
        
        if ((evlist->events & EPOLLRDHUP) 
            || (evlist->events & EPOLLERR)
            || (evlist->events & EPOLLHUP)){
            quit_epoll(e_m_struct, evlist);
        }
        else{
            //printf("No es un cliente desconectado\n");

            /*Si es el fd del socket del server debemos atender a un nuevo cliente.*/
            if ((CAST_DATA_PTR->fd == e_m_struct->sockfd_text) ||
                (CAST_DATA_PTR->fd == e_m_struct->sockfd_binary)){
                
                printf("Hay que aceptar a alguien.\n");
                /*Aceptamos al nuevo cliente*/
                new_client(e_m_struct, evlist, CAST_DATA_PTR->text_or_binary);
                /*Lo añadimos a la instancia de epoll para monitorearlo.*/
            }
            else{

                if (!CAST_DATA_PTR->text_or_binary){

                    printf("Es un cliente de pedidos\n");
                    /*Este cliente no es nuevo por lo que nos hará consultas.*/
                    /*Hacer un if para diferenciar entre cliente de texto y binario mediante el data.u32*/  

                    text_consume(e_m_struct, evlist);
                    //if (tokens != NULL){
                    //    manage_client(e_m_struct, evlist, tokens, cant_comm);
                    //}
//
                    //free(tokens[0]);
                    //free(tokens);

                }
                else{
                    binary_consume(e_m_struct, evlist);
                }
                
                struct epoll_event ev;
                ev.events = EPOLLIN | EPOLLONESHOT | EPOLLRDHUP; 
                ev.data.ptr = evlist->data.ptr;
                epoll_ctl(e_m_struct->epollfd, EPOLL_CTL_MOD, CAST_DATA_PTR->fd, &ev);
            }   
        }
    }
    return NULL; 
}

/*Esta función pasa a llamarse serv_init() y es la encargada de prender/manejar el server*/
int main (int argc, char* argv[]){

    int sockfd_text; /*Este seria el socket de texto*/
    int sockfd_binary;; /*Este seria el socket binario -> 9999 testing*/
    int epollfd; /*fd referente a la instancia de epoll*/

    memc_t mem = memc_init((HasFunc)hash_len, 2, 500, 0);

    /*Crea los sockets y devuelve su fd ya bindeado y en escucha*/
    sock_creation(&sockfd_text, PORT_NUM_TEXT);

    sock_creation(&sockfd_binary, PORT_NUM_BIN);

    /*Inicia la instancia de epoll y devuelve el fd que hace referencia a la misma*/
    epoll_initiate(&epollfd);

    /*Añade el fd del socket de texto creado a la instancia de epoll para monitorearlo.*/
    epoll_add(sockfd_text, epollfd, 0);

    /*Añade el fd del socket binario creado a la instancia de epoll para monitorearlo.*/
    epoll_add(sockfd_binary, epollfd, 1);

    int test = 5283612;
    unsigned char len[4];
    printf("%p\n", len);
    int_to_binary(test, (void*)len);
    for (int i = 0; i < 4; i++){
        int temp = (int)len[i];
        printf("temp: %d\n", temp);
    }

    //printf("EPOLLONESHOT: %d\n", (int)EPOLLONESHOT);
    //printf("EPOLLIN: %d\n", (int)EPOLLIN);
    //printf("EPOLLRDHUP: %d\n", (int)EPOLLRDHUP);
    
    int cores = sysconf(_SC_NPROCESSORS_CONF);

    pthread_t t[cores];

    struct args_epoll_monitor args;
    args.epollfd = epollfd;
    args.sockfd_text = sockfd_text;
    args.sockfd_binary = sockfd_binary;
    args.mem = mem;

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

13 0 80 159 28 key 

*/