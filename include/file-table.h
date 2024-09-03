#ifndef FILE_TABLE
#define FILE_TABLE

typedef struct FILE_TABLE {
  char *file;
  struct file_table *next;
} FILE_TABLE;

int get_files_in_dir(char *dir, FILE_TABLE **files);

#endif
