#ifndef __MEMC__NODE__
#define __MEMC__NODE__

enum ArrowMode {
  HASH_N,
  HASH_P,
  QUEUE_N,
  QUEUE_P
};

typedef enum StructMode arrow_flag;

struct _MemcNode {
  size_t          key_len;
  size_t          data_len;
  void*           key_buff;
  void*           data_buff;
  str __MemcNode  arrows[4];
  int             mode;    
};

typedef struct  __MemcNode node_t;

node_t node_init();

node_free(node_t* node);

#endif
