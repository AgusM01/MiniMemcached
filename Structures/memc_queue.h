#ifndef __MEMC__QUEUE__
#define __MEMC__QUEUE__
#include "memc_node.h"

//struct __MemcQueue;
struct __MemcQueue ;
typedef struct __MemcQueue queue_t;

/* Inicializa la estructura*/
queue_t* queue_init();

/* Destructor de la estructura*/
void queue_destroy(queue_t* queue);

/* Agrega un nodo a la queue*/
void queue_addmru(queue_t *queue, node_t *node);

/* Elimina un nodo de la queue (independientemente de donde este)*/
void queue_delnode(queue_t* queue, node_t *node);

/* función pop de la queue, retorna el menos usado*/
node_t* queue_dqlru(queue_t* queue);

#endif

