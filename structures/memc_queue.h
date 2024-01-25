#ifndef __MEMC__QUEUE__
#define __MEMC__QUEUE__

#include "memc_node.h"

struct _MemcQueue {
 node_t* mru, lru;
};

typedef struct _MemcQueue queue_t;

queue_t* queue_init();

void queue_destroy(queue_t* queue);

void queue_addmru(queue_t *queue, node_t *node);

void queue_delnode(node_t *node);

node_t* queue_dqlru(queue_t* queue);

#endif

