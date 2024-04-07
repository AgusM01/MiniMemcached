#include "utils.h"
#include <stdio.h>
#include <sys/resource.h>
#include <sys/capability.h>

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

size_t limit_mem(size_t bytes) {
	struct rlimit rlm;
	int ret = 0;
	getrlimit(RLIMIT_DATA, &rlm);
	if ( bytes <= rlm.rlim_max ) {
		rlm.rlim_cur = bytes;
	} else {
		rlm.rlim_cur = rlm.rlim_max;
	}
	ret = setrlimit(RLIMIT_DATA, &rlm);
	return ret;
 }

void drop_privileges() {
	
    cap_t caps;
    const cap_value_t cap_list[1] = {CAP_NET_BIND_SERVICE};

    caps = cap_get_proc();

    if (caps == NULL)
        puts("Holanda caps");
    
    if (cap_set_flag(caps, CAP_EFFECTIVE, 1, cap_list, CAP_CLEAR) == -1)
        puts("Holanda set flag");

    if (cap_set_flag(caps, CAP_PERMITTED, 1, cap_list, CAP_CLEAR) == -1)
        puts("Holanda set flag");

    if (cap_set_proc(caps) == -1)
        puts("Holanda set proc");

    if (cap_free(caps) == -1)
        puts("Holanda free");

}