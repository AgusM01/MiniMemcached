#ifndef __MEMC_STAT__
#define __MEMC_STAT__
#include <semaphore.h>

struct __Stat;

typedef struct __Stat stat_t;

stat_t* stat_init(size_t init_val);

void stat_add(stat_t* st, size_t n);

void stat_destroy(stat_t* st);

size_t stat_get(stat_t* st);

void stat_dec(stat_t *st);

void stat_put(stat_t* st, size_t n);

void stat_lock(stat_t* st);

void stat_unlock(stat_t* st);

void stat_inc(stat_t* st);

#endif