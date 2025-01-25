#ifndef XSTRING_H
#define XSTRING_H

#include <string.h>
#include <ctype.h>

static inline char *trim(char *s)
{
   char *p = s;
   while (*p && isspace(*p)) p++;
   if (p!= s) {
      for (size_t n = 0; n<=strlen(p); n++)
         s[n] = p[n];
   }
   return s;
}

static inline char *chomp(char *s)
{
   char *p;
   p = strstr(s, "\n");
   if (p) *p = '\0';
   return s;
}

static inline bool streq(const char *s1, const char *s2)
{
   return strcmp(s1, s2) == 0;
}

#endif
