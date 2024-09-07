#include "ipc-connection-definition.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>

#define RECEIVING_STREAM_BUFFER_SIZE 1024

typedef int SOCKET;

int main(int argc, char **argv) {
  SOCKET data_socket;
  struct sockaddr_un server_addr;
  ssize_t r, w;
  int ret;

  // Get args
  if (argc < 2) {
    fprintf(stderr, "Not enough args.\n");
    exit(EXIT_FAILURE);
  } else if (argc > 2) {
    fprintf(stderr, "Too many args.\n");
    exit(EXIT_FAILURE);
  }
  char *command = argv[1];

  // Create Unix Domain Socket for connecting to server
  printf("Creating IPC socket client.\n");
  data_socket = socket(AF_UNIX, SOCK_STREAM, 0);
  if (data_socket == -1) {
    fprintf(stderr, "Error creating IPC socket.\n");
    exit(EXIT_FAILURE);
  }

  // Bind IPC socket to name
  // For portability. See:
  // https://www.man7.org/linux/man-pages/man7/unix.7.html
  memset(&server_addr, 0, sizeof(server_addr));
  server_addr.sun_family = AF_UNIX;
  strncpy(server_addr.sun_path, SOCKET_NAME, sizeof(server_addr.sun_path) - 1);

  ret = connect(data_socket, (const struct sockaddr *)&server_addr,
                sizeof(server_addr));
  if (ret == -1) {
    fprintf(stderr, "Server not responding.\n");
    exit(EXIT_FAILURE);
  }

  // Send command to server
  int success_state = EXIT_SUCCESS;
  w = write(data_socket, command, strlen(command) + 1);
  if (w == -1) {
    fprintf(stderr, "Issues sending command to server.\n");
    success_state = EXIT_FAILURE;
  }

  // Closing socket and exiting
  close(data_socket);
  exit(success_state);
}
