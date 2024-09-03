#include "syncer.h"
#include "file-table.h"
#include <errno.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

typedef int SOCKET;

#define MAX_PENDING_CONECTIONS 10
#define MAX_RECEIVER_BUFFER_SIZE 128

void *syncer_init(SYNCER_ARGS *args) {
  SOCKET listener;
  struct sockaddr_in addr;
  FILE_TABLE *files;

  // Create Listener Socket and Bind it to Port
  listener = socket(AF_INET, SOCK_STREAM, 0);
  if (listener < 0) {
    perror("Wasn't able to create listener socket.");
    free(args);
    pthread_exit(NULL);
  }

  addr.sin_family = AF_INET;
  addr.sin_port = htons(args->port);
  addr.sin_addr.s_addr = INADDR_ANY;
  if (bind(listener, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
    perror("Wasn't able to bind listener socket to port %d.", args->port);
    free(args);
    pthread_exit(NULL);
  }

  if (listen(listener, MAX_PENDING_CONECTIONS) != 0) {
    perror("Wasn't able to listen for connection on listener socket.");
    free(args);
    pthread_exit(NULL);
  }

  // Scan for Files in Folder and generate Files Table
  if (get_files_in_dir(args->dir, &files) != 0) {
    perror("Wasn't able to generate files table for %s.", args->dir);
    free(args);
    pthread_exit(NULL);
  }
  while (1) {
    SOCKET client;
    struct sockaddr client_addr;
    socklen_t client_addr_len;
    // Wait for new connections
    client = accept(listener, &client_addr, &client_addr_len);
    if (client < 0) {
      perror("Accepting conection of client failed.");
      continue;
    }
    // Create threads for each connecting client
    // TODO CREATE LIST OF CREATED THREADS AND THEN WAIT FOR THEM TO CLOSE WHEN
    // FREEING STUFF
  }

  // Close listening socket
  if (close(listener) != 0) {
    perror("Failed to close listener socket.");
  }

  // Free and Exit
  free(args);
  FILE_TABLE *next;
  while (files != NULL) {
    next = files->next;
    free(files->file);
    free(files);
    files = next;
  }
  pthread_exit(NULL);
}

void *syncer_receiver(SYNCER_RECEIVER_ARGS *args) {
  SOCKET client = args->socket;
  FILE_TABLE *file_table = args->files;
  char buffer[MAX_RECEIVER_BUFFER_SIZE];
  int received_amount;
  int connected = 0;
  while (connected == 0) {
    received_amount = recv(client, buffer, MAX_RECEIVER_BUFFER_SIZE, 0);
    if (received_amount == -1) {
      perror("Error receiving data from client.");
      break;
    }
  }

  pthread_exit(NULL);
}

// Message Format:
// HEADER - AMOUNT OF FILES - (STRING-SIZE STRING)
void send_file_table(SOCKET client, FILE_TABLE *files) {
  char *buffer;
  int amount_of_files = 0;
  FILE_TABLE *pointer;
  // Get Amount of files
  pointer = files;
  while (pointer != NULL) {
    amount_of_files++;
    pointer = pointer->next;
  }
}
