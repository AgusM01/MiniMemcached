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
  struct __MemcNode*     arrows[3];
  unsigned                 key_len;
  unsigned                data_len;
  mod_t                      mode;    
};

typedef struct  __MemcNode node_t;

 
//node_t* node_init(
//    void* key,
    //void* data,
    //unsigned int len_key,
    //unsigned int len_data,
    //mod_t md
    //);

void node_free(node_t* node);

void node_frees(int hq, node_t* node);

node_t* node_search(node_t* node, void* key, unsigned int len, mod_t hq);

node_t* node_arrow(node_t* node, int hq, dir_t dir);

void node_discc(int hq, node_t* node);

int node_addhd(int hq, node_t* node, node_t* head);

//int node_getdata(node_t* node, void** buff);

//void node_setdata(node_t* node, void* buff, unsigned len);

//int node_getkey(node_t* node, void** buff);

//void node_setkey(node_t* node, void* buff, unsigned len);

//void node_setmode(node_t* node, mod_t mod);

//mod_t node_getmode(node_t* node);

#endif
