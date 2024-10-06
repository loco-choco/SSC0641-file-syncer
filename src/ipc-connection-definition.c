#include "ipc-connection-definition.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

typedef int SOCKET;
int read_ipc_socket_string(SOCKET ipc_client, char **string) {
  char buffer[20];
  int r;
  char *tmp = NULL;
  char *_string;
  while (1) {
    r = recv(ipc_client, &buffer, sizeof(buffer) - 1, MSG_PEEK);
    if (r == -1) {
      if (_string != NULL)
        free(_string);
      if (tmp != NULL)
        free(tmp);
      return -1;
    }
    // we preprend to the string what was from the last iteration, if anything,
    // and read the buffer contents, untill the '\0' read. If there is no '\0',
    // then read the whole contents. A way to know which case is this is by
    // making buffer a null terminated string, and then its strlen will tell us
    // if we read the '\0' from peek (when strlen < sizeof(buffer) - 1), or if
    // there is more on the stream to read (strlen == sizeof(buffer) - 1).
    buffer[sizeof(buffer) - 1] = '\0';
    int string_size = r;
    if (tmp != NULL)
      string_size += strlen(tmp);
    _string = malloc(sizeof(char) * (string_size + 1));

    if (tmp != NULL)
      strcpy(_string, tmp);
    r = read(ipc_client, &_string[string_size - r],
             sizeof(char) * (strlen(buffer) + 1));
    // we got to the '\0' in the stream (aka, the \0 is not the one added at the
    // end of the buffer), stop reading
    if (strlen(buffer) < (sizeof(buffer) - 1)) {
      break;
    }
    if (tmp != NULL)
      free(tmp);
    tmp = _string; // current string is the prefix from the next iteration
  }

  *string = _string;
  if (tmp != NULL)
    free(tmp);

  return 0;
}
