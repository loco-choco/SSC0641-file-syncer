#include "file-table.h"
#include "syncee.h"
#include "syncer.h"
#include <dirent.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

int main(int argc, char **argv) {
  pthread_t syncer_thread;
  SYNCEE_THREAD_LIST *syncee_threads;
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
  //

  pthread_join(syncer_thread, NULL);

  return 0;
}
