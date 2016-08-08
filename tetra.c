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
#include <string.h> 
#include <poll.h> 
#include <sys/epoll.h>

#define DATA_FNAME "tetra-data.txt" 

struct timeval start, end;
char method[10], url[1024], params[1024];  

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
int (*real_accept4_fn)(int, struct sockaddr*, socklen_t*, int) = NULL;  
int (*real_close_fn)(int) = NULL;
int (*real_select_fn)(int, fd_set*, fd_set*, fd_set*, struct timeval*) = NULL; 
int (*real_pselect_fn)(int, fd_set*, fd_set*, fd_set*, const struct timespec*, const sigset_t *sigmask) = NULL; 
int (*real_poll_fn)(struct pollfd *fds, nfds_t nfds, int timeout) = NULL; 
int (*real_ppoll_fn)(struct pollfd *fds, nfds_t nfds, 
          const struct timespec *tmo_p, const sigset_t *sigmask) = NULL; 

// saved state across sys calls
fd_set *parent_readfds = NULL;  
int child_accept_fd = -1; 
int is_child = 0; 

void reg_all_fn() { 
    real_select_fn = dlsym(RTLD_NEXT, "select");
    real_pselect_fn = dlsym(RTLD_NEXT, "pselect");
    real_close_fn = dlsym(RTLD_NEXT, "close");
    real_accept_fn = dlsym(RTLD_NEXT, "accept");
    real_accept4_fn = dlsym(RTLD_NEXT, "accept4");
    real_poll_fn = dlsym(RTLD_NEXT, "poll");
    real_ppoll_fn = dlsym(RTLD_NEXT, "ppoll");
    reg_handler();   
} 

int internal_select(int nfds, fd_set* readfds, fd_set *writefds, 
    fd_set *except_fds, struct timeval *timeout1,
    const struct timespec *timeout2, const sigset_t *sigmask, 
    char is_pselect) {  
    
    if (!real_select_fn) {
        reg_all_fn();  
    }
    printf("in internal_select (pid = %d)!\n", getpid());
    parent_readfds = readfds;
    int res = 0;
    while(1) {
        if(is_pselect){ 
            res = real_pselect_fn(nfds, readfds, writefds,
                                 except_fds, timeout2, sigmask);

        } else {  
            res = real_select_fn(nfds, readfds, writefds,
                                 except_fds, timeout1);
        } 
        if (res == -1 && errno == EINTR) { 
            printf("ignoring interrupted selected call\n"); 
        } else { 
            return res;  
        } 
    } 
} 

int select(int nfds, fd_set* readfds, fd_set *writefds, 
    fd_set *except_fds, struct timeval *timeout) { 
    internal_select(nfds,readfds,writefds,except_fds,timeout,
                    NULL, NULL, 0);  
} 

int pselect(int nfds, fd_set *readfds, fd_set *writefds, 
    fd_set *except_fds, const struct timespec *timeout, 
    const sigset_t *sigmask) { 
    internal_select(nfds,readfds,writefds,except_fds,NULL,
                    timeout, sigmask, 1);  
} 


int internal_poll(struct pollfd * fds, nfds_t nfds, int timeout1, 
    const struct timespec *timeout2, const sigset_t *sigmask, 
    char is_ppoll) {  
    
    if (!real_select_fn) {
        reg_all_fn();  
    }
    printf("in internal_poll (pid = %d)!\n", getpid());
    
    // TODO (need equiv for poll?) parent_readfds = readfds;
    int res = 0;
    while(1) {
        if(is_ppoll){ 
            res = real_ppoll_fn(fds, nfds, timeout2, sigmask);
        } else {  
            res = real_poll_fn(fds, nfds, timeout1);
        } 
        // is below still relevant for poll? 
        if (res == -1 && errno == EINTR) { 
            printf("ignoring interrupted poll call\n"); 
        } else { 
            return res;  
        } 
    } 
} 



int poll(struct pollfd *fds, nfds_t nfds, int timeout) { 
    printf("Poll called \n");
    internal_poll(fds,nfds,timeout,NULL, NULL, 0);  
} 
int ppoll(struct pollfd *fds, nfds_t nfds, const struct timespec *tmo_p, 
          const sigset_t *sigmask) { 
    printf("P Poll called \n"); 
    internal_poll(fds,nfds,0,tmo_p, sigmask, 1); 
} 



void parse_http_req(int sockfd) { 
    char buf[2048]; 
    // only peek at data, so app sees the HTTP request as well 
    int size = recv(sockfd, buf, 2048, MSG_PEEK); // TODO: handle small reads
    printf("peek read %d bytes \n", size); 
    *(buf + size) = 0; // null terminate
    char * start = buf;  
    char * end = strstr(start, "\r\n"); // gcc not happy with strnstr? 
    if (end == NULL) { 
        printf("Unable to parse HTTP request %s\n", buf); 
        exit(1);  // TODO: fail gracefully 
    }    
    printf("found CRLF\n");  
    *end = 0; // only look at first line
    printf("first line = '%s'\n", start);    
 
    // get method 
    end = strstr(start," ");
    printf("end = '%s'\n", end);  
    printf("start = '%s'\n", start);  
    if (!end) { 
        printf("malformed http request (no method): %s\n", buf); 
        params[0] = 0; // empty string
        return;  
    } 

    *end = 0; 
    strncpy(method, start,10);
    printf("method = '%s'\n", method);  

    // get URL 
    start = end + 1;   
    end = strstr(start, "?");
    if (!end) { 
        printf("malformed http request (no url): %s\n", buf); 
        params[0] = 0; // empty string
        return;  
    } 
    *end = 0; 
    strncpy(url, start, 1024); 
    printf("url = '%s'\n", url);  

    // get query params
    start = end + 1; 
    end = strstr(start, " "); 
    if (!end) { 
        printf("malformed http request (no params): %s\n", buf); 
        params[0] = 0; // empty string
        return;  
    } 
    *end = 0; 
    strncpy(params, start, 1024);

    printf("params = '%s'\n", params);  
} 


int accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen) { 
    accept4(sockfd, addr, addrlen, 0); 
}

int epoll_wait(int epfd, struct epoll_event *events, 
               int maxevents, int timeout) { 
    printf("epoll wait\n"); 
    exit(1);  
}  

int accept4(int sockfd, struct sockaddr *addr, socklen_t *addrlen, 
            int flags){ 
   
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
        parse_http_req(fd); 
 
        // start latency measurement for this request
        gettimeofday(&start, NULL);  
        printf("child accept - pid = %d\n", getpid());
        return fd;     
    } else { 
        // parent just closes sock, then 
        // loops around to get another request
        if (pid == -1) { 
            perror("fork"); 
        }
        printf("parent accept - pid = %d \n", getpid());
        if(parent_readfds) { 
            FD_CLR(sockfd, parent_readfds);
        }  
        real_close_fn(fd);
        return -1; 
    } 
} 

int close(int fd) {
    if (!real_close_fn) { 
        reg_all_fn();  
    } 
    if (is_child && child_accept_fd == fd) { 
        // child
        pid_t pid = getpid();
        printf("child close \n");  
        real_close_fn(fd);
        /* 
        // TODO:  get per-process memory usage info.  
        char smaps_fname[64]; 
        char output_fname[64]; 
        snprintf(smaps_fname, 64, "/proc/%d/smaps", pid);
        snprintf(output_fname, 64, "/tmp/tetra-%d.out", pid);
        cp(output_fname, smaps_fname);  
        */
        printf("closing socket %d, exiting pid %d \n", fd, pid);
        gettimeofday(&end, NULL); 
        long total_usec =  (end.tv_sec - start.tv_sec)*1000000L
           + (end.tv_usec - start.tv_usec);  
        printf("execution duration is %ld usec, pid = %d\n", 
               total_usec, getpid());

        FILE *data_file = fopen(DATA_FNAME, "a"); 
        fprintf(data_file, "%s,%s,%s,%ld\n", url, method, params, total_usec);
        fclose(data_file);   
        exit(0);  
    } else if (is_child) { 
        // other child close
        return real_close_fn(fd); 
    } else { 
        // parent  
        return real_close_fn(fd); 
    } 
} 





// generic copy file function (borrowed from internet) 
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

