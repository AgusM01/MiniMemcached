#include "memc_node.h"
#include "memc_queue.h"
#include <stdlib.h>

struct __MemcQueue {
 node_t* mru;
 node_t* lru;
};

queue_t* queue_init() {
  queue_t* new_queue = malloc(sizeof(queue_t));
  if (!new_queue)
    return new_queue;
  new_queue->mru = NULL;
  new_queue->lru = NULL;
  return new_queue;
}

void queue_destroy(queue_t* queue) {
  free(queue);
}

void queue_addmru(queue_t *queue, node_t *node) {
  if (!queue->mru) {
    queue->mru = node;
    queue->lru = node;
    return;
  }
  node_addhd(QUEUE, node, queue->mru);
}

void queue_delnode(node_t *node) {
 node_discc(QUEUE, node); 
}

node_t* queue_dqlru(queue_t* queue) {
  if (!queue->lru)
    return NULL;

  node_t* temp = queue->lru;

  queue->lru = node_arrow(temp, QUEUE, LEFT); 
  node_discc(QUEUE, temp);
  return temp;
}
