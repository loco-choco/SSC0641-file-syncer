#include <pthread.h>
#include <sys/socket.h>

#ifndef THREAD_LIST_H
#define THREAD_LIST_H

typedef struct syncee_thread_list {
  pthread_t thread;
  char *server_addr;
  struct thread_list *next;
} SYNCEE_THREAD_LIST;

typedef struct syncee_args {
  char *server_addr;
  int ip_type;
} SYNCEE_ARGS;

void *syncee_init(SYNCEE_ARGS *args);

#endif
