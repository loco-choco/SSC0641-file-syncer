#include "syncer.h"
#include "file-table.h"
#include "message-definition.h"
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
#define MAX_SENDING_BUFFER_SIZE 1024

void send_file_table(SOCKET client, FILE_TABLE *files);
void send_file_content(SOCKET client, char *file);

void *syncer_init(SYNCER_ARGS *args) {
  SOCKET listener;
  struct sockaddr_in addr;
  FILE_TABLE *files;

  // Create Listener Socket and Bind it to Port
  listener = socket(AF_INET, SOCK_STREAM, 0);
  if (listener < 0) {
    fprintf(stderr, "Wasn't able to create listener socket.\n");
    free(args);
    pthread_exit(NULL);
  }

  addr.sin_family = AF_INET;
  addr.sin_port = htons(args->port);
  addr.sin_addr.s_addr = INADDR_ANY;
  if (bind(listener, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
    fprintf(stderr, "Wasn't able to bind listener socket to port %d.\n",
            args->port);
    free(args);
    pthread_exit(NULL);
  }

  if (listen(listener, MAX_PENDING_CONECTIONS) != 0) {
    fprintf(stderr,
            "Wasn't able to listen for connection on listener socket.\n");
    free(args);
    pthread_exit(NULL);
  }

  // Scan for Files in Folder and generate Files Table
  if (get_files_in_dir(args->dir, &files) != 0) {
    fprintf(stderr, "Wasn't able to generate files table for %s.\n", args->dir);
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
      fprintf(stderr, "Accepting conection of client failed.\n");
      continue;
    }
    // Create threads for each connecting client
    // TODO CREATE LIST OF CREATED THREADS AND THEN WAIT FOR THEM TO CLOSE WHEN
    // FREEING STUFF
    // TODO CREATE syncer_receiver THREADS receving the socker
    // TODO CREATE syncee from this IP
  }

  // Close listening socket
  if (close(listener) != 0) {
    fprintf(stderr, "Failed to close listener socket.\n");
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
  int message_header;
  int received_amount;
  int connected = 0;
  while (connected == 0) {
    // Read Message Header
    int32_t received_header;
    received_amount =
        recv(client, &received_header, sizeof(received_header), 0);
    if (received_amount < sizeof(received_amount)) {
      fprintf(stderr, "Error receiving Header from client.\n");
      connected = -1;
      continue;
    }
    // Filter message type
    message_header = ntohl(received_header);
    if (message_header == FILE_TABLE_REQUEST) { // TABLE REQUEST
      // Send file table
      send_file_table(client, file_table);
    } else if (message_header == FILE_CONTENT_REQUEST) { // FILE CONTENT REQUEST
      // Receive string size
      int32_t received_string_size;
      int string_size;
      received_amount =
          recv(client, &received_string_size, sizeof(received_string_size), 0);
      if (received_amount < sizeof(received_string_size)) {
        fprintf(stderr, "Error receiving String Size from client.\n");
        connected = -1;
        continue;
      }
      string_size = ntohl(received_string_size);
      // Receive the string
      int string_left = string_size;
      char *file_name = calloc(string_size + 1, sizeof(char));
      // If it doesnt come all at once, read it in parts untill the whole size
      // of the string is read
      while (string_left > 0) {
        char *end_of_string = file_name + (string_size - string_left);
        received_amount =
            recv(client, end_of_string, string_left * sizeof(char), 0);
        if (received_amount < sizeof(received_string_size)) {
          fprintf(stderr, "Error receiving String from client.\n");
          connected = -1;
          break;
        }
        string_left -= received_amount;
      }
      // If during the string read something went wrong, continue
      if (connected < 0)
        continue;
      // Send file contents
      send_file_content(client, file_name);
    }
  }

  pthread_exit(NULL);
}

// File Table Request Message Format:
// HEADER (FILE_TABLE_REQUEST)
// [ STRING_SEPARATOR STRING_SIZE STRING]*
// MESSAGE_END
void send_file_table(SOCKET client, FILE_TABLE *files) {
  int32_t header = htonl(FILE_TABLE_REQUEST);
  char string_separator = '|';
  char message_end = '.';
  FILE_TABLE *pointer;
  // Send Header
  send(client, &header, sizeof(header), MSG_MORE);
  // Send Files
  pointer = files;
  while (pointer != NULL) {
    int32_t file_name_size = htonl(strlen(pointer->file));
    send(client, &string_separator, sizeof(char), MSG_MORE);
    send(client, &file_name_size, sizeof(file_name_size), MSG_MORE);
    send(client, pointer->file, sizeof(char) * file_name_size, MSG_MORE);
    pointer = pointer->next;
  }
  // Send end of Messgae
  send(client, &message_end, sizeof(char), 0);
}
// File Content Message Format:
// HEADER (FILE_CONTENT_REQUEST)
// FILE CONTENT
// EOF
void send_file_content(SOCKET client, char *file) {
  char buffer[MAX_SENDING_BUFFER_SIZE];
  int32_t header = htonl(FILE_CONTENT_REQUEST);
  char message_end = EOF;
  FILE *file_handler;
  // Send Header
  send(client, &header, sizeof(header), MSG_MORE);
  // Send File
  file_handler = fopen(file_name, "r");
  if (file_handler != NULL) {
    size_t read_bytes;
    while ((read_bytes = read(file_handler, buffer, MAX_SENDING_BUFFER_SIZE)) >
           0) {
      send(client, buffer, sizeof(char) * read_bytes, MSG_MORE);
    }
    fclose(file_handler);
  } else {
    fprintf(stderr,
            "Failed to open file %s, returning empty bytes to client.\n", file);
  }
  // Send end of Messgae
  send(client, &message_end, sizeof(char), 0);
}