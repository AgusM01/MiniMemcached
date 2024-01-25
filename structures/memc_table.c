#include "memc_node.h"
#include "memc_table.h"
#include "utils.h"

table_t* table_init(size_t buckets) {
  table_t* new_table = malloc(sizeof(table_t));

  if (!new_table)
    return NULL;

  new_table->table = malloc(sizeof(node_t*) * buckets);

  if (!new_table->table) {
    free(new_table);
    return NULL;
  }

  new_table->buckets = buckets;
  new_table->elements = 0; 

  return new_table;
}

void table_destroy(table_t* tab) {
  for (unsigned int i = 0; i < tab->elements; i++)
    node_free(tab->buckets[i])
  free(tab);
}

node_t* table_insert(
    table_t*          tbl,
    void*             key,
    unsigned          len_key,
    void*             data,
    unsigned          len_data,
    mode_t            md
    ) {

  node_t* new_node; 
  unsigned i = hash_len((char*)key, key_len) % hstb->buckets;

  if (!tbl->table[i]) {
    if (!(new_node = node_init(key, data, len_key, len_data,md)))
       return NULL;
    tbl->table[i] = new_node;
  }
  else {
    if (!(new_node = node_search(tbl->table[i], key , key_len))) {
      node_addhd(HASH, new_node ,tbl->table[i]);
      return nwe_node;
    }
    if(!(node_newdata(new_node, data, len_data)))
      return NULL;
  }
  return new_node;
}

node_t* table_search(

    table_t*          tb,
    void*             key,
    size_t            len,
    void*             data,
    unsigned*         data_len

    ) {

  unsigned i = hash_len((char*)key, key_len) % hstb->buckets;
  node_t* ret = node_search(tbl->table[i]);

  if (ret)
    *data_len = node_cpydata(ret, data);

  return ret 
}

void table_delete(table_t* tbl, void* key, unsigned key_len) {

  unsigned i = hash_len((char*)key, key_len) % hstb->buckets;
  node_t* ret = node_search(tbl->table[i]);

  if (ret)
    node_discc(HASH, ret);

  return ret;
}

