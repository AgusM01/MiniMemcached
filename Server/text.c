#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <math.h>
#include "text.h"
#include "../Structures/utils.h"
#include "manage_clients.h"
#include "comunicate.h"
#include "binary.h"

#define MAX_CHAR 2048
#define N_COMMANDS 10
#define CAST_DATA_PTR ((struct data_ptr*)evlist->data.ptr)
#define CAST_DATA_PTR_BINARY CAST_DATA_PTR->binary
#define DELIMITER " \n"

char** text_parser(char* text, int* cant_comm, char* delimiter, memc_t mem){

    char** res = memc_alloc(mem, sizeof(char*)*4, MALLOC, NULL);
    char* tokbuf[1] ;
    int i = 0;
    /*Reinicio la cantidad de comandos*/     
    *cant_comm = 0;
    tokbuf[0] = NULL;
    
    res[i] = strtok_r(text, delimiter, tokbuf);
    while(res[i] != NULL){
        *cant_comm += 1;
        i++;
        if (i > 3){
            free(res);
            return NULL;
        }
        res[i] = strtok_r(NULL, delimiter, tokbuf);
    }

    return res;
}

int text_consume(struct args_epoll_monitor* e_m_struct, struct epoll_event* evlist){

    int rv = 0;
    int snd = 0;
    char err[6] = "EBIG\n";
    char einval[8] = "EINVAL\n";
    char comm[MAX_CHAR + 1];
    int cant_comm;
    int bye;

    struct data_ptr* ptr;
    ptr = CAST_DATA_PTR;

    /*La idea es: leer 2048, responder todos los pedidos, si hay alguno cortado, meterlo al epoll y esperar que llegue completo.*/

    /*Recibo lo que me mandó*/
    rv = recv(ptr->fd, ptr->command, MAX_CHAR, 0);
    if (rv <= 0){
        if (rv == -1){
            quit_epoll(e_m_struct, evlist);
            return -1; //No volverlo a meter al epoll
        }
        return 0; //Volverlo a meter al epoll
    }

    /*Debo ir leyendo los comandos y respondiendolos*/
    while (rv > 0){

        /*Busco los \n ya que de esa forma encontraré los pedidos*/
        for (int i = 0; i < rv; i++){

                if (ptr->command[ptr->actual_pos_arr + i] == '\n'){
                    ptr->actual_pos_arr += (i  + 1); /*Actualizo la posicion de donde termina el comando*/
                    ptr->is_command = 1; /*Seteo la bandera para indicar que es un comando*/
                    i = rv; /*Corto el for*/
                }
        }

        /*Leí 2048 sin \n*/
        /*Sea un comando cortado, o no, es erróneo.*/
        if (ptr->is_command == 0 && 
            rv >= MAX_CHAR){
            ptr->missing = 0;
            rv = -1;
            snd = writen(ptr->fd, &err, strlen(err));
            if (snd == -1)
                perror("error_send");
            if (snd != 0){
                quit_epoll(e_m_struct, evlist);
                return -1;
            }
            return 0;
        }

        /*No leí \n, falta que llegue el comando entero*/
        /*En este caso lo guardo en el buffer iterno que guarda el comando cortado.*/
        if (ptr->is_command == 0){
            
            /*El comando ya es demasiado largo*/
            if (ptr->pos_to_complete + rv >= MAX_CHAR){
                
                ptr->missing = 0;
                ptr->is_command = 0;
                ptr->pos_to_complete = 0;
                ptr->prev_pos_arr = 0;
                rv = -1;
                
                snd = writen(ptr->fd, &err, strlen(err));
                if (snd == -1)
                    perror("error_send");
                if (snd != 0){
                    quit_epoll(e_m_struct, evlist);
                    return -1;
                }
            }
            else{
                /*Es correcto por lo que debo almacenarlo en el buffer hasta que llegue el resto del comando*/
                int i = 0;
                for (; i < rv; i++)
                    ptr->to_complete[ptr->pos_to_complete + i] = ptr->command[ptr->actual_pos_arr + i];
    
                ptr->to_complete[ptr->pos_to_complete + i] = '\0';
                ptr->pos_to_complete += i;
                ptr->missing = 1;
                rv = -1; /*Corto el while*/
            }
        }

        if (ptr->is_command == 1){
            int cut = 1;
            /*Lo que leí forma parte de uno que vino antes partido*/
            /*Entonces debo unir las partes del comando y mandarlo al parser.*/
            if (ptr->missing == 1){

                int len = strlen(ptr->to_complete);
                /*El comando total es demasiado largo*/
                
                if (ptr->actual_pos_arr + len >= 2048){
                    ptr->missing = 0;
                    rv = -1; /*Corto el while*/
                    cut = 0;
                    
                    snd = writen(ptr->fd, &err, strlen(err));
                    if (snd == -1)
                        perror("error_send");
                    if (snd != 0){
                        quit_epoll(e_m_struct, evlist);
                        return -1;
                    }
                }

                else {
                    
                    int tot = fmax(len, ptr->actual_pos_arr);
                    
                    /*Uno las partes del comando*/
                    for (int i = 0; i < tot; i++){
                        if (i < len)
                            comm[i] = ptr->to_complete[i];
                        if (i < ptr->actual_pos_arr)
                            comm[len + i] = ptr->command[i];
                    }
                    comm[len + ptr->actual_pos_arr] = '\0';
                    
                }
            }

            /*Lo que leí es nuevo y es un comando normal.*/
            /*Lo copio al buffer temporal y lo mando al parser.*/
            else{

                int c = 0;

                for (int i = ptr->prev_pos_arr; i < ptr->actual_pos_arr; i++){
                    comm[c] = ptr->command[i];
                    c++;
                }
                comm[c] = '\0';
            }
            
            if (cut){
                /*El comando es válido y lo debo parsear*/
                char** token_commands;
                token_commands = text_parser(comm, &cant_comm, DELIMITER, e_m_struct->mem);
                if (token_commands != NULL){
                    bye = manage_txt_client(e_m_struct, evlist, token_commands, cant_comm);
                    free(token_commands);
                    ptr->missing = 0;
                    if (bye != 0)
                        return bye;
                }
                else{
                    snd = writen(ptr->fd, &einval, strlen(einval));
                    if (snd == -1) 
                        perror("error_send");
                    if (snd != 0){
                        quit_epoll(e_m_struct, evlist);
                        return -1;
                    }
                    ptr->missing = 0;
                }
            }
        }

        /*Reiniciamos la bandera que indica que es un comando*/
            
        if (!ptr->missing)
            ptr->pos_to_complete = 0;
        ptr->is_command = 0;
        if (rv > 0)
            rv = rv - (ptr->actual_pos_arr - ptr->prev_pos_arr); /*Actualizo todas las posiciones de los punteros del buffer que recibí para así leer el prox. comando*/   
        ptr->prev_pos_arr = ptr->actual_pos_arr;
    }
    ptr->actual_pos_arr = 0;
    ptr->prev_pos_arr = 0;
    ptr->is_command = 0;
    return 0;
}

