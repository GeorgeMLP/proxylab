/*
 * cache.h - Header file for the proxy's cache.
 *
 * Name: 马江岩
 * ID: 2000012707
 *
 */
#ifndef __proxy_cache
#define __proxy_cache
#include "csapp.h"

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 1000000
#define MAX_BLOCK_COUNT 20

struct _Str_list
{
    char *str;
    struct _Str_list *next;
};
typedef struct _Str_list Str_list;

struct _Cache
{
    char *host, *port, *path, *response;
    int length, last_time;
    struct _Cache *next;
};
typedef struct _Cache Cache;

struct _RW_worker
{
    sem_t lock;
    char type;
    struct _RW_worker *next;
};
typedef struct _RW_worker RW_worker;

struct _RW_queue
{
    sem_t mutex;
    int size;
    struct _RW_worker *head, *tail;
};
typedef struct _RW_queue RW_queue;

extern RW_queue queue;
extern sem_t time_lock;

void append_list(Str_list **head, char *str);
void free_list(Str_list **head);
void free_cache(Cache *p);
int read_queue(char *host, char *port, char *path, char **response);
void write_queue(Cache *r);
#endif