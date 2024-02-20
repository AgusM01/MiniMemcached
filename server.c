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
#include <sys/signal.h>
#include "server.h"
#include "text_parser.h"
#include "structures/memc.h"
#include "structures/utils.h"

#define PORT_NUM_TEXT   8888
#define PORT_NUM_BIN    8889
#define MAX_EVENTS 1
#define MAX_CHAR 2048
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
    perror("sock_creation");

    /*Instancia la estructura*/
    struct sockaddr_in sa;
    sa.sin_family = AF_INET;
    sa.sin_port = htons(port_num);
    sa.sin_addr.s_addr = htonl(INADDR_ANY);

    /*Bindea el socket al puerto a utilizar y les asigna las características requeridas*/
    bind(*sockfd, (struct sockaddr*) &sa, sizeof(sa));

    // Ponemos el socket en escucha.
    listen(*sockfd, SOMAXCONN);
    perror("sock_listen");
    return;
}

/*Crea una instancia de epoll*/
void epoll_initiate(int* epollfd){

    *epollfd = epoll_create(1);
    perror("epoll_create");
    
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
    assert(d_ptr != NULL);
    d_ptr->fd = sockfd;
    d_ptr->text_or_binary = mode;
    if (mode == 0){
        d_ptr->cant_comm_ptr = 0;
        d_ptr->actual_pos_arr = 0;
        d_ptr->delimit_pos = malloc(4*N_COMMANDS); /*Liberar esto*/
        assert(d_ptr->delimit_pos != NULL);
        d_ptr->binary = NULL;
        d_ptr->command = malloc(MAX_CHAR + 1);
        assert(d_ptr->command != NULL);
        d_ptr->missing = 0;
        d_ptr->readed = 0;
    }
    if (mode == 1){
        struct data_ptr_binary* binary = malloc(sizeof(struct data_ptr_binary));
        assert(binary != NULL);
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
        assert(d_ptr->binary->commands != NULL);
        d_ptr->delimit_pos = NULL;        
    }
    ev.data.ptr = d_ptr;
    

    ev.events = EPOLLIN | EPOLLONESHOT | EPOLLRDHUP; //NO USAR EDGE TRIGGERED YA QUE SI UN HILO MANDA MUCHISIMO NO VA A AVISAR PENSAR CHARLADO CON NERI ESCUCHAR AUDIO ZOE
    
    fcntl(sockfd, F_SETFL, (fcntl(sockfd, F_GETFL, 0) | O_NONBLOCK));
    perror("fcntl_ret");

    epoll_ctl(epollfd, EPOLL_CTL_ADD, sockfd, &ev);
    return;
}

/*Conecta a un nuevo cliente.*/
void new_client(struct args_epoll_monitor* e_m_struct, struct epoll_event* evlist, int mode){

    struct data_ptr* ptr;
    ptr = CAST_DATA_PTR;
    
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
            epoll_ctl(e_m_struct->epollfd, EPOLL_CTL_MOD, ptr->fd, &ev);
            return;
        }
        else{
            perror("Error al aceptar nuevo cliente\n");
            exit(1);
        } 
    }

    epoll_ctl(e_m_struct->epollfd, EPOLL_CTL_MOD, ptr->fd, &ev);
    epoll_add(client_sockfd, e_m_struct->epollfd, ptr->text_or_binary);

    
    
    return;
}

void read_length(struct args_epoll_monitor* e_m_struct, struct epoll_event* evlist){

    int resp = 0;
    int tot_read = 0;
    int pos = 0;

    struct data_ptr* ptr;
    ptr = CAST_DATA_PTR;
    
    struct data_ptr_binary* ptr_bin;
    ptr_bin = CAST_DATA_PTR_BINARY;


    pos = 5 - ptr_bin->binary_to_read_commands;

    resp = recv(ptr->fd, (ptr_bin->commands + pos), ptr_bin->binary_to_read_commands, 0);
    perror("error_recv_read_length");
    if  (resp == 0 || 
        (resp == -1 && 
        (errno != EWOULDBLOCK 
        && errno != EAGAIN))){
        quit_epoll(e_m_struct, evlist);
        return;
    }
    
    if (resp != -1)
        tot_read = ptr_bin->binary_to_read_commands - resp;
    else
        tot_read = ptr_bin->binary_to_read_commands;
    //printf("tot_read: %d\n", tot_read);
    
    if (tot_read > 0){ /*Todavia no llegó toda la longitud de la clave.*/
        ptr_bin->binary_to_read_commands = tot_read;

        if (resp != -1)
            ptr_bin->comandos_leidos = resp;
        
        return;
    }

    if (tot_read == 0 && ptr_bin->key == NULL){
        ptr_bin->length_key = ntohl(*(int*)(ptr_bin->commands + 1));
        //length_binary(CAST_DATA_PTR_BINARY->commands + 1, &CAST_DATA_PTR_BINARY->length_key);
        ptr_bin->key = malloc(ptr_bin->length_key + 1);
        assert(ptr_bin->key != NULL);
        ptr_bin->to_consumed = ptr_bin->length_key;
        ptr_bin->comandos_leidos = 0;
        return;
    }

    if (tot_read == 0 && ptr_bin->dato == NULL){
        ptr_bin->length_dato = ntohl(*(int*)(ptr_bin->commands + 1));
        ptr_bin->dato = malloc(ptr_bin->length_dato + 1);
        assert(ptr_bin->dato != NULL);
        ptr_bin->to_consumed = ptr_bin->length_dato;
        ptr_bin->comandos_leidos = 0;

        return;
    }
}

int writen(int fd, const void *buf, int len)
{
	int i = 0, rc;

	while (i < len) {

		int chunk = len - i;
		rc = write(fd, buf + i, chunk);
		if (rc <= 0)
			return -1;
		i += rc;
    }
    return 0;
}

int my_length(char* buf){
    int i = 0;
    while(buf[i] != '\0'){
        if (buf[i] < 32 || buf[i] > 126)
            return -1;
        i++;
    }
    return i;
}

void read_content(struct args_epoll_monitor* e_m_struct, struct epoll_event* evlist, int data_or_key){

    int pos = 0;
    int resp = 0;
    int tot_read = 0;
    void* content;

    struct data_ptr* ptr;
    ptr = CAST_DATA_PTR;
    
    struct data_ptr_binary* ptr_bin;
    ptr_bin = CAST_DATA_PTR_BINARY;

    printf("Data or Ky : %d\n", ptr_bin->data_or_key);
    //printf("to_consume: %d\n", CAST_DATA_PTR_BINARY->to_consumed);
    if(!data_or_key){
        content = ptr_bin->key;
        pos = ptr_bin->length_key - ptr_bin->to_consumed;
    }
    else{
        printf("length dato: %d\n", ptr_bin->length_dato);
        content = ptr_bin->dato;
        pos = ptr_bin->length_dato - ptr_bin->to_consumed;
    }
    
    resp = recv(ptr->fd, content + pos, ptr_bin->to_consumed, 0);
    perror("error_recv");
    if  (resp == 0 || 
        (resp == -1 && 
        (errno != EWOULDBLOCK 
        && errno != EAGAIN))){
            printf("resp : %d\n", resp);
            puts("HOLA\n");
        quit_epoll(e_m_struct, evlist);
        return;
    }
    
    if (resp != -1)
        tot_read = ptr_bin->to_consumed - resp;
    else 
        tot_read = ptr_bin->to_consumed;
    printf("tots_read: %d\n", tot_read);
    if (tot_read > 0){ 
        ptr_bin->to_consumed = tot_read;
        return;
    }
    if (tot_read == 0){
        ptr_bin->to_consumed = 0; /*Ya terminó de consumir todo*/
    }
    return;

}

/*Consume el binario del fd de un cliente.*/
void binary_consume(struct args_epoll_monitor* e_m_struct, struct epoll_event* evlist){
    
    struct data_ptr* ptr;
    ptr = CAST_DATA_PTR;
    
    struct data_ptr_binary* ptr_bin;
    ptr_bin = CAST_DATA_PTR_BINARY;

    int resp = 0;
    int mng = -1; 
    /*En la primera vez consume el primer comando (PUT/GET/DEL/...)*/
    if (ptr_bin->binary_to_read_commands == 5){
        //printf("Leo de fd: %d\n", CAST_DATA_PTR->fd);
        resp = recv(ptr->fd, ptr_bin->commands, 1, 0); 

        if  (resp == 0 || 
        (resp == -1 && 
        (errno != EWOULDBLOCK 
        && errno != EAGAIN))){
        quit_epoll(e_m_struct, evlist);
        return;
        }

        ptr_bin->binary_to_read_commands -= 1; /*Comandos a leer, ya leí el primero*/

    }
    /*Si no es ningun comando conocido o que requiera leer valores (STATS) directamente lo maneja*/
    if ((ptr_bin->commands[0] != 11)
        && (ptr_bin->commands[0] != 12)
        && (ptr_bin->commands[0] != 13)){
            manage_client_binary(e_m_struct, evlist);
            return;
        }

    /*Llama a la funcion que lee las longitudes y calcula cuando mide la clave.*/
    printf("to read commands after primer comando: %d\n", ptr_bin->binary_to_read_commands);
    if(ptr_bin->key == NULL)
        read_length(e_m_struct, evlist);

    //printf("Command = %d\n", (int)CAST_DATA_PTR_BINARY->commands[0]);
    //printf("Command = %d\n", (int)CAST_DATA_PTR_BINARY->commands[1]);
    //printf("Command = %d\n", (int)CAST_DATA_PTR_BINARY->commands[2]);
    //printf("Command = %d\n", (int)CAST_DATA_PTR_BINARY->commands[3]);
    //printf("Command = %d\n", (int)CAST_DATA_PTR_BINARY->commands[4]);

    /*Si la clave es NULL, significa que todavia no terminó de leer las longitudes, lo meto al epoll y espero a que mande mas longitudes.*/
    if (ptr_bin->key == NULL){
        printf("vuelve a epoll, me falta clave\n");
        printf("to read commands: %d\n", ptr_bin->binary_to_read_commands);
        return;
    }

    /*Si el dato es NULL deberia leer la clave*/
    if (ptr_bin->data_or_key == 0)
        read_content(e_m_struct, evlist, ptr_bin->data_or_key);

    /*Todavía no terminó de leer el contenido*/
    if (ptr_bin->to_consumed != 0 && ptr_bin->data_or_key == 0){
        printf("me falta contenido\n");
        return;
    }

    /*Si es un put tengo que hacer otra lectura*/
    if ((int)ptr_bin->commands[0] == 11){
        ptr_bin->binary_to_read_commands = 4 - ptr_bin->comandos_leidos;
        ptr_bin->data_or_key = 1;

        if (ptr_bin->dato == NULL)
            read_length(e_m_struct, evlist);
        
        if (ptr_bin->dato == NULL){
            return;
        }

        read_content(e_m_struct, evlist, ptr_bin->data_or_key);

        /*Todavía no terminó de leer el contenido*/
        if (ptr_bin->to_consumed != 0){
            return;
        }
        
    }
    /*En teoria aca ya tengo la key completa y el dato, en caso que haya sido un put*/
    
    mng = manage_client_binary(e_m_struct, evlist);
    
    if(mng == 0)
        restart_binary(evlist);

    return;
}

void restart_binary(struct epoll_event* evlist){
    
    struct data_ptr_binary* ptr_bin;
    ptr_bin = CAST_DATA_PTR_BINARY;

    free(ptr_bin->key);
    free(ptr_bin->dato);
    ptr_bin->key = NULL;
    ptr_bin->dato = NULL;
    ptr_bin->data_or_key = 0;
    ptr_bin->binary_to_read_commands = 5;
    ptr_bin->comandos_leidos = 0;
    ptr_bin->length_dato = 0;
    ptr_bin->length_key = 0;
    ptr_bin->to_consumed = 0;
    return;

}

void length_binary(unsigned char* commands, int* length){

    for (int i = 0; i < 4; i++){
        int temp = (int)commands[i];
        *length = *length + temp * pow(256, 3 - i);
        printf("temp en %d = %d\n",i,temp);
    }
    return;    
}

int manage_client_binary(struct args_epoll_monitor* e_m_struct, struct epoll_event* evlist){

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

    struct data_ptr* ptr;
    ptr = CAST_DATA_PTR;
    
    struct data_ptr_binary* ptr_bin;
    ptr_bin = CAST_DATA_PTR_BINARY;

    /*Checkeo si es PUT*/
    if ((int)ptr_bin->commands[0] == 11){
        command = 0;
        res = memc_put(e_m_struct->mem, 
                        ptr_bin->key,
                        ptr_bin->dato,
                        ptr_bin->length_key,
                        ptr_bin->length_dato,
                        ptr->text_or_binary);
        
        snd = writen(ptr->fd, &ok, 1);
        perror("snd_put_binary");
        if (snd != 0){
            quit_epoll(e_m_struct, evlist);
            return -1;
        }
            
        return 0;
    }
    /*Checkeo si es GET*/
    if ((int)ptr_bin->commands[0] == 13){
        command = 1;
        res = memc_get(e_m_struct->mem,
                        ptr_bin->key,
                        ptr_bin->length_key,
                        (void**)&buf,
                        ptr->text_or_binary);
        if (!res){

            snd = writen(ptr->fd, &enotfound, 1);
            perror("error_send");
            if (snd != 0){
                quit_epoll(e_m_struct, evlist);
                return -1;
            }
        }
        else{
            
            int_to_binary(res, (void*)&len);
            
            snd = writen(ptr->fd, &ok, 1);
            if (snd != 0){
                quit_epoll(e_m_struct, evlist);
                free(buf);
                return -1;
            }

            snd = writen(ptr->fd, len, 4);
            if (snd != 0){
                quit_epoll(e_m_struct, evlist);
                free(buf);
                return -1;
            }
            
            snd = writen(ptr->fd, buf, res);
            if (snd != 0){
                quit_epoll(e_m_struct, evlist);
                free(buf);
                return -1;
            }
        }
        return 0;
    }

    /*Checkeo si es DEL*/
    if ((int)ptr_bin->commands[0] == 12){
        command = 2;
        res = memc_del(e_m_struct->mem,
                        ptr_bin->key,
                        ptr_bin->length_key,
                        ptr->text_or_binary);

        if (res == -1){
            snd = writen(ptr->fd, &enotfound, 1);
            perror("error_send");
            if (snd != 0){
                quit_epoll(e_m_struct, evlist);
                return -1;
            }
        }
        else{
            snd = writen(ptr->fd, &ok, 1);
            perror("error_send");
            if (snd != 0){
                quit_epoll(e_m_struct, evlist);
                return -1;
            }
        }
        return 0;
    }
    /*Checkeo si es STATS*/
    if ((int)ptr_bin->commands[0] == 21){
        command = 3;
        stats = memc_stats(e_m_struct->mem);
        len_stat_buf = sprintf(stat_buf,"PUTS=%lu DELS=%lu GETS= %lu KEYS=%lu\n", stats->puts, stats->dels, stats->gets, stats->keys);

        int_to_binary(len_stat_buf, (void*)&len);
        
        snd = writen(ptr->fd, &ok, 1);
        perror("error_send");
        if (snd != 0){
            quit_epoll(e_m_struct, evlist);
            return -1;
        }
        
        snd = writen(ptr->fd, &len, 4);
        perror("error_send");
        if (snd != 0){
            quit_epoll(e_m_struct, evlist);
            return -1;
        }
        
        
        snd = writen(ptr->fd, &stat_buf, strlen(stat_buf));
        perror("error_send");
        if (snd != 0){
            quit_epoll(e_m_struct, evlist);
            return -1;
        }

    }
    /*Comando erróneo*/
    if (command == -1){

        snd = writen(ptr->fd, &einval, 1);
        perror("error_send");
        if (snd != 0){
            quit_epoll(e_m_struct, evlist);
            return -1;
        }

    }
    return 0;
}

/*Consume el texto del fd de un cliente.*/
void text_consume(struct args_epoll_monitor* e_m_struct, struct epoll_event* evlist){
    /*Usar la funcion recv para leer del buffer de lectura. Usar ntohl para pasar de network-byte-order (big endian) a host-byte-order <- binario*/ 
    /*Funcion readline() lee hasta un '\n' <- posible*/
    /*Falta que termine de llegar un comando*/    

    /*Buffer temporal para procesar el comando.*/
    char temp[MAX_CHAR + 1];

    struct data_ptr* ptr;
    ptr = CAST_DATA_PTR;
    
    char err[6] = "EBIG\n";
    ssize_t fst_read = 0, err_read = 0;
    int valid = 0; /*Bandera para indicar si el pedido es válido (<= 2048 caracteres)*/
    int cant_commands = N_COMMANDS;
    int snd = 0;
    int rec = 0;

    printf("Leo de fd: %d\n", ptr->fd); 

    /*Checkeamos si no hay lugares de \n ya leidos, de esta manera si es que hay el hilo simplemente puede leer lo que necesita*/
    if (ptr->cant_comm_ptr == 0){

        /*Primer recv para ubicar la posicion del \n*/
        fst_read = recv(ptr->fd, temp, MAX_CHAR, MSG_PEEK); 
        perror("error_read");
    
        if (fst_read <= 0){
            if(fst_read != -1 || (errno != EWOULDBLOCK && errno != EAGAIN))            
                quit_epoll(e_m_struct, evlist);
            return;
        }
        printf("fst_read: %ld\n", fst_read);
        /*Ubicamos terminador de cadena.*/
        temp[fst_read] = '\0';
            
        /*Recorro lo leido para guardar los \n*/
        /*Si entró aca significa que no hay \n guardados, por lo que reiniciamos la pos del array.*/
        ptr->actual_pos_arr = 0;
        for (int i = 0; i < fst_read; i++){
            if(temp[i] == '\n'){
                printf("Lo encontré i= %d\n", i);
                valid = 1;
                ptr->cant_comm_ptr += 1;
                /*Posiblemente nunca deba ejecutarse pero por las dudas.*/
                if(ptr->cant_comm_ptr == cant_commands){ 
                    cant_commands *= 2;
                    ptr->delimit_pos = realloc(ptr->delimit_pos, cant_commands * 4);
                }
                /*Guardo n° de byte del \n*/
                ptr->delimit_pos[ptr->actual_pos_arr] = i + 1; 
                ptr->actual_pos_arr += 1;
            }
        }
        /*Reiniciamos la posicion del array*/
        ptr->actual_pos_arr = 0; 
        
        if ((!valid && fst_read == MAX_CHAR) || 
            ptr->readed >= 2048){ /*No hay ningun \n en 2048 bytes. Debo leer más para encontrarlo y descartar ese comando*/
            puts("Lei error");
            
            err_read = recv(ptr->fd, temp, fst_read, MSG_WAITALL);
            if (err_read <= 0){
                quit_epoll(e_m_struct, evlist);
                return;
            }
            snd = writen(ptr->fd, &err, strlen(err));
            perror("error_send");
            if (snd != 0){
                quit_epoll(e_m_struct, evlist);
                return;
            }

            ptr->readed = 0;
            return;
        }

        if(!valid){ /*No leyó ningun \n, debo esperar a que llegue el resto del pedido.*/

            if (ptr->readed + fst_read >= MAX_CHAR){
                char err_buf[fst_read];
                fst_read = recv(ptr->fd, err_buf, fst_read, MSG_WAITALL); 
                if (fst_read <= 0){
                    quit_epoll(e_m_struct, evlist);
                    return;
                }
                
                snd = writen(ptr->fd, &err, strlen(err));
                perror("error_send");
                if (snd != 0){
                    quit_epoll(e_m_struct, evlist);
                    return;
                }
                    
                ptr->readed = 0;
                return;
            }
            fst_read = recv(ptr->fd, ptr->command + ptr->readed, fst_read, MSG_WAITALL); /*Lo consumo*/
            perror("error_recv");
            if (fst_read <= 0){
                if(fst_read != -1 || (errno != EWOULDBLOCK && errno != EAGAIN))            
                    quit_epoll(e_m_struct, evlist);
                return;
            }

            ptr->readed += fst_read; /*Si ya pasaron 2048 sin \n lo vuelo*/
            
            if (ptr->readed < MAX_CHAR)
                ptr->command[ptr->readed] = '\0';
            else{
                snd = writen(ptr->fd, &err, strlen(err));
                perror("error_send");
                if (snd != 0){
                    quit_epoll(e_m_struct, evlist);
                    return;
                }
                
                ptr->readed = 0;
                return;
            }

            ptr->missing = 1;
            printf("Command: %s\n", ptr->command);
            return;
        }
    }
    
    /*Los hilos que ya encuentren posiciones de \n solo haran esta parte. Los que no, deberán hacer todo lo anterior.*/
    /*Reiniciamos la posicion actual del array*/
    //CAST_DATA_PTR->actual_pos_arr = 0; 
    int d = 0;
    int s = ptr->delimit_pos[ptr->actual_pos_arr];
    int read_comm;
    int cant_comm;
    char** token_commands; /*Liberar esto*/
    char einval[8] = "EINVAL\n";
    
    if (ptr->actual_pos_arr > 0)
        d = ptr->delimit_pos[ptr->actual_pos_arr - 1];
    
    s = s - d;
    if (ptr->missing){ //Lo tengo que unir
        rec = recv(ptr->fd, ptr->command + ptr->readed, s, MSG_WAITALL);
        perror("error_recv");
        if (rec <= 0){
            if(rec != -1 || (errno != EWOULDBLOCK && errno != EAGAIN))            
                quit_epoll(e_m_struct, evlist);
            return;
        }
        
        printf("command pre token: %s\n", ptr->command);
        ptr->command[ptr->readed + rec] = '\0';
        token_commands = text_parser(ptr->command, &cant_comm, DELIMITER);
    }
    else{
        char* comm = malloc(s + 1);
        assert(comm != NULL);

        read_comm = recv(ptr->fd, comm, s, MSG_WAITALL);
        perror("error_recv");
        if (read_comm <= 0){
            if(read_comm != -1 || (errno != EWOULDBLOCK && errno != EAGAIN))            
                quit_epoll(e_m_struct, evlist);
            free(comm);
            return;
        }

        comm[s] = '\0'; /*Le agrego el terminador de cadena*/
        token_commands = text_parser(comm, &cant_comm, DELIMITER);
    }

    if (token_commands != NULL){
        manage_client(e_m_struct, evlist, token_commands, cant_comm);
        if (!ptr->missing)
            free(token_commands[0]);
        free(token_commands);
    }
   else{
        snd = writen(ptr->fd, &einval, strlen(einval));
        perror("error_send");
        if (snd != 0){
            quit_epoll(e_m_struct, evlist);
            return;
        }
    }
    
    ptr->actual_pos_arr += 1;
    ptr->cant_comm_ptr -= 1; 
    ptr->missing = 0;
    ptr->readed = 0;
    return; 

}

void quit_epoll(struct args_epoll_monitor* e_m_struct, struct epoll_event* evlist){

    struct data_ptr* ptr;
    ptr = CAST_DATA_PTR;

    struct data_ptr_binary* ptr_bin;
    ptr_bin = CAST_DATA_PTR_BINARY;

    printf("Lo saque a: %d\n", ptr->fd);
    epoll_ctl(e_m_struct->epollfd, EPOLL_CTL_DEL, ptr->fd, evlist); /*En lugar de null puede ser e_m_struct->evlist para portabilidad pero ver bien*/
    
    if(!ptr->text_or_binary){
        close(ptr->fd);
        free(ptr->command);
        free(ptr->delimit_pos);
    }
    else{
        close(ptr->fd);
        free(ptr_bin->commands);
        free(ptr_bin->dato);
        free(ptr_bin->key);
        free(ptr_bin);
    }
    free(ptr);
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

    struct data_ptr* ptr;
    ptr = CAST_DATA_PTR;

    printf("cant_comm: %d\n", cant_comm);
    if (cant_comm == 0)
        return;
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
        
        //for (int i = 0; i <  cant_comm; i++)
        //    printf("token_command[%d]: %s\n", i, token_comands[i]);

        snd = writen(ptr->fd, einval, strlen(einval));
        perror("error_send");
        if (snd != 0)
            quit_epoll(e_m_struct, evlist);
            
        return;
    }

    if (command == 0){
        
        //printf("Pongo clave: %s, valor: %s\n Longitudes clave: %ld, valor: %ld\n", token_comands[1], token_comands[2], strlen(token_comands[1]),strlen(token_comands[2]));
        int len_tok_1 = my_length(token_comands[1]);
        int len_tok_2 = my_length(token_comands[2]);

        //printf("Longitud token 1: %ld\n", strlen(token_comands[1]));
        //printf("len_tok_1: %d\n", len_tok_1);

        if (len_tok_1 == -1 || len_tok_2 == -2){
            snd = writen(ptr->fd, einval, strlen(einval));
            if (snd != 0)
                quit_epoll(e_m_struct, evlist);
            return;    
        }

        res = memc_put  (e_m_struct->mem, 
                        token_comands[1], 
                        token_comands[2], 
                        len_tok_1,
                        len_tok_2,
                        ptr->text_or_binary); /*TEXTO*/

        snd = writen(ptr->fd, ok, strlen(ok));
        if (snd != 0)
            quit_epoll(e_m_struct, evlist);
        //snd = send(CAST_DATA_PTR->fd, ok, strlen(ok), MSG_NOSIGNAL);
        //perror("error_send");
        //if (snd == 0 || snd == -1){
        //    quit_epoll(e_m_struct, evlist);
        //    return;
        //}
    }
    if (command == 1){
        int len_tok_1 = my_length(token_comands[1]);
        
        if (len_tok_1 == -1){
            snd = writen(ptr->fd, einval, strlen(einval));
            if (snd != 0)
                quit_epoll(e_m_struct, evlist);
            return;    
        }

        res = memc_del  (e_m_struct->mem,
                        token_comands[1],
                        len_tok_1,
                        ptr->text_or_binary);
        
        if (res == 0)
            snd = writen(ptr->fd, ok, strlen(ok));
        else{
            snd = writen(ptr->fd, einval, strlen(einval));
        }

        if (snd != 0){
            quit_epoll(e_m_struct, evlist);
            return;
        }

    }
    if (command == 2){

        int len_tok_1 = my_length(token_comands[1]);
        
        if (len_tok_1 == -1){
            snd = writen(ptr->fd, einval, strlen(einval));
            if (snd != 0)
                quit_epoll(e_m_struct, evlist);
            return;    
        }

        //printf("Busco: %s, longitud: %ld\n", token_comands[1], strlen(token_comands[1]));
        res = memc_get  (e_m_struct->mem,
                        token_comands[1], 
                        len_tok_1, 
                        (void**)&get_value, 
                        ptr->text_or_binary);

        if (res == -1)
            snd = writen(ptr->fd, ebinary, strlen(ebinary));
        if (res == 0)
            snd = writen(ptr->fd, enotfound, strlen(enotfound));
        if (res > 0){
            char resp[res + 5];
            sprintf(resp, "OK %s\n", get_value);
            if (strlen(ok_get) <= 2048)
                snd = writen(ptr->fd, resp, strlen(resp));
            else    
                snd = writen(ptr->fd, ebig, strlen(ebig));
        }

        perror("error_send");
        if (snd != 0)
            quit_epoll(e_m_struct, evlist);

        if (res > 0)
            free(get_value);
    
    }
    if (command == 3){

        stats = memc_stats(e_m_struct->mem);
        len_stat_buf = sprintf(stat_buf,"OK PUTS=%lu DELS=%lu GETS= %lu KEYS=%lu\n", stats->puts, stats->dels, stats->gets, stats->keys);
        assert(len_stat_buf > 0);
        snd = writen(ptr->fd, stat_buf, len_stat_buf);
        perror("error_send");
        if (snd != 0)
            quit_epoll(e_m_struct, evlist);
    }
    return;
}

/*Le da un fd listo a cada thread*/
void* epoll_monitor(void* args){
    

    int a = 0;
    struct args_epoll_monitor* e_m_struct;
    e_m_struct = (struct args_epoll_monitor*) args;
    
    /*Inicializa la lista de los fd listos*/
    
    struct epoll_event* evlist = malloc(sizeof(struct epoll_event) * MAX_EVENTS); //Liberar esto al final
    assert(evlist != NULL);
    
    //e_m_struct->evlist = evlist;
    //printf("Entro al while soy %ld\n", pthread_self());
    while(1){

        /*Retorna los fd listos para intercambio de datos*/
        /*La idea es que los hilos checkeen si hay disponibles fd, si hay un hilo irá a 
            atender a ese cliente y las siguientes llamadas a epoll_wait no avisaran de este fd.
            La bandera EPOLLONESHOT ayuda a esto haciendo que no aparezca en la lista de ready 
            devuelta por epoll_wait en el caso de tener nuevo input que ya está siendo 
            atendida por un hilo. Ese hilo debe volver a activar las notificaciones de
            ese cliente. El hilo responde una consulta sola y lo vuelve a meter al epoll asi puede ir a atender a mas hilos*/
        do{
            a = epoll_wait(e_m_struct->epollfd, evlist, MAX_EVENTS, -1);
        }while(a < 0 && errno == EINTR);
        //perror("epoll_wait");
        //assert(a != -1);
        struct data_ptr* ptr;
        ptr = CAST_DATA_PTR;

        printf("Atiendo a fd: %d, soy hilo: %ld\n", ptr->fd, pthread_self());
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
            if ((ptr->fd == e_m_struct->sockfd_text) ||
                (ptr->fd == e_m_struct->sockfd_binary)){
                
                printf("Hay que aceptar a alguien.\n");
                /*Aceptamos al nuevo cliente*/
                new_client(e_m_struct, evlist, ptr->text_or_binary);
                /*Lo añadimos a la instancia de epoll para monitorearlo.*/
            }
            else{

                if (!ptr->text_or_binary){

                    printf("Es un cliente de pedidos\n");
                    /*Este cliente no es nuevo por lo que nos hará consultas.*/
                    /*Hacer un if para diferenciar entre cliente de texto y binario mediante el data.u32*/  

                    text_consume(e_m_struct, evlist);

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

    size_t byte_limit = 300000000;
    limit_mem(byte_limit);

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

    //int test = 5283612;
    //unsigned char len[4];
    //printf("%p\n", len);
    //int_to_binary(test, (void*)len);
    //for (int i = 0; i < 4; i++){
    //    int temp = (int)len[i];
    //    printf("temp: %d\n", temp);
    //}

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

    signal(SIGPIPE, SIG_IGN);
    
    for (int i = 0; i < 2; i++){
        pthread_create(&t[i], NULL, epoll_monitor, &args);
    }
    for(int i = 0; i < 2; i++)
        pthread_join(t[i], NULL);

    /*Capturar señal SIGPIPE*/

    
    return 0;
}