#include <stdio.h>
#include <pthread.h>
#include "csapp.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400
#define __MAC_OS_X
#define SBUFSIZE 1049000
#define NTHREADS 4

/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";

void		doit      (int fd);
void		read_requesthdrs(rio_t * rp, char* headers);
int		    parse_uri  (char *uri, char *filename, char *cgiargs);
void		serve_static(int fd, char *filename, int filesize);
void		get_filetype(char *filename, char *filetype);
void		serve_dynamic(int fd, char *filename, char *cgiargs);
void        clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg);

typedef struct {
    int *buf;          /* Buffer array */
    int n;             /* Maximum number of slots */
    int front;         /* buf[(front+1)%n] is first item */
    int rear;          /* buf[rear%n] is last item */
    sem_t mutex;       /* Protects accesses to buf */
    sem_t slots;       /* Counts available slots */
    sem_t items;       /* Counts available items */
} sbuf_t;

void sbuf_init(sbuf_t *sp, int n);
void sbuf_deinit(sbuf_t *sp);
void sbuf_insert(sbuf_t *sp, int item);
int sbuf_remove(sbuf_t *sp);
sbuf_t sbuf;

// struct addrinfo {
//     int       ai_flags;
//     int       ai_family;
//     int       ai_socktype;
//     int       ai_protocol;
//     socklen_t ai_addrlen;
//     struct    sockaddr* ai_addr;
//     char*     ai_canonname;      /* canonical name */
//     struct    addrinfo* ai_next; /* this struct can form a linked list */
// };

void sigchld_handler(int sig)
{
    while (waitpid(-1, 0, WNOHANG) > 0)
    {

    }
    return;
}

/* Create an empty, bounded, shared FIFO buffer with n slots */
void sbuf_init(sbuf_t *sp, int n)
{
    sp->buf = Calloc(n, sizeof(int));
    sp->n = n;                    /* Buffer holds max of n items */
    sp->front = sp->rear = 0;     /* Empty buffer iff front == rear */
    Sem_init(&sp->mutex, 0, 1);   /* Binary semaphore for locking */
    Sem_init(&sp->slots, 0, n);   /* Initially, buf has n empty slots */
    Sem_init(&sp->items, 0, 0);   /* Initially, buf has 0 items */
}

/* Clean up buffer sp */
void sbuf_deinit(sbuf_t *sp)
{
    Free(sp->buf);
}

/* Insert item onto the rear of shared buffer sp */
void sbuf_insert(sbuf_t *sp, int item)
{
    P(&sp->slots);                         /* Wait for available slot */
    P(&sp->mutex);                         /* Lock the buffer */
    sp->rear = sp->rear+1;
    sp->buf[(sp->rear)%(sp->n)] = item;  /* Insert the item */
    V(&sp->mutex);                         /* Unlock the buffer */
    V(&sp->items);                         /* Announce available item */
}

int sbuf_remove(sbuf_t *sp)
{
    int item;
    P(&sp->items);                         /* Wait for available item */
    P(&sp->mutex);
    sp->front = sp->front+1;                       /* Lock the buffer */
    item = sp->buf[(sp->front)%(sp->n)]; /* Remove the item */
    V(&sp->mutex);                         /* Unlock the buffer */
    V(&sp->slots);                         /* Announce available slot */
    return item;
}

void *thread(void *vargp)
{
    // int connfd = *((int *)vargp);
    // Pthread_detach(pthread_self());
    // Free(vargp);
    // doit(connfd);
    // Close(connfd);
    Pthread_detach(pthread_self());
    while (1) {
        int connfd = sbuf_remove(&sbuf); /* Remove connfd from buf */
        doit(connfd);                /* Service client */
        Close(connfd);
    }
    // return NULL;
}

int main(int argc, char **argv)
{
    // int listenfd, *connfdp;
    // socklen_t clientlen;
    // struct sockaddr_storage clientaddr;
    // pthread_t tid;
    //
    // listenfd = Open_listenfd(argv[1]);
    // while (1) {
    // 	clientlen=sizeof(struct sockaddr_storage);
    // 	connfdp = Malloc(sizeof(int));
    // 	*connfdp = Accept(listenfd,(SA *) &clientaddr, &clientlen);
    //
    // 	Pthread_create(&tid, NULL, thread, connfdp);
    // }
    int i, listenfd, connfd;
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;
    pthread_t tid;

    listenfd = Open_listenfd(argv[1]);
    sbuf_init(&sbuf, SBUFSIZE);
    for (i = 0; i < NTHREADS; i++) { /* Create worker threads */
        Pthread_create(&tid, NULL, thread, NULL);
    }
    while (1) {
        clientlen = sizeof(struct sockaddr_storage);
        connfd = Accept(listenfd, (SA *) &clientaddr, &clientlen);
        sbuf_insert(&sbuf, connfd); /* add connection to the shared queue */
    }
    // return 0;
}

void parseReq()
{

}

int isHTTP(char* uri)
{
    char* p;
    p = strstr(uri, "http://");
    if(p)
    {
        free(p);
        return 1;
    }
    else
    {
        free(p);
        return 0;
    }
}

void parseURI(char* uri, char* method, char* version, int clientfd, char* extraHeaders)
{
    char* hostname = malloc(1024);
    char* path = malloc(1024);
    char* port = malloc(1024);

    int gettingHost = 0;
    int gettingPath = 0;
    int gettingPort = 0;
    int hostI = 0;
    int pathI = 0;
    int portI = 0;
    for(int i = 0; i < strlen(uri); i += 1)
    {
        if(uri[i] == '/' && !gettingHost)
        {
            gettingHost = 1;
            i += 1;
        }
        else if(uri[i] == ':' && gettingHost)
        {
            gettingPort = 1;
        }
        else if(uri[i] == '/' && gettingHost && !gettingPath)
        {
            gettingPath = 1;
            path[pathI] = uri[i];
            pathI += 1;
        }
        else if(gettingHost && !gettingPath && !gettingPort)
        {
            hostname[hostI] = uri[i];
            hostI += 1;
        }
        else if (gettingPath)
        {
            path[pathI] = uri[i];
            pathI += 1;
        }
        else if (gettingPort)
        {
            port[portI] = uri[i];
            portI += 1;
        }

    }

    if (!gettingPort) {
        port[0] = '8';
        port[1] = '0';
        portI = 2;
    }

    int skt = open_clientfd(hostname,port);

    // put together the request
    char* str = malloc(1024);

    strcat(method, " ");
    strcat(method, path);
    strcat(method, " ");
    strcat(method, version);
    strcat(method, "\r\n");
    strcat(method, "Host: ");
    strcat(method, hostname);
    strcat(method, "\r\n");
    strcat(method, "Connection: close\r\n");
    strcat(method, "Proxy-Connection: close\r\n\r\n");

    char* request = method;

    // send the request to the Socket
    send(skt, request, strlen(request),0);
    char* buffer = malloc(MAX_OBJECT_SIZE); // or whatever you like, but best to keep it large
    int count = 0;
    int total = 0;

    while((count = recv(skt, buffer+total, sizeof(buffer), 0)) > 0) {
        total += count;
    }

    int len = 0;
    while(len < total) {
        len += send(clientfd, buffer, total,0);
    }

    free(str);
    free(buffer);
    free(hostname);
    free(path);
    free(port);
}

/*
 * doit - handle one HTTP request/response transaction
 */
/* $begin doit */
void doit(int fd) // maybe add void pointers
{
	int		is_static;
	struct stat	sbuf;
	char		buf       [MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
	char		filename  [MAXLINE], cgiargs[MAXLINE];
	rio_t		rio;

	/* Read request line and headers */
	Rio_readinitb(&rio, fd);
	if (!Rio_readlineb(&rio, buf, MAXLINE))
			return;
	printf("%s", buf);
	sscanf(buf, "%s %s %s", method, uri, version);
	if (strcasecmp(method, "GET"))
	{

    } //line: netp: doit:endrequesterr

    char* moreHeaders = malloc(1024);
	read_requesthdrs(&rio, moreHeaders);

    printf("headers more: %s\n", moreHeaders);

	/* Parse URI from GET request */

    // check if in cache

    if (isHTTP(uri))
    {
        printf("ITS HTTP\n");
        parseURI(uri, method, version, fd, moreHeaders);
    }
    else
    {
        // is_static = parse_uri(uri, filename, cgiargs);
        // char* s1 = "./tiny";
        //
        // char nfilename[1000];
        // memset(nfilename, 0, 1000);
        //
        // for (int i = 0; i < strlen(s1); i += 1) {
        //     nfilename[i] = s1[i];
        // }
        // int size = strlen(nfilename);
        // for (int i = 0; i < strlen(filename) - 1; i += 1) {
        //
        //         nfilename[i + size] = filename[i+1];
        //
        // }
        //
        // // strcpy(nfilename,s1);
        // printf("Filename: %s\n", nfilename);
        // // strncpy(nfilename, filename+1, strlen(filename));
    	// if (stat(nfilename, &sbuf) < 0)
    	// {
        //     //line: netp: doit:beginnotfound
    	// 	clienterror(fd, nfilename, "404", "Not found","Tiny couldn't find this file");
    	// 	return;
        // } //line: netp: doit:endnotfound
        //
    	// if (is_static)
    	// {			/* Serve static content */
    	// 	if (!(S_ISREG(sbuf.st_mode)) || !(S_IRUSR & sbuf.st_mode))
    	// 	{
    	//         //line: netp: doit:readable
    	// 		clienterror(fd, nfilename, "403", "Forbidden","Tiny couldn't read the file");
    	// 		return;
    	// 	}
    	// 	serve_static(fd, nfilename, sbuf.st_size);
        //     //line: netp: doit:servestatic
    	// }
        // else
    	// {			/* Serve dynamic content */
    	// 	if (!(S_ISREG(sbuf.st_mode)) || !(S_IXUSR & sbuf.st_mode))
    	// 	{
    	//         //line: netp: doit:executable
    	// 		clienterror(fd, nfilename, "403", "Forbidden","Tiny couldn't run the CGI program");
    	// 		return;
    	// 	}
    	// 	serve_dynamic(fd, nfilename, cgiargs);
        //     //line: netp: doit:servedynamic
    	// }
    }
    free(moreHeaders);

}
/* $end doit */

/*
 * read_requesthdrs - read HTTP request headers
 */
/* $begin read_requesthdrs */
void read_requesthdrs(rio_t * rp, char* headers)
{
	char buf [MAXLINE];

	Rio_readlineb(rp, buf, MAXLINE);
	while (strcmp(buf, "\r\n"))
	{
		Rio_readlineb(rp, buf, MAXLINE);
		printf("%s", buf);
        strcat(headers,buf);
	}
	return;
}
/* $end read_requesthdrs */

/*
 * parse_uri - parse URI into filename and CGI args return 0 if dynamic
 * content, 1 if static
 */
/* $begin parse_uri */
int parse_uri(char *uri, char *filename, char *cgiargs)
{
	char *ptr;
    printf("Parsing URI\n");
	if (!strstr(uri, "cgi-bin"))
	{			/* Static content */
		strcpy(cgiargs, "");
		strcpy(filename, ".");
		strcat(filename, uri);
		if (uri[strlen(uri) - 1] == '/')
			strcat(filename, "home.html");
		return 1;
	}
    else
	{			/* Dynamic content */
		ptr = index(uri, '?');
		if (ptr)
		{
			strcpy(cgiargs, ptr + 1);
			*ptr = '\0';
		}
        else
			strcpy(cgiargs, "");
		strcpy(filename, ".");
		strcat(filename, uri);
		return 0;
	}
}
/* $end parse_uri */

/*
 * serve_static - copy a file back to the client
 */
/* $begin serve_static */
void
serve_static(int fd, char *filename, int filesize)
{
	int		srcfd;
	char           *srcp, filetype[MAXLINE], buf[MAXBUF];

	/* Send response headers to client */
	get_filetype(filename, filetype);
//line: netp: servestatic:getfiletype
		sprintf(buf, "HTTP/1.0 200 OK\r\n");
//line: netp: servestatic:beginserve
		sprintf(buf, "%sServer: Tiny Web Server\r\n", buf);
	sprintf(buf, "%sConnection: close\r\n", buf);
	sprintf(buf, "%sContent-length: %d\r\n", buf, filesize);
	sprintf(buf, "%sContent-type: %s\r\n\r\n", buf, filetype);
	Rio_writen(fd, buf, strlen(buf));
//line: netp: servestatic:endserve
		printf("Response headers:\n");
	printf("%s", buf);

	/* Send response body to client */
	srcfd = Open(filename, O_RDONLY, 0);
//line: netp: servestatic:open
		srcp = Mmap(0, filesize, PROT_READ, MAP_PRIVATE, srcfd, 0);
//line: netp: servestatic:mmap
		Close(srcfd);
//line: netp: servestatic:close
		Rio_writen(fd, srcp, filesize);
//line: netp: servestatic:write
		Munmap(srcp, filesize);
//line: netp: servestatic:munmap
}

/*
 * get_filetype - derive file type from file name
 */
void
get_filetype(char *filename, char *filetype)
{
	if (strstr(filename, ".html"))
		strcpy(filetype, "text/html");
	else if (strstr(filename, ".gif"))
		strcpy(filetype, "image/gif");
	else if (strstr(filename, ".png"))
		strcpy(filetype, "image/png");
	else if (strstr(filename, ".jpg"))
		strcpy(filetype, "image/jpeg");
	else
		strcpy(filetype, "text/plain");
}
/* $end serve_static */

/*
 * serve_dynamic - run a CGI program on behalf of the client
 */
/* $begin serve_dynamic */
void
serve_dynamic(int fd, char *filename, char *cgiargs)
{
	char		buf       [MAXLINE], *emptylist[] = {NULL};

	/* Return first part of HTTP response */
	sprintf(buf, "HTTP/1.0 200 OK\r\n");
	Rio_writen(fd, buf, strlen(buf));
	sprintf(buf, "Server: Tiny Web Server\r\n");
	Rio_writen(fd, buf, strlen(buf));

	if (Fork() == 0)
	{			/* Child */
//line: netp: servedynamic:fork
		/* Real server would set all CGI vars here */
			setenv("QUERY_STRING", cgiargs, 1);
//line: netp: servedynamic:setenv
			Dup2(fd, STDOUT_FILENO);	/* Redirect stdout to
							 * client */
//line: netp: servedynamic:dup2
			Execve(filename, emptylist, environ);	/* Run CGI program */
//line: netp: servedynamic:execve
	}
	Wait(NULL);		/* Parent waits for and reaps child */
//line: netp: servedynamic:wait
}
/* $end serve_dynamic */

/*
 * clienterror - returns an error message to the client
 */
/* $begin clienterror */
void
clienterror(int fd, char *cause, char *errnum,
	    char *shortmsg, char *longmsg)
{
	char		buf       [MAXLINE], body[MAXBUF];

	/* Build the HTTP response body */
	sprintf(body, "<html><title>Tiny Error</title>");
	sprintf(body, "%s<body bgcolor=" "ffffff" ">\r\n", body);
	sprintf(body, "%s%s: %s\r\n", body, errnum, shortmsg);
	sprintf(body, "%s<p>%s: %s\r\n", body, longmsg, cause);
	sprintf(body, "%s<hr><em>The Tiny Web server</em>\r\n", body);

	/* Print the HTTP response */
	sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
	Rio_writen(fd, buf, strlen(buf));
	sprintf(buf, "Content-type: text/html\r\n");
	Rio_writen(fd, buf, strlen(buf));
	sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body));
	Rio_writen(fd, buf, strlen(buf));
	Rio_writen(fd, body, strlen(body));
}
/* $end clienterror */
