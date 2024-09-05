#include "syncee.h"
#include "file-table.h"
#include "message-definition.h"
#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

typedef struct named_pipes_list {
  FILE pipe;
  struct named_pipes_list *next;
} NAMED_PIPES_LIST;

typedef int SOCKET;

void *syncee_init(SYNCEE_ARGS *args) {
  SOCKET client;
  struct sockaddr_in server_addr_in;
  char *server_addr = args->server_addr;
  int server_ip_type = args->ip_type;

  FILE_TABLE *server_files;
  NAMED_PIPES_LIST *pipes_list;

  printf("Syncee thread created.\n");

  // Create Client Socket and Connect it to Server
  client = socket(AF_INET, SOCK_STREAM, 0);
  if (client < 0) {
    fprintf(stderr, "Wasn't able to create client socket.\n");
    free(args->server_addr);
    free(args);
    pthread_exit(NULL);
  }
  server_addr_in.sin_family = server_ip_type;
  server_addr_in.sin_port = htons(args->port);
  // Convert IP ADDR to number
  if (inet_pton(server_ip_type, server_addr, &server_addr_in.sin_addr) <= 0) {
    fprintf(stderr, "Invalid address %s\n", server_addr);
    free(args->server_addr);
    free(args);
    pthread_exit(NULL);
  }
  // Conect to server
  printf("Conecting to server in %s.\n", server_addr);
  if (connect(client, (struct sockaddr *)&server_addr_in,
              sizeof(server_addr_in)) < 0) {
    fprintf(stderr, "Wasn't able to connect to server in %s.\n", server_addr);
    goto CLOSE_FREE_AND_EXIT;
  }
  int connected = 0;

  // Ask For File Table from server
  printf("Asking server in %s for file table.\n", server_addr);

  // Send Request For File Table from server
  char file_table_request_header = FILE_TABLE_REQUEST;
  send(client, &file_table_request_header, sizeof(char), 0);
  // Receive File Table
  char message_header;
  int received_amount;
  received_amount = recv(client, &message_header, sizeof(message_header), 0);
  if (received_amount < sizeof(message_header)) {
    fprintf(stderr, "Error receiving Header from server.\n");
    connected = -1;
    goto CLOSE_FREE_AND_EXIT;
  }
  // Check if the Header is correct
  if (message_header != FILE_TABLE_REQUEST) {
    fprintf(stderr, "Server didnt send File Table Header.\n");
    connected = -1;
    goto CLOSE_FREE_AND_EXIT;
  }
  // Read and create file table
  printf("Receiving File Table from server.\n");
  int received_it_all = 0;
  char separator_read;
  char *file_name_string;
  server_files = NULL;
  while (received_it_all == 0) {
    received_amount = recv(client, &separator_read, sizeof(char), 0);
    if (received_amount < sizeof(char)) {
      fprintf(stderr, "Error receiving Separator from server.\n");
      connected = -1;
      goto CLOSE_FREE_AND_EXIT;
    }
    if (separator_read == FILE_TABLE_STRING_SEPARATOR) {
      int status = read_string_from_socket(client, &file_name_string);
      if (status == -1) {
        fprintf(stderr, "Error receiving string size from server.\n");
        connected = -1;
        goto CLOSE_FREE_AND_EXIT;

      } else if (status == -2) {
        fprintf(stderr, "Error receiving string from server.\n");
        connected = -1;
        goto CLOSE_FREE_AND_EXIT;
      }
      FILE_TABLE *new_entry = calloc(1, sizeof(FILE_TABLE));
      new_entry->file = file_name_string;
      new_entry->next = server_files;
      server_files = new_entry;

      printf("FILE: %s.\n", file_name_string);

    } else if (separator_read == FILE_TABLE_MESSAGE_END) {
      printf("Received end of file table.\n");
      received_it_all = 1;
    }
  }
  // TODO USE THIS EXAMPLE CODE
  /*
  // Ask For File Data from server
  printf("Asking server in %s for file data of '%s'.\n", server_addr,
         server_files->file);

  // Send Request For File Data from server
  char table_content_request_header = FILE_CONTENT_REQUEST;
  send(client, &table_content_request_header, sizeof(char), MSG_MORE);
  write_string_to_socket(server_files->file, client);
  send(client, NULL, 0, 0); // Confirm to send the buffer

  // Receive data From server
  received_amount = recv(client, &message_header, sizeof(message_header), 0);
  if (received_amount < sizeof(message_header)) {
    fprintf(stderr, "Error receiving Header from server.\n");
    connected = -1;
    goto CLOSE_FREE_AND_EXIT;
  }
  // Check if the Header is correct
  if (message_header != FILE_CONTENT_REQUEST) {
    fprintf(stderr, "Server didnt send File Content Header.\n");
    connected = -1;
    goto CLOSE_FREE_AND_EXIT;
  }
  printf("Receiving data from server.\n");

  char buffer[1024];
  int buffer_ocupied;
  buffer_ocupied = recv(client, buffer, sizeof(char) * 1024, 0);
  if (buffer_ocupied >= 1024)
    printf("More then buffer\n");
  else {
    buffer[buffer_ocupied - 1] = '\0';
    printf("Contents:\n%s\n", buffer);
  }*/

  // Create Named Pipes from each file name
  FILE_TABLE *file = server_files;
  while (file != NULL) {
    char *prefix = "pipes/";
    char *pipe_name =
        calloc(strlen(prefix) + strlen(file->file) + 1, sizeof(char));
    strcat(pipe_name, prefix);
    strcat(pipe_name, file->file);
    mkfifo(pipe_name, 0666);
    file = file->next;
    printf("making pipe %s.\n", pipe_name);
    free(pipe_name);
  }
  // Add inotify for each

CLOSE_FREE_AND_EXIT:
  printf("Closing connection to %s.\n", server_addr);
  // Close Socket
  if (close(client) != 0) {
    fprintf(stderr, "Failed to close client socket.\n");
  }
  // Free and Exit
  FILE_TABLE *next;
  while (server_files != NULL) {
    next = server_files->next;
    free(server_files->file);
    free(server_files);
    server_files = next;
  }
  free(args->server_addr);
  free(args);
  pthread_exit(NULL);
}
