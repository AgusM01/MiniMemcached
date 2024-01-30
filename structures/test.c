#include "memc_node.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "memc_queue.h"
#include "memc_table.h"

void imprimr_nodo(node_t* node) {
    char* buff = NULL; // malloc(sizeof(char) * 10);
    int n = 0;

    n = node_getdata(node, (void**)&buff);

    printf("Data:%s.\nLen:%d.\n", buff,n);

    n = node_getkey(node, (void**)&buff);

    printf("Key:%s.\nLen:%d.\n", buff,n);
}

/*int queue_test() {
    char texto[10] = "Holatengo";
    char clave[4]  = "key";

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
}*/

int main(){

    node_t* nodes[10];
        
    char key[6] = "Node0";

    char data[2] = "a";

    int buckets = 10; 

    table_t tab = table_init(10);

    //char* buff = NULL;

    node_t* temp = NULL;
    
    unsigned j = 0;

    for (int i = 0; i < 10; i++) {
        nodes[i] = node_init(key,data,6,2,1);
        j = hash_len(key, 5) % buckets;
        imprimr_nodo(nodes[i]);
        printf("table --> %d\n",j);
        table_insert(tab, nodes[i], 4);
        key[4]++;
        key[3]++;
        data[0]++;
    }

    assert(!table_search(tab, "Nodi4", 6, 4,&temp));
    imprimr_nodo(temp);

    table_rehash(&tab, buckets, buckets * 2, (HasFunc) hash_len);

    for (int i = 0; i < buckets * 2; i++) {
        if (tab[i])
            imprimr_nodo(tab[i]);
    }

    table_destroy(tab,buckets * 2);

    return 0;
}