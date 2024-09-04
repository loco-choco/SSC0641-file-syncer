#include <pthread.h>
#ifndef THREAD_LIST_H
#define THREAD_LIST_H

typedef struct syncee_thread_list {
  pthread_t thread;
  int socket;
  struct thread_list *next;
} SYNCEE_THREAD_LIST;

#endif
