#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "memc_node.h"

/*!
 *
 */

/*! Nodo principal 
 *  
 *  Este nodo contiene los datos guardados en memoria.
 *  Son las bases de la tabla hash y la cola de prioridad 
 */
struct __MemcNode {
  void*                   key_buff;
  void*                  data_buff;
  struct __MemcNode*     arrows[4];
  unsigned                 key_len;
  unsigned                data_len;
  mode_t                      mode;    
};

/*Tested*/
node_t* node_init(
    void*                   key,
    void*                  data,
    unsigned            len_key,
    unsigned           len_data,
    mod_t                   md
    ) {

  node_t *newnode = malloc(sizeof(struct __MemcNode));

  if (!newnode)
    return NULL;

  if (!(newnode->data_buff = malloc(len_data))) {
    free(newnode);
    return NULL;
  }

  if (!(newnode->key_buff = malloc(len_key))) {
    free(newnode->data_buff);
    free(newnode);
    return NULL;
  } 

  memcpy(newnode->data_buff, data,  len_data);
  memcpy(newnode->key_buff, key, len_key);

  newnode->data_len = len_data;
  newnode->key_len = len_key;
  newnode->mode = md;
  for(int i = 0; i < 4 ; i++) 
    newnode->arrows[i] = NULL;
  return newnode;
}

/*Tested*/
void node_free(node_t* node) {
  free(node->data_buff);
  free(node->key_buff);
  free(node);
}

void node_frees(int hq, node_t* node) {
  node_t* temp = node;
  node_t* next = node->arrows[hq + LEFT];
  node_free(node); 
  while(!next) {
    temp = next;
    next = next->arrows[hq + LEFT];
    node_free(temp);
  }
}

node_t* node_search(node_t* node, void* key, unsigned int len, mod_t hq) {
  node_t* temp = node;

  for(;
    temp &&                          
    (temp->key_len != len || 
    memcmp(temp->key_buff, key, len)); 
    temp = temp->arrows[hq + LEFT]
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

/*Tested para queue*/
node_t* node_arrow(node_t* node, int hq, dir_t dir) {
  return node->arrows[hq + dir];
}

/*Tested para queue*/
int node_addhd(int hq, node_t* node, node_t* head) {
  if (head->arrows[hq + LEFT])
    return -1;

  head->arrows[hq + LEFT] = node;
  node->arrows[hq + RIGHT] = head;
  return 0;
}

int node_newdata(node_t* node, void* data, int len_data) {
  free(node->data_buff);

  if(!(node->data_buff = malloc(len_data)))
    return -1;

  memcpy(node->data_buff, data, len_data);

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
