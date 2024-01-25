#ifndef __MEMC_TABLE__
#define __MEMC_TABLE__
#include "memc_node.h"

struct _MemcTable {
  node_t**            table;
  size_t              buckets;
  size_t              elements;
};

typedef struct _MemcTable table_t;

table_t* table_init(int buckets);

void table_destroy(table_t* table);

void table_delete(table_t* tbl, node_t* node);

void table_insert(
    table_t*        tbl,
    void*             key,
    unsigned int  len_key,
    void*            data,
    unsigned int len_data,
    mode_t md
);

node_t* table_search(

    table_t*          tb,
    void*             key,
    size_t            len,
    void*             data,
    unsigned*         data_len

);


#endif
