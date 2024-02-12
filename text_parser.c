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

    char** res = malloc(sizeof(char*)*SIZE);
    int i = 0;

    /*Reinicio la cantidad de comandos*/     
    *cant_comm = 0;
    
    //int m = SIZE;
    /*Por qué no realocar? -> Si realoco se rompe porque cambia las direcciones de memoria y los punteros se desactualizan*/
    /*Asumo que strtok hace algo falopa que realloca solo.*/
    res[i] = strtok(text, delimiter);
    while(res[i] != NULL){
        *cant_comm += 1;
        i++;
        res[i] = strtok(NULL, delimiter);
    }

    //printf("cant_comm: %d\n", *cant_comm);
    //for (int i = 0; i < *cant_comm; i++)
    //    printf("res: %s\n", res[i]);
    /*La idea es recibir una linea en text y devolver un array de arrays con cada comando.*/
    /*Usar strtok utilizando " " como delimitador*/
    return res;
}