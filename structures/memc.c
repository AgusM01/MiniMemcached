#include "memc.h"
#include <assert.h>
#include "lightswitch.h"
#include "memc_table.h"
#include "memc_queue.h"
#include "memc_node.h"
#include "memc_stat.h"
#include "utils.h"
#include <stdlib.h>
#include <string.h>
#include <semaphore.h>
#include <stdio.h>



struct MemCache {
    //Estadisticas de la cache.
    stat_t* puts;
    stat_t* gets;
    stat_t* dels;
    stat_t* keys;

    //Variables para la Tabla Hash.
    table_t tab;
    unsigned buckets;
    HasFunc hash;

    //Variables para la Priority Queue
    queue_t* queue;

    //Variables para sincronización
    ls_t* evic;           //---> Estructura para el LightSwitch
    ls_t* rhsh;           //---> Estructura para el LightSwitch

    sem_t* evic_mutex;    //---> Mutex para el LightSwitch
    sem_t* rehash_mutex;  //---> Mutex para rehash
    sem_t* queue_mutex;   //---> Mutex para la Queue
    sem_t* turnstile;     //---> Mutex para funciones de rehash y memoria. 

    //sem_t* memory_mutex;  //---> Mutex para manejo de memoria
    sem_t** tab_shield;   //---> tabla de mutex para regiones en la tabla
    int shield_size;      //---> Cantidad de mutex

};

memc_t memc_init(
        HasFunc hash,
        unsigned  tab_size,
        unsigned shield_size,
        unsigned memory_limit
        ) {


    memc_t mem;
    //assert(!limit_mem(memory_limit, RLIMIT_MEMLOCK));
    assert((mem = malloc(sizeof(struct MemCache))));

    mem->dels = stat_init(0);
    mem->puts = stat_init(0);
    mem->gets = stat_init(0);
    mem->keys = stat_init(0);

    mem->tab = table_init(tab_size);
    mem->buckets = INITIAL_BUCKETS;
    mem->hash = hash;

    mem->queue = queue_init();

    mem->evic = ls_init();
    mem->rhsh = ls_init();

    assert((mem->evic_mutex = malloc(sizeof(sem_t))));
    assert((mem->rehash_mutex = malloc(sizeof(sem_t))));
    assert((mem->queue_mutex = malloc(sizeof(sem_t))));
    assert((mem->turnstile = malloc(sizeof(sem_t))));

    sem_init(mem->evic_mutex, 0, 1);
    sem_init(mem->rehash_mutex, 0, 1);
    sem_init(mem->queue_mutex, 0, 1);
    sem_init(mem->turnstile, 0, 1);

    mem->tab_shield = malloc(sizeof(sem_t*) * shield_size);
    mem->shield_size = shield_size;

    for (int i = 0; i < shield_size; i++) {
        assert((mem->tab_shield[i] = malloc(sizeof(sem_t))));
        sem_init(mem->tab_shield[i], 0, 1);
    }

    return mem;
}

void memc_destroy(memc_t mem) {
    stat_destroy(mem->dels);
    stat_destroy(mem->puts);
    stat_destroy(mem->gets);
    stat_destroy(mem->keys);

    table_destroy(mem->tab, mem->buckets);

    queue_destroy(mem->queue);

    ls_destroy(mem->evic);
    ls_destroy(mem->rhsh);

    sem_destroy(mem->evic_mutex);
    sem_destroy(mem->rehash_mutex);
    sem_destroy(mem->queue_mutex);
    sem_destroy(mem->turnstile);

    free(mem->evic_mutex);
    free(mem->rehash_mutex);
    free(mem->queue_mutex);
    free(mem->turnstile);

    for (int i = 0; i < mem->shield_size; i++) {
        sem_destroy(mem->tab_shield[i]);
        free(mem->tab_shield[i]);
    }

    free(mem->tab_shield);
    free(mem);
}

// Setea los datos de un nuevo nodo.
// TODO -> Manejar errores me memoria con función auxiliar.
void node_set(
    node_t*             newnode,
    void*                   key,
    void*                  data,
    unsigned            len_key,
    unsigned           len_data,
    mod_t                   md
    ) {

  newnode->data_buff = data;
  newnode->data_len = len_data;
  newnode->key_buff = key;
  newnode->key_len = len_key;
  newnode->mode = md;
  for(int i = 0; i < 4 ; i++) 
    newnode->arrows[i] = NULL;
}

//Lock para requerir memoria
//Bloquea los nuevos threads
// 
void memc_lock(memc_t mem) {
    sem_wait(mem->turnstile);
    sem_wait(mem->evic_mutex);
}

void memc_unlock(memc_t mem) {
    sem_post(mem->turnstile);
    sem_post(mem->evic_mutex);
}

int memc_eviction(memc_t mem, size_t bytes) {

    int count = 0;
    size_t memory = 0; //Lleva control de la memoria liberada.
    node_t* tbd;       //Nodo To Be Destroy 

    // Mientras la queue tenga nodos para liberar y le memoria
    // liberada sea menor a la requerida.
    while(memory < bytes && (tbd = queue_dqlru(mem->queue))) {
        memory += sizeof(node_t) + tbd->data_len + tbd->key_len;
        node_free(tbd);
        count++;
    }

    //Si tbd es NULL, no hay memoria en el sistema, por lo que devolvemos -1;
    return tbd ? count : -1; 
}

void* memc_alloc(memc_t mem, size_t bytes) {
    void* ret;
    int keys;

    //sem_wait(mem->memory_mutex);
    if (!(ret = malloc(bytes))) { // ret == NULL
        memc_lock(mem);
        // REGIÓN CRÍTICA 
        if((keys = memc_eviction(mem, bytes)) == -1)
            assert(0);
        
        stat_add(mem->keys, -keys);
        memc_unlock(mem);
        ret = malloc(bytes);
    }
    //sem_post(mem->memory_mutex);
    return ret;
}

void memc_rehash(memc_t mem){

    //stat_lock(mem->keys); // TODO -> Hace falta ?
        //Se tiene que dar que justo la condición, capaz que 
        //Cunado terminan el resto de threas la desigualdad no vale
        //Pero nos basta con que en un instante se supere el factor
        //Compara poder rehashear.
    if ((100 * stat_get(mem->keys)) >= (75 * mem->buckets)) {
        //Liberamos las keyS para que terminene el resto de procesos
        //   stat_unlock(mem->keys);

        unsigned new_size = mem->buckets * 2;
        table_t new_tab = memc_alloc(mem, sizeof(node_t*) * new_size);

        for (int i = 0; i < new_size; i++) {
            new_tab[i] = NULL;
        }

        //Frenamos futuros threads
        sem_wait(mem->turnstile);
        //Esperamos que todos los trhead dentro de la ls terminen.
        sem_wait(mem->rehash_mutex);
        //Rehash
        table_rehash(
            mem->tab,
            new_tab,
            mem->buckets,
            new_size,
            mem->hash
        );
        //free(mem->tab);
        mem->tab = new_tab;
        mem->buckets = new_size;
        //Liberamos threads y liberamos la ls
        sem_post(mem->rehash_mutex);
        sem_post(mem->turnstile);
    } //else {
      //  stat_unlock(mem->keys);
    //}
}

int memc_put(
    memc_t mem,
    void *key,
    void *data,
    unsigned int key_len,
    unsigned int data_len,
    mode_t md
    ) {

    memc_rehash(mem);

    //Turnstile
    sem_wait(mem->turnstile);
    sem_post(mem->turnstile);

    //LightSwitch On -> rehash
    ls_lock(mem->rhsh, mem->rehash_mutex);

    int search;
    unsigned i = mem->hash(key,key_len);
    unsigned tab_index = i % mem->buckets;
    unsigned shield_index = i % mem->shield_size;
    node_t* temp = NULL;

    void* buff_data = memc_alloc(mem, data_len);
    memcpy(buff_data, data, data_len);

    //Mutex Hash
    sem_wait(mem->tab_shield[shield_index]);

    //LightSwitch On -> alloc
    ls_lock(mem->evic, mem->evic_mutex);

    //REGION CRÍTICA HASH

    //Busqueda de key ya existente
    if (!(search = table_search(mem->tab, key, key_len, tab_index, &temp))) {
        //Key encotrada, remplazo del Dato
        free(temp->data_buff);
        temp->data_buff = buff_data;
        temp->data_len = data_len;
        temp->mode = md;
    } else {
        //LightSwitch Off -> alloc
        ls_unlock(mem->evic, mem->evic_mutex);  

        //Alocamos memoria para crear el nodo;
        temp = memc_alloc(mem, sizeof(node_t));
        void* buff_key = memc_alloc(mem, key_len);

        //LightSwitch On -> alloc
        ls_lock(mem->evic, mem->evic_mutex);

        //Terminamos de armar el nodo y lo agregamos a la tabla
        memcpy(buff_key, key, key_len);
        node_set(temp, buff_key, buff_data, key_len, data_len, md);
        table_insert(mem->tab, temp, tab_index);
        stat_inc(mem->keys);
    }

    //Damos prioridad al Nodo en la Queue
    sem_wait(mem->queue_mutex);
    if (!search) // Hay un nodo ya existente en la Queue
        node_discc(QUEUE, temp);
    queue_addmru(mem->queue, temp);
    sem_post(mem->queue_mutex);

    //FIN REGION CRITICA HASH
    sem_post(mem->tab_shield[shield_index]);

    //LightSwitch Off -> Both 
    ls_unlock(mem->evic, mem->evic_mutex);
    ls_unlock(mem->rhsh, mem->rehash_mutex);

    stat_inc(mem->puts);

    return 0;
}

int memc_get(memc_t mem, void *key, unsigned key_len, void **data_buff, mode_t md) {

    //Turnstile
    sem_wait(mem->turnstile);
    sem_post(mem->turnstile);
    ls_lock(mem->rhsh, mem->rehash_mutex);
    
    unsigned i = mem->hash(key,key_len);
    unsigned tab_index = i % mem->buckets;
    unsigned shield_index = i % mem->shield_size;
    unsigned len = 0;
    node_t* temp;

    sem_wait(mem->tab_shield[shield_index]);

    //LightSwitch
    ls_lock(mem->evic, mem->evic_mutex); 

    //REGION CRTITICA
    if (table_search(mem->tab, key, key_len, tab_index, &temp) == 0) {


        if (md == TEXTO && temp->mode == BINARIO ) {
            len = -1;
            sem_post(mem->tab_shield[shield_index]);
        } else {

            sem_wait(mem->queue_mutex);
            //Retiramos el nodo de la Queue para que no
            //sea eliminado en el desalojo.
            node_discc(QUEUE, temp); 
            sem_post(mem->queue_mutex);

            len = temp->data_len;

            ls_unlock(mem->evic, mem->evic_mutex);
            //Pedimos memoria para devolver el dato
            *data_buff = memc_alloc(mem,len);
            ls_lock(mem->evic, mem->evic_mutex);

            memcpy(*data_buff, temp->data_buff, len);

            sem_wait(mem->queue_mutex);
            //Agregamos el nodo al principio de la queue
            queue_addmru(mem->queue, temp); 
            sem_post(mem->queue_mutex);

            //Buffer copiado, liberamos el mutex.
            sem_post(mem->tab_shield[shield_index]);
        }
    } else {
        //No se encontró el nodo, liberamos el mutex
        sem_post(mem->tab_shield[shield_index]);
    }

    ls_unlock(mem->evic, mem->evic_mutex);
    ls_unlock(mem->rhsh, mem->rehash_mutex);

    stat_inc(mem->gets);
    return len;
}


int memc_del(memc_t mem, void *key, unsigned key_len, mod_t md) {

    //Turnstile
    sem_wait(mem->turnstile);
    sem_post(mem->turnstile);

    //LightSwitch -> Rehash
    ls_lock(mem->rhsh, mem->rehash_mutex);
    
    unsigned i = mem->hash(key,key_len);
    unsigned tab_index = i % mem->buckets;
    unsigned shield_index = i % mem->shield_size;
    node_t* temp = NULL;

    sem_wait(mem->tab_shield[shield_index]);

    //LightSwitch -> alloc
    ls_lock(mem->evic, mem->evic_mutex);
    if (table_search(mem->tab, key, key_len, tab_index, &temp)) {
        // No se encontró la key, liberamos los mutex
        sem_post(mem->tab_shield[shield_index]);
        ls_unlock(mem->evic, mem->evic_mutex);
        ls_unlock(mem->rhsh, mem->rehash_mutex);
        stat_inc(mem->dels);
        return -1;
    }

    //Encontramos el Nodo.

    //Desconectamos de las estructuras y liberamos
    node_discc(HASH ,temp);
    stat_dec(mem->keys);
    sem_wait(mem->queue_mutex);
    node_discc(QUEUE ,temp);
    sem_post(mem->queue_mutex);
    sem_post(mem->tab_shield[shield_index]);
    node_free(temp);
    
    if (temp == mem->tab[tab_index])
        mem->tab[tab_index] = NULL;

    //LightSwitch Off -> Both
    ls_unlock(mem->evic, mem->evic_mutex);
    ls_unlock(mem->rhsh, mem->rehash_mutex);

    stat_inc(mem->dels);

    return 0;
}

//Xd -> Cambiar la onda, tal vez con el buffer ya armado.
stats_t memc_stats(memc_t mem) {
    stats_t st = memc_alloc(mem, sizeof(struct Stats));
    stat_lock(mem->gets);
    stat_lock(mem->puts);
    stat_lock(mem->dels);
    stat_lock(mem->keys);
    st->gets = stat_get(mem->gets);
    st->puts = stat_get(mem->puts);
    st->dels = stat_get(mem->dels);
    st->keys = stat_get(mem->keys);
    stat_unlock(mem->gets);
    stat_unlock(mem->puts);
    stat_unlock(mem->dels);
    stat_unlock(mem->keys);
    return st;
}