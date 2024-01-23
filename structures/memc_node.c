#include <stdlib.h>
#include "memc_node.h"

node_t* node_init() {
  node_t *newnode = malloc(sizeof(struct _MemcNode));

  if (!newnode)
    return NULL

  newnode->data_len = 0;
  newnode->key_len = 0;
  newnode->mode = 0;
  newnode->data_buff = NULL;
  newnode->key_buff = NULL;
  for(int i = 0; i < 4 ; i++) 
    newnode->arrows[i] = NULL;
  return newnode;
}


void node_free(node_t* node) {
  free(newnode->data_buff);
  free(newnode->key_buff);
  for(int i = 0; i < 4 ; i++) 
    free(newnode->arrows[i]);
  free(node);
}
