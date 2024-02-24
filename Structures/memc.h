#ifndef __MEMC__
#define __MEMC__
#include "memc_node.h"
#include "memc_table.h"
#include <semaphore.h>
#include "memc_queue.h"
#include "lightswitch.h"
#include "memc_stat.h"
#include <stdlib.h>

#define INITIAL_BUCKETS 2
#define SHIELD_AMOUNT 500
#define D_MEMORY_BLOCK 4096

#define TEXTO 0
#define BINARIO 1

struct MemCache {
    //Estadisticas de la cache.
    stat_t* puts;
    stat_t* gets;
    stat_t* dels;
    stat_t* keys;

    //Variables para la Tabla Hash.
    table_t tab;
    unsigned buckets;
    HasFunc hash;

    //Variables para la Priority Queue
    queue_t* queue;

    //Variables para sincronización
    ls_t* evic;           //---> Estructura para el LightSwitch

    sem_t* evic_mutex;    //---> Mutex para el LightSwitch
    sem_t* queue_mutex;   //---> Mutex para la Queue
    sem_t* turnstile;     //---> Mutex para funciones de rehash y memoria. 

    //sem_t* memory_mutex;  //---> Mutex para manejo de memoria
    sem_t** tab_shield;   //---> tabla de mutex para regiones en la tabla
    int shield_size;      //---> Cantidad de mutex

};
//Opciones para memc_alloc
typedef enum fun {
    MALLOC,
    CALLOC,
    REALLOC
} fun_t;

struct Stats {
  size_t puts;
  size_t dels; 
  size_t gets;
  size_t keys;
};

typedef struct Stats* stats_t;

typedef struct MemCache* memc_t;



// Función para iniciar la estructura de la memcache.
// 
// 
memc_t memc_init(
    HasFunc hash,
    unsigned tab_size,
    unsigned shield_size 
    );

void memc_destroy(memc_t mem);

int memc_put(
    memc_t m,
    void* key,
    void* data,
    unsigned key_len,
    unsigned data_len,
    mode_t md
    );

int memc_get(
    memc_t m,
    void* key,
    unsigned key_len,
    void** data_buff,
    mode_t md
    );

int memc_del(
    memc_t m,
    void* key,
    unsigned key_len,
    mod_t md
    );

stats_t memc_stats(memc_t m);

void* memc_alloc(memc_t mem, size_t bytes, fun_t fun, void* rea);

int memc_eviction(memc_t mem);
#endif
