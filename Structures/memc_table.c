#include "memc_node.h"
#include "memc_table.h"
#include <assert.h>
#include <stdlib.h>

table_t table_init(unsigned buckets) {

  table_t table = calloc(buckets, sizeof(node_t*));
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

/*Inserta nodos adelante*/
node_t* table_insert(table_t tbl, node_t* new_node, unsigned i) {

  if (tbl[i] != NULL) 
    node_addhd(HASH, new_node ,tbl[i]);

  tbl[i] = new_node;
  return new_node;
}

/* Busca el nodo y lo deja en ret*/
int table_search(
    table_t            tb,
    void*             key,
    unsigned          len,
    unsigned            i,
    node_t**           ret
    ) {

  if (tb[i] == NULL)
    return -1;

  *ret = node_search(tb[i], key, len, HASH);

  if (*ret == NULL) {
    *ret = tb[i];
    return -1;
  } 

  return 0; 
}