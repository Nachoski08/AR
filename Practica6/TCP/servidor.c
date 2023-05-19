#define _GNU_SOURCE

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netdb.h>
#include <time.h>

#define BUF_SIZE 500

int procesaOrden(char* buf, ssize_t *nread);
char* getTime();
char* getDate();

int main(int argc, char *argv[])
{
    struct addrinfo hints;
    struct addrinfo *result, *rp;
    int sfd, s;
    int finished = 0;
    struct sockaddr_storage peer_addr;
    socklen_t peer_addr_len;
    ssize_t nread;
    char* buf;
    char host[NI_MAXHOST];
    char service[NI_MAXSERV];

    if (argc != 2) {
        fprintf(stderr, "Usage: %s port\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET6;        /* Allow IPv4 or IPv6 */
    hints.ai_socktype = SOCK_STREAM;    /* Stream socket */
    hints.ai_flags = AI_PASSIVE;        /* For wildcard IP address */
    hints.ai_protocol = 0;            /* Any protocol */
    hints.ai_canonname = NULL;
    hints.ai_addr = NULL;
    hints.ai_next = NULL;

    s = getaddrinfo(NULL, argv[1], &hints, &result);
    if (s != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
        exit(EXIT_FAILURE);
    }

    /* getaddrinfo() returns a list of address structures.
       Try each address until we successfully bind(2).
       If socket(2) (or bind(2)) fails, we (close the socket
       and) try the next address. */

    for (rp = result; rp != NULL; rp = rp->ai_next) {
        sfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        
        if (sfd == -1) continue;

        if (bind(sfd, rp->ai_addr, rp->ai_addrlen) == 0)
            break;                  /* Success */

        close(sfd);
    }

    if (rp == NULL) {               /* No address succeeded */
        fprintf(stderr, "Could not bind\n");
        exit(EXIT_FAILURE);
    }

    freeaddrinfo(result);           /* No longer needed */

    if (listen(sfd, 1) == -1) {
        fprintf(stderr, "Could not listen\n");
        exit(EXIT_FAILURE);
    }

    // Aceptamos la conexion (posible fork en caso de varias conexiones)
    int cfd;
    peer_addr_len = sizeof(struct sockaddr_storage);
    cfd = accept(sfd, (struct sockaddr *) &peer_addr, &peer_addr_len);
    if (cfd == -1) {
        fprintf(stderr, "Could not accept\n");
        exit(EXIT_FAILURE);
    }
    
    buf = malloc(BUF_SIZE);

    while (!finished) {
        
        nread = recvfrom(cfd, buf, BUF_SIZE, 0,
                (struct sockaddr *) &peer_addr, &peer_addr_len);

        if (nread == -1 || nread == 0) continue; // Ignore failed request

        s = getnameinfo((struct sockaddr *) &peer_addr,
                        peer_addr_len, host, NI_MAXHOST,
                        service, NI_MAXSERV, NI_NUMERICSERV);
        if (s == 0){
            printf("Received %zd bytes from %s:%s\n", nread, host, service);
        }
        else{
            fprintf(stderr, "getnameinfo: %s\n", gai_strerror(s));
        }

        finished = procesaOrden(buf, &nread);

        if (sendto(cfd, buf, (size_t)nread, 0, (struct sockaddr *) &peer_addr, peer_addr_len) != nread)
            fprintf(stderr, "Error sending response\n");
    }
}

int procesaOrden (char* buf, ssize_t *nread){
    char *s;
    int finished = 0;
    int response = 0;

    if (strcmp(buf, "h") == 0) {
        s = getTime();
        response = 1;
    }
    else if (strcmp(buf, "d") == 0) {
        s = getDate();
        response = 1;
    }
    else if (strcmp(buf, "q") == 0) {
        finished = 1;
    }
    else {
        printf("Unknown command: %s\n", buf);
    }

    if (response) {
        *nread = (ssize_t)strlen(s);
        memcpy(buf, s, (size_t)*nread);
        printf("Result string is \"%s\"\n", s);
        free(s);
    }

    return finished;
}

char* getTime(){
    time_t t = time(NULL);
    struct tm *tm = localtime(&t);
    char *time = malloc(30);
    strftime(time, 30, "%H:%M:%S", tm);
    return time;
}

char *getDate()
{
    time_t t = time(NULL);
    struct tm *tm = localtime(&t);
    char *date = malloc(30);
    strftime(date, 30, "%d/%m/%Y", tm);
    return date;
}