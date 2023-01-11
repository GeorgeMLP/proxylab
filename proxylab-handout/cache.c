/*
 * cache.c - Implementation of the proxy's cache.
 *
 * Name: 马江岩
 * ID: 2000012707
 *
 * The cache blocks are stored in a singly linked list. Their time stamps
 * are updated on each read and write. The eviction policy is LRU.
 *
 */
#include "csapp.h"
#include "cache.h"


int time_stamp = 0;
int cache_count = 0;
Cache *header;
RW_queue queue;
sem_t time_lock;


/*
 * append_list - Append a string to a list of strings.
 */
void append_list(Str_list **head, char *str)
{
    Str_list *tmp = Malloc(sizeof(Str_list));
    int len = strlen(str) + 1;
    tmp->str = Malloc(len);
    strcpy(tmp->str, str);
    tmp->next = *head;
    *head = tmp;
}


/*
 * free_list - Free a list of strings.
 */
void free_list(Str_list **head)
{
    for (Str_list *listp = *head; listp;)
    {
        Free(listp->str);
        Str_list *tmp = listp;
        listp = listp->next;
        Free(tmp);
    }
    *head = NULL;
}


/*
 * free_cache - Free a cache block.
 */
void free_cache(Cache *p)
{
    Free(p->host);
    Free(p->port);
    Free(p->path);
    Free(p->response);
    Free(p);
}


/*
 * read_queue - Read from cache. Return size of the response body on cache
 *     hit, -1 on cache miss.
 */
int read_queue(char *host, char *port, char *path, char **response)
{
    RW_worker *tmp = Malloc(sizeof(RW_worker));
    int flag = 0, ret = -1;
    Sem_init(&tmp->lock, 0, 0);
    tmp->type = 0;
    tmp->next = NULL;
    P(&queue.mutex);
    if (queue.size) { queue.tail->next = tmp; queue.tail = tmp; }
    else { queue.head = queue.tail = tmp; flag = 1; }
    queue.size++;
    V(&queue.mutex);
    if (flag)V(&tmp->lock);
    P(&tmp->lock);
    P(&time_lock);
    int cur_time = ++time_stamp;
    V(&time_lock);
    P(&queue.mutex);
    queue.head = queue.head->next;
    if (queue.head && queue.head->type == 0)V(&queue.head->lock);
    queue.size--;
    V(&queue.mutex);
    Cache *p;
    for (p = header; p; p = p->next)
        if (!strcmp(host, p->host) && !strcmp(port, p->port) &&
            !strcmp(path, p->path))break;
    if (p)
    {
        p->last_time = cur_time;
        *response = Malloc(p->length + 1);
        memcpy(*response, p->response, p->length);
        ret = p->length;
    }
    if (tmp->next && tmp->next->type == 1)V(&tmp->next->lock);
    Free(tmp);
    return ret;
}


/*
 * write_queue - Write to cache. If the cache blocks are all ocupied,
 *     use LRU policy to evict a block.
 */
void write_queue(Cache *r)
{
    RW_worker *tmp = Malloc(sizeof(RW_worker));
    Sem_init(&tmp->lock, 0, 0);
    tmp->type = 1;
    tmp->next = NULL;
    int flag = 0;
    P(&queue.mutex);
    if (queue.size > 1) { queue.tail->next = tmp; queue.tail = tmp; }
    else { queue.head = queue.tail = tmp; flag = 1; }
    queue.size++;
    V(&queue.mutex);
    if (flag)V(&tmp->lock);
    P(&tmp->lock);
    int cur_time = ++time_stamp;
    if (cache_count == MAX_BLOCK_COUNT)
    {
        int lru_time = 2147483647;
        Cache *q = NULL, *qr = NULL;
        for (Cache *p = header, *pr = NULL; p; pr = p, p = p->next)
        {
            if (p->last_time < lru_time)
            { lru_time = p->last_time; q = p; qr = pr; }
        }
        if (qr)qr->next = q->next;
        else header = q->next;
        free_cache(q);
        cache_count--;
    }
    r->last_time = cur_time;
    r->next = header;
    header = r;
    cache_count++;
    P(&queue.mutex);
    queue.size--;
    queue.head = tmp->next;
    if (queue.head)V(&queue.head->lock);
    V(&queue.mutex);
    Free(tmp);
}
