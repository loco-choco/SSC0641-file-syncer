#ifndef FILE_TABLE
#define FILE_TABLE

typedef struct file_table {
  char* file;
  struct file_table* next;
} file_table;

int get_files_in_dir(char* dir, file_table** files);

#endif


