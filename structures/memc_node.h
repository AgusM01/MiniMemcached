#ifndef __MEMC__NODE__
#define __MEMC__NODE__

#define HASH 0
#define QUEUE 2

enum Dir {
  LEFT, RIGHT
}

typedef char mode_t;

typedef enum dir dir_t;

struct _MemcNode {
  void*                   key_buff;
  void*                  data_buff;
  str __MemcNode*        arrows[4];
  unsigned int             key_len;
  unsigned int            data_len;
  mode_t                      mode;    
};

typedef struct  __MemcNode node_t;

node_t* node_init();

void node_free(int hq, node_t* node);

node_t node_search(node_t* node, void* key, unsigned int len);

int node_getdata(node_t* node, void* buff);

void node_setdata(node_t* node, void* buff, size_t len);

int node_getkey(node_t* node; void* buff);

void node_setkey(node_t* node, void* buff, size_t len);

void node_setmode(node_t* node, mode_t mod);

mode_t node_getmode(node_t* node);

node_t* node_arrow(node_t* node, int hq, dir_t dir);

#endif
