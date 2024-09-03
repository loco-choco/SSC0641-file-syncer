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
  file_table *files;
  if (get_files_in_dir(argv[1], &files) == 0) {
    file_table *entry = files;
    while (entry != NULL) {
      puts(entry->file);
      entry = entry->next;
    }
  } else
    perror("Couldn't open the directory");

  return 0;
}
