#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <errno.h>
#include "comunicate.h"

#define MAX_CHAR 2048
#define CAST_DATA_PTR ((struct data_ptr*)evlist->data.ptr)
#define CAST_DATA_PTR_BINARY CAST_DATA_PTR->binary

int writen(int fd, const void *buf, int len)
{
	int i = 0, rc;

	while (i < len) {

		int chunk = len - i;
		rc = write(fd, buf + i, chunk);
		if (rc <= 0)
			return -1;
		i += rc;
    }
    return 0;
}

int my_length(char* buf){
    int i = 0;
    while(buf[i] != '\0'){
        if (buf[i] < 32 || buf[i] > 126)
            return -1;
        i++;
    }
    return i;
}
