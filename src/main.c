#include "file-table.h"
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

int main(int argc, char **argv) {

  if (argc != 2) {
    perror("Not enough args");
    return -1;
  }
  char *ip = argv[1];

  return 0;
}
