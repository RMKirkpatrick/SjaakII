#include <stdio.h>
#include "bitboard.h"
#include "board.h"
#include "movegen.h"
#include "aligned_malloc.h"
#include "game.h"

int main(void)
{
   game_template_t<uint64_t> *game = new game_template_t<uint64_t>;
   bitboard_t<uint128_t> large(u128(1,0));
   bitboard_t<uint64_t> bb((uint64_t)0x06ll);
   bitboard_t<uint64_t> b(3);
   bitboard_t<uint64_t> b2;
   bitboard_t<uint32_t> b3;
   bitboard_t<uint32_t> tiny(0);

   bitboard_t<uint16_t>::initialise_bitboards(4,4);
   bitboard_t<uint32_t>::initialise_bitboards(4,4);
   bitboard_t<uint128_t>::initialise_bitboards(11,11);

   movegen_t<uint16_t>::initialise();
   movegen_t<uint32_t>::initialise();
   movegen_t<uint128_t>::initialise();

   movegen_t<uint16_t>::initialise_slider_tables();
   movegen_t<uint32_t>::initialise_slider_tables();
   movegen_t<uint128_t>::initialise_slider_tables();

   movegen_t<uint16_t>::initialise_super_tables();
   movegen_t<uint32_t>::initialise_super_tables();
   movegen_t<uint128_t>::initialise_super_tables();

   game->set_board_size(8,8);

   //large = large << 16;
   move_flag_t fb = movegen_t<uint64_t>::define_piece_move("slide (A,D)");
   move_flag_t fr = movegen_t<uint64_t>::define_piece_move("slide (H,V)");
   move_flag_t fq = fb | fr;
   move_flag_t fn = movegen_t<uint64_t>::define_piece_move("leap (1,2)");
   move_flag_t fk = movegen_t<uint64_t>::define_piece_move("leap (1,0)|(1,1)");

   bitboard_t<uint64_t> pz[2];
   game->add_piece_type(fn, fn, 0, pz, "", "Knight", "N,n", "N");
   game->add_piece_type(fb, fb, 0, pz, "", "Bishop", "B,b", "B");
   game->add_piece_type(fr, fr, 0, pz, "", "Rook", "R,r", "R");
   game->add_piece_type(fq, fq, 0, pz, "", "Queen", "Q,q", "Q");
   game->add_piece_type(fk, fk, 0, pz, "", "King", "K,k", "K");

   movegen_t<uint64_t>::initialise_super_tables();
   movegen_t<uint64_t>::deduce_castle_flags(WHITE, 4, 6, 7);
   movegen_t<uint64_t>::deduce_castle_flags(WHITE, 4, 2, 0);
   movegen_t<uint64_t>::deduce_castle_flags(BLACK, 60, 62, 63);
   movegen_t<uint64_t>::deduce_castle_flags(BLACK, 60, 58, 56);

   b2 = b << 1;

   printf("%d %d %d\n", large.onebit(), large.popcount(), large.bitscan());
   printf("%d %d %d\n", bb.onebit(), bb.popcount(), bb.bitscan());
   printf("%d %d %d %d\n", b.onebit(), b.popcount(), b.bitscan(), b.test(4));
   printf("%d %d %d %d\n", b2.onebit(), b2.popcount(), b2.bitscan(), b2.test(4));
   b2 = b2 >> 1;
   printf("%d\n", b2 == b);

   printf("%d %d %d\n", b3.onebit(), b3.popcount(), b3.bitscan());

   printf("0x%016llx\n", bitboard_t<uint64_t>::board_light.value());

   for (int n = 0; n<8; n++)
      printf("%08x\n", large.get_rank(n));
   printf("\n");
   for (int n = 0; n<8; n++)
      printf("%08x\n", large.get_file(n));

#if 0
   uint32_t occ = (1<<1)|(1<<2);
   int where = 0;
   large = u128(occ, 0);

   tiny ^= tiny;
   printf("\n");
   for (int n = 0; n<tiny.board_ranks; n++)
      printf("%s\n", tiny.rank_string(tiny.board_ranks-n-1));

   movegen_t<uint32_t> movegen;
   printf("\n");
   for (int n = 0; n<tiny.board_ranks; n++)
      printf("%s\n", movegen.horizontal_slider_move[where][occ].rank_string(tiny.board_ranks-n-1));

   printf("\n");
   for (int n = 0; n<tiny.board_ranks; n++)
      printf("%s\n", movegen.horizontal_hopper_move[where][occ].rank_string(tiny.board_ranks-n-1));

   printf("\n");
   for (int n = 0; n<tiny.board_ranks; n++)
      printf("%s\n", movegen.vertical_hopper_move[where][occ].rank_string(tiny.board_ranks-n-1));

   printf("\n");
   for (int n = 0; n<tiny.board_ranks; n++)
      printf("%s\n", movegen.vertical_slider_move[where][occ].rank_string(tiny.board_ranks-n-1));

   int from = 2*tiny.board_ranks+1;
   int to = from + 1*(tiny.board_ranks+1);
   printf(":\n");
   for (int n = 0; n<tiny.board_ranks; n++)
      printf("%s\n", tiny.connecting_ray[from][to].rank_string(tiny.board_ranks-n-1));
#endif

   bitboard_t<uint64_t> occb;
   occb.set(0);
   occb.set(3);
   occb.set(2*8);
   occb.set(14);
   occb.set(36);
   occb.set(36 + 2*9);
   occb.set(36 - 7);
   occb.set(36 + 2*7);
   bitboard_t<uint64_t> moves = movegen_t<uint64_t>::generate_slider_move_bitboard(MF_SLIDER_H|MF_SLIDER_V, WHITE, 0, occb);
   bitboard_t<uint64_t> dmoves = movegen_t<uint64_t>::generate_hopper_move_bitboard(MF_HOPPER_D|MF_HOPPER_A, WHITE, 36, occb);

   printf("\nOccupied:\n");
   occb.print();

   printf("\n");
   moves.print();

   printf("\n");
   dmoves.print();

   printf("%08x\n", fr);
   printf("%08x\n", fk);
   printf("%d\n", get_leaper_index(fk));

   movegen_t<uint64_t>::super[36].print();


   bitboard_t<uint64_t> knightm = movegen_t<uint64_t>::generate_leaper_move_bitboard(fn, WHITE, 36, occb);

   printf("\n");
   movegen_t<uint64_t>::leaper[get_leaper_index(fk)][0].print();
   printf("\n");
   movegen_t<uint64_t>::leaper[get_leaper_index(fk)][36].print();
   printf("\n");
   knightm.print();

   //printf("\n");
   //occb.fill_north().print();
   //printf("\n");
   //occb.fill_south().print();


#if 0
   for (to = 0; to < tiny.board_ranks*tiny.board_files; to++) {
      printf("\n");
      for (int n = 0; n<tiny.board_ranks; n++)
         printf("%s\n", tiny.square_bitboards[to].rank_string(tiny.board_ranks-n-1));
      printf("%d\n", tiny.diagonal_nr[to]);
   }
#endif

   return 0;
}
