#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "memc_node.h"


/*Tested*/
void node_free(node_t* node) {
  free(node->data_buff);
  free(node->key_buff);
  free(node);
}

/*Tested*/
void node_frees(int hq, node_t* node) {
  node_t* temp = node;
  node_t* next = node->arrows[hq + RIGHT];
  node_free(node); 
  while(next) {
    temp = next;
    next = next->arrows[hq + RIGHT];
    node_free(temp);
  }
}

/*Tested*/
node_t* node_search(node_t* node, void* key, unsigned int len, mod_t hq) {
  node_t* temp = node;

  for(;
    temp && //temp != NULL                         
    (temp->key_len != len || // Checkea longitudes
    memcmp(temp->key_buff, key, len)); // Checkea keys
    temp = temp->arrows[hq + RIGHT] // Retorna el nodo
  );

  return temp;
}

/*Tested*/
node_t* node_arrow(node_t* node, int hq, dir_t dir) {
  return node->arrows[hq + dir];
}

/*Tested*/
int node_addhd(int hq, node_t* node, node_t* head) {
  head->arrows[hq + LEFT] = node;
  node->arrows[hq + RIGHT] = head;
  return 0;
}

/*Tested*/
void node_discc(int hq, node_t* node) {
  if (node->arrows[hq + LEFT]) {
    node->arrows[hq + LEFT]->arrows[hq + RIGHT] = node->arrows[hq + RIGHT];
  }

  if (node->arrows[hq + RIGHT]) {
    node->arrows[hq + RIGHT]->arrows[hq + LEFT] = node->arrows[hq + LEFT]; 
  }

  node->arrows[hq + LEFT] = node->arrows[hq + RIGHT] = NULL;
}
