void add_rule(uint32_t rule) {
   board.rule_flags |= rule;
}

void remove_rule(uint32_t rule) {
   board.rule_flags &= ~rule;
}

uint32_t get_rules(void) {
    return board.rule_flags;
}

void set_board_size(int files, int ranks) {
   assert(files*ranks <= 8*sizeof(kind));
   initialise_square_names(files, ranks);

   bitboard_t<kind>::initialise_bitboards(files,ranks);
   movegen.initialise();
   movegen.initialise_slider_tables();

   this->ranks = ranks; 
   this->files = files; 

   top_left = files*(ranks-1);

   initialise_base_evaluation_tables(files, ranks);
}

void remove_square(int square) {
   bitboard_t<kind> bb = ~bitboard_t<kind>::square_bitboards[square];
   int n;

   bitboard_t<kind>::board_all &= bb;
   bitboard_t<kind>::board_edge &= bb;
   bitboard_t<kind>::board_east_edge &= bb;
   bitboard_t<kind>::board_west_edge &= bb;
   bitboard_t<kind>::board_north_edge &= bb;
   bitboard_t<kind>::board_south_edge &= bb;
   bitboard_t<kind>::board_south &= bb;
   bitboard_t<kind>::board_north &= bb;
   bitboard_t<kind>::board_corner &= bb;
   bitboard_t<kind>::board_light &= bb;
   bitboard_t<kind>::board_dark &= bb;
   bitboard_t<kind>::board_centre &= bb;
   bitboard_t<kind>::board_xcentre &= bb;
   bitboard_t<kind>::board_xxcentre &= bb;
   bitboard_t<kind>::board_homeland[0] &= bb;
   bitboard_t<kind>::board_homeland[1] &= bb;

   for (n=0; n<16; n++) {
      bitboard_t<kind>::board_rank[n] &= bb;
      bitboard_t<kind>::board_file[n] &= bb;
      bitboard_t<kind>::board_northward[n] &= bb;
      bitboard_t<kind>::board_southward[n] &= bb;
      bitboard_t<kind>::board_eastward[n] &= bb;
      bitboard_t<kind>::board_westward[n] &= bb;
   }
   for (n=0; n<32; n++) {
      bitboard_t<kind>::board_diagonal[n] &= bb;
      bitboard_t<kind>::board_antidiagonal[n] &= bb;
   }

   int board_size = 8*sizeof(kind);
   for (int n=0; n<board_size; n++) {
      bitboard_t<kind>::neighbour_board[n] &= bb;
      for (int k=0; k<board_size; k++)
         bitboard_t<kind>::board_between[n][k] &= bb;
   }

}

void place_flag(side_t side, int square) {
   board.flag[side].set(square);
}

void remove_flag(side_t side, int square) {
   board.flag[side].reset(square);
}


