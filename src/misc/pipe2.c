#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include "pipe2.h"

/* Bidirectional pipe.
 * See
 *  http://www.unixwiz.net/techtips/remap-pipe-fds.html
 * and
 *  http://www.linuxquestions.org/questions/programming-9/where-do-i-get-a-bidirectional-popen-pipe-320699/
 * Also:
 *  http://stackoverflow.com/questions/13710003/execvp-fork-how-to-catch-unsuccessful-executions
 */

pipe2_t *p2open(const char *cmd, char *const argv[])
{
   pipe2_t *p2 = NULL;
   int writepipe[2] = {-1,-1},	/* parent -> child */
       readpipe [2] = {-1,-1};	/* child -> parent */
   pid_t	child_pid;

   /* Open read and write pipes */
   if (pipe(readpipe) < 0)
      goto done;
   if (pipe(writepipe) < 0) {
      close(readpipe[0]);
      close(readpipe[1]);
      goto done;
   }

   /* Convenient defines to make code easier to read */
#define PARENT_READ	readpipe[0]
#define CHILD_WRITE	readpipe[1]
#define CHILD_READ	writepipe[0]
#define PARENT_WRITE	writepipe[1]

   int lifeline[2];
   if (pipe(lifeline) < 0) {
      close(readpipe[0]);
      close(readpipe[1]);
      close(writepipe[0]);
      close(writepipe[1]);
      goto done;
   }

   /* Spawn child process */
   child_pid = fork();

   /* Failed to launch child process */
   if (child_pid < 0) {
      close(readpipe[0]);
      close(readpipe[1]);
      close(writepipe[0]);
      close(writepipe[1]);
      goto done;
   }

   /* Child process */
   if (child_pid == 0) {
      close(lifeline[0]);
      fcntl(lifeline[1], F_SETFD, FD_CLOEXEC);

      /* Close parent file descriptors */
      close(PARENT_WRITE);
      close(PARENT_READ);

      /* Attach input and output pipes to stdin, stdout */
      dup2(CHILD_READ,  STDIN_FILENO);  close(CHILD_READ);
      dup2(CHILD_WRITE, STDOUT_FILENO); close(CHILD_WRITE);
      execvp(cmd, argv);

      /* Write error code to the lifeline so the parent will know execvp
       * has failed.
       */
      write(lifeline[1], &errno, sizeof errno);
      exit(EXIT_FAILURE);
   }

   /* Monitor the lifeline */
   close(lifeline[1]);
   char buf[10];
   ssize_t res = read(lifeline[0], buf, 10);
   if (res > 0) { /* Received error code from child process */
      close(lifeline[0]);
      close(readpipe[0]);
      close(readpipe[1]);
      close(writepipe[0]);
      close(writepipe[1]);
      goto done;
   }
   close(lifeline[0]);

   /* Set up parent end of the pipe */
	close(CHILD_READ);
	close(CHILD_WRITE);

   FILE *in, *out;
   if (!(in=fdopen(PARENT_READ,"r"))) {
      close(PARENT_READ);
      close(PARENT_WRITE);
      goto done;
   }
   if (!(out=fdopen(PARENT_WRITE,"w"))) {
      fclose(out);
      close(PARENT_WRITE);
      goto done;
   }

   /* Turn off buffering for pipes */
   setvbuf(in,  NULL, _IONBF, 0);
   setvbuf(out, NULL, _IONBF, 0);

   p2 = malloc(sizeof *p2);

   p2->in_fd  = PARENT_READ;
   p2->in     = in;
   p2->out_fd = PARENT_WRITE;
   p2->out    = out;
   p2->pid    = child_pid;

done:
   return p2;
}

int p2close(pipe2_t *pipe)
{
   if (pipe) {
      int res1 = fclose(pipe->in);
      int res2 = fclose(pipe->out);
      if (res1==EOF || res2==EOF) return EOF;

      int status;
      while (waitpid(pipe->pid,&status,0)<0)
         if (errno != EINTR) return EOF;
   }

   return 0;
}

char *p2gets(char *s, size_t n, pipe2_t *pipe)
{
   return fgets(s, n, pipe->in);
}

static bool input_waiting(FILE *file)
{
   if (!file) return false;

   struct timeval timeout;
   fd_set readfds;

   FD_ZERO(&readfds);
   FD_SET(fileno(file), &readfds);

   /* Set to timeout immediately */
   timeout.tv_sec = 0;
   timeout.tv_usec = 0;
   int ready = select(fileno(file)+1, &readfds, 0, 0, &timeout);

   return (FD_ISSET(fileno(file), &readfds));
}

bool p2_input_waiting(pipe2_t *pipe)
{
   int status;
   if (!pipe) return false;

   if (waitpid(pipe->pid, &status, WNOHANG) != 0) return false;

   return input_waiting(pipe->in);
}

size_t p2write(pipe2_t *pipe, const void *ptr, size_t size)
{
   return fwrite(ptr, size, 1, pipe->out);
}
