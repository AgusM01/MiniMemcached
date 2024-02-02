#include <assert.h>
#include <semaphore.h>
#include <stdlib.h>
#include "lightswitch.h"

struct LightSwitch {
    sem_t* mutex;
    unsigned count;
};



ls_t* ls_init() {
    ls_t* new_ls; 
    assert(!(new_ls = malloc(sizeof(ls_t*))));

    sem_init(new_ls->mutex, 0, 1);
    new_ls->count = 0;

    return new_ls;
} 

void ls_destroy(ls_t* ls) {
    sem_destroy(ls->mutex);
    free(ls);
}

void ls_lock(ls_t* ls, sem_t* sem) {
    sem_wait(ls->mutex);
        
    ls->count++;

    if (ls->count == 1)
        sem_wait(sem);

    sem_post(ls->mutex);
}

void ls_unlock(ls_t* ls, sem_t* sem) {
    sem_wait(ls->mutex);
        
    ls->count--;

    if (ls->count == 0)
        sem_post(sem);

    sem_post(ls->mutex);
}