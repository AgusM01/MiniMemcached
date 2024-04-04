#include "memc_node.h"
#include <assert.h>
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
  if (!queue->mru) { //Si es NULL, es el primero
    queue->mru = node;
    queue->lru = node;
    return;
  }

  if (node == queue->mru) // Ya es el mru
    return;

  node_addhd(QUEUE, node, queue->mru); //Lo agg a la queue y es el mru.
  queue->mru = node;
}

/* Tested */
void queue_delnode(queue_t* queue, node_t *node) {
  if (queue->lru == node)
    queue->lru = node_arrow(node, QUEUE, LEFT); // Actualiza el lru
  if (queue->mru == node)
    queue->mru = node_arrow(node, QUEUE, RIGHT); //Actualiza mru
  node_discc(QUEUE, node); //No hace free 
}

/* Tested */
/*Elimina el lru y lo retorna*/
node_t* queue_dqlru(queue_t* queue) {
  if (!queue->lru)
    return NULL;

  node_t* temp = queue->lru;
  queue_delnode(queue, queue->lru);

  return temp;
}

int queue_test(node_t* node, dir_t dir) {
  if (!node)
      return 0;
  return 1 + queue_test(node->arrows[QUEUE + dir], dir);
}
