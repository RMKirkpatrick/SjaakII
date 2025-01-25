#include <stdarg.h>
#include "compilerdef.h"
#include "game.h"

static void printfstderr(const char *msg, ...)
{
   static char buf[65536];

   va_list ap;
   va_start(ap, msg);
   vsnprintf(buf, sizeof buf, msg, ap);
   va_end(ap);

   fprintf(stderr, "%s", buf);
}


void (*default_iteration_output)(const char *, ...) = NULL;
void (*default_uci_output)(const char *, ...) = NULL;
void (*default_xboard_output)(const char *, ...) = NULL;
void (*default_error_output)(const char *, ...) = printfstderr;

size_t default_hash_size = HASH_TABLE_SIZE;

