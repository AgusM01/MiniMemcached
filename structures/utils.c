#include "utils.h"
#include <stdio.h>

unsigned hash_len(char* wrd, int len) {
	unsigned hashval = 0;
	for (int i = 0; i < len; i++)
		hashval += wrd[i] + 31 * hashval;
	return hashval;
}

unsigned hash(char* wrd){ 
	unsigned hashval = 0;
	for (char* s = wrd; *s != 0; s++) {
		hashval += *s + 31 * hashval;
	}
	return hashval;
}
