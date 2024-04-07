#include "utils.h"
#include <stdio.h>
#include <string.h>
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

static inline uint32_t murmur_32_scramble(uint32_t k) {
    k *= 0xcc9e2d51;
    k = (k << 15) | (k >> 17);
    k *= 0x1b873593;
    return k;
}

uint32_t murmur3_32(const uint8_t* key, size_t len)
{
	uint32_t h = SEED;
    uint32_t k;
    /* Read in groups of 4. */
    for (size_t i = len >> 2; i; i--) {
        // Here is a source of differing results across endiannesses.
        // A swap here has no effects on hash properties though.
        memcpy(&k, key, sizeof(uint32_t));
        key += sizeof(uint32_t);
        h ^= murmur_32_scramble(k);
        h = (h << 13) | (h >> 19);
        h = h * 5 + 0xe6546b64;
    }
    /* Read the rest. */
    k = 0;
    for (size_t i = len & 3; i; i--) {
        k <<= 8;
        k |= key[i - 1];
    }
    // A swap is *not* necessary here because the preceding loop already
    // places the low bytes in the low places according to whatever endianness
    // we use. Swaps only apply when the memory is copied in a chunk.
    h ^= murmur_32_scramble(k);
    /* Finalize. */
	h ^= len;
	h ^= h >> 16;
	h *= 0x85ebca6b;
	h ^= h >> 13;
	h *= 0xc2b2ae35;
	h ^= h >> 16;
	return h;
}