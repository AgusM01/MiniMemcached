#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "memc_node.h"

/*!
 *
 */




/*Tested, no usada*/


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
int node_getdata(node_t* node, void** buff) {
  *buff = node->data_buff;
  return node->data_len;
}

int node_cpydata(node_t* node, void* buff) {
  memcpy(buff , node->data_buff, node->data_len);
  return node->data_len;
}

void node_setdata(node_t* node, void* buff, unsigned len) {
  node->data_buff = buff;
  node->data_len = len;
}

/*Tested*/
int node_getkey(node_t* node, void** buff) {
  *buff = node->key_buff;
  return node->key_len;
}

void node_setkey(node_t* node, void* buff, unsigned len) {
  node->key_buff = buff;
  node->key_len = len;
}

void node_setmode(node_t* node, mod_t mod) {
  node->mode = mod;
}

mod_t node_getmode(node_t* node) {
  return node->mode;
}

/*Tested*/
node_t* node_arrow(node_t* node, int hq, dir_t dir) {
  return node->arrows[hq + dir];
}

/*Tested*/
int node_addhd(int hq, node_t* node, node_t* head) {
  if ((head == NULL) || head->arrows[hq + LEFT])
    return -1;

  head->arrows[hq + LEFT] = node;
  node->arrows[hq + RIGHT] = head;
  return 0;
}

// int node_newdata(node_t* node, void* data, int len_data) {
//   free(node->data_buff);

//   if(!(node->data_buff = malloc(len_data)))
//     return -1;

//   memcpy(node->data_buff, data, len_data);

//   return 0;
// }

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
