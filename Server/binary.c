#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <math.h>
#include <sys/signal.h>
#include "manage_clients.h"
#include "comunicate.h"
#include "binary.h"
#include "epoll.h"

#define CAST_DATA_PTR ((struct data_ptr*)evlist->data.ptr)
#define CAST_DATA_PTR_BINARY CAST_DATA_PTR->binary

int binary_consume(struct args_epoll_monitor* e_m_struct, struct epoll_event* evlist){
    
    struct data_ptr_binary* ptr_bin;
    ptr_bin = CAST_DATA_PTR_BINARY;
    
    int resp = 0;
    int mng = -1;
    int bye = 0; 

    /*En la primera vez consume el primer comando (PUT/GET/DEL/...)*/
    if (ptr_bin->binary_to_read_commands == 5){

        resp = recv(CAST_DATA_PTR->fd, ptr_bin->commands, 1,0);        
        if (resp <= 0){
            if (resp == -1){
                quit_epoll(e_m_struct, evlist);
                return -1; //No volverlo a meter al epoll
            }
            return 0; //Volverlo a meter al epoll
        }

        ptr_bin->binary_to_read_commands -= 1; /*Comandos a leer, ya leí el primero*/

    }

    /*Si no es ningun comando conocido o que requiera leer valores (STATS) directamente lo maneja*/
    if ((ptr_bin->commands[0] != 11)
        && (ptr_bin->commands[0] != 12)
        && (ptr_bin->commands[0] != 13)){
            bye = manage_bin_client(e_m_struct, evlist);
            return bye;
        }

    /*Llama a la funcion que lee las longitudes y calcula cuando mide la clave.*/
    if(ptr_bin->key == NULL)
        bye = read_length(e_m_struct, evlist);
    
    if (bye == -1)
        return -1;

    /*Si la clave es NULL, significa que todavia no terminó de leer las longitudes, lo meto al epoll y espero a que mande mas longitudes.*/
    if (ptr_bin->key == NULL)
        return 0;

    /*Si el dato es NULL deberia leer la clave*/
    if (ptr_bin->data_or_key == 0)
        bye = read_content(e_m_struct, evlist, ptr_bin->data_or_key);
    
    /*Caso que se el recv haya tenido algún error*/
    if (bye == -1)
        return -1;
    
    /*Todavía no terminó de leer el contenido*/
    if (ptr_bin->to_consumed != 0 && ptr_bin->data_or_key == 0)
        return 0;

    /*Si es un put tengo que hacer otra lectura*/
    if ((int)ptr_bin->commands[0] == 11){
        
        /*Debo leer la longitud del dato ahora*/
        ptr_bin->binary_to_read_commands = 4 - ptr_bin->comandos_leidos;
        ptr_bin->data_or_key = 1;

        /*Leo longitud del dato*/
        if (ptr_bin->dato == NULL)
            bye = read_length(e_m_struct, evlist);
        
        if (bye == -1)
            return -1;

        /*En este caso el dato todavia no llegó completo por lo que lo meto al epoll hasta que llegue completo*/
        if (ptr_bin->dato == NULL){
            return 0;
        }

        /*Una vez adquirida la longitud, debo leer el contenido*/
        bye = read_content(e_m_struct, evlist, ptr_bin->data_or_key);
        
        
        if (bye == -1)
            return -1;

        /*Todavía no terminó de leer el contenido*/
        if (ptr_bin->to_consumed != 0){
            return 0;
        }
        
    }

    /*En teoria aca ya tengo la key completa y el dato, en caso que haya sido un put*/
    
    mng = manage_bin_client(e_m_struct, evlist);
    
    if(mng == 0)
        restart_binary(evlist);

    return mng;
}

int read_length(struct args_epoll_monitor* e_m_struct, struct epoll_event* evlist){

    int resp = 0;
    int tot_read = 0;
    int pos = 0;
    
    struct data_ptr_binary* ptr_bin;
    ptr_bin = CAST_DATA_PTR_BINARY;


    pos = 5 - ptr_bin->binary_to_read_commands;

    resp = recv(CAST_DATA_PTR->fd, (ptr_bin->commands + pos), ptr_bin->binary_to_read_commands, 0); 
        if (resp <= 0){
            if (resp == -1){
                quit_epoll(e_m_struct, evlist);
                return -1; //No volverlo a meter al epoll
            }
            return 0; //Volverlo a meter al epoll
    }
    
    /*Actualizo la cantidad de comandos leídos*/
    if (resp != -1)
        tot_read = ptr_bin->binary_to_read_commands - resp;
    else
        tot_read = ptr_bin->binary_to_read_commands;
    
    if (tot_read > 0){ /*Todavia no llegó toda la longitud de la clave.*/
        ptr_bin->binary_to_read_commands = tot_read;

        if (resp != -1)
            ptr_bin->comandos_leidos += resp;
        
        return 0;
    }
    
    /*Si la clave es null, significa que estoy leyendo la longitud de la clave.*/
    if (tot_read == 0 && ptr_bin->key == NULL){
        ptr_bin->length_key = ntohl(*(int*)(ptr_bin->commands + 1));
        ptr_bin->key = memc_alloc(e_m_struct->mem, ptr_bin->length_key + 1, MALLOC, NULL);
        ptr_bin->to_consumed = ptr_bin->length_key;
        ptr_bin->comandos_leidos = 0;
        return 0;
    }
    
    /*Si el dato es null significa que estoy leyendo la longitud del dato*/
    if (tot_read == 0 && ptr_bin->dato == NULL){
        ptr_bin->length_dato = ntohl(*(int*)(ptr_bin->commands + 1));
        ptr_bin->dato = memc_alloc(e_m_struct->mem, ptr_bin->length_dato + 1, MALLOC, NULL);
        ptr_bin->to_consumed = ptr_bin->length_dato;
        ptr_bin->comandos_leidos = 0;
        return 0;
    }

    return 0;
}

int read_content(struct args_epoll_monitor* e_m_struct, struct epoll_event* evlist, int data_or_key){

    int pos = 0;
    int resp = 0;
    int tot_read = 0;
    void* content;
    
    struct data_ptr_binary* ptr_bin;
    ptr_bin = CAST_DATA_PTR_BINARY;

    /*Checkeo si estoy leyendo la clave o el dato*/
    if(!data_or_key){
        content = ptr_bin->key;
        pos = ptr_bin->length_key - ptr_bin->to_consumed;
    }
    else{
        content = ptr_bin->dato;
        pos = ptr_bin->length_dato - ptr_bin->to_consumed;
    }
    
    resp = recv(CAST_DATA_PTR->fd, content + pos, ptr_bin->to_consumed, 0);
    if (resp <= 0){
        if (resp == -1){
            quit_epoll(e_m_struct, evlist);
            return -1; //No volverlo a meter al epoll
        }
        return 0; //Volverlo a meter al epoll
    }

   
    /*Actualizo lo que me falta por consumir, que es lo que calculé previamente*/
    if (resp != -1)
        tot_read = ptr_bin->to_consumed - resp;
    else 
        tot_read = ptr_bin->to_consumed;
    
    if (tot_read > 0){ 
        ptr_bin->to_consumed = tot_read;
        return 0;
    }
    if (tot_read == 0){
        ptr_bin->to_consumed = 0; /*Ya terminó de consumir todo*/
    }
    return 0;

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

