#include "file-table.h"
#include "syncee.h"
#include "syncer.h"
#include <arpa/inet.h>
#include <dirent.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

int main(int argc, char **argv) {
  pthread_t syncer_thread;
  SYNCEE_THREAD_LIST *syncee_threads;
  pthread_t syncee_thread_ex;
  pthread_mutex_t syncee_threads_mutex;
  if (argc != 2) {
    perror("Not enough args");
    return -1;
  }
  char *ip = argv[1];

  // Create syncer_init thread
  SYNCER_ARGS *syncer_args = calloc(1, sizeof(SYNCER_ARGS));
  syncer_args->port = 8080;
  syncer_args->dir = calloc(2, sizeof(char));
  strcpy(syncer_args->dir, ".");
  pthread_create(&syncer_thread, NULL, (void *)syncer_init, syncer_args);

  // TODO Create syncee_init thread
  // printf("Creating client thread in 10s\n");
  // sleep(10);
  SYNCEE_ARGS *syncee_args = calloc(1, sizeof(SYNCEE_ARGS));
  syncee_args->port = 8080;
  syncee_args->server_addr = calloc(strlen(ip) + 1, sizeof(char));
  syncee_args->ip_type = AF_INET;
  strcpy(syncee_args->server_addr, ip);
  pthread_create(&syncee_thread_ex, NULL, (void *)syncee_init, syncee_args);

  pthread_join(syncer_thread, NULL);
  pthread_join(syncee_thread_ex, NULL);

  return 0;
}
