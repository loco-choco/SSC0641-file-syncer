#include "file-table.h"
#include "syncer.h"
#include <arpa/inet.h>
#include <dirent.h>
#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>

#define SOCKET_NAME "/tmp/file-syncer.socket"
#define LISTENER_BACKLOG_SIZE 20
#define RECEIVING_BUFFER_SIZE 12

typedef int SOCKET;

int main(int argc, char **argv) {
  pthread_t syncer_thread;
  SOCKET connection_socket;
  SOCKET data_socket;
  struct sockaddr_un connection_socket_name;
  char buffer[RECEIVING_BUFFER_SIZE];
  char *endptr;
  int ret;

  // Get args
  if (argc != 4) {
    fprintf(stderr, "Not enough args.\n");
    exit(EXIT_FAILURE);
  }
  char *dir = argv[1];
  int port;

  errno = 0;
  port = strtol(argv[2], &endptr, 10);

  if (errno == ERANGE) {
    fprintf(stderr, "Port is not a valid value.\n");
    exit(EXIT_FAILURE);
  }

  // Create Unix Domain Socket for IPC
  connection_socket = socket(AF_UNIX, SOCK_STREAM, 0);
  if (connection_socket == -1) {
    fprintf(stderr, "Error creating IPC socket.\n");
    exit(EXIT_FAILURE);
  }

  // Bind IPC socket to name
  // For portability. See:
  // https://www.man7.org/linux/man-pages/man7/unix.7.html
  memset(&connection_socket_name, 0, sizeof(connection_socket_name));
  connection_socket_name.sun_family = AF_UNIX;
  strncpy(connection_socket_name.sun_path, SOCKET_NAME,
          sizeof(connection_socket_name.sun_path) - 1);

  ret =
      bind(connection_socket, (const struct sockaddr *)&connection_socket_name,
           sizeof(connection_socket_name));
  if (ret == -1) {
    fprintf(stderr, "Error binding IPC socket to name %s.\n", SOCKET_NAME);
    exit(EXIT_FAILURE);
  }

  // Create syncer_init thread
  SYNCER_ARGS *syncer_args = calloc(1, sizeof(SYNCER_ARGS));
  syncer_args->port = port;
  syncer_args->dir = calloc(strlen(dir) + 1, sizeof(char));
  strcpy(syncer_args->dir, dir);
  pthread_create(&syncer_thread, NULL, (void *)syncer_init, syncer_args);

  // Listen for commands from clients
  ret = listen(connection_socket, LISTENER_BACKLOG_SIZE);
  if (ret == -1) {
    fprintf(stderr, "Error listening for connections on IPC socket.\n");
    exit(EXIT_FAILURE);
  }
  // Wait for commands from clients
  // Handle client connections
  while (1) {
    data_socket = accept(connection_socket, NULL, NULL);
    if (data_socket == -1) {
      fprintf(stderr, "Failed to accept client from  IPC socket.\n");
      exit(EXIT_FAILURE);
    }

    int end = 0;
    while (1) {
      int r;
      r = read(data_socket, buffer, sizeof(buffer));
      if (r == -1) {
        fprintf(stderr, "Failed to read data from client from  IPC socket.\n");
        exit(EXIT_FAILURE);
      }
      buffer[sizeof(buffer) - 1] = 0;

      if (!strncmp(buffer, "L BOZO", sizeof(buffer))) {
        end = 1;
        break;
      }
    }
    close(data_socket);

    if (end)
      break;
  }
  printf("Exiting program.\n");

  // Waiting for server to close
  pthread_join(syncer_thread, NULL);
  // Closing IPC socket
  close(connection_socket);
  // Removing tmp socket file
  unlink(SOCKET_NAME);

  exit(EXIT_SUCCESS);
}
