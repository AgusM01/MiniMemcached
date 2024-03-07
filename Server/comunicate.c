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

int recv_mem(struct args_epoll_monitor* e_m_struct, struct epoll_event* evlist, void* buf, size_t len, int flags){
    
    int rcv;
    int flag = 0;

    struct data_ptr* ptr;
    ptr = CAST_DATA_PTR;
    

    rcv = recv(ptr->fd, buf, len, flags);
    if (!rcv) 
        perror("Recieve 1");

    if (rcv == 0 || (rcv == -1 && (errno == EWOULDBLOCK || errno == EAGAIN)))
        return 0;
    
    if (rcv == -1 && (errno != EWOULDBLOCK && errno != EAGAIN && errno != ENOMEM))            
        return -1;
    
    if (rcv == -1 && errno == ENOMEM)
        flag = 1;
    

    //if (rcv <= 0){
    //        if(rcv == -1 && (errno != EWOULDBLOCK && errno != EAGAIN && errno != ENOMEM))            
    //            return -1;
    //    return 0;    
    //}

    while (flag){

        flag = 0;

        memc_eviction(e_m_struct->mem);
        
        rcv = recv(ptr->fd, buf, len, flags);
        
        if (rcv == 0 || (rcv == -1 && (errno == EWOULDBLOCK || errno == EAGAIN)))
        return 0;
    
        if (rcv == -1 && (errno != EWOULDBLOCK && errno != EAGAIN && errno != ENOMEM))            
            return -1;

        if (rcv == -1 && errno == ENOMEM)
            flag = 1;

    }
    
    return rcv;
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