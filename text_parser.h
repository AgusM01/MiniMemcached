#ifndef __TEXT_PARSER_H__
#define __TEXT_PARSER_H__

#include "structures/memc.h"

#define SIZE 10

/*  
    * Toma un string, un delimitador y un puntero a int
    * Devuelve un char** que contiene la cadena tokenizada separada por el delimitador
    * y llena el puntero a int con la cantidad de tokens.*/
char** text_parser(char* text, int* cant_comm, char* delimiter, memc_t mem);

#endif /*__TEXT_PARSER_H__*/