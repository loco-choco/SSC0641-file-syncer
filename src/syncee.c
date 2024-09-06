#include "syncee.h"
#include "file-table.h"
#include "message-definition.h"
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <poll.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/inotify.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

typedef int SOCKET;
typedef int INOTIFY_FD;
typedef int WATCHER;
typedef int PIPE;

typedef struct named_pipes_list {
  WATCHER pipe_watcher;
  char *name;
  struct named_pipes_list *next;
} NAMED_PIPES_LIST;

// int handle_inotify_event(INOTIFY_API inotify, SOCKET client);

void *syncee_init(SYNCEE_ARGS *args) {
  SOCKET client;
  struct sockaddr_in server_addr_in;
  char *server_addr = args->server_addr;
  int server_ip_type = args->ip_type;

  FILE_TABLE *server_files;
  NAMED_PIPES_LIST *pipes_list;
  INOTIFY_FD inotify;

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
  // Create fd for inotify API
  inotify = inotify_init1(IN_NONBLOCK);
  if (inotify == -1) {
    perror("inotify_init1");
    fprintf(stderr, "Issues creating inotify api file descriptor, exiting and "
                    "disconecting.\n");
    connected = -1;
    goto CLOSE_FREE_AND_EXIT;
  }
  // POLLOUT

  // Create Named Pipes from each file name, add add watchers for each
  printf("Creating pipe files.\n");
  pipes_list = NULL;
  FILE_TABLE *file = server_files;
  while (file != NULL) {
    char *prefix = "server-";
    char *pipe_name =
        calloc(strlen(prefix) + strlen(file->file) + 1, sizeof(char));
    strcat(pipe_name, prefix);
    strcat(pipe_name, file->file);
    // Add to list and create named pipe
    NAMED_PIPES_LIST *new = malloc(sizeof(NAMED_PIPES_LIST));
    new->next = pipes_list;
    new->name = pipe_name;
    mkfifo(pipe_name, 0666);
    // Open pipe as write only
    new->pipe_watcher =
        inotify_add_watch(inotify, pipe_name, IN_OPEN | IN_CLOSE);
    if (new->pipe_watcher == -1) {
      fprintf(stderr, "Issues watching %s, removing named pipe.\n", pipe_name);
      unlink(pipe_name);
      free(pipe_name);
      free(new);
      file = file->next;
      continue;
    }
    pipes_list = new;

    file = file->next;
  }

  // Prepare for inotify pooling
  struct pollfd inotify_pool;
  inotify_pool.events = POLLIN; // read events
  inotify_pool.fd = inotify;

  int watching = 0;
  printf("Pooling file events.\n");

  while (watching == 0 && connected == 0) {
    int ready = poll(&inotify_pool, 1, -1);
    if (ready == -1) {
      if (errno == EINTR)
        continue;
      fprintf(
          stderr,
          "Issues while pooling inotivy events, exiting and disconecting.\n");
      watching = -1;
      continue;
    }
    // Receive pipe Events
    printf("Receiving inotify events.\n");
    if (inotify_pool.revents & POLLOUT) {
      printf("Event :).\n");
    }
  }

CLOSE_FREE_AND_EXIT:
  printf("Closing connection to %s.\n", server_addr);
  // Close Socket
  if (close(client) != 0) {
    fprintf(stderr, "Failed to close client socket.\n");
  }
  // Free file table and pipe list, and exit
  NAMED_PIPES_LIST *pipe_next;
  while (pipes_list != NULL) {
    pipe_next = pipes_list->next;
    unlink(pipes_list->name);
    free(pipes_list->name);
    free(pipes_list);
    pipes_list = pipe_next;
  }
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
/*
int handle_inotify_event(INOTIFY_API inotify, SOCKET client) {
  // Aligned buffer, see reason in the example at
  // https://man7.org/linux/man-pages/man7/inotify.7.html
  char buf[4096] __attribute__((aligned(__alignof__(struct inotify_event))));
  const struct inotify_event *event;
  ssize_t len;
  int read_all_events = 0;
  while (read_all_events == 0) {
    // Read events to buffer
    len = read(inotify, buf, sizeof(buf));
    if (len == -1 && errno != EAGAIN) {
      fprintf(stderr, "Issue reading inotify events.\n");
      return -1;
    }
    // No more events, exit
    if (len <= 0) {
      read_all_events = 1;
      break;
    }
    // Loop over events
    for (char *ptr = buf; ptr < buf + len;
         ptr += sizeof(struct inotify_event) + event->len) {

      event = (const struct inotify_event *)ptr;
      if (event->mask & IN_OPEN)
        printf("Someone Opened Pipe %s.\n", event->name);
    }
  }
  return 0;
}*/
