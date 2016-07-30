#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>  
#include <dlfcn.h> 
#include <unistd.h> 
#include <sys/socket.h> 
#include <sys/types.h> 
#include <sys/select.h> 
#include <sys/time.h> 
#include <signal.h> 
#include <fcntl.h>
#include <errno.h>


int cp(const char *to, const char *from);

// avoid defunct children
void handle_sigchld(int sig) {
  while (waitpid((pid_t)(-1), 0, WNOHANG) > 0) {}
}

void reg_handler() { 
  struct sigaction sa;
  sa.sa_handler = &handle_sigchld;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = SA_RESTART | SA_NOCLDSTOP;
  if (sigaction(SIGCHLD, &sa, 0) == -1) {
    perror(0);
    exit(1);
  }
} 

// fn pointers to real libc calls
int (*real_accept_fn)(int, struct sockaddr*, socklen_t*) = NULL;  
int (*real_close_fn)(int) = NULL;
int (*real_select_fn)(int, fd_set*, fd_set*, fd_set*, struct timeval*) = NULL; 

// saved state across sys calls
fd_set *parent_readfds = NULL;  
int child_accept_fd = -1; 
int is_child = 0; 

void reg_all_fn() { 
  real_select_fn = dlsym(RTLD_NEXT, "select");
  real_close_fn = dlsym(RTLD_NEXT, "close");
  real_accept_fn = dlsym(RTLD_NEXT, "accept");
  reg_handler();   
} 


int select(int nfds, fd_set* readfds, fd_set *writefds, fd_set *except_fds, 
	   struct timeval *timeout) { 
   if (!real_select_fn) {
     reg_all_fn();  
   }
   printf("in select (pid = %d)!\n", getpid());
   parent_readfds = readfds;
   int res = 0;
   while(1) { 
      res = real_select_fn(nfds, readfds, writefds, except_fds, timeout);
      if (res == -1 && errno == EINTR) { 
        printf("ignoring interrupted selected call\n"); 
      } else { 
        return res;  
      } 
   } 
} 

int accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen){ 
   
   printf("Accept on server socket %d\n", sockfd); 
   if (!real_accept_fn) { 
     reg_all_fn();  
   }

        int fd = real_accept_fn(sockfd, addr, addrlen);
        int pid = fork(); 
   	if (pid == 0) { 
	   // child
           is_child = 1;  
           child_accept_fd = fd; 
           printf("child accept - pid = %d\n", getpid());
   	   return fd;     
        } else { 
           // parent just closes sock, then 
           // loops around to get another request
	   if (pid == -1) { 
               perror("fork"); 
           }
           FD_CLR(sockfd, parent_readfds);  
           real_close_fn(fd);
           return -1; 
	} 
} 

/*
int accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen){ 
   
   fd_set read_fd_set; 

   printf("Accept on server socket %d\n", sockfd); 
   if (!real_accept_fn) { 
     reg_all_fn();  
   }

   while(1) { 
        FD_ZERO(&read_fd_set); 
        FD_SET(sockfd, &read_fd_set); 
        if (select(sockfd + 1, &read_fd_set, NULL, NULL, NULL) < 0) { 
          perror("select"); 
          exit(EXIT_FAILURE); 
        }
        printf("select returned\n");  
   	if (!fork()) { 
           int fd = real_accept_fn(sockfd, addr, addrlen);
	   // child
           is_child = 1;  
           child_accept_fd = fd; 
           printf("child accept - pid = %d\n", getpid());
   	   return fd;     
        }
        // parent just loops around to get another request 
   } 
} 
*/


int close(int fd) {
   if (!real_close_fn) { 
     reg_all_fn();  
   } 
   if (is_child && child_accept_fd == fd) { 
        // child
  	pid_t pid = getpid();
        printf("child close \n");  
        real_close_fn(fd);/* 
        // TODO:  get per-process usage info.  
	char smaps_fname[64]; 
	char output_fname[64]; 
	snprintf(smaps_fname, 64, "/proc/%d/smaps", pid);
	snprintf(output_fname, 64, "/tmp/tetra-%d.out", pid);
	cp(output_fname, smaps_fname);  
        */
	printf("closing socket %d, exiting pid %d \n", fd, pid);
	exit(0);  
   } else if (is_child) { 
     // other child close
     printf("other child close \n"); 
  } else { 
      // parent  
      printf("parent close \n");  
      return real_close_fn(fd); 
  } 
} 

// generic copy file function 
int cp(const char *to, const char *from)
{
    int fd_to, fd_from;
    char buf[4096];
    ssize_t nread;
    int saved_errno;

    fd_from = open(from, O_RDONLY);
    if (fd_from < 0)
        return -1;

    fd_to = open(to, O_WRONLY | O_CREAT | O_EXCL, 0666);
    if (fd_to < 0)
        goto out_error;

    while (nread = read(fd_from, buf, sizeof buf), nread > 0)
    {
        char *out_ptr = buf;
        ssize_t nwritten;

        do {
            nwritten = write(fd_to, out_ptr, nread);

            if (nwritten >= 0)
            {
                nread -= nwritten;
                out_ptr += nwritten;
            }
            else if (errno != EINTR)
            {
                goto out_error;
            }
        } while (nread > 0);
    }

    if (nread == 0)
    {
        if (close(fd_to) < 0)
        {
            fd_to = -1;
            goto out_error;
        }
        close(fd_from);

        /* Success! */
        return 0;
    }

  out_error:
    saved_errno = errno;

    close(fd_from);
    if (fd_to >= 0)
        close(fd_to);

    errno = saved_errno;
    return -1;
}

