/*  Sjaak, a program for playing chess variants
 *  Copyright (C) 2011  Evert Glebbeek
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <stdio.h>
#include <ctype.h>
#include <stdarg.h>
#include <inttypes.h>
#include <limits.h>
#include <signal.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <math.h>
#include "pipe2.h"
#include "sprt.h"
#include "timer.h"
#include "genrand.h"

#define streq(s1, s2) (strcmp((s1), (s2)) == 0)

typedef enum side_t { NONE=-1, WHITE, BLACK, NUM_SIDES } side_t;

typedef struct {
   pipe2_t *f;
   char *name;
   uint32_t state;
   int64_t clock;       /* Clock time, in msec */
   int ping;
   int moves;           /* Number of moves played */
   int id;
   int win;
   int draw;
   int nv;
   int depth, score, time;
   char **variants;
   char *fen;
   char *move;
   char *result_str;
} program_t;

static struct {
   char *move;
   int depth, score, time;
} *history = NULL;
static int history_size = 0;
static int moves_played = 0;

/* Program state flags */
#define PF_INIT      0x00000001
#define PF_DEAD      0x00000002
#define PF_PING      0x00000004
#define PF_TIME      0x00000008
#define PF_VARS      0x00000010
#define PF_FORCE     0x00000020
#define PF_THINK     0x00000040
#define PF_USERMOVE  0x00000080
#define PF_FORFEIT   0x00000100
#define PF_CLAIMD    0x00000200
#define PF_CLAIMW    0x00000400
#define PF_CLAIMB    0x00000800
#define PF_CLAIM     (PF_CLAIMD | PF_CLAIMW | PF_CLAIMB)
#define PF_RESIGN    0x00001000
#define PF_FLAG      0x00002000
#define PF_MEMORY    0x00004000
#define PF_SJEF      0x00010000
#define PF_SYNC      0x00020000
#define PF_FEN       0x00040000
#define PF_MOVE      0x00080000
#define PF_UNLOG     0x00100000

static program_t *prog[3];
static program_t *referee;
chess_clock_t chess_clock;

static uint64_t start_time = 10000;  /* Msec */
static int moves_per_tc = 40;
static double time_inc = 0;
static int min_time_per_move = 0;

static bool check_children;
static FILE *logfile = NULL;
static char *buf;
#define BUF_SIZE  65536

bool child_is_alive(program_t * const prog)
{
   int status;
   pid_t result = waitpid(prog->f->pid, &status, WNOHANG);
   if (result == 0) {
      return true;
   } else if (result == -1) {
      //perror(NULL);
   }

   // Child exited
   return false;
}

static void child_signal_handler(int sig)
{
   /* Reap zombie processes */
   int saved_errno = errno;
   while (waitpid((pid_t)(-1), NULL, WNOHANG) > 0) {}
   errno = saved_errno;

   /* If the referee died, we terminate immediately */
   if (!child_is_alive(referee)) {
      fprintf(stderr, "*** Referee died, exit.\n");
      exit(EXIT_FAILURE);
   }

   check_children = true;
   if (sig == SIGCHLD)
      signal(sig, child_signal_handler);
}

static void record_move(const char *move)
{
   if (moves_played >= history_size) {
      history_size += 200;
      history = realloc(history, history_size * sizeof *history);
   }

   while (*move && isspace(*move)) move++;
   history[moves_played].move = strdup(move);
   moves_played++;
}

static void clear_history(void)
{
   for (int n=0; n<moves_played; n++)
      free(history[n].move);
   moves_played = 0;
}

static void parse_engine_input(program_t *prog, char *input);

static void send_to_program(program_t *prog, const char *msg, ...)
{
   va_list ap;
   static bool newline = true;
   va_start(ap, msg);
   vsnprintf(buf, BUF_SIZE-1, msg, ap);
   va_end(ap);

   if (logfile && !(prog->state & PF_UNLOG)) {
      if (newline)
         fprintf(logfile, "%d> ", prog->id);
      newline = false;
      fprintf(logfile, "%s", buf);
      if (strstr(buf, "\n"))
         newline = true;
      fflush(logfile);
   }


   if (strlen(buf)) {
      ssize_t res = p2write(prog->f, buf, strlen(buf));
      if (res < 0) {
         int e = errno;
         perror(NULL);
         if (logfile && !(prog->state & PF_UNLOG))
            fprintf(logfile, "Write error to program %d (%s): %s\n",
                  prog->id, prog->name, strerror(e));
         exit(EXIT_FAILURE);
      }
   }
}

void msleep(int msec)
{
   struct timespec timeout;

   timeout.tv_sec = msec / 1000;
   timeout.tv_nsec = (msec % 1000)*1000000;
   nanosleep(&timeout, NULL);
}

void wait_input(program_t **prog, int nprog, int musec)
{

   struct timeval timeout;
   fd_set readfds, errfds;

   FD_ZERO(&readfds);
   FD_ZERO(&errfds);
   for (int n = 0; n<nprog; n++) {
      int fd = prog[n]->f->in_fd;
      FD_SET(fd, &readfds);
      FD_SET(fd, &errfds);
   }

   //printf("Waiting for input...\n");
   timeout.tv_sec = musec / 1000000;
   timeout.tv_usec = musec % 1000000;
   select(32, &readfds, NULL, &errfds, &timeout);

   //printf("Received input\n");
}

void move_to_program(program_t *prog, const char *move)
{
   send_to_program(prog, "%s%s\n", (prog->state & PF_USERMOVE) ?  "usermove " : "", move);
}

/* Synchronise communication with a program */
bool synchronise(program_t *prog)
{
   if (prog->state & PF_PING) {
      prog->ping++;
      prog->state &= ~PF_SYNC;

      send_to_program(prog, "ping %d\n", prog->ping);

      while(child_is_alive(prog) && !(prog->state & PF_SYNC)) {
         while(!p2_input_waiting(prog->f));
         while(p2_input_waiting(prog->f)) {
            if (!p2gets(buf, BUF_SIZE, prog->f))
               perror(NULL);

            parse_engine_input(prog, buf);
         }
      }
   } else {
      prog->state |= PF_SYNC;
   }

   return (prog->state & PF_SYNC);
}

const char *get_fen(void)
{
   if (referee->state & PF_SJEF) {
      referee->state |= PF_UNLOG;
      synchronise(referee);
      referee->state |= PF_FEN;
      send_to_program(referee, "fen\n");
      synchronise(referee);
      referee->state &= ~PF_FEN;
      referee->state &= ~PF_UNLOG;
      return referee->fen;
   }
   return NULL;
}

const char *get_san_move(const char *move)
{
   if (referee->state & PF_SJEF) {
      synchronise(referee);
      referee->state |= PF_MOVE;
      send_to_program(referee, "san %s\n", move);
      synchronise(referee);
      referee->state &= ~PF_MOVE;
      return referee->move;
   }
   return move;
}

bool move_to_referee(const char *move)
{
   while (*move && isspace(*move)) move++;
   synchronise(referee);
   referee->state &= ~PF_FORFEIT;
   move_to_program(referee, move);
   synchronise(referee);
   return !(referee->state & PF_FORFEIT);
}

static void parse_engine_input(program_t *prog, char *input)
{
   char *line = input;
   char *token;
   char *next = NULL;

   if (!input || input[0] == '\0') return;

   line = input;

   while ((token = strchr(line, '\r'))) *token = '\n';

   /* Remove leading white space */
   while (isspace(*line)) line++;

   /* Break up lines at linebreaks */
   next = strstr(line, "\n");
   if (next) {
      next[0] = '\0';
      next++;
   }

   if (logfile) {
      fprintf(logfile, "%d< %s\n", prog->id, line);
      fflush(logfile);
   }

   /* Remove comments */
   token = strstr(line, "#");
   if (token) *token = '\0';

   /* Remove trailing white space */
   token = line+strlen(line);
   while (token>line && isspace(token[-1])) {
      token--; *token='\0';
   }

   /* Skip empty line */
   if (!line[0]) goto done;

   //printf(">%s<\n", line);
   /* Parse engine input */
   if (strstr(line, "move") == line) {
      /* Stop the clock */
      uint64_t time = peek_timer(&chess_clock);

      /* Store the move in the game history */
      record_move(line+5);
      const char *movestr = history[moves_played-1].move;
      history[moves_played-1].score = prog->score;
      history[moves_played-1].depth = prog->depth;
      history[moves_played-1].time  = (int)time;

      /* Play the move on the board. If it is illegal, the engine forfeits the game. */
      bool legal = move_to_referee(movestr);
      prog->clock -= time;
      prog->moves++;
      prog->state &= ~PF_THINK;

      /* Update the engine's clock */
      prog->clock += time_inc;
      if (moves_per_tc && prog->moves > moves_per_tc-1 && (prog->moves % moves_per_tc) == 0) {
         if (logfile) fprintf(logfile, "Time control: %"PRIi64" -> %"PRIu64"\n", prog->clock, prog->clock+start_time);
         prog->clock += start_time;
      }
      if (prog->clock < 0) {
         prog->state |= PF_FLAG;
         prog->clock = 0;
      }
      if (legal) {
         if (logfile) {
            fprintf(logfile, "Move '%s' legal, time = %"PRIu64" (%"PRIu64" remaining), move %d/%d\n",
                  movestr, time, prog->clock, prog->moves, 40);
            fflush(logfile);
         }
      } else {
         prog->state |= PF_FORFEIT;
         if (logfile) {
            fprintf(logfile, "Illegal move: '%s'\n", movestr);
            fflush(logfile);
         }
      }
   } else if (strstr(line, "#") == line) {
      /* Comment, ignored */
   } else if (strstr(line, "tellusererror") == line) {
      fprintf(stderr, "%s\n", line+9);
   } else if (strstr(line, "telluser") == line) {
      printf("%s\n", line+9);
   } else if (strstr(line, "resign") == line) {
      prog->state &= ~PF_THINK;
      prog->state |= PF_RESIGN;
   } else if (strstr(line, "1-0") == line) {
      prog->state &= ~PF_THINK;
      prog->state |= PF_CLAIMW;
      free(prog->result_str);
      prog->result_str = strdup(line);
   } else if (strstr(line, "0-1") == line) {
      prog->state &= ~PF_THINK;
      prog->state |= PF_CLAIMB;
      free(prog->result_str);
      prog->result_str = strdup(line);
   } else if (strstr(line, "1/2-1/2") == line) {
      prog->state &= ~PF_THINK;
      prog->state |= PF_CLAIMD;
      free(prog->result_str);
      prog->result_str = strdup(line);
   } else if (strstr(line, "draw") == line) {
   } else if (strstr(line, "pong") == line) {
      int pong = 0;
      sscanf(line, "pong %d", &pong);
      if (pong == prog->ping)
         prog->state |= PF_SYNC;
   } else if (strstr(line, "Illegal move")) {
      /* Verify claim.
       * Referees are trusted, engines that make illegal move claims are forfeited.
       */
      prog->state &= ~PF_THINK;
      prog->state |= PF_FORFEIT;
      if (logfile) {
         char *movestr = strstr(line, ":");
         if (movestr) {
            movestr++;
            while (*movestr && isspace(*movestr)) movestr++;
         }
         if (!movestr) movestr = line;
         movestr = strdup(movestr);
         const char *fen = get_fen();
         if (!fen) fen = "(unknown)";
         if (prog == referee)
            fprintf(logfile, "Referee rejected move %s in position %s\n", movestr, fen);
         else
            fprintf(logfile, "False illegal move claim '%s' by %s in position %s\n", movestr, prog->name, fen);
         free(movestr);
         fflush(logfile);
      }
   } else if (strstr(line, "Error") == line) {
   } else if (strstr(line, "feature") == line) {
      /* Parse the different feature options */
      char *ss = strstr(line, " ");
      while (ss && ss[0]) {
         char *feature;
         char *value;

         feature = ss;
         while (*feature && isspace(*feature)) feature++;

         value = strstr(feature, "=");
         if (!value) break;
         value[0] = '\0';
         value++;

         /* String or numeric? */
         if (value[0] == '"') {
            value++;

            char *s = value;
            while (*s && *s != '"') s++;
            s[0] = '\0';
            ss = s+1;
         } else {
            char *s = value;
            while (*s && !isspace(*s)) s++;
            ss = s+1;
         }
         //printf("'%s' '%s'\n", feature, value);

         bool accept = false;
         if (streq(feature, "myname")) {
            int len = strlen(value);
            prog->name = realloc(prog->name, len+1);
            int n;
            for (n=0; n<len; n++)
               prog->name[n] = value[n];
            prog->name[len] = '\0';
            accept = true;
            if (logfile) {
               fprintf(logfile, "Set name '%s' for program %d\n", prog->name, prog->id);
               fflush(logfile);
            }
         } else if (streq(feature, "sigint")) {
            accept = true;
         } else if (streq(feature, "setboard")) {
            accept = true;
         } else if (streq(feature, "memory")) {
            accept = true;
            prog->state |= PF_MEMORY;
         } else if (streq(feature, "time")) {
            accept = true;
            prog->state |= PF_TIME;
         } else if (streq(feature, "sjef")) {
            accept = true;
            prog->state |= PF_SJEF;
         } else if (streq(feature, "variants")) {
            accept = true;
            prog->state |= PF_VARS;
            int nv = 1;
            for (char *s = value; *s; s++) nv += (*s == ',');

            prog->nv = nv;
            prog->variants = calloc(nv, sizeof *prog->variants);

            char *str = value, *s;
            int n = 0;
            while ((s = strtok(str, ","))) {
               str = NULL;
               prog->variants[n++] = strdup(s);
               //printf("%d '%s'\n", n, prog->variants[n-1]);
            }
         } else if (streq(feature, "ping")) {
            accept = true;
            prog->state |= PF_PING;
         } else if (streq(feature, "usermove")) {
            accept = true;
            prog->state |= PF_USERMOVE;
         } else if (streq(feature, "done")) {
            accept = true;
            int res;
            sscanf(value, "%d", &res);
            if (res == 0)
               prog->state |= PF_INIT;
            else
               prog->state &= ~PF_INIT;
         }
         send_to_program(prog, "%s %s\n", accept?"accepted":"rejected", feature);
      }
   } else {    /* Thinking or ponder output? */
      if (prog->state & PF_FEN) {
         free(prog->fen);
         prog->fen = strdup(line);
      }
      if (prog->state & PF_MOVE) {
         char *s = line;
         while (*s && isspace(*s)) s++;
         free(prog->move);
         prog->move = strdup(s);
      }
      /* Thinking output */
      int depth, score, time;
      if (sscanf(line, "%d %d %d", &depth, &score, &time) == 3) {
         prog->depth = depth;
         prog->score = score;
         prog->time  = time;
      }
   }

done:
   if (next)
      parse_engine_input(prog, next);
}

void start_new_game(program_t *prog, const char *variant_name)
{
   uint32_t clearf = PF_THINK | PF_FORFEIT | PF_CLAIM | PF_RESIGN | PF_FLAG;

   prog->state &= ~clearf;
   prog->clock = start_time;
   prog->moves = 0;

   send_to_program(prog, "new\n");

   if (!streq(variant_name, "normal"))
      send_to_program(prog, "variant %s\n", variant_name);

   /* Randomise initial moves */
   send_to_program(prog, "random\n");

   /* Switch engines to "force" mode, switch off pondering. */
   send_to_program(prog, "force\neasy\npost\n");
   prog->state |= PF_FORCE;
}

int main(int argc, char **argv)
{
   FILE *pgnf = NULL;
   char *fcp = NULL;
   char *scp = NULL;
   char *ref = NULL;
   char *variant_name = "normal";
   char *logfilename = NULL;
   char *epdfile = NULL;
   char finit[65536];
   char sinit[65536];
   char *finit_str = finit;
   char *sinit_str = sinit;
   char host[255];
   bool use_sprt = false;
   double elo0 = 0.0;
   double elo1 = 4.0;
   double a    =  0.05;
   double b    =  0.05;
   int mg = 1;
   int epd_count = 0;
   int lpi = 0;
   int new_random_fen = 0;
   unsigned memory = 64;
   
   gethostname(host, sizeof host);
   char *s = strstr(host, ".");
   if (s) *s = '\0';

   finit_str[0] = '\0';
   sinit_str[0] = '\0';

   buf = malloc(BUF_SIZE);

   /* Parse command-line options. */
   if (argc>1) {
      int n;
      for (n=1; n<argc; n++) {
         if (strstr(argv[n], "-variant")) {
            if (n+1 >= argc) {
               fprintf(stderr, "error: -variant passed, but no variant specified");
               exit(EXIT_FAILURE);
            }
            n++;

            variant_name = argv[n];
         } else if (strstr(argv[n], "-mg")) {
            if (n+1 >= argc) {
               fprintf(stderr, "error: number of games/match not specified\n");
               exit(EXIT_FAILURE);
            }
            n++;
            sscanf(argv[n], "%d", &mg);
         } else if (strstr(argv[n], "-lpi")) {
            if (n+1 >= argc) {
               fprintf(stderr, "error: position index not specified\n");
               exit(EXIT_FAILURE);
            }
            n++;
            sscanf(argv[n], "%d", &lpi);
         } else if (strstr(argv[n], "-nrf")) {
            if (n+1 >= argc) {
               fprintf(stderr, "error: random position frequency not specified\n");
               exit(EXIT_FAILURE);
            }
            n++;
            sscanf(argv[n], "%d", &new_random_fen);
         } else if (strstr(argv[n], "-fcp")) {
            if (n+1 >= argc) {
               fprintf(stderr, "error: first program not specified\n");
               exit(EXIT_FAILURE);
            }
            n++;
            fcp = argv[n];
         } else if (strstr(argv[n], "-scp")) {
            if (n+1 >= argc) {
               fprintf(stderr, "error: second program not specified\n");
               exit(EXIT_FAILURE);
            }
            n++;
            scp = argv[n];
         } else if (strstr(argv[n], "-referee")) {
            if (n+1 >= argc) {
               fprintf(stderr, "error: referee program not specified\n");
               exit(EXIT_FAILURE);
            }
            n++;
            ref = argv[n];
         } else if (strstr(argv[n], "-finit")) {
            if (n+1 >= argc) {
               fprintf(stderr, "error: init line for first program not specified\n");
               exit(EXIT_FAILURE);
            }
            n++;
            snprintf(finit_str, sizeof finit - (finit_str - finit), "%s\n", argv[n]);
            finit_str = finit + strlen(finit);
         } else if (strstr(argv[n], "-sinit")) {
            if (n+1 >= argc) {
               fprintf(stderr, "error: init line for second program not specified\n");
               exit(EXIT_FAILURE);
            }
            n++;
            snprintf(sinit_str, sizeof sinit - (sinit_str - sinit), "%s\n", argv[n]);
            sinit_str = sinit + strlen(sinit);
         } else if (strstr(argv[n], "-tc")) {
            /* Either just a total time in [minutes:]seconds, or moves/time+increment */
            if (n+1 >= argc) {
               fprintf(stderr, "error: second program not specified\n");
               exit(EXIT_FAILURE);
            }
            n++;
            char *s = argv[n];
            char *p = s;
            if (strstr(s, "/")) {
               p = strstr(s, "/");
               sscanf(s, "%d", &moves_per_tc);
               p[0] = '\0';
               p++;
            }

            int minutes = 0, seconds=0, msec=0;
            if (strstr(p,":")) {
               sscanf(p, "%d:%d", &minutes, &seconds);
               seconds += 60*minutes;
            } else {
               sscanf(p, "%d", &seconds);
            }
            if (strstr(p, ".")) {
               char *pp = strstr(p, ".");
               double m;
               sscanf(pp, "%lg", &m);
               msec = 1000. * m;
            }
            start_time = seconds * 1000 + msec;

            if (strstr(p, "+")) {
               p = strstr(p, "+");
               p++;
               sscanf(p, "%lg", &time_inc);
               time_inc *= 1000;
            }
         } else if (strstr(argv[n], "-inc")) {
            if (n+1 >= argc) {
               fprintf(stderr, "error: no increment specified\n");
               exit(EXIT_FAILURE);
            }
            n++;
            sscanf(argv[n], "%lg", &time_inc);
            time_inc *= 1000;
         } else if (strstr(argv[n], "-mps")) {
            if (n+1 >= argc) {
               fprintf(stderr, "error: moves per session not specified\n");
               exit(EXIT_FAILURE);
            }
            n++;
            sscanf(argv[n], "%d", &moves_per_tc);
         } else if (strstr(argv[n], "-mtpm") ){
            if (n+1 >= argc) {
               fprintf(stderr, "error: minimum time per move not specified\n");
               exit(EXIT_FAILURE);
            }
            n++;
            sscanf(argv[n], "%d", &min_time_per_move);
         } else if (strstr(argv[n], "-sgf")) {
            if (n+1 >= argc) {
               fprintf(stderr, "error: no safe file specified\n");
               exit(EXIT_FAILURE);
            }
            n++;
            pgnf = fopen(argv[n], "a");
         } else if (strstr(argv[n], "-sprt")) {
            use_sprt = true;
         } else if (strstr(argv[n], "-sprt_better")) {
            /* Test if program 1 is better than program 2 */
            use_sprt = true;
            elo0 = 0;
            elo1 = 4;
         } else if (strstr(argv[n], "-sprt_worse")) {
            /* Test if program 1 is a regression against program 2 */
            use_sprt = true;
            elo0 = -4;
            elo1 = 0;
         } else if (strstr(argv[n], "-memory")) {
            if (n+1 < argc && argv[n+1][0] != '-') {
               sscanf(argv[n+1], "%d", &memory);
               if (memory < 1) memory = 1;
               n++;
            }
         } else if (strstr(argv[n], "-log")) {
            logfilename = "sjef.log";
            if (n+1 < argc && argv[n+1][0] != '-') {
               logfilename = argv[n+1];
               n++;
            }
         } else if (strstr(argv[n], "-epd")) {
            if (n+1 >= argc) {
               fprintf(stderr, "error: no EPD file specified\n");
               exit(EXIT_FAILURE);
            }
            n++;
            epdfile = argv[n];
         } else {
            fprintf(stderr, "Unknown option: %s\n", argv[n]);
            exit(EXIT_FAILURE);
         }
      }
   }

   if (ref == NULL)
      ref = strdup("sjaakii");

   if (moves_per_tc < 0) moves_per_tc = 0;
   if (time_inc < 0) time_inc = 0;

   if (!fcp || !scp) {
      fprintf(stderr, "error: need two programs to play\n");
      exit(EXIT_FAILURE);
   }

   if (!mg) {
      printf("No games to play\n");
      exit(0);
   }

   /* Start a game from a (random) position in the epd file.
    * Count the number of positions in the file.
    */
   if (epdfile) {
      FILE *f = fopen(epdfile, "r");
      if (f) {
         while(fgets(buf, BUF_SIZE, f))
            epd_count++;
         sgenrand(time(NULL));
         fclose(f);
      } else {
         fprintf(stderr, "error: cannot read EPD file %s\n", epdfile);
         exit(EXIT_FAILURE);
      }
   }

   if (logfilename)
      logfile = fopen(logfilename, "w");

   pipe2_t *first, *second, *third;
   char **args = calloc(64, sizeof *args);
   int max_args = 64;
   int num_args = 0;

   /* Parse argument list */
   num_args = 0;
   for (char *p = ref; p; ) {
      while (isspace(*p)) p++;
      if (!*p) break;
      if (num_args >= max_args) {
         max_args+=16;
         args = realloc(args, max_args*sizeof *args);
      }
      args[num_args++] = p;
      if(*p == '"' || *p == '\'')
         p = strchr(++args[num_args-1], *p);
      else
         p = strchr(p, ' ');
      if (p == NULL) break;
      *p++ = '\0';
   }
   args[num_args] = NULL;

   /* Start the referee program */
   printf("Starting referee %s\n", ref);
   fflush(stdout);
   third = p2open(args[0], args);
   if (!third) {
      printf("*** Failed to start referee %s\n", args[0]);
      exit(EXIT_FAILURE);
   }

   referee = calloc(1, sizeof *referee);
   referee->f = third;
   referee->name = strdup(args[0]);
   referee->state = PF_INIT;
   referee->id = 0;
   referee->win = 0;
   referee->draw = 0;

   send_to_program(referee, "xboard\nprotover 2\nforce\n");
   while(child_is_alive(referee) && (referee->state & PF_INIT)) {
      while(!p2_input_waiting(referee->f));
      while(p2_input_waiting(referee->f)) {
         if (!p2gets(buf, BUF_SIZE, referee->f))
            perror(NULL);

         parse_engine_input(referee, buf);
      }
   }
   printf("Referee is %s\n", referee->name);

   /* Test if the referee supports the requested variant */
   bool variant_ok = false;
   for (int n=0; n<referee->nv && !variant_ok; n++) 
      if (streq(referee->variants[n], variant_name))
         variant_ok = true;

   if (!variant_ok) {
      fprintf(stderr, "*** Referee does not understand variant '%s'\n", variant_name);
      exit(EXIT_FAILURE);
   }

   check_children = true;
   signal(SIGCHLD, child_signal_handler);

   /* Parse argument list and start first program */
   num_args = 0;
   for (char *p = fcp; p; ) {
      while (isspace(*p)) p++;
      if (!*p) break;
      if (num_args >= max_args) {
         max_args+=16;
         args = realloc(args, max_args*sizeof *args);
      }
      args[num_args++] = p;
      if(*p == '"' || *p == '\'')
         p = strchr(++args[num_args-1], *p);
      else
         p = strchr(p, ' ');
      if (p == NULL) break;
      *p++ = '\0';
   }
   args[num_args] = NULL;

   printf("Starting first program %s\n", fcp);
   first = p2open(fcp, args);
   if (!first) {
      printf("*** Failed to start first program %s\n", args[0]);
      exit(EXIT_FAILURE);
   }

   /* Parse argument list and start second program */
   num_args = 0;
   for (char *p = scp; p; ) {
      while (isspace(*p)) p++;
      if (!*p) break;
      if (num_args >= max_args) {
         max_args+=16;
         args = realloc(args, max_args*sizeof *args);
      }
      args[num_args++] = p;
      if(*p == '"' || *p == '\'')
         p = strchr(++args[num_args-1], *p);
      else
         p = strchr(p, ' ');
      if (p == NULL) break;
      *p++ = '\0';
   }
   args[num_args] = NULL;

   printf("Starting second program %s\n", scp);
   second = p2open(scp, args);
   if (!second) {
      printf("*** Failed to start second program %s\n", args[0]);
      exit(EXIT_FAILURE);
   }

   prog[0] = calloc(1, sizeof *prog[0]);
   prog[1] = calloc(1, sizeof *prog[1]);

   prog[0]->f = first;
   prog[0]->name = strdup(fcp);
   prog[0]->state = PF_INIT;
   prog[0]->id = 1;
   prog[0]->win = 0;
   prog[0]->draw = 0;

   prog[1]->f = second;
   prog[1]->name = strdup(scp);
   prog[1]->state = PF_INIT;
   prog[1]->id = 2;
   prog[1]->win = 0;
   prog[1]->draw = 0;

   prog[2] = referee;

   /* Send setup */
   send_to_program(prog[0], "xboard\nprotover 2\n");
   send_to_program(prog[1], "xboard\nprotover 2\n");

   /* Parse feature options */
   for (int i=0; i<2; i++) {
      printf("Intialising program %d...", i+1);
      fflush(stdout);
      while(child_is_alive(prog[i]) && (prog[i]->state & PF_INIT)) {
         while(!p2_input_waiting(prog[i]->f));
         while(p2_input_waiting(prog[i]->f)) {
            if (!p2gets(buf, BUF_SIZE, prog[i]->f))
               perror(NULL);

            parse_engine_input(prog[i], buf);
         }
      }
      printf("done\n");
   }

   /* Verify that both programs can play the variant in question */
   for (int k=0; k<2; k++) {
      variant_ok = false;
      for (int n=0; n<prog[k]->nv && !variant_ok; n++) 
         if (streq(prog[k]->variants[n], variant_name))
            variant_ok = true;

      if (!variant_ok) {
         fprintf(stderr, "*** Program %d (%s) does not understand variant '%s'\n", k+1, prog[k]->name, variant_name);
         exit(EXIT_FAILURE);
      }
   }

   /* Set memory size */
   for (int k=0; k<2; k++)
      if (prog[k]->state & PF_MEMORY)
         send_to_program(prog[k], "memory %d\n", memory);

   /* Send init strings (if needed) */
   if (logfile)
      fprintf(logfile, "init1 = '%s'\ninit2 = '%s'\n", finit, sinit);
   if (finit[0])
      send_to_program(prog[0], "%s", finit);
   if (sinit[0])
      send_to_program(prog[1], "%s", sinit);

   int n;
   char tc_string[256];
   snprintf(tc_string, sizeof tc_string, "%d/%"PRIu64":%02"PRIu64"+%g", moves_per_tc, (start_time/60000), (start_time%60000) / 1000, time_inc/1000);

   printf("Time control: %s\n", tc_string);
   if (logfile)
      fprintf(logfile, "Time control: %s\n", tc_string);
   if (moves_per_tc * time_inc) printf("Warning: both moves per session and increment specified\n");

   int epd_pos = 0;
   
   if (lpi > 0) epd_pos = lpi;
   
   if (new_random_fen) epd_pos = genrandf()*epd_count;

   for (int n=0; n<3; n++)
      synchronise(prog[n]);

   for (n=0; n<mg; n++) {
      /* Decide who plays white */
      int white = (n&1);
      int black = 1 - white;

      /* Determine new starting position? */
      if (epd_count) {
         if (lpi < 0) {
            if (n && (n % -lpi) == 0) epd_pos = (epd_pos + 1)%epd_count;
         } else {
            epd_pos = lpi;
            if (new_random_fen && (n%new_random_fen == 0)) epd_pos = genrandf()*epd_count;
         }
      }

      /* Start a new game of the appropriate variant */
      printf("Starting variant '%s' (%s - %s) game %d of %d\n", variant_name, prog[white]->name, prog[black]->name, n+1, mg);

      /* Inform the engines about the new variant game */
      for (int k=0; k<3; k++)
         start_new_game(prog[k], variant_name);
      clear_history();

      /* Setup time control */
      send_to_program(prog[0], "level %d %"PRIu64":%02"PRIu64" %d\n",
         moves_per_tc, (start_time)/(60000), (start_time)%(60000)/1000, (int)time_inc);
      send_to_program(prog[1], "level %d %"PRIu64":%02"PRIu64" %d\n",
         moves_per_tc, (start_time)/(60000), (start_time)%(60000)/1000, (int)time_inc);

      /* Send initial position */
      if (epdfile && epd_count) {
         FILE *f = fopen(epdfile, "r");
         int n;
         for (n=0; n<=epd_pos; n++)
            fgets(buf, BUF_SIZE, f);
         fclose(f);

         char *fen = strdup(buf);
         char *s = strstr(fen, "\n");
         if (s) *s = '\0';

         send_to_program(prog[0], "setboard %s\n", fen);
         send_to_program(prog[1], "setboard %s\n", fen);
         send_to_program(referee, "setboard %s\n", fen);

         if (logfile)
            fprintf(logfile, "Loaded position #%d from %s (FEN: %s)\n", epd_pos, epdfile, fen);

         free(fen);
      }

      /* Play out the game */
      bool game_is_decided = false;
      while (!game_is_decided) {
         char s[4096];

         if (logfile) fflush(logfile);
         wait_input(prog, 2, 100);

         /* Check whether the engines are both still alive */
         if (check_children) {
            bool done = false;
            for (int i=0; i<3; i++) {
               if(!child_is_alive(prog[i])) {
                  prog[i]->state |= PF_DEAD;
                  done = true;
                  printf("Child %d died\n", prog[i]->id);
               }
            }
            if (done) break;
            check_children = false;
         }

         /* Parse input from the engines engines */
         for (int i=0; i<3; i++) {
            while(p2_input_waiting(prog[i]->f)) {
               if (!p2gets(buf, BUF_SIZE, prog[i]->f))
                  perror(NULL);
               parse_engine_input(prog[i], buf);
            }
         }

         /* If either program has performed an illegal move or claim, it has forfeited the game */
         if ((prog[0]->state & PF_FORFEIT) || (prog[1]->state & PF_FORFEIT)) {
            game_is_decided = true;
            break;
         }

         /* If either program resigned, we're likewise done */
         if ((prog[0]->state & PF_RESIGN) || (prog[1]->state & PF_RESIGN)) {
            game_is_decided = true;
            break;
         }

         /* A program claimed the game was over, see whether we agree but abort the game anyway */
         if ((prog[0]->state & PF_CLAIM) || (prog[1]->state & PF_CLAIM)) {
            game_is_decided = true;
            if (logfile) {
               synchronise(referee);
               bool end = (referee->state & PF_CLAIM);
               int  desc = 3;
               if (referee->state & PF_CLAIMD) desc = 2;
               if (referee->state & PF_CLAIMW) desc = 0;
               if (referee->state & PF_CLAIMB) desc = 1;
               const char *side_string[4] = { "1-0", "0-1", "draw", "-" };
               fprintf(logfile, "Claim that game ended, referee %s (%s)\n", end? "agrees": "disagrees", side_string[desc]);
               fprintf(logfile, "Claim made in position %s\n", get_fen());
            }
            break;
         }

         /* If neither program is thinking... */
         if (!(prog[0]->state & PF_THINK) && !(prog[1]->state & PF_THINK)) {
            /* Check for end-of-game conditions */
            
            /* Check for out-of-time */
            if (min_time_per_move == 0 && prog[0]->clock*prog[1]->clock <=0) {
               game_is_decided = true;
               if (prog[0]->clock <= prog[1]->clock) prog[0]->state |= PF_FLAG;
               if (prog[1]->clock <= prog[0]->clock) prog[1]->state |= PF_FLAG;
            }

            side_t stm = moves_played & 1;
            int ptm = (stm + white) & 1;

            /* Update program's clock */
            int time = prog[ptm]->clock;
            if (time < min_time_per_move) time = min_time_per_move+9;
            send_to_program(prog[ptm^1], "otim %g\n", prog[ptm^1]->clock/10.);
            send_to_program(prog[ptm], "time %g\n", time/10.);

            /* Send the last move played in the game (as needed) */
            prog[ptm]->state |= PF_THINK;
            if (moves_played)
               move_to_program(prog[ptm], history[moves_played-1].move);

            /* Switch off force mode if needed */
            if (prog[ptm]->state & PF_FORCE) {
               prog[ptm]->state &= ~PF_FORCE;
               send_to_program(prog[ptm], "go\n");
            }

            /* Start the referee's clock */
            start_clock(&chess_clock);
         }
      }

      char *result_str = "*";
      if (game_is_decided) {
         if ((prog[white]->state & PF_RESIGN)) {
            result_str = "0-1 {White resigns}";
            prog[black]->win++;
         } else if ((prog[black]->state & PF_RESIGN)) {
            result_str = "1-0 {Black resigns}";
            prog[white]->win++;
         } else if ((prog[white]->state & PF_FORFEIT)) {
            result_str = "0-1 {White forfeits due to illegal move or illegal move claim}";
            prog[black]->win++;
         } else if ((prog[black]->state & PF_FORFEIT)) {
            result_str = "1-0 {Black forfeits due to illegal move or illegal move claim}";
            prog[white]->win++;
         } else if (!min_time_per_move && (prog[0]->state & PF_FLAG) && (prog[1]->state & PF_FLAG)) {
            result_str = "1/2-1/2 {Both flags fell}";
            prog[0]->draw++;
            prog[1]->draw++;
         } else if (!min_time_per_move && prog[white]->state & PF_FLAG) {
            result_str = "0-1 {White lost on time}";
            prog[black]->win++;
         } else if (!min_time_per_move && prog[black]->state & PF_FLAG) {
            result_str = "1-0 {Black lost on time}";
            prog[white]->win++;
         } else {
            synchronise(referee);
            if (referee->state & PF_CLAIMD) {
               prog[0]->draw++;
               prog[1]->draw++;
            }
            if (referee->state & PF_CLAIMW)
               prog[white]->win++;
            if (referee->state & PF_CLAIMB)
               prog[BLACK]->win++;

            if (referee->result_str)
               result_str = referee->result_str;
         }

         if (use_sprt) {
            sprt_result_t sprt_result = sprt(prog[0]->win,
                                             prog[1]->win,
                                             prog[0]->draw, elo0, elo1, a, b);
            if (logfile) print_sprt(logfile, prog[0]->win,
                                             prog[1]->win,
                                             prog[0]->draw, elo0, elo1, a, b);
            print_sprt(stdout, prog[0]->win, prog[1]->win, prog[0]->draw,
                        elo0, elo1, a, b);
            if (sprt_result != SPRT_UNKNOWN)
               break;
         }
      }

      if (logfile) fprintf(logfile, "Result: %s\n", result_str);
      printf("Game result: %s\n", result_str);

      /* Inform players that game has ended */
      send_to_program(prog[0], "result %s\n", result_str);
      send_to_program(prog[1], "result %s\n", result_str);

      /* Write the game to a .pgn file */
      if (pgnf) {
         start_new_game(referee, variant_name);
         char *short_result_str = strdup(result_str);
         char *s = strstr(short_result_str, " ");
         char *reason = NULL;
         if (s) {
            *s = '\0';
            reason = s+1;
         }
         const char *fen = get_fen();
         fprintf(pgnf,
                "[Event \"Computer Match\"]\n"
                "[Site \"%s\"]\n"
                "[Date \"\"]\n"
                "[Round \"%d\"]\n"
                "[White \"%s\"]\n"
                "[Black \"%s\"]\n"
                "[Referee \"%s\"]\n"
                "[Result \"%s\"]\n"
                "[TimeControl \"%s\"]\n"
                "[Variant \"%s\"]\n",
                host, n+1, prog[white]->name, prog[black]->name, referee->name, short_result_str, tc_string, variant_name);

         if (fen)
            fprintf(pgnf, "[FEN \"%s\"]\n", fen);

         fprintf(pgnf, "\n");
         int l = 0;

         for (int n=0; n<moves_played; n++) {
            char s[128];
            int k = 0;

            if ((n&1) == 0) {
               snprintf(s, sizeof s, "%d. ", n / 2 + 1);
               k = strlen(s);
            }

            const char *movestr = get_san_move(history[n].move);
            move_to_program(referee, history[n].move);


            snprintf(s+k, sizeof s - k, "%s ", movestr);
            if ( (l + strlen(s)) >= 80) {
               fprintf(pgnf, "\n");
               l = 0;
            }
            l += strlen(s);
            fprintf(pgnf, "%s", s);

            snprintf(s, sizeof s, "{%+.2f/%d %.2f} ", history[n].score/100., history[n].depth, history[n].time/1000.);
            if ( (l + strlen(s)) >= 80) {
               fprintf(pgnf, "\n");
               l = 0;
            }
            l += strlen(s);
            fprintf(pgnf, "%s", s);
         }
         if (reason)
            fprintf(pgnf, "\n%s %s\n\n", reason, short_result_str);
         else
            fprintf(pgnf, "\n%s\n\n", short_result_str);
         fflush(pgnf);
         free(short_result_str);
      }
   }

   printf("Match result: + %d - %d = %d (%.1f-%.1f)\n", prog[0]->win, prog[1]->win, prog[0]->draw,
      1.0*prog[0]->win+0.5*prog[0]->draw, 1.0*prog[1]->win+0.5*prog[1]->draw);

  int wins = prog[0]->win;
  int losses = prog[1]->win;
  int draws = prog[0]->draw;

  double games = wins + losses + draws; 
  double winning_fraction = (wins + 0.5*draws) / games; 
  double elo_difference = -log10(1.0/winning_fraction-1.0)*400.0; 
  double los = 0.5 + 0.5 * erf((wins-losses)/sqrt(2.0*(wins+losses))); 

  printf("Elo difference:   %+g\n", elo_difference); 
  printf("LOS:              % g\n", los); 

   if (pgnf)
      fclose(pgnf);

   /* Shutdown. Avoid waiting indefinitely by setting an alarm. A timeout of 10s should be plenty. */
   alarm(10);

   /* Flush buffers */
   for (int i=0; i<2; i++) {
      while(p2_input_waiting(prog[i]->f)) {
         p2gets(buf, BUF_SIZE, prog[i]->f);
      }
   }

   /* Shut down children */
   signal(SIGCHLD, SIG_IGN);
   send_to_program(referee, "quit\n");
   send_to_program(prog[0], "quit\n");
   send_to_program(prog[1], "quit\n");
   msleep(10);
   wait_input(prog, 2, 50000);
   if (check_children) {
      check_children = false;
      for (int i=0; i<2; i++) {
         if(!(prog[i]->state & PF_DEAD) && child_is_alive(prog[i])) {
            kill(prog[i]->f->pid, SIGTERM);
            if (logfile) fprintf(logfile, "Send terminate signal to %s (%d)\n", prog[i]->name, prog[i]->id);
         } else {
            prog[i]->state |= PF_DEAD;
            if (logfile) fprintf(logfile, "%s (%d) exited\n", prog[i]->name, prog[i]->id);
         }
      }
   }
   for (int i=0; i<2; i++) {
      if(!(prog[i]->state & PF_DEAD)) {
         msleep(10);
         if (child_is_alive(prog[i])) {
            if (logfile) fprintf(logfile, "Send kill signal to %s (%d)\n", prog[i]->name, prog[i]->id);
            kill(prog[i]->f->pid, SIGKILL);
            while (child_is_alive(prog[i]));
         }
      }
   }

   if (logfile)
      fclose(logfile);
   return 0;
}
