#ifndef __UTILS__
#define __UTILS__	

#define SEED 102938745

#include <stdlib.h>
#include <stdint.h>
#include <sys/resource.h>
#include <sys/time.h>

/*
 * Funciones hash
 */

size_t limit_mem(size_t bytes);

void drop_privileges();

unsigned hash_len(char* wrd, int len);

unsigned hash(char* wrd);

uint32_t murmur3_32(const uint8_t* key, size_t len);

#endif
