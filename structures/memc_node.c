#include <stdlib.h>
#include "memc_node.h"

node_t* node_init(
    void* key,
    void* data,
    unsigned int len_key,
    unsigned int len_data,
    mode_t md
    ) {

  node_t *newnode = malloc(sizeof(struct _MemcNode));

  if (!newnode)
    return NULL;

  if (!(newnode->data_buff = malloc(len_key))) {
    free(newnode);
    return NULL;
  }

  if (!(newnode->key_buff = malloc(len_data))) {
    free(newnode->data_buff);
    free(newnode);
    return NULL;
  } 

  memcp(newnode->data_buff, data, len_data);
  memcp(newnode->key_buff, key, len_key);

  newnode->data_len = 0;
  newnode->key_len = 0;
  newnode->mode = md;
  for(int i = 0; i < 4 ; i++) 
    newnode->arrows[i] = NULL;
  return newnode;
}

void node_free_aux(node_t* node) {
  free(newnode->data_buff);
  free(newnode->key_buff);
  free(node);
}

void node_free(int hq, node_t* node) {
  node_t* temp = node;
  node_t* next = node->[hq + LEFT];
  node_free_aux(node); 
  while(!next) {
    temp = next;
    next = next->[hq + LEFT];
    node_free_aux(temp);
  }
}


node_t* node_search(node_t* node, void* key, unsigned int len) {
  node_t* temp = node;

  for(;
    temp &&                          
    (temp->key_len != len || 
    memcmp(temp->key_buff, key, len)); 
    temp = temp->arrows[hq + LEFT]
  );

  return temp;
}

int node_getdata(node_t* node, void* buff) {
  buff = node->data_buff;
  return node->data_len;
}

int node_cpydata(node_t* node, void* buff) {
  memcpy(buff , node->data_buff, node->data_len);
  return node->data_len;
}

void node_setdata(node_t* node, void* buff, size_t len) {
  node->data_buff = buff;
  node->data_len = len;
}

int node_getkey(node_t* node; void* buff) {
  buff = node->key_buff;
  return node->key_len;
}

void node_setkey(node_t* node, void* buff, size_t len) {
  node->key_buff = buff;
  node->key_len = len;
}

void node_setmode(node_t* node, mode_t mod) {
  node->mode = mod;
}

mode_t node_getmode(node_t* node) {
  return node->mode;
}

node_t* node_arrow(node_t* node, int hq, dir_t dir) {
  return node->arrows[hq + dir];
}

int node_addhd(int hq, node_t* node, node_t* hd) {
  if (!hd->[hq + LEFT])
    return -1;

  hd->[hq + LEFT] = node;
  node->[hq + RIGHT] = hd;
  return 0;
}

int node_newdata(node_t* node, void* data, int len_data) {
  free(node->data_buff);

  if(!(node->data_buff = malloc(len_data)));
    return -1;

  memcp(newnode->data_buff, data, len_data);

  return 0;
}

void node_discc(int hq, node_t* node) {
  if (node->[hq + LEFT]) {
    node->[hq + LEFT]->[hq + RIGHT] = node->[hq + LEFT];
    node->[hq + LEFT] = NULL;
  }

  if (node->[hq + RIGHT]) {
    node->[hq + RIGHT]->[hq + LEFT] = node->[hq + RIGHT]; 
    node->[hq + LEFT] = NULL;
  }
}
