#ifndef __LIGHT_SWITCH__
#define __LIGHT_SWITCH__
#include <semaphore.h>

struct LightSwitch;

typedef struct LightSwitch ls_t;

/*
    Estrucutura de sincronización.
    Excluye dos grupos distintos de threads.
    El grupo A de thread utiliza ls_lock y ls_unlock, y actuan
    concurrentemente, bloqueando sem solo cuando el primero de los thread
    hace un lock y desbloqueando sem cuando el último hace unlock.
    Cuando todos los threads de tipo A desbloquean, Los thread del
    grupo B pueden peliar por la exclusión mútua de sem.
*/

/* Inicia la estrucutura */
ls_t* ls_init(); 

/* Destructor de la estructura */
void ls_destroy(ls_t* ls);

/*
    Entrada: Estrucutura y semáforo del Grupo B
*/
void ls_lock(ls_t* ls, sem_t* sem);

/*
    Entrada: Estrucutura y semáforo del Grupo B
*/
void ls_unlock(ls_t* ls, sem_t* sem);

#endif