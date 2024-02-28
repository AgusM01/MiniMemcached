#include "memc_node.h"
#include <stdio.h>
#include "memc_queue.h"
#include <complex.h>
#include <stdlib.h>

// struct __MemcQueue {
//  node_t* mru;
//  node_t* lru;
// };

/* Tested */
queue_t* queue_init() {
  queue_t* new_queue = malloc(sizeof(queue_t));
  if (!new_queue)
    return new_queue;
  new_queue->mru = NULL;
  new_queue->lru = NULL;
  return new_queue;
}



/* Tested */
int queue_empty(queue_t* queue) {
  int ret = 0;
  if (!queue->lru) { 
      ret = -1;
  }
  return ret;
}

/* Tested */
void queue_destroy(queue_t* queue) {
  free(queue);
}

/* Tested */
void queue_addmru(queue_t *queue, node_t *node) {
  if (!queue->mru) {
    queue->mru = node;
    queue->lru = node;
    return;
  }

  if (node == queue->mru)
    return;

  node_discc(QUEUE, node);
  node_addhd(QUEUE, node, queue->mru);
  queue->mru = node;
}

/* Tested */
void queue_delnode(node_t *node) {
 node_discc(QUEUE, node); 
}

/* Tested */
node_t* queue_dqlru(queue_t* queue) {
  if (!queue->lru)
    return NULL;

  node_t* temp = queue->lru;

  if (queue->lru == queue->mru) {
    queue->lru = queue->mru = NULL;  
  } else {
    queue->lru = node_arrow(temp, QUEUE, LEFT); 
    node_discc(QUEUE, temp);
  }

  return temp;
}

int queue_test(node_t* node, dir_t dir) {
  if (!node)
      return 0;
  return 1 + queue_test(node->arrows[QUEUE + dir], dir);
}
