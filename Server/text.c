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

/*Consume el texto del fd de un cliente.*/
int text_consume(struct args_epoll_monitor* e_m_struct, struct epoll_event* evlist){
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

    //printf("Leo de fd: %d\n", ptr->fd); 

    /*Checkeamos si no hay lugares de \n ya leidos, de esta manera si es que hay el hilo simplemente puede leer lo que necesita*/
    if (ptr->cant_comm_ptr == 0){

        /*Primer recv para ubicar la posicion del \n*/
        fst_read = recv_mem(e_m_struct, evlist, temp, MAX_CHAR, MSG_PEEK);
        if (fst_read <= 0){
            if (fst_read == -1){
                quit_epoll(e_m_struct, evlist);
                return -1; //No volverlo a meter al epoll
            }
            return 0; //Volverlo a meter al epoll
        }

        //fst_read = recv(ptr->fd, temp, MAX_CHAR, MSG_PEEK);
        //puts("HAGO PEEK");
        //perror("error_read");
//
        //if (fst_read <= 0){
        //    if(fst_read != -1 || (errno != EWOULDBLOCK && errno != EAGAIN))            
        //        quit_epoll(e_m_struct, evlist);
        //    return;
        //}

        //printf("fst_read: %ld\n", fst_read);
        /*Ubicamos terminador de cadena.*/
        temp[fst_read] = '\0';
            
        /*Recorro lo leido para guardar los \n*/
        /*Si entró aca significa que no hay \n guardados, por lo que reiniciamos la pos del array.*/
        ptr->actual_pos_arr = 0;
        for (int i = 0; i < fst_read; i++){
            if(temp[i] == '\n'){
                valid = 1;
                ptr->cant_comm_ptr += 1;
                /*Posiblemente nunca deba ejecutarse pero por las dudas.*/
                if(ptr->cant_comm_ptr == cant_commands){ 
                    cant_commands *= 2;
                    ptr->delimit_pos = memc_alloc(e_m_struct->mem, cant_commands * 4, REALLOC ,ptr->delimit_pos );
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
            
            err_read = recv_mem(e_m_struct, evlist, temp, fst_read, MSG_WAITALL);
            if (err_read <= 0){
                if (err_read == -1){
                    quit_epoll(e_m_struct, evlist);
                    return -1; //No volverlo a meter al epoll
                }
                return 0; //Volverlo a meter al epoll
            }

            //err_read = recv(ptr->fd, temp, fst_read, MSG_WAITALL);
            //if (err_read <= 0){
            //    quit_epoll(e_m_struct, evlist);
            //    return;
            //}
            snd = writen(ptr->fd, &err, strlen(err));
            if (snd == -1)
                perror("error_send");
            if (snd != 0){
                quit_epoll(e_m_struct, evlist);
                return -1;
            }

            ptr->readed = 0;
            return 0;
        }

        if(!valid){ /*No leyó ningun \n, debo esperar a que llegue el resto del pedido.*/

            if (ptr->readed + fst_read >= MAX_CHAR){
                char err_buf[fst_read];

                fst_read = recv_mem(e_m_struct, evlist, err_buf, fst_read, MSG_WAITALL);
                if (fst_read <= 0){
                    if (fst_read == -1){
                        quit_epoll(e_m_struct, evlist);
                        return -1; //No volverlo a meter al epoll
                    }
                    return 0; //Volverlo a meter al epoll
                }

                //fst_read = recv(ptr->fd, err_buf, fst_read, MSG_WAITALL); 
                //if (fst_read <= 0){
                //    quit_epoll(e_m_struct, evlist);
                //    return;
                //}
                
                snd = writen(ptr->fd, &err, strlen(err));
                if (snd == -1)
                    perror("error_send");
                if (snd != 0){
                    quit_epoll(e_m_struct, evlist);
                    return -1;
                }
                    
                ptr->readed = 0;
                return 0;
            }

            fst_read = recv_mem(e_m_struct, evlist, ptr->command + ptr->readed, fst_read, MSG_WAITALL);
            if (fst_read <= 0){
                if (fst_read == -1){
                    quit_epoll(e_m_struct, evlist);
                    return -1; //No volverlo a meter al epoll
                }
                return 0; //Volverlo a meter al epoll
            }

            //fst_read = recv(ptr->fd, ptr->command + ptr->readed, fst_read, MSG_WAITALL); /*Lo consumo*/
            //perror("error_recv");
            //if (fst_read <= 0){
            //    if(fst_read != -1 || (errno != EWOULDBLOCK && errno != EAGAIN))            
            //        quit_epoll(e_m_struct, evlist);
            //    return;
            //}

            ptr->readed += fst_read; /*Si ya pasaron 2048 sin \n lo vuelo*/
            
            if (ptr->readed < MAX_CHAR)
                ptr->command[ptr->readed] = '\0';
            else{
                snd = writen(ptr->fd, &err, strlen(err));
                if (snd == -1) 
                    perror("error_send");
                if (snd != 0){
                    quit_epoll(e_m_struct, evlist);
                    return -1;
                }
                
                ptr->readed = 0;
                return 0;
            }

            ptr->missing = 1;
            //printf("Command: %s\n", ptr->command);
            return 0;
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


        rec = recv_mem(e_m_struct, evlist, ptr->command + ptr->readed, s, MSG_WAITALL);
        if (rec <= 0){
            if (rec == -1){
                quit_epoll(e_m_struct, evlist);
                return -1; //No volverlo a meter al epoll
            }
            return 0; //Volverlo a meter al epoll
        }

        //rec = recv(ptr->fd, ptr->command + ptr->readed, s, MSG_WAITALL);
        //perror("error_recv");
        //if (rec <= 0){
        //    if(rec != -1 || (errno != EWOULDBLOCK && errno != EAGAIN))            
        //        quit_epoll(e_m_struct, evlist);
        //    return;
        //}
        
        //printf("command pre token: %s\n", ptr->command);
        ptr->command[ptr->readed + rec] = '\0';
        token_commands = text_parser(ptr->command, &cant_comm, DELIMITER, e_m_struct->mem);
    }
    else{
        char* comm = memc_alloc(e_m_struct->mem, s + 1, MALLOC, NULL);
        assert(comm != NULL);

        read_comm = recv_mem(e_m_struct, evlist, comm, s, MSG_WAITALL);
        if (read_comm <= 0){
            if (read_comm == -1){
                quit_epoll(e_m_struct, evlist);
                return -1; //No volverlo a meter al epoll
            }
            return 0; //Volverlo a meter al epoll
        }
        
        //read_comm = recv(ptr->fd, comm, s, MSG_WAITALL);
        //perror("error_recv");
        //if (read_comm <= 0){
        //    if(read_comm != -1 || (errno != EWOULDBLOCK && errno != EAGAIN))            
        //        quit_epoll(e_m_struct, evlist);
        //    free(comm);
        //    return;
        //}

        comm[s] = '\0'; /*Le agrego el terminador de cadena*/
        token_commands = text_parser(comm, &cant_comm, DELIMITER, e_m_struct->mem);
    }
    int bye = 0;
    if (token_commands != NULL){
        bye = manage_txt_client(e_m_struct, evlist, token_commands, cant_comm);
        if (!ptr->missing)
            free(token_commands[0]);
        free(token_commands);
    }
    else {
        snd = writen(ptr->fd, &einval, strlen(einval));
        if (snd == -1) 
            perror("error_send");
        if (snd != 0){
            quit_epoll(e_m_struct, evlist);
            return -1;
        }
        return 0;
    }
    
    ptr->actual_pos_arr += 1;
    ptr->cant_comm_ptr -= 1; 
    ptr->missing = 0;
    ptr->readed = 0;

    return bye; 

}

char** text_parser(char* text, int* cant_comm, char* delimiter, memc_t mem){

    char** res = memc_alloc(mem, sizeof(char*)*4, MALLOC, NULL);
    char* tokbuf[1] ;
    int i = 0;
    /*Reinicio la cantidad de comandos*/     
    *cant_comm = 0;
    tokbuf[0] = NULL;
    //int m = SIZE;
    /*Por qué no realocar? -> Si realoco se rompe porque cambia las direcciones de memoria y los punteros se desactualizan*/
    /*Asumo que strtok hace algo falopa que realloca solo.*/
    res[i] = strtok_r(text, delimiter, tokbuf);
    while(res[i] != NULL){
        *cant_comm += 1;
        i++;
        if (i > 3){
            //printf("Text en parser: %s", text);
            free(res);
            return NULL;
        }
        res[i] = strtok_r(NULL, delimiter, tokbuf);
    }

    //printf("cant_comm: %d\n", *cant_comm);
    //for (int i = 0; i < *cant_comm; i++)
    //    printf("res: %s\n", res[i]);
    /*La idea es recibir una linea en text y devolver un array de arrays con cada comando.*/
    /*Usar strtok utilizando " " como delimitador*/
    return res;
}

int text2(struct args_epoll_monitor* e_m_struct, struct epoll_event* evlist){

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
    printf("rv after recv: %d\n", rv);
    if (rv <= 0){
        if (rv == -1){
            quit_epoll(e_m_struct, evlist);
            return -1; //No volverlo a meter al epoll
        }
        return 0; //Volverlo a meter al epoll
    }
    printf("recibo: %s\n", ptr->command);
    /*Debo ir leyendo los comandos y respondiendolos*/

    while (ptr->actual_pos_arr < rv){

        for (int i = 0; i < rv; i++){

            /*Si cae acá significa que no encontré \n en todo lo que leí y me pasé*/
            if (ptr->actual_pos_arr + i >= rv){
                i = rv;
            }
            else{
                if (ptr->command[ptr->actual_pos_arr + i] == '\n'){
                    ptr->actual_pos_arr += (i  + 1);
                    ptr->is_command = 1;
                    i = rv;
                }
            }
        }


        printf("actual: %d\n", ptr->actual_pos_arr);
        /*Leí 2048 sin \n*/
        /*Sea un comando cortado, o no, es erróneo.*/
        if (ptr->is_command == 0 && 
            rv >= MAX_CHAR){
            ptr->missing = 0;
            ptr->actual_pos_arr = rv;
            puts("que hago aca");
            snd = writen(ptr->fd, &err, strlen(err));
            if (snd == -1)
                perror("error_send");
            if (snd != 0){
                quit_epoll(e_m_struct, evlist);
                return -1;
            }
            //puts("mande big");
            return 0;
        }

        /*No leí \n, falta que llegue el comando entero*/
        /*En este caso lo guardo en el buffer iterno que guarda el comando cortado.*/
        if (ptr->is_command == 0){
            //printf("pos to complete: %d\n", ptr->pos_to_complete);
            //printf("suma: %d\n", ptr->pos_to_complete + rv);
            /*El comando ya es demasiado largo*/
            if (ptr->pos_to_complete + rv >= MAX_CHAR){
                
                ptr->missing = 0;
                ptr->is_command = 0;
                ptr->pos_to_complete = 0;
                ptr->prev_pos_arr = 0;
                ptr->actual_pos_arr = rv; /*-> Corto el while*/
                puts("voy a cortar el while 1");
                snd = writen(ptr->fd, &err, strlen(err));
                if (snd == -1)
                    perror("error_send");
                if (snd != 0){
                    quit_epoll(e_m_struct, evlist);
                    return -1;
                }
                //puts("mando big 2");
            }
            else{
                printf("rv: %d\n", rv);
                
                int i = 0;
                for (; i < ptr->actual_pos_arr; i++){
                    ptr->to_complete[ptr->pos_to_complete + i] = ptr->command[ptr->actual_pos_arr + i];
                }
                puts("Llego");
                ptr->to_complete[ptr->pos_to_complete + i] = '\0';
                ptr->pos_to_complete += i;
                ptr->missing = 1;
                ptr->is_command = 0;
                ptr->prev_pos_arr = 0;
                ptr->actual_pos_arr = rv; /*Corto el while -> no afecta ya que cuando lea nuevamente lo que falta se actualiza*/
                puts("voy a cortar el while 2");
            }
        }

        if (ptr->is_command == 1){
            
            /*Lo que leí forma parte de uno que vino antes partido*/
            /*Entonces debo unir las partes del comando y mandarlo al parser.*/
            if (ptr->missing == 1){
                puts("Estoy");
                int len = strlen(ptr->to_complete);
                printf("len: %d\n", len);
                printf("act_pos_arr: %d\n", ptr->actual_pos_arr);
                /*El comando total es demasiado largo*/
                if (ptr->actual_pos_arr + len >= 2048){
                    ptr->missing = 0;
                    ptr->is_command = 0;
                    ptr->pos_to_complete = 0;
                    ptr->prev_pos_arr = 0;
                    ptr->actual_pos_arr = rv; /*Corto el while*/
                    puts("voy a cortar el while 3");
                    snd = writen(ptr->fd, &err, strlen(err));
                    if (snd == -1)
                        perror("error_send");
                    if (snd != 0){
                        quit_epoll(e_m_struct, evlist);
                        return -1;
                    }
                }
                else {
                    printf("RV: %d\n", rv);
                    int tot = fmax(len, ptr->actual_pos_arr);
                    int c = 0;
                    for (int i = ptr->prev_pos_arr; i < tot; i++){
                        if (c < len){
                            //printf("to_complete[%d]: %c\n", i, ptr->to_complete[i]);
                            comm[c] = ptr->to_complete[i];
                        }
                        if (i < ptr->actual_pos_arr){
                            //printf("command[%d]: %c\n", i, ptr->command[i]);
                            comm[len + c] = ptr->command[i];
                        }
                        c++;
                    }
                    comm[len + c] = '\0';
                    //for (int c = 0; c < len + tot; c++)
                    //    printf("comm[%d]: %c\n", c, comm[c]);
                }
            }

            /*Lo que leí es nuevo y es un comando normal.*/
            /*Lo copio al buffer temporal y lo mando al parser.*/
            if (ptr->missing == 0 ){

                int c = 0;
                printf("prev: %d\n", ptr->prev_pos_arr);
                printf("act: %d\n", ptr->actual_pos_arr);

                for (int i = ptr->prev_pos_arr; i < ptr->actual_pos_arr; i++){
                    comm[c] = ptr->command[i];
                    c++;
                }
                comm[c] = '\0';
                puts("lo formo normal");
            }
            
            printf("comm: %s\n", comm);

            char** token_commands;
            token_commands = text_parser(comm, &cant_comm, DELIMITER, e_m_struct->mem);
            if (token_commands != NULL){
                bye = manage_txt_client(e_m_struct, evlist, token_commands, cant_comm);
                free(token_commands);
                ptr->missing = 0;
                //rv -= (ptr->actual_pos_arr - ptr->prev_pos_arr);
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

        /*Reiniciamos la bandera que es un comando*/
        if (!ptr->missing)
            ptr->pos_to_complete = 0;
        ptr->is_command = 0;
        printf("ACTUALPOS: %d\n", ptr->actual_pos_arr);
        printf("PREVPOS: %d\n", ptr->prev_pos_arr);
        rv = rv - (ptr->actual_pos_arr - ptr->prev_pos_arr);        
        ptr->prev_pos_arr = ptr->actual_pos_arr;
        printf("ERREVE: %d\n", rv);
    }

    ptr->actual_pos_arr = 0;
    ptr->prev_pos_arr = 0;
    ptr->is_command = 0;
    return 0;

    



}

/*PUT agus merino\nGET agus\nPUT*/
/*0123456789     15            27*/                                 
/*                */