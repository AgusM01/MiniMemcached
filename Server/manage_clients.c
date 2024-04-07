#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include "manage_clients.h"
#include "comunicate.h"
#include "epoll.h"




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

    struct data_ptr_text* ptr;
    ptr = CAST_DATA_PTR_TEXT;

    /*Checkeo si la cantidad de comandos es correcta*/  
    if (cant_comm == 0){
        snd = writen(ptr->fd, einval, strlen(einval));
        if (snd == -1)
            perror("error_send");
        if (snd != 0){
            quit_epoll(e_m_struct, evlist);
            return -1; 
        }
        return 0;
    }

    /*Checkeo el tipo de comando*/
    if (!strcmp(token_comands[0], "PUT")) 
        command = 0;
    if (!strcmp(token_comands[0], "DEL")) 
        command = 1;
    if (!strcmp(token_comands[0], "GET")) 
        command = 2;
    if (!strcmp(token_comands[0], "STATS")) 
        command = 3;
    
    /*Analizo todos los posibles errores que responderia con error*/
    if ((cant_comm == 0)
        || (command == -1)
        || ((command == 0) && (cant_comm != 3))
        || ((command == 1) && (cant_comm != 2))
        || ((command == 2) && (cant_comm != 2))
        || ((command == 3) && (cant_comm != 1)))
        {

        snd = writen(ptr->fd, einval, strlen(einval));
        if (snd == -1)
            perror("error_send");
        if (snd != 0){
            quit_epoll(e_m_struct, evlist);
            return -1;
        }
        return 0;
    }
    
    /*PUT*/
    if (command == 0){
        int len_tok_1 = my_length(token_comands[1]);
        int len_tok_2 = my_length(token_comands[2]);

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
    }

    /*DEL*/
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

    /*GET*/
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

        if (snd == -1)
            perror("error_send");

        if (snd != 0){
            quit_epoll(e_m_struct, evlist);
            return -1;
        }

        if (res > 0)
            free(get_value);
    
    }

    /*STATS*/
    if (command == 3){

        memc_stats(e_m_struct->mem, &stats);
        
        len_stat_buf = sprintf(stat_buf,"OK PUTS=%lu DELS=%lu GETS= %lu KEYS=%lu\n", stats.puts, stats.dels, stats.gets, stats.keys);
        assert(len_stat_buf > 0);
        
        snd = writen(ptr->fd, stat_buf, len_stat_buf);
        if (snd == -1)
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
                        ptr_bin->text_or_binary);
        
        snd = writen(ptr_bin->fd, &ok, 1);
        if (snd == -1)
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
                        ptr_bin->text_or_binary);
        if (!res){

            /*No encontró la clave*/
            snd = writen(ptr_bin->fd, &enotfound, 1);
            if (snd == -1)
                perror("error_send");
            if (snd != 0){
                quit_epoll(e_m_struct, evlist);
                return -1;
            }
        }
        else{

            /*Encontré la clave*/
            len = htonl(res);
            
            snd = writen(ptr_bin->fd, &ok, 1);
            if (snd != 0){
                quit_epoll(e_m_struct, evlist);
                free(buf);
                return -1;
            }

            snd = writen(ptr_bin->fd, (void*)&len, 4);
            if (snd != 0){
                quit_epoll(e_m_struct, evlist);
                free(buf);
                return -1;
            }
            
            snd = writen(ptr_bin->fd, buf, res);
            if (snd != 0){
                quit_epoll(e_m_struct, evlist);
                free(buf);
                return -1;
            }
            free(buf);
        }
        return 0;
    }

    /*Checkeo si es DEL*/
    if ((int)ptr_bin->commands[0] == 12){
        
        command = 2;
        
        res = memc_del(e_m_struct->mem,
                        ptr_bin->key,
                        ptr_bin->length_key,
                        ptr_bin->text_or_binary);

        if (res == -1){
            snd = writen(ptr_bin->fd, &enotfound, 1);
            if (snd == -1)
                perror("error_send");
            if (snd != 0){
                quit_epoll(e_m_struct, evlist);
                return -1;
            }
        }
        else{
            snd = writen(ptr_bin->fd, &ok, 1);
            if (snd == -1)
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
        
        memc_stats(e_m_struct->mem, &stats);
        len_stat_buf = sprintf(stat_buf,"PUTS=%lu DELS=%lu GETS= %lu KEYS=%lu\n", stats.puts, stats.dels, stats.gets, stats.keys);

        len = htonl(len_stat_buf);
        
        snd = writen(ptr_bin->fd, &ok, 1);
        if (snd == -1)
            perror("error_send");
        if (snd != 0){
            quit_epoll(e_m_struct, evlist);
            return -1;
        }
        
        snd = writen(ptr_bin->fd, (void*)&len, 4);
        if (snd == -1)
            perror("error_send");
        if (snd != 0){
            quit_epoll(e_m_struct, evlist);
            return -1;
        }
        
        
        snd = writen(ptr_bin->fd, &stat_buf, strlen(stat_buf));
        if (snd == -1)
            perror("error_send");
        if (snd != 0){
            quit_epoll(e_m_struct, evlist);
            return -1;
        }

    }

    /*Comando erróneo*/
    if (command == -1){

        snd = writen(ptr_bin->fd, &einval, 1);
        if (snd == -1)
            perror("error_send");
        if (snd != 0){
            quit_epoll(e_m_struct, evlist);
            return -1;
        }

    }
    return 0;
}
