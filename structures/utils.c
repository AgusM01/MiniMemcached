#include "utils.h"
#include <stdio.h>
#include <sys/resource.h>

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

size_t limit_mem(size_t bytes, int flag) {
	struct rlimit rlm;
	int ret = 0;
	getrlimit(flag, &rlm);
	if ( bytes <= rlm.rlim_max ) {
		rlm.rlim_cur = bytes;
	} else {
		rlm.rlim_cur = rlm.rlim_max;
	}
	ret = setrlimit(flag, &rlm);
	return ret;
 }