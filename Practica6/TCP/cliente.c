#define _GNU_SOURCE

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define BUF_SIZE 500

int main(int argc, char *argv[])
{
    struct addrinfo hints;
    struct addrinfo *result, *rp;
    int sfd, s;
    int finished = 0;
    ssize_t len;
    ssize_t nread;
    char buf[BUF_SIZE];

    if (argc < 3) {
        fprintf(stderr, "Usage: %s host port msg...\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    /* Obtain address(es) matching host/port */

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET6;        /* Allow IPv4 or IPv6 */
    hints.ai_socktype = SOCK_STREAM;    /* Stream socket */
    hints.ai_flags = 0;
    hints.ai_protocol = 0;            /* Any protocol */

    s = getaddrinfo(argv[1], argv[2], &hints, &result);
    if (s != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
        exit(EXIT_FAILURE);
    }

    /* getaddrinfo() returns a list of address structures.
       Try each address until we successfully connect(2).
       If socket(2) (or connect(2)) fails, we (close the socket
       and) try the next address. */

    for (rp = result; rp != NULL; rp = rp->ai_next) {
        sfd = socket(rp->ai_family, rp->ai_socktype,
                     rp->ai_protocol);
        if (sfd == -1)
            continue;

        if (connect(sfd, rp->ai_addr, rp->ai_addrlen) != -1)
            break;                  /* Success */

        close(sfd);
    }

    if (rp == NULL) {               /* No address succeeded */
        fprintf(stderr, "Could not connect\n");
        exit(EXIT_FAILURE);
    }

    freeaddrinfo(result);           /* No longer needed */

    while (!finished){
        printf("Introduce el comando: ");
        fscanf(stdin, "%s", buf);

        len = (ssize_t)strlen(buf) + 1; /* +1 for terminating null byte */

        if (len + 1 > BUF_SIZE) {
            fprintf(stderr, "Ignoring long command");
            continue;
        }

        if (sendto(sfd, buf, (size_t)len, 0, rp->ai_addr, rp->ai_addrlen) != len) {
            fprintf(stderr, "partial/failed write\n");
            exit(EXIT_FAILURE);
        }

        memset(buf, 0, BUF_SIZE);
        nread = recvfrom(sfd, buf, BUF_SIZE, 0, rp->ai_addr, &rp->ai_addrlen);
        if (nread == -1) {
            perror("read");
            exit(EXIT_FAILURE);
        }
        else if (nread == 0 || strcmp(buf, "q") == 0){
            finished = 1;
        }
        else{
            printf("Received %zd bytes: %s\n", nread, buf);
        }
    }

    exit(EXIT_SUCCESS);
}