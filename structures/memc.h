#ifndef __MEMC__
#define __MEMC__
#include "memc_node.h"
#include "memc_table.h"
#include <semaphore.h>
#include "memc_queue.h"
#include "lightswitch.h"
#include <stdlib.h>

#define INITIAL_BUCKETS 2
#define SHIELD_AMOUNT 500

#define TEXTO 0
#define BINARIO 1

struct MemCache;

struct Stats {
  size_t puts;
  size_t dels; 
  size_t gets;
  size_t keys;
};

typedef struct Stats* stats_t;

typedef struct MemCache* memc_t;

memc_t memc_init(
    HasFunc hash,
    unsigned tab_size,
    unsigned shield_size,
    unsigned memory_limit 
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

void* memc_alloc(memc_t mem, size_t bytes);

#endif
