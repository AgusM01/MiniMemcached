#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <math.h>
#include "server.h"
#include "text_parser.h"
#include "structures/memc.h"
#include "structures/utils.h"
#include <sys/types.h>
#include <sys/wait.h>

int main(){
    
    int sock = socket(AF_INET, SOCK_STREAM, 0);

    /*Instancia la estructura*/
    struct sockaddr_in sa;
    sa.sin_family = AF_INET;
    sa.sin_port = htons(8889);
    sa.sin_addr.s_addr = htonl(INADDR_ANY);

    connect(sock, (struct sockaddr*) &sa, sizeof(sa));
    
    char get = 13;
    char put = 11;
    char del = 12;
    char stats = 21;
    char s1 = 0;
    char s2 = 0;
    char s3 = 0;
    char s4 = 4;
    char snd[9];
    
    snd[0] = put;
    //snd[1] = s1;
    //snd[2] = s2;
    //snd[3] = s3;
    //snd[4] = s4;
    //snd[5] = 'h';
    //snd[6] = 'o';
    //snd[7] = 'l';
    //snd[8] = 'a';
    //snd[9] = 0;
    //snd[10] = 0;
    //snd[11] = 0;
    //snd[12] = 7;
    //snd[13] = 'a';
    //snd[14] = 'g';
    //snd[15] = 'u';
    //snd[16] = 's';
    //snd[17] = 't';
    //snd[18] = 'i';
    //snd[19] = 'n';

    char buf[130];
    buf[129] = '\0';
    int s = send(sock,snd, 1, 0);
    assert(s != -1);
    sleep(3);
    //s = send(sock,snd + 7, 2, 0);
    //sleep(3);
    s = read(sock, &buf, 130);
    printf("s: %d\n", s);
    printf("buf[0]: %d\n", buf[0]);
    printf("%s\n", buf + 5);
    close(sock);
    return 0;
}