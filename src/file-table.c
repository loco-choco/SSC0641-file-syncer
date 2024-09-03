#include <stdlib.h>
#include <sys/types.h>
#include <dirent.h>
#include <string.h>
#include "file-table.h"

int get_files_in_dir(char* dir, file_table** files){
  DIR *dp;
  struct dirent *ep;
  file_table* _files;
  dp = opendir(dir);
  if(dp==NULL) return -1;

  _files = NULL;
  while(ep = readdir(dp)){
    if(ep->d_type == DT_REG){
      file_table* new = malloc(sizeof(file_table));
      new->file = calloc(strlen(ep->d_name) + 1, sizeof(char));
      strcpy(new->file, ep->d_name);
      new->next = _files;
      _files = new;
    }
  }
  *files = _files;
  (void) closedir(dp);
  return 0;
}
