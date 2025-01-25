#define __STDC_FORMAT_MACROS 1
#include <inttypes.h>
#include <stdio.h>
#include <sys/time.h>
#include <stdint.h>
#include <limits.h>
#include <string.h>
#include "bitboard.h"
#include "board.h"
#include "movegen.h"
#include "aligned_malloc.h"
#include "hashkey.h"
#include "game.h"
#include "variants.h"

uint64_t perft(game_t *game, int depth, int root)
{
   movelist_t movelist;
   side_t me = game->get_side_to_move();
   uint64_t nodes = 0;
   int n;

   if (depth == 0) return 1;

   /* Check if previous move left the player in check */
   game->generate_moves(&movelist);

   for (n=0; n<movelist.num_moves; n++) {
      uint64_t count = 0;
      game->playmove(movelist.move[n]);
      if (!game->player_in_check(me))  /* Don't count illegal moves */
         count = perft(game, depth-1, root - 1);
      nodes += count;
      if (root > 0)
         printf("%8s %10"PRIu64" %10"PRIu64"\n", move_to_string(movelist.move[n], NULL), count, nodes);
      game->takeback();
   }
   return nodes;
}

#define streq(s1, s2) (strcmp((s1), (s2)) == 0)
game_t *create_variant_game(const char *variant_name)
{

   if (streq(variant_name, "normal")) {
      return create_standard_game<uint64_t>();
   } else if (streq(variant_name, "losalamos")) {
      return create_losalamos_game<uint64_t>();
   } else if (streq(variant_name, "micro")) {
      return create_micro_game<uint64_t>();
   } else if (streq(variant_name, "capablanca")) {
      return create_capablanca_game<uint128_t>();
   } else if (streq(variant_name, "create_pocketknight_game")) {
      return create_pocketknight_game<uint64_t>();
   }

   return NULL;
}

int main(void)
{
   movelist_t movelist;

   initialise_hash_keys();

   game_t *game = create_variant_game("normal");

   if (!game) {
      printf("Could not create game object!\n");
      exit(0);
   }

   game->start_new_game();
   //game->setup_fen_position("rnbqkbnr/1pp1pppp/p7/3P4/8/8/PP1PPPPP/RNBQKBNR b KQkq -");
   //game->setup_fen_position("rnbqkbnr/ppp1pppp/8/3P4/8/8/PP1PPPPP/RNBQKBNR b KQkq -");
   uint64_t t = get_timer();
   game->print_bitboards();
   game->print_board();
   game->generate_moves(&movelist);
   movelist.print();

   //printf("%d\n", game->eval());

   for (int n = 1; n<4+1; n++) {
      uint64_t nodes = perft(game, n, 0);
      uint64_t tt = get_timer();

      if (tt == t) tt++;

      printf("%2d %10lld %5.2f %12.2fnps\n", n, (long long int)nodes,
            (double)(tt - t)/1000000.0,nodes*1.0e6/(tt-t));

      t = tt;
   }
   exit(0);

   move_t move = movelist.move[0];
   printf("%s\n", move_to_string(move));

   game->playmove(move);
   game->print_bitboards();
   game->takeback();
   game->print_bitboards();


   return 0;
}
