/* simple-server.c
 *
 * Copyright (c) 2000 Sean Walton and Macmillan Publishers.  Use may be in
 * whole or in part in accordance to the General Public License (GPL).
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
*/

/*****************************************************************************/
/*** simple-server.c                                                       ***/
/***                                                                       ***/
/*****************************************************************************/

/**************************************************************************
*	This is a simple echo server.  This demonstrates the steps to set up
*	a streaming server.
**************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <strings.h> 
#include <errno.h>
#include <sys/socket.h>
#include <resolv.h>
#include <arpa/inet.h>
#include <errno.h>
#include <unistd.h>
#include <string.h> 

#define MY_PORT		8000
#define MAXBUF		1024

int main(int Count, char *Strings[])
{   int sockfd;
	struct sockaddr_in self;
	char buffer[MAXBUF];

        printf("Hello from server, pid = %d \n", getpid()); 

	/*---Create streaming socket---*/
    if ( (sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0 )
	{
		perror("Socket");
		exit(errno);
	}

     /* Enable the socket to reuse the address */
     int reuseaddr = 1; // true
     if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, 
         &reuseaddr, sizeof(int)) == -1) {
        perror("setsockopt");
        return 1;
    }



	/*---Initialize address/port structure---*/
	bzero(&self, sizeof(self));
	self.sin_family = AF_INET;
	self.sin_port = htons(MY_PORT);
	self.sin_addr.s_addr = INADDR_ANY;

	/*---Assign a port number to the socket---*/
    if ( bind(sockfd, (struct sockaddr*)&self, sizeof(self)) != 0 )
	{
		perror("socket--bind");
		exit(errno);
	}

	/*---Make it a "listening socket"---*/
	if ( listen(sockfd, 20) != 0 )
	{
		perror("socket--listen");
		exit(errno);
	}

	/*---Forever... ---*/
	while (1)
	{	int clientfd;
		struct sockaddr_in client_addr;
		int addrlen=sizeof(client_addr);

		/*---accept a connection (creating a data pipe)---*/
		clientfd = accept(sockfd, (struct sockaddr*)&client_addr, &addrlen);
                if (clientfd < 0) 
		   continue; 

		printf("%s:%d connected\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

        int size = recv(clientfd, buffer, MAXBUF, 0);
        char *start = strstr(buffer, "?"); 
        char *end = strstr(start, " "); 
        int len = (int)(end - start); 
        char params[256]; 
        memcpy(params, start + 1, len); 
        *(start + len) = 0; // null terminate
        printf("params = %s \n", params);
        char *cur = start + 1, *split = NULL;
        int arr[3] = { -1, -1, -1 }; 
        int i;  
        for(i = 0; i < 3; i++) { 
            split = strstr(cur,"&"); 
            if(split) { 
                *split = 0;
            } 
            char *eq = strstr(cur,"="); 
            if(eq) {  
                arr[i] = atoi(eq + 1);
            }       
            if(split) { 
                cur = split + 1;
            } 
        }  
        printf("a = %d  b = %d  c = %d \n", arr[0], arr[1], arr[2]);
        int j,k;
        // make latency O(a^2)
        for(j = 0; j < arr[0]; j++){ 
            for(k = 0; k < arr[0]; k++) { 
                usleep(100); 
            } 
        }  
        // make latency O(b) 
        for(j = 0; j < arr[1]; j++) { 
            usleep(100); 
        } 
        // have latency be O(1) with respect to c. 

		/*---Echo back anything sent---*/
		send(clientfd, buffer, size, 0);
	        printf("data sent, close next (pid = %d)\n", getpid()); 
		/*---Close data connection---*/
		close(clientfd);
	}

	/*---Clean up (should never get here!)---*/
	close(sockfd);
	return 0;
}
