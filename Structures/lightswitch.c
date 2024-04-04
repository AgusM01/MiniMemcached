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
    assert((new_ls = malloc(sizeof(ls_t))));
    assert((new_ls->mutex = malloc(sizeof(sem_t))));

    assert(!sem_init(new_ls->mutex, 0, 1));
    new_ls->count = 0;

    return new_ls;
} 

void ls_destroy(ls_t* ls) {
    sem_destroy(ls->mutex);
    free(ls->mutex);
    free(ls);
}

/*Lectores / Escritores*/
void ls_lock(ls_t* ls, sem_t* sem) {
    assert(!sem_wait(ls->mutex)); // Entro a la habitacion
        
    ls->count++; // Me anoto que entro

    if (ls->count == 1) // Soy el unico
        assert(!sem_wait(sem)); // No quiero que entren escritores (desalojo)

    assert(!sem_post(ls->mutex));   // Modifique contador (Region Critica).
                                    // Dejo que entre otro.
}

void ls_unlock(ls_t* ls, sem_t* sem) {
    assert(!sem_wait(ls->mutex));
        
    ls->count--;

    if (ls->count == 0)
        assert(!sem_post(sem)); // Se fueron todos lectores, 
                                // dejo que entren escritores (desalojo).

    assert(!sem_post(ls->mutex));
}