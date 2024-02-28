#include "memc_node.h"
#include "memc_table.h"
#include <assert.h>
#include <stdlib.h>

table_t table_init(unsigned buckets) {

  table_t table= calloc(buckets, sizeof(node_t*));
  assert(table != NULL);

  return table;
}

void table_destroy(table_t tab, unsigned elements) {
  for (unsigned i = 0; i < elements; i++) {
    if (tab[i] != NULL)
      node_frees(HASH, tab[i]);
  }
  free(tab);
}

node_t* table_insert(table_t tbl, node_t* new_node, unsigned i) {

  if (tbl[i] != NULL) 
    assert(node_addhd(HASH, new_node ,tbl[i]) == 0);

  tbl[i] = new_node;
  return new_node;
}

int table_search(
    table_t            tb,
    void*             key,
    unsigned          len,
    unsigned            i,
    node_t**           ret
    ) {

//  asm("mfence");
  if (tb[i] == NULL)
    return -1;

  *ret = node_search(tb[i], key, len, HASH);

  if (*ret == NULL) {
    *ret = tb[i];
    return -1;
  } 

  return 0; 
}

int table_rehash(
    table_t tb,
    table_t new_tb,
    unsigned old_size,
    unsigned new_size,
    unsigned (*hash)(void*,unsigned)
    ) {


  for(unsigned i = 0; i < old_size; i++) {
    node_t* tbi = tb[i];
    while(tbi != NULL) {
      tb[i] = node_arrow(tbi, HASH, RIGHT); 
      node_discc(HASH, tbi);
      table_insert(
        new_tb,
        tbi,
        hash(tbi->key_buff, tbi->key_len) % new_size
        );
      tbi = tb[i];
    }
  }

  free(tb);

  return 0;
}