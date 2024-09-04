#include <pthread.h>
#ifndef THREAD_LIST_H
#define THREAD_LIST_H
typedef struct threads_list {
  pthread_t thread;
  struct threads_list *next;
} THREAD_LIST;
#endif
