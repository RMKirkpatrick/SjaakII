#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include "compilerdef.h"
#include "squares.h"

char *square_names[128] = { NULL };
char *file_names[16] = { NULL };
char *rank_names[16] = { NULL };
uint8_t packed_file_rank[128];
int rank_offset = 1;

const char *kingside_castle;
const char *queenside_castle;

static bool virgin = true;
int div_file = 0;
bool keep_labels = false;

static int board_files = 0;
static int board_ranks = 0;

void initialise_square_names(int files, int ranks)
{
   board_files = files;
   board_ranks = ranks;

   /* Initialise square names and file/rank masks */
   for (int n=0; n<128; n++) {
      if (!virgin) free(square_names[n]);
      square_names[n] = NULL;
   }
   for (int n=0; n<16; n++) {
      if (!virgin) {
         free(rank_names[n]);
         free(file_names[n]);
      }
      rank_names[n] = NULL;
      file_names[n] = NULL;
   }
   virgin = false;

   for (int f=0; f<files; f++) {
      for (int r=0; r<ranks; r++) {
         int n = f + r*files;
         square_names[n] = (char *)malloc(10);
         snprintf(square_names[n], 10, "%c%d", f+'a', r+rank_offset);
      }
   }

   for (int f=0; f<16; f++) {
      file_names[f] = (char *)malloc(10);
      snprintf(file_names[f], 10, "%c", f+'a');
   }
   for (int r=0; r<16; r++) {
      rank_names[r] = (char *)malloc(10);
      snprintf(rank_names[r], 10, "%d", r+rank_offset);
   }

   for (int f=0; f<files; f++) {
      for (int r=0; r<ranks; r++) {
         int n = f + r*files;
         packed_file_rank[n] = f | (r << 4);
      }
   }

   div_file = files / 2;

   /* Castling notation */
   kingside_castle  = "O-O";
   queenside_castle = "O-O-O";
}

void relabel_shogi_square_names(void)
{
   for (int f=0; f<board_files; f++) {
      for (int r=0; r<board_ranks; r++) {
         int n = f + r*board_files;
         snprintf(square_names[n], 10, "%d%c", board_files - f, 'a' + (board_ranks-1 - r));
      }
   }

   for (int f=0; f<16; f++)
      snprintf(file_names[f], 10, "%d", board_files - f);
   for (int r=0; r<16; r++)
      snprintf(rank_names[r], 10, "%c", 'a' + (board_ranks-1 - r));
}

void relabel_chess_square_names(void)
{
   if (!keep_labels)
   for (int f=0; f<board_files; f++) {
      for (int r=0; r<board_ranks; r++) {
         int n = f + r*board_files;
         snprintf(square_names[n], 10, "%c%d", f+'a', r+rank_offset);
      }
   }

   for (int f=0; f<16; f++)
      snprintf(file_names[f], 10, "%c", f+'a');
   for (int r=0; r<16; r++)
      snprintf(rank_names[r], 10, "%d", r+rank_offset);
}

int square_from_string(const char *str)
{
   char s[128];

   while (str[0] == ' ') str++;
   int len = 0;

   if (isalpha(*str)) {
      while (*str && isalpha(*str)) {
         s[len++] = *str;
         str++;
      }
   } else {
      while (*str && isdigit(*str)) {
         s[len++] = *str;
         str++;
      }
   }

   if (isalpha(*str)) {
      while (*str && isalpha(*str)) {
         s[len++] = *str;
         str++;
      }
   } else {
      while (*str && isdigit(*str)) {
         s[len++] = *str;
         str++;
      }
   }
   s[len++] = 0;

   for (int square = 0; square < board_files*board_ranks; square++) {
      if (!square_names[square]) continue;
      if (strcmp(s, square_names[square]) == 0) return square;
   }

   return -1;
}
