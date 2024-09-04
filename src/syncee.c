#include "syncee.h"
#include "file-table.h"
#include "message-definition.h"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>

typedef int SOCKET;
void *syncee_init(SYNCEE_ARGS *args) {
  SOCKET client;
  struct sockaddr_in server_addr_in;
  char *server_addr = args->server_addr;
  int server_ip_type = args->ip_type;
  FILE_TABLE *server_files;

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
    goto FREE_AND_EXIT;
  }
  int connected = 0;

  // Ask For File Table from server
  printf("Asking server in %s for file table.\n", server_addr);

  // Send Request For File Table from server
  int32_t file_table_request_header = htonl(FILE_TABLE_REQUEST);
  send(client, &file_table_request_header, sizeof(int32_t), 0);
  // Receive File Table
  int message_header;
  int32_t received_header;
  int received_amount;
  received_amount = recv(client, &received_header, sizeof(received_amount), 0);
  if (received_amount < sizeof(received_amount)) {
    fprintf(stderr, "Error receiving Header from server.\n");
    connected = -1;
    free(args->server_addr);
    free(args);
    pthread_exit(NULL);
  }
  // Check if the Header is correct
  message_header = ntohl(received_header);
  if (message_header != FILE_TABLE_REQUEST) {
    fprintf(stderr, "Server didnt send File Table Header.\n");
    connected = -1;
    free(args->server_addr);
    free(args);
    pthread_exit(NULL);
  }
  // Read and create file table
  printf("Receiving File Table from server.\n");
  int received_it_all = 0;
  char separator_read;
  int file_name_size;
  int32_t received_file_name_size;
  char *file_name_string;
  server_files = NULL;
  while (received_it_all == 0) {
    received_amount = recv(client, &separator_read, sizeof(char), 0);
    if (received_amount < sizeof(char)) {
      fprintf(stderr, "Error receiving Separator from server.\n");
      connected = -1;
      goto FREE_AND_EXIT;
    }
    if (separator_read == '|') {
      received_amount =
          recv(client, &received_file_name_size, sizeof(int32_t), 0);
      if (received_amount < sizeof(int32_t)) {
        fprintf(stderr, "Error receiving string size from server.\n");
        connected = -1;
        goto FREE_AND_EXIT;
      }
      file_name_size = ntohl(received_file_name_size);
      file_name_string = calloc(file_name_size + 1, sizeof(char));
      int string_left = file_name_size;

      while (string_left > 0) {
        char *end_of_string = file_name_string + (file_name_size - string_left);
        received_amount =
            recv(client, end_of_string, string_left * sizeof(char), 0);
        if (received_amount <= 0) {
          fprintf(stderr, "Error receiving string from server.\n");
        }

        string_left -= received_amount;
      }

      FILE_TABLE *new_entry = calloc(1, sizeof(FILE_TABLE));
      new_entry->file = file_name_string;
      new_entry->next = server_files;
      server_files = new_entry;

      printf("FILE: %s.\n", file_name_string);

    } else if (separator_read == '.') {
      received_it_all = 1;
    }
  }
FREE_AND_EXIT:
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
