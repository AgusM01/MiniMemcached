#include "memc_stat.h"
#include <semaphore.h>
#include <stddef.h>
#include <stdlib.h>
#include <assert.h>

struct __Stat {
    size_t count;
    sem_t* mutex;
};

stat_t* stat_init(size_t init_val) {
    stat_t* new_stat;
    assert((new_stat = malloc(sizeof(struct __Stat))));
    new_stat->mutex = malloc(sizeof(sem_t));
    sem_init(new_stat->mutex, 0, 1);
    new_stat->count = init_val;
    return new_stat;
}

void stat_destroy(stat_t* st) {
    sem_destroy(st->mutex);
    free(st->mutex);
    free(st);
}

void stat_add(stat_t* st, size_t n) {
    st->count += n;
}

void stat_lock(stat_t* st) {
    sem_wait(st->mutex);
}

void stat_unlock(stat_t *st) {
    sem_post(st->mutex);
}

void stat_inc(stat_t *st) {
    sem_wait(st->mutex);
    st->count++;
    sem_post(st->mutex);
}

void stat_dec(stat_t *st) {
    sem_wait(st->mutex);
    st->count--;
    sem_post(st->mutex);
}

size_t stat_get(stat_t *st) {
    return st->count;
}

void stat_put(stat_t *st, size_t n){
    sem_wait(st->mutex);
    st->count = n;
    sem_post(st->mutex);
}