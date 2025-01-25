#ifndef PST_H
#define PST_H

extern int psq_map[NUM_SIDES][128];

/* Base centralisation score for each square */
extern int centre_table[128];

/* Base advancement score for each square */
extern int advance_table[128];

/* Extra advancement score for proximity to promotion zone, for pawns */
extern int promo_table[128];

/* End game bonus for driving the defending king to the edge */
extern int lone_king_table[128];

/* Base shield score, for pawn shields */
extern int shield_table[128];

extern void initialise_base_evaluation_tables(int files, int ranks);

#endif
