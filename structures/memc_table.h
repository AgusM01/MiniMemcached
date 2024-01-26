#ifndef __MEMC_TABLE__
#define __MEMC_TABLE__
#include "memc_node.h"

typedef node_t** table_t;

table_t table_init(unsigned buckets);

void table_destroy(table_t table, unsigned elements);

node_t* table_insert(table_t tbl, node_t* new_node, unsigned i);

int table_search(
    table_t              tb,
    void*               key,
    unsigned            len,
    unsigned              i,
    node_t*             ret
);

int table_rehash(
    table_t tb,
    unsigned old_size,
    unsigned new_size,
    unsigned (*hash)(void*,unsigned)   
);

#endif
