#include "memc.h"
#include "memc_table.h"
#include "utils.h"
#include <stdio.h>
#include <assert.h>
#include <sys/resource.h>
#include <unistd.h>

#define MEMORY 10

int main(){
    memc_t mem = memc_init((HasFunc)hash_len, INITIAL_BUCKETS, SHIELD_AMOUNT, MEMORY);
    memc_destroy(mem);
    puts("Holamundo");
    mem = memc_init((HasFunc)hash_len, INITIAL_BUCKETS, SHIELD_AMOUNT, MEMORY);

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
    
    for (int i = 0; i < 12; i++) {
        memc_put(mem, claves[i], datos[i], len_cla[i], len_dat[i], TEXTO);
    }


    char* para_imprimir;

    memc_get(mem, "como", 5, (void**)&para_imprimir, TEXTO);

    printf("%s-\n",para_imprimir);

    if (!memc_get(mem, "todo", 5, (void**)&para_imprimir, TEXTO))
        puts("Holanda");

    printf("%s-\n",para_imprimir);

    memc_get(mem, "como", 5, (void**)&para_imprimir, TEXTO);

    printf("%s-\n",para_imprimir);

    free(para_imprimir);


    memc_del(mem, "todo", 5, TEXTO);

    if (!memc_get(mem, "todo", 5, (void**)&para_imprimir, TEXTO))
        puts("Holanda");

    stats_t st = memc_stats(mem);
    printf("%lu\n",st->keys);
    printf("%lu\n",st->puts);
    printf("%lu\n",st->gets);
    printf("%lu\n",st->dels);
    free(st);

    memc_destroy(mem);
    return 0;
}