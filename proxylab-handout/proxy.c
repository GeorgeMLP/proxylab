/*
 * proxy.c - A simple HTTP and HTTPS proxy that caches web objects.
 *
 * Name: 马江岩
 * ID: 2000012707
 *
 */
#include "csapp.h"
#include "cache.h"

/* You won't lose style points for including this long line in your code */
static char *user_agent_hdr = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";


void *thread(void *vargp);
void doit(int fd);
int parse_request(char *request, char *host, char *port, char *path);
int request_server(char *host, char *port, char *path, Str_list *head,
                   char **response);
void https(rio_t *client_rio_p, char *host, char *port);
void *transmit(void *vargp);
void http_error(int fd, char *errnum, char *msg);


/*
 * main - Create a thread for each client request.
 */
int main(int argc, char **argv)
{
    if (argc < 2)
    {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        exit(1);
    }
    Sem_init(&queue.mutex, 0, 1);
    Sem_init(&time_lock, 0, 1);
    int listenfd = Open_listenfd(argv[1]);

    while ("serve forever")
    {
        struct sockaddr_storage clientaddr;
        socklen_t clientlen = sizeof(struct sockaddr_storage);
        int connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);
        pthread_t tid;
        Pthread_create(&tid, NULL, thread, (void *)(long)connfd);
    }
}


/*
 * thread - The thread routine function for client requests.
 */
void *thread(void *vargp)
{
    int connfd = (long)vargp;
    Pthread_detach(pthread_self());
    doit(connfd);
    Close(connfd);
    return NULL;
}

/*
 * doit - Forward client request to server and fetch the response from server.
 *     Use or update cache if possible.
 */
void doit(int fd)
{
    char buf[MAXLINE], host_str[MAXLINE], key[MAXLINE], value[MAXLINE];
    char host[MAXLINE], port[MAXLINE], path[MAXLINE];
    rio_t rio;
    Rio_readinitb(&rio, fd);
    if (!Rio_readlineb(&rio, buf, MAXLINE))return;
    int rc = parse_request(buf, host, port, path);
    if (rc == -2)
    {
        https(&rio, host, port); Close(fd); return;
    }
    else if (rc == -1)
    {
        http_error(fd, "400", "Bad Request");
        Close(fd); return;
    }
    sprintf(host_str, "Host: ");
    strcat(host_str, host);
    strcat(host_str, "\r\n");
    Str_list *head = NULL;
    while ("read forever")
    {
        Rio_readlineb(&rio, buf, MAXLINE);
        if (!strcmp(buf, "\r\n"))break;
        sscanf(buf, "%s %s", key, value);
        if (!strcmp(key, "User-Agent:"))continue;
        if (!strcmp(key, "Connection:"))continue;
        if (!strcmp(key, "Proxy-Connection:"))continue;
        if (!strcmp(key, "Host:"))strcpy(host_str, buf);
        else append_list(&head, buf);
    }
    append_list(&head, host_str);
    append_list(&head, user_agent_hdr);
    append_list(&head, "Connection: close\r\n");
    append_list(&head, "Proxy-Connection: close\r\n");
    char *response;
    int read_size = request_server(host, port, path, head, &response);
    if (read_size < 0)
    {
        http_error(fd, "502", "Bad Gateway");
        free_list(&head);
        Close(fd);
        return;
    }
    free_list(&head);
    Rio_writen(fd, response, read_size);
    Close(fd);
    Free(response);
}


/*
 * parse_request - Parse host, port and path from the request header. Return
 *     0 on success, -1 on error, -2 on HTTPS requests.
 */
int parse_request(char *request, char *host, char *port, char *path)
{
    char method[MAXLINE], scm[MAXLINE], version[MAXLINE];
    sprintf(port, "80");
    if (sscanf(request, "%s %[a-z]://%[^/]%s %s",
               method, scm, host, path, version) != 5)
    {
        if (sscanf(request, "%s %s %s",
                   method, host, version) != 3)return -1;
        *path = 0;
    }
    char *pos = strchr(host, ':');
    if (pos) { *pos = 0; strcpy(port, pos + 1); }
    if (!strcmp(host, "127.0.0.1.nip.io"))
        strcpy(host, "127.0.0.1");
    if (!strcasecmp(method, "CONNECT"))return -2;
    return 0;
}


/*
 * request_server - Connect to server and fetch its response. Return -1 on
 *     error, length of the response on success.
 */
int request_server(char *host, char *port, char *path, Str_list *head,
                   char **response)
{
    int read_size = read_queue(host, port, path, response);
    if (read_size >= 0)return read_size;
    int fd = Open_clientfd(host, port);
    if (fd < 0)return -1;
    char buf[MAXLINE], ans[MAX_OBJECT_SIZE + 1];
    sprintf(buf, "GET %s HTTP/1.0\r\n", path);
    Rio_writen(fd, buf, strlen(buf));
    for (Str_list *p = head; p; p = p->next)
        Rio_writen(fd, p->str, strlen(p->str));
    Rio_writen(fd, "\r\n", strlen("\r\n"));
    read_size= Rio_readn(fd, ans, MAX_OBJECT_SIZE);
    *response = Malloc(read_size + 1);
    memcpy(*response, ans, read_size);
    Close(fd);
    Cache *r = Malloc(sizeof(Cache));
    r->host = Malloc(strlen(host) + 1); strcpy(r->host, host);
    r->port = Malloc(strlen(port) + 1); strcpy(r->port, port);
    r->path = Malloc(strlen(path) + 1); strcpy(r->path, path);
    r->length = read_size;
    r->response = Malloc(read_size + 1);
    memcpy(r->response, ans, read_size);
    write_queue(r);
    return read_size;
}


/*
 * https - Use two threads to handle HTTPS requests.
 */
void https(rio_t *client_rio_p, char *host, char *port)
{
    int clientfd = client_rio_p->rio_fd;
    char buf[MAXLINE];
    while (strcmp(buf, "\r\n"))Rio_readlineb(client_rio_p, buf, MAXLINE);
    int serverfd = Open_clientfd(host, port);
    if (serverfd < 0)return;
    rio_t server_rio;
    Rio_readinitb(&server_rio, serverfd);
    sprintf(buf, "HTTP/1.1 200 Connection Established\r\n");
    strcat(buf, "Connection: close\r\n\r\n");
    Rio_writen(clientfd, buf, strlen(buf));
    int *client_to_server = Malloc(sizeof(int));
    int *server_to_client = Malloc(sizeof(int));
    *client_to_server = clientfd << 16 | serverfd;
    *server_to_client = serverfd << 16 | clientfd;
    pthread_t tid_1, tid_2;
    Pthread_create(&tid_1, NULL, transmit, client_to_server);
    Pthread_create(&tid_2, NULL, transmit, server_to_client);
    Pthread_join(tid_1, NULL);
    Pthread_join(tid_2, NULL);
    Close(serverfd);
}


/*
 * transmit - The thread routine function for HTTPS requests. Read from sender
 *     and forward to receiver.
 */
void *transmit(void *vargp)
{
    int senderfd = (*(int *)vargp) >> 16;
    int receiverfd = (*(int *)vargp) & 0xffff;
    Free(vargp);
    int read_size;
    char buf[MAXLINE];
    while ((read_size = Read(senderfd, buf, MAXLINE)))
    {
        if (read_size == -1)return NULL;
        Rio_writen(receiverfd, buf, read_size);
    }
    return NULL;
}


/*
 * http_error - Print error message in HTTP format.
 */
void http_error(int fd, char *errnum, char *msg)
{
    char buf[MAXLINE];
    sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, msg);
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-type: text/html\r\n\r\n");
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "<html><title>Request Error</title>");
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "<body bgcolor=\"ffffff\">\r\n");
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "%s: %s\r\n", errnum, msg);
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "<hr><em>The Tiny Proxy Server</em>\r\n");
    Rio_writen(fd, buf, strlen(buf));
}
