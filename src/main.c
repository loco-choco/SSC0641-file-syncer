#include "file-table.h"
#include "ipc-connection-definition.h"
#include "syncee.h"
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

#define LISTENER_BACKLOG_SIZE 20

typedef int SOCKET;

int main(int argc, char **argv) {
  pthread_t syncer_thread;
  SOCKET connection_socket;
  SOCKET data_socket;
  struct sockaddr_un connection_socket_name;
  char *endptr;
  int ret;

  // Get args
  if (argc != 3) {
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
  printf("Creating IPC socket.\n");
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
  printf("Waiting for conections in the IPC socket.\n");
  while (1) {
    data_socket = accept(connection_socket, NULL, NULL);
    printf("New client in IPC socket.\n");
    if (data_socket == -1) {
      fprintf(stderr, "Failed to accept client from  IPC socket.\n");
      exit(EXIT_FAILURE);
    }
    // TODO FIX READING SO IT DOESNT END UP READING THE WHOLE INPUT IN THE
    // BUFFER MAKE IT STOP IN \0
    int end = 0;
    int status;
    char *command;
    status = read_ipc_socket_string(data_socket, &command);
    if (status == -1) {
      fprintf(stderr, "Failed to read data from client from  IPC socket.\n");
      exit(EXIT_FAILURE);
    }

    if (!strcmp(command, "close")) {
      end = 1;
    } else if (!strcmp(command, "list") || !strcmp(command, "cat")) {
      char *file_name = NULL; // If it isnt a file cat, it being NULL creates a
                              // list file request
      int cat = strcmp(command, "cat");
      if (cat == 0) {
        status = read_ipc_socket_string(data_socket, &file_name);
        if (status == -1) {
          fprintf(stderr,
                  "Failed to read file name from client from  IPC socket.\n");
          close(data_socket);
          continue;
        }
        printf("Client wants to cat file.\n");
      } else {
        printf("Client wants to list files.\n");
      }
      // Receive ip from client
      char *ip;
      status = read_ipc_socket_string(data_socket, &ip);
      printf("ip. %s\n", ip);
      if (status == -1) {
        fprintf(stderr, "Failed to read ip from client from  IPC socket.\n");
        free(file_name);
        close(data_socket);
        continue;
      }

      // Receive port from client
      char *port;
      status = read_ipc_socket_string(data_socket, &port);
      printf("port. %s\n", port);
      if (status == -1) {
        fprintf(stderr, "Failed to read ip from client from  IPC socket.\n");
        free(file_name);
        free(ip);
        close(data_socket);
        continue;
      }

      int server_port;
      errno = 0;
      server_port = strtol(port, &endptr, 10);
      free(port);
      printf("port %d\n", server_port);
      if (errno == ERANGE) {
        fprintf(stderr, "Port is not a valid value.\n");
        free(file_name);
        close(data_socket);
        free(ip);
        continue;
      }

      // Create syncer_init thread to deal with the request
      SYNCEE_ARGS *syncee_args = calloc(1, sizeof(SYNCEE_ARGS));
      syncee_args->ipc_client = data_socket;
      syncee_args->port = server_port;
      syncee_args->ip_type = AF_INET;
      syncee_args->server_addr = ip;
      syncee_args->requested_file =
          file_name; // This requests for the file table
      syncee_init(syncee_args);
    } else { // Not valid request
      close(data_socket);
    }

    if (end) {
      close(data_socket);
      break;
    }
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
