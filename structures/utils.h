#ifndef __UTILS__
#define __UTILS__	


#include <stdlib.h>
#include <sys/resource.h>
#include <sys/time.h>

/*
 * Funciones hash
 */

size_t limit_mem(size_t bytes);

unsigned hash_len(char* wrd, int len);

unsigned hash(char* wrd);


#endif
