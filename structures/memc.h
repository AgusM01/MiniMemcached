#ifndef __MEMC__
#define __MEMC__
#include "memc_node.h"
#include "memc_table.h"
#include "memc_queue.h"
#include <stdlib.h>

struct MemCache;

struct Stats {
  size_t puts;
  size_t dels; 
  size_t gets;
  size_t keys;
};

typedef struct Stats* stats_t;

typedef struct MemCache* memc_t;

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
    void* key_len,
    void* data_buff,
    mode_t md
    );

int memc_del(
    memc_t m,
    void* key,
    void* key_len,
    mod_t md
    );

int memc_stats(memc_t m, stats_t st);

#endif
