#ifndef __COMUNICATE_H__
#define __COMUNICATE_H__

#include "manage_clients.h"
#include "epoll.h"
#include <pthread.h>

/*Funcion que se asegura de escribir correctamente en el buffer de respuesta*/
int writen  (int fd, 
            const void *buf, 
            int len);

/*Funcion que checkea que todos los caracteres sean correctos/válidos*/
int my_length(char* buf);

#endif // __COMUNICATE_H__
