#include "memc_node.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "memc_queue.h"

void imprimr_nodo(node_t* node) {
    char* buff = NULL; // malloc(sizeof(char) * 10);
    int n = 0;

    n = node_getdata(node, (void**)&buff);

    printf("Data:%s.\nLen:%d.\n", buff,n);

    n = node_getkey(node, (void**)&buff);

    printf("Key:%s.\nLen:%d.\n", buff,n);
}

int main() {
    char texto[10] = "Holatengo";
    char clave[4]  = "key";

    node_t* nodo = node_init(clave, texto, 4, 10, 1);
    assert(nodo);

    node_free(nodo);

    queue_t* cola = queue_init();

    node_t* nodes[10];
        
    char* buff = NULL;

    node_t* temp;

    for (int i = 0; i < 10; i++) {
        nodes[i] = node_init("node0","a",6,2,1);
        queue_addmru(cola, nodes[i]);
        node_getdata(nodes[i], (void**)&buff);
        buff[0] = buff[0] + i;
        node_getkey(nodes[i],(void**)&buff);
        buff[4] = buff[4] + i;
    }

    queue_delnode(nodes[5]);
    node_free(nodes[5]);

    while((temp = queue_dqlru(cola))) {
        if (queue_empty(cola))
            puts("Vacía");
        imprimr_nodo(temp);
        node_free(temp);
    }

    queue_destroy(cola);

    
    return 0;
}
