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
#include "text_parser.h"


char** text_parser(char* text, int* cant_comm, char* delimiter){

    char** res = malloc(sizeof(char*)*4);
    char* tokbuf[1] ;
    int i = 0;
    printf("Text en parser: %s", text);
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
            free(res[0]);
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