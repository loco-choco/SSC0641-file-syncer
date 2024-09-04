#ifndef SYNCER_H
#define SYNCER_H
typedef struct syncer_args {
  int port;
  char *dir;
} SYNCER_ARGS;

void *syncer_init(SYNCER_ARGS *args);
#endif
