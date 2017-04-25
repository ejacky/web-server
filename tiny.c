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
#define MAXBUF 8192
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

ssize_t rio_written(int fd, void *usrbuf, size_t n)
{
	size_t nleft = n;
	ssize_t nwritten;
	char *bufp = usrbuf;
	
	while (nleft > 0) {
		if ((nwritten = write(fd, bufp, nleft)) <=0) {
			if (errno == EINTR) nwritten = 0;
			else return -1; 
		}
		nleft -= nwritten;
		bufp += nwritten;
	}
	return n;
}

void Rio_written(int fd, void *usrbuf, size_t n)
{
	if (rio_written(fd, usrbuf, n) != n) unix_error("Rio_written error");
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
void client_error(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg);
void read_requesthdrs(rio_t *rp);

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
	printf("port:%d", port); 
	
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
	char filename[MAXLINE], cgiargs[MAXLINE];
	int is_static;
	rio_t rio;
	
	rio_readinitb(&rio, fd);
	rio_readlineb(&rio, buf, MAXLINE);
	sscanf(buf, "%s %s %s", method, uri, version);
	
	if (strcasecmp(method, "GET")) { // 若不为 GET 

		client_error(fd, method, "501", "未实现" , "服务器未实现该方法");
		return ;
	}
	
	read_requesthdrs(&rio);  // 处理报头 
	
	is_static = parse_uri(uri, filename, cgiargs); 
	if (stat(filename, &sbuf) < 0) {
		client_error(fd, filename, "404", "Not Found", "Can not find this file");
		return;
	}
	
	if (is_static) {
		
	} else {
		 
	}
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

void read_requesthdrs(rio_t *rp)
{
	char buf[MAXLINE];
	
	rio_readlineb(rp, buf, MAXLINE); 
	while (strcmp(buf, "\r\n")) {
		rio_readlineb(rp, buf, MAXLINE);	
		printf("%s", buf);
//		fprintf(stderr, "%s"); 
//		fprintf(stderr, "%s", buf); 
	}
	return;
}

int parse_uri(char *uri, char *filename, char *cgiargs)
{
	char *ptr;
	
	if (!strstr(uri, "cgi-bin")) {
		strcpy(cgiargs, "");
		strcpy(filename, ".");
		strcat(filename, uri);
		if (uri[strlen(uri) - 1] == '/')
			strcat(filename, "home.html");
		return 1;
	} else return 0;
} 

void client_error(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg) 
{
	char buf[MAXLINE], body[MAXBUF];
    
    // BODY
    sprintf(body, "<html  lang=""zh""><head><meta charset=""UTF-8""><title>NEW SERVER</title><head>");
    sprintf(body, "%s<body>\r\n", body);
    sprintf(body, "%s%s: %s\r\n", body, errnum, shortmsg);
    sprintf(body, "%s<p>%s: %s\r\n", body, longmsg, cause);
    sprintf(body, "%s<hr><em>Tiny server</em>\r\n", body);
    sprintf(body, "%s </body></html>", body);
	//sprintf(body, "<html><head><title>NEW SERVER</title></head><body><P>HTTP request method not supported.\r\n</p></BODY></HTML>\r\n");
    
	// HEADER
    sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
    Rio_written(fd, buf, strlen(buf));
    sprintf(buf, "Server: Tiny Server\r\n");
    Rio_written(fd, buf, strlen(buf));
    sprintf(buf, "Content-Type: text/html\r\n");   
    Rio_written(fd, buf, strlen(buf));
    sprintf(buf, "Content-length: 150\r\n\r\n");
    Rio_written(fd, buf, strlen(buf));
    
	Rio_written(fd, body, strlen(body));	
}
  
