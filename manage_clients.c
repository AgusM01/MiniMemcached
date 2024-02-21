
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include "server.h"
#include "manage_clients.h"
#include "comunicate.h"




int manage_txt_client(struct args_epoll_monitor* e_m_struct, struct epoll_event* evlist, char** token_comands, int cant_comm){
    
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

    //printf("cant_comm: %d\n", cant_comm);
    if (cant_comm == 0){
        snd = writen(ptr->fd, einval, strlen(einval));
        perror("error_send");
        if (snd != 0){
            quit_epoll(e_m_struct, evlist);
            return -1; 
        }
        return 0;
    }
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
        if (snd != 0){
            quit_epoll(e_m_struct, evlist);
            return -1;
        }
        return 0;
    }

    if (command == 0){
        //printf("Pongo clave: %s, valor: %s\n Longitudes clave: %ld, valor: %ld\n", token_comands[1], token_comands[2], strlen(token_comands[1]),strlen(token_comands[2]));
        int len_tok_1 = my_length(token_comands[1]);
        int len_tok_2 = my_length(token_comands[2]);

        //printf("Longitud token 1: %ld\n", strlen(token_comands[1]));
        //printf("len_tok_1: %d\n", len_tok_1);

        if (len_tok_1 == -1 || len_tok_2 == -2){
            snd = writen(ptr->fd, einval, strlen(einval));
            if (snd != 0){
                quit_epoll(e_m_struct, evlist);
                return -1;    
            }
            return 0;
        }

        res = memc_put  (e_m_struct->mem, 
                        token_comands[1], 
                        token_comands[2], 
                        len_tok_1,
                        len_tok_2,
                        ptr->text_or_binary); /*TEXTO*/

        snd = writen(ptr->fd, ok, strlen(ok));
        if (snd != 0){
            quit_epoll(e_m_struct, evlist);
            return -1;
        }
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
            if (snd != 0){
                quit_epoll(e_m_struct, evlist);
                return -1;    
            }
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
            return -1;
        }

    }
    if (command == 2){

        int len_tok_1 = my_length(token_comands[1]);
        
        if (len_tok_1 == -1){
            snd = writen(ptr->fd, einval, strlen(einval));
            if (snd != 0){
                quit_epoll(e_m_struct, evlist);
                return -1;
            }
            return 0;    
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
        if (snd != 0){
            quit_epoll(e_m_struct, evlist);
            return -1;
        }

        if (res > 0)
            free(get_value);
    
    }
    if (command == 3){

        stats = memc_stats(e_m_struct->mem);
        len_stat_buf = sprintf(stat_buf,"OK PUTS=%lu DELS=%lu GETS= %lu KEYS=%lu\n", stats->puts, stats->dels, stats->gets, stats->keys);
        assert(len_stat_buf > 0);
        snd = writen(ptr->fd, stat_buf, len_stat_buf);
        perror("error_send");
        if (snd != 0){
            quit_epoll(e_m_struct, evlist);
            return -1;
        }
    }
    return 0;
}

int manage_bin_client(struct args_epoll_monitor* e_m_struct, struct epoll_event* evlist){

    char ok = 101; 
    char einval = 111;
    char enotfound = 112;
    int command = -1;
    int res = 0;
    int snd = 0;
    char* buf;
    int len;
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
            
            len = htonl(res);
            //int_to_binary(res, (void*)&len);
            
            snd = writen(ptr->fd, &ok, 1);
            if (snd != 0){
                quit_epoll(e_m_struct, evlist);
                free(buf);
                return -1;
            }

            snd = writen(ptr->fd, (void*)&len, 4);
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

        len = htonl(len_stat_buf);
        //int_to_binary(len_stat_buf, (void*)&len);
        
        snd = writen(ptr->fd, &ok, 1);
        perror("error_send");
        if (snd != 0){
            quit_epoll(e_m_struct, evlist);
            return -1;
        }
        
        snd = writen(ptr->fd, (void*)&len, 4);
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