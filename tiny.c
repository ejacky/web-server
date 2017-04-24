#include <stdio.h>
#include <sys/socket.h>
#include <pthread.h>
#include <sys/types.h>
#include <errno.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>


/* 头文件 */
#define MAXLINE 8192
#define RIO_BUFSIZE 8192

/* 通用函数 */
void unix_error(char *msg)
{
	fprintf(stderr, "%s: %s\n", msg, strerror(errno));
	exit(0);
} 

typedef struct {
	int rio_fd;
	int rio_cnt;
	char *rio_bufptr;
	char rio_buf[RIO_BUFSIZE];
} rio_t;

void rio_readinitb(rio_t *rp, int fd)
{
	rp->rio_fd = fd;
	rp->rio_cnt = 0;
	rp->rio_bufptr = rp->rio_buf;
}

static ssize_t rio_read(rio_t *rp, char *usrbuf, size_t n)
{
	int cnt;	
	
	while (rp->rio_cnt <= 0) {
		rp->rio_cnt = read(rp->rio_fd, rp->rio_buf, sizeof(rp->rio_buf));
		if (rp->rio_cnt < 0) {
			if  (errno != EINTR) return -1;
		} else if (rp->rio_cnt == 0) return 0;
		else rp->rio_bufptr = rp->rio_buf;
	}
	
	cnt = n;
	if (rp->rio_cnt < n) cnt = rp->rio_cnt;
	memcpy(usrbuf, rp->rio_bufptr, cnt);
	rp->rio_bufptr += cnt;
	rp->rio_cnt -= cnt;
	return cnt; 
}

ssize_t rio_readlineb(rio_t *rp, void *usrbuf, size_t maxlen)
{
	int n, rc;
	char c, *bufp = usrbuf;
	
	for (n = 1; n < maxlen; n++) {
		if (rc = (rio_read(rp, &c, 1)) == 1) {
			*bufp++ = c;
			if (c == '\n') break;
		} else if (rc == 0) {
			if (n == 1) return 0;
			else break;
		} else return -1;
	}
} 
   


/* 主文件 */ 
void doit(int fd);
int open_listenfd(int port);

int main(int argc, char **argv)
{
	int  listenfd, port;
	int clientsock = -1; 
	struct sockaddr_in clientaddr;
	socklen_t clientlen = sizeof(clientaddr);
	
	if (argc != 2) {
		fprintf(stderr, "usage: %s <port>\n", argv[0]);
		exit(1);
	}
	port = atoi(argv[1]);
	fprintf(stderr, "port:%d", port); 
	
	if ((listenfd = open_listenfd(port)) < 0) {
		unix_error("open listenfd error");
	}
	fprintf(stderr, "listenfd:%d", listenfd); 
	while (1) {
		// accept
		clientsock = accept(listenfd, (struct sockaddr *)&clientaddr, &clientlen);
		fprintf(stderr, "clientsock:%d", clientsock); 
		if (clientsock < 0) { 
			unix_error("accept error");
		}
			
		doit(clientsock);
	}

}

void doit(int fd)
{
	char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
	rio_t rio;
	
	rio_readinitb(&rio, fd);
	rio_readlineb(&rio, buf, MAXLINE);
	sscanf(buf, "%s %s %s", method, uri, version);
	fprintf(stderr, "||method:%s", method);
	fprintf(stderr, "||method:%s", uri);
	fprintf(stderr, "||method:%s", version);
}

int open_listenfd(int port)
{
	int listenfd;
	int optval = 1;
	struct sockaddr_in serveraddr;
	
	// socket
	if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0 )
		return -1;
		
	// setsockopt
	if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, (const void *)&optval, sizeof(int)) < 0)
		return -1; 
		
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serveraddr.sin_port = htons((unsigned short)port);
	
	// bind
	if (bind(listenfd, (struct sockaddr *)&serveraddr, sizeof(serveraddr)) < 0) {
		return -1;
	}
	
	// listen
	if (listen(listenfd, 5) < 0)
		return -1;
	
	return listenfd;
}
  
