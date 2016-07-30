/* 
 *  A server using select()
 *  by Martin Broadhurst (www.martinbroadhurst.com)
 */

#include <stdio.h>
#include <string.h> /* memset() */
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <time.h> 

#define MY_PORT    8000 /* Port to listen on */
#define BACKLOG     10  /* Passed to listen() */
#define MAXBUF		1024

void handle(int newsock, fd_set *set)
{
    /* send(), recv(), close() */
    /* Call FD_CLR(newsock, set) on disconnection */
}

int main(void)
{
    int sock;
    fd_set socks;
    fd_set readsocks;
    int maxsock;
    int reuseaddr = 1; /* True */
	
	struct sockaddr_in self;
        /*---Initialize address/port structure---*/
	bzero(&self, sizeof(self));
	self.sin_family = AF_INET;
	self.sin_port = htons(MY_PORT);
	self.sin_addr.s_addr = INADDR_ANY;

    /* Create the socket */
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        perror("socket");
        return 1;
    }

    /* Enable the socket to reuse the address */
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &reuseaddr, sizeof(int)) == -1) {
        perror("setsockopt");
        return 1;
    }

    /* Bind to the address */
    if (bind(sock, (struct sockaddr*)&self, sizeof(self)) == -1) {
        perror("bind");
        return 1;
    }

    /* Listen */
    if (listen(sock, BACKLOG) == -1) {
        perror("listen");
        return 1;
    }

    /* Set up the fd_set */
    FD_ZERO(&socks);
    FD_SET(sock, &socks);
    maxsock = sock;

    printf("entering main loop (pid = %d)\n", getpid()); 

    /* Main loop */
    while (1) {
        unsigned int s;
        readsocks = socks;
        if (select(maxsock + 1, &readsocks, NULL, NULL, NULL) == -1) {
            printf("select error (pid = %d) \n", getpid());  
            perror("select");
            return 1;
        }
        printf("select success (pid = %d) \n", getpid()); 
        for (s = 0; s <= maxsock; s++) {
            if (FD_ISSET(s, &readsocks)) {
                printf("socket %d was ready (pid = %d)\n", s, getpid());
                if (s == sock) {
                    /* New connection */
                    int newsock;
                    struct sockaddr_in their_addr;
                    int size = sizeof(struct sockaddr_in);
                    newsock = accept(sock, (struct sockaddr*)&their_addr, &size);
                    if (newsock == -1) {
                        perror("accept");
                    }
                    else {
                        printf("Got a connection from %s on port %d (pid = %d)\n", 
                                inet_ntoa(their_addr.sin_addr), htons(their_addr.sin_port), getpid());
                        FD_SET(newsock, &socks);
                        if (newsock > maxsock) {
                            maxsock = newsock;
                        }
                    }
                }
                else {
	            char buffer[MAXBUF];
		    /*---Echo back anything sent---*/
		    send(s, buffer, recv(s, buffer, MAXBUF, 0), 0);
	            printf("data sent, close next (pid = %d)\n", getpid()); 
		    /*---Close data connection---*/
		    close(s);
                }
            }
        }
        sleep(1); 

    }

    close(sock);

    return 0;
}

