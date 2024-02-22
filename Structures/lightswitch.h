#ifndef __LIGHT_SWITCH__
#define __LIGHT_SWITCH__
#include <semaphore.h>

struct LightSwitch;

typedef struct LightSwitch ls_t;

ls_t* ls_init(); 

void ls_destroy(ls_t* ls);

void ls_lock(ls_t* ls, sem_t* sem);

void ls_unlock(ls_t* ls, sem_t* sem);

#endif