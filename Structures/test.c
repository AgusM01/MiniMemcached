#include "memc.h"
#include "memc_node.h"
#include "memc_table.h"
#include "memc_queue.h"
#include "utils.h"
#include <stdio.h>
#include <assert.h>
#include <pthread.h>    
#include <sys/resource.h>
#include <unistd.h>

#define MEMORY 10

char *claves[12] = { "hola",    "como", "estas", "todo",
                    "bien",    "por",  "casa",  "como",
                    "andamos", "perri", "todo", "piola"};

int len_cla[12] = { 5, 5, 6, 5,
                    5, 4, 5, 5, 
                    8, 7, 5, 6};

char* datos[12] = { "Ule", "Dani", "caballo", "pata",
                    "central", "auriculares", "helado", "muelle",
                    "notas adhesivas", "pandereta", "messi", "lampara de lava"};

int len_dat[12] = { 5, 5, 8, 5,
                    8, 12, 7, 7, 
                    16, 11, 6, 16};

void* test_puts(void* args) {
    for(;;) {
    for (int i = 0; i < 1; i++) {
        printf("Pongo %s.\n", claves[i]);
        memc_put((memc_t) args, claves[i], datos[i], len_cla[i], len_dat[i], TEXTO);
    }
    }
}

void* test_get(void* args) {
    for(;;) {
    char* imprimible = NULL;

    int einval;
    for (int i = 0; i < 1; i++) {
        einval = memc_get((memc_t) args,
                          claves[i],
                          len_cla[i],
                          (void**)&imprimible,
                          TEXTO
                 );
        if (einval == 0)
            printf("No se encontro %s.\n", claves[i]);
        else {
            printf(" Clave : %s.\n Dato : %s.\n", claves[i], imprimible);
            free(imprimible);
        }
    }
    }
}


void* test_dels(void* args) {
    for(;;) {
    int einval;
    for (int i = 0; i < 1; i++) {
        einval = memc_del((memc_t) args, claves[i], len_cla[i], TEXTO);
        if (einval == 0)
            printf("Se elimino %s\n", claves[i]);
        else
            printf("No se pudo eliminar: %s\n", claves[i]);
    }
    }
}

int main(){
    memc_t mem = memc_init((HasFunc)hash_len, 1000000, SHIELD_AMOUNT);

    printf("buckets -> %d.\n", mem->buckets);
    printf("shield -> %d.\n", mem->shield_size);
    pthread_t t[3];


    // for (int i = 0; i < 12; i++) {
    //     unsigned asdf = (hash_len(claves[i], len_cla[i]) % 1000000);
    //     printf("Clave : %s , Indice : %d, Shield : %d\n",claves[i], asdf , asdf % 500);
    // }

    // fflush(stdout);

//    sleep(10);
    pthread_create(&t[0], NULL, test_puts, (void*)mem);
    pthread_create(&t[1], NULL, test_get, (void*)mem);
    pthread_create(&t[2], NULL, test_dels, (void*)mem);
    pthread_join(t[0], NULL);
    pthread_join(t[1], NULL);
    pthread_join(t[2], NULL);



    return 0;
}