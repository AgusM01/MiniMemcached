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
#include "../Server/epoll.h"
#include "../Structures/memc.h"
#include "../Structures/utils.h"
#include <sys/types.h>
#include <sys/wait.h>

int main(){
    
    int sock = socket(AF_INET, SOCK_STREAM, 0);

    /*Instancia la estructura*/
    struct sockaddr_in sa;
    sa.sin_family = AF_INET;
    sa.sin_port = htons(8888);
    sa.sin_addr.s_addr = htonl(INADDR_ANY);

    connect(sock, (struct sockaddr*) &sa, sizeof(sa));

    int readed = 0;
    char buf[100];
    char err[2049];
    for(int i = 0; i < 2049; i++)
        err[i] = 'a';
    
    send(sock, "PUT agus merino\n", 16, 0);
    readed = read(sock, &buf, 100);
    buf[readed] = '\0';
    printf("%s", buf);

    send(sock, "GET agus\n", 9, 0);
    readed = read(sock, &buf, 100);
    buf[readed] = '\0';
    printf("%s", buf);

    send(sock, "DEL agus\n", 9, 0);
    readed = read(sock, &buf, 100);
    buf[readed] = '\0';
    printf("%s", buf);

    send(sock, "STATS\n", 6, 0);
    readed = read(sock, &buf, 100);
    buf[readed] = '\0';
    printf("%s", buf);

    send(sock, "PUT luis", 8, 0);
    sleep(7);
    send(sock, " alberto\n", 9, 0);
    readed = read(sock, &buf, 100);
    buf[readed] = '\0';
    printf("%s", buf);

    send(sock, "DEL ", 4, 0);
    sleep(5);
    send(sock, "luis\n", 5, 0);
    readed = read(sock, &buf, 100);
    buf[readed] = '\0';
    printf("%s", buf);

    puts("mando primer error");
    send(sock, err, 2049, 0);
    readed = read(sock, &buf, 100);
    buf[readed] = '\0';
    printf("%s", buf);

    puts("mando segundo error 1");
    send(sock, err, 49, 0);
    sleep(5);
    puts("mando segundo error 2");
    send(sock, err, 2000, 0);
    readed = read(sock, &buf, 100);
    buf[readed] = '\0';
    printf("%s", buf);

    send(sock, "PUT luis", 8, 0);
    sleep(7);
    send(sock, " alberto\n", 9, 0);
    readed = read(sock, &buf, 100);
    buf[readed] = '\0';
    printf("%s", buf);

    send(sock, "PUT luis", 8, 0);
    sleep(7);
    send(sock, " alberto\n", 9, 0);
    readed = read(sock, &buf, 100);
    buf[readed] = '\0';
    printf("%s", buf);
    
    while(1){}
    return 0;


}