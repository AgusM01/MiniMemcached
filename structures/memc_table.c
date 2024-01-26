#include "memc_node.h"
#include "memc_table.h"
#include <stdlib.h>

table_t table_init(unsigned buckets) {

  table_t table = malloc(sizeof(node_t*) * buckets);

  return table;
}

void table_destroy(table_t tab, unsigned elements) {
  for (unsigned i = 0; i < elements; i++)
    node_frees(HASH, tab[i]);
  free(tab);
}

node_t* table_insert(table_t tbl, node_t* new_node, unsigned i) {

  if (!tbl[i]) 
    tbl[i] = new_node;
  else 
    node_addhd(HASH, new_node ,tbl[i]);
  return new_node;
}

int table_search(
    table_t            tb,
    void*             key,
    unsigned          len,
    unsigned            i,
    node_t*           ret
    ) {

  ret = node_search(tb[i], key, len, HASH);

  if (!ret) {
    ret = tb[i];
    return -1;
  } 

  return 0; 
}

int table_rehash(
    table_t tb,
    unsigned old_size,
    unsigned new_size,
    unsigned (*hash)(void*,unsigned)
    ) {

  void* buff;
  table_t temp = tb;
  tb = table_init(new_size);

  for(unsigned i = 0; i < old_size; i++) {
    node_t* tbi = temp[i];
    while((temp[i] = node_arrow(tbi, HASH, RIGHT))) {
      node_discc(HASH, tbi);
      table_insert(tb, tbi, hash(buff, node_getkey(tbi, buff)));
    }
  }
  return 0;
}