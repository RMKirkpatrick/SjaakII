#ifndef PIPE2_H
#define PIPE2_H

#include <stdbool.h>
#include <unistd.h>
#include <stdio.h>

typedef struct {
   FILE *in;      /* Input stream for parent */
   FILE *out;     /* Output stream for parent */
   int in_fd, out_fd;
   pid_t pid;     /* pid of child process */
} pipe2_t;

pipe2_t *p2open(const char *cmd, char *const argv[]);
int p2close(pipe2_t *pipe);
char *p2gets(char *s, size_t n, pipe2_t *pipe);
bool p2_input_waiting(pipe2_t *pipe);
size_t p2write(pipe2_t *pipe, const void *ptr, size_t size);

#endif
