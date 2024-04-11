#ifndef __MEMC__NODE__
#define __MEMC__NODE__

#define HASH 0
#define QUEUE 2

enum Dir {
  LEFT, RIGHT
};

typedef enum Dir dir_t;

typedef char mod_t;

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
  mod_t                      mode;    
};

typedef struct  __MemcNode node_t;


void node_free(node_t* node);

/* Dada una lista de HASH o QUEUE, libera los nodos */
void node_frees(int hq, node_t* node);

/* 
  Busca un nodo dependiendo de la key y retorna el puntero al nodo
  En caso de no encontrarlo retorna NULL.
*/
node_t* node_search(node_t* node, void* key, unsigned int len, mod_t hq);

/*
  Retorna las direcciones a los nodos conectados en la lista,
  dependiendo de la estructura.
*/
node_t* node_arrow(node_t* node, int hq, dir_t dir);

/*
  Toma un nodo y lo desconecta (conectando el resto de lista).
  Setea los punteros a los nodos en NULL.
*/
void node_discc(int hq, node_t* node);

/* Agrega un nodo como head de una lista*/
int node_addhd(int hq, node_t* node, node_t* head);

#endif
