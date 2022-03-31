#include <stdio.h>
#include "csapp.h"
#include "cache.h"
/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";
static const char *conn_hdr = "Connection: close\r\n";
static const char *prox_hdr = "Proxy-Connection: close\r\n";


void doit(int client_fd);   //事件处理逻辑
void *thread(void *vargp);  //并发, 设置线程分离

void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg);     //错误返回
void parse_uri(char *uri, char *hostname, char *path, int *port);  //解析请求
void build_requesthdrs(rio_t *rp, char *newreq, int len, char *hostname, int port);    //构建响应

int main(int argc, char *argv[]) {
    int listenfd, *connfd_ptr;
    pthread_t tid;
    socklen_t  clientlen;
    char hostname[MAXLINE],port[MAXLINE];
    //客户端地址
    struct sockaddr_storage clientaddr;

    if(argc != 2) {
        fprintf(stderr, "usage :%s <port> \n",argv[0]);
        exit(1);
    }
    init_cache();
    listenfd = Open_listenfd(argv[1]);
    while(1) {
        clientlen = sizeof(clientaddr);
        connfd_ptr = Malloc(sizeof(int));
        *connfd_ptr = Accept(listenfd,(SA *)&clientaddr,&clientlen);

        Getnameinfo((SA*)&clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE, 0);
        printf("Accepted connection from (%s %s).\n",hostname,port);

        Pthread_create(&tid, NULL, thread, connfd_ptr); //处理函数
    }
    free_cache();
    return 0;
}

void *thread(void *vargp) {
    int connfd = *((int *)vargp);
    Pthread_detach(pthread_self());//线程分离, 并发
    Free(vargp);
    doit(connfd);   //处理事件逻辑                                             
    Close(connfd);  
    return NULL;
}

void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg) {
    char buf[MAXLINE], body[MAXBUF];

    //构造响应体 body
    int len = snprintf(body, MAXBUF, "<html><title>Proxy Error</title>");
    len += snprintf(body + len, MAXBUF - len, "<body bgcolor=""ffffff"">\r\n");
    len += snprintf(body + len, MAXBUF - len, "%s: %s\r\n", errnum, shortmsg);
    len += snprintf(body + len, MAXBUF - len, "<p>%s: %s\r\n", longmsg, cause);
    len += snprintf(body + len, MAXBUF - len, "<hr><em>The Proxy Web server</em>\r\n");

    /* Print the HTTP response */
    len = snprintf(buf, MAXLINE, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);   //Line
    Rio_writen(fd, buf, strlen(buf));

    len += snprintf(buf + len, MAXLINE - len, "Content-type: text/html\r\n");    //Header
    Rio_writen(fd, buf, strlen(buf));

    len += snprintf(buf + len, MAXLINE - len, "Content-length: %d\r\n\r\n", (int)strlen(body));  //Header
    Rio_writen(fd, buf, strlen(buf));

    Rio_writen(fd, body, strlen(body));     //body
}

//客户端连接来了, proxy进行处理, 转发给服务器
void doit(int client_fd) {
    int endserver_fd;
    char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
    rio_t from_client, to_endserver;
    /*store the request line arguments*/
    char hostname[MAXLINE], path[MAXLINE];//hostname www.cum.edu path  /hub/index.html 
    int port;

    /* Read request line and headers */
    Rio_readinitb(&from_client, client_fd);

    if (!Rio_readlineb(&from_client, buf, MAXLINE)) return;
    sscanf(buf, "%s %s %s", method, uri, version);       
    if (strcasecmp(method, "GET")) {   
        //错误处理                  
        clienterror(client_fd, method, "501", "Not Implemented", \
        "Proxy Server does not implement this method");
        return;
    }
    /* read cache */
    int ret = read_cache(uri, client_fd);
    if(ret == 1) {
        return;
    }

    // 从uri中解析 hostname, path, port
    // parse uri and proxy as a client connect to server
    char port_str[10];
    parse_uri(uri, hostname, path, &port); //=== 
    sprintf(port_str, "%d", port);

    // 转接到服务器
    endserver_fd = Open_clientfd(hostname, port_str);
    if(endserver_fd < 0){
        printf("connection failed\n");
        return;
    }
    Rio_readinitb(&to_endserver, endserver_fd);

    char newreq[MAXLINE]; //for end server http req headers
    //set up first line eg.GET /hub/index.html HTTP/1.0
    int len = 0;
    len = snprintf(newreq, MAXLINE, "GET %s HTTP/1.0\r\n", path);

    build_requesthdrs(&from_client, newreq, len, hostname, port);
    
    Rio_writen(endserver_fd, newreq, strlen(newreq)); //send client header to real server
    int n, size = 0;
    char data[MAX_OBJECT_SIZE];
    while ((n = Rio_readlineb(&to_endserver, buf, MAXLINE))) {//real server response to buf
        //printf("proxy received %d bytes,then send\n",n);
        if(size <= MAX_CACHE_SIZE) {
            memcpy(data + size, buf, n);
            size += n;
        }
        printf("proxy received %d bytes, then send\n", n);
        Rio_writen(client_fd, buf, n);  //real server response to real client
    }
    printf("size : %d\n", size);
    if(size <= MAX_OBJECT_SIZE) {
        write_cache(uri, data, size);
    }
    Close(endserver_fd);
    
}

void parse_uri(char *uri, char *hostname, char *path, int *port) {
    *port = 80;
    //uri http://www.cmu.edu/hub/index.html
    char* pos1 = strstr(uri,"//");
    if (pos1 == NULL) {
        pos1 = uri;
    } else pos1 += 2;

    //printf("parse uri pos1 %s\n",pos1);//pos1 www.cmu.edu/hub/index.html

    char* pos2 = strstr(pos1,":");
    /*pos1 www.cmu.edu:8080/hub/index.html, pos2 :8080/hub/index.html */
    if (pos2 != NULL) {
        *pos2 = '\0'; //pos1 www.cmu.edu/08080/hub/index.html
        strncpy(hostname, pos1, MAXLINE);
        sscanf(pos2+1,"%d%s", port, path); //pos2+1 8080/hub/index.html
        *pos2 = ':';
    } else {
        pos2 = strstr(pos1, "/");//pos2 /hub/index.html
        if (pos2 == NULL) {/*pos1 www.cmu.edu*/
            strncpy(hostname, pos1, MAXLINE);
            strcpy(path, "");
            return;
        }
        *pos2 = '\0';
        strncpy(hostname,pos1,MAXLINE);
        *pos2 = '/';
        strncpy(path,pos2,MAXLINE);
    }
}

void build_requesthdrs(rio_t *rp, char *newreq, int len, char *hostname, int port) {
    //already have sprintf(newreq, "GET %s HTTP/1.0\r\n", path);
    char buf[MAXLINE];
    while(Rio_readlineb(rp, buf, MAXLINE) > 0) {          
        if (!strcmp(buf, "\r\n")) break;
        if (strstr(buf,"Host:") != NULL) continue;
        if (strstr(buf,"User-Agent:") != NULL) continue;
        if (strstr(buf,"Connection:") != NULL) continue;
        if (strstr(buf,"Proxy-Connection:") != NULL) continue;

        len += snprintf(newreq + len, MAXLINE - len, "%s", buf);
    }
    len += snprintf(newreq + len, MAXLINE - len, "Host: %s:%d\r\n", hostname, port);
    len += snprintf(newreq + len, MAXLINE - len, "%s%s%s", user_agent_hdr, conn_hdr, prox_hdr);
    len += snprintf(newreq + len, MAXLINE - len, "\r\n");
}

