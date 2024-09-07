#include "file-table.h"
#include "syncer.h"
#include <arpa/inet.h>
#include <dirent.h>
#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#define SOCKET_NAME "/tmp/file-syncer.socket"
#define RECEIVING_BUFFER_SIZE 12

typedef int SOCKET;

int main(int argc, char **argv) {
  pthread_t syncer_thread;
  SOCKET connection_socket;
  char buffer[RECEIVING_BUFFER_SIZE];
  char *endptr;

  // Get args
  if (argc != 4) {
    perror("Not enough args");
    return -1;
  }
  char *dir = argv[1];
  int port;

  errno = 0;
  port = strtol(argv[2], &endptr, 10);

  if (errno == ERANGE) {
    perror("Port not a valid value");
    exit(EXIT_FAILURE);
  }

  // Create syncer_init thread
  SYNCER_ARGS *syncer_args = calloc(1, sizeof(SYNCER_ARGS));
  syncer_args->port = port;
  syncer_args->dir = calloc(strlen(dir) + 1, sizeof(char));
  strcpy(syncer_args->dir, dir);
  pthread_create(&syncer_thread, NULL, (void *)syncer_init, syncer_args);

  // Create Unix Domain Socket for IPC
  // Receive commands from clients

  // Waiting for server to close
  pthread_join(syncer_thread, NULL);
  // Clean up
  return 0;
}
