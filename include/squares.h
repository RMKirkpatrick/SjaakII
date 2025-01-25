#ifndef SQUARE_H
#define SQUARE_H

#include <stdint.h>

extern char *square_names[128];
extern char *file_names[16];
extern char *rank_names[16];
extern int rank_offset;
extern uint8_t packed_file_rank[128];

extern const char *kingside_castle;
extern const char *queenside_castle;
extern int div_file;
extern bool keep_labels;

void initialise_square_names(int files, int ranks);
void relabel_shogi_square_names(void);
void relabel_chess_square_names(void);

int square_from_string(const char *str);

/********************************************************************
 * Functions for dealing with packing/unpacking square rows/files *
 ********************************************************************/

static inline int unpack_rank(int packed)
{
   return packed_file_rank[packed] >> 4;
}

static inline int unpack_file(int packed)
{
   return packed_file_rank[packed] & 0xf;
}

#endif
