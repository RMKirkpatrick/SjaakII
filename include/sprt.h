#ifndef SPRT_H
#define SPRT_H

typedef enum { SPRT_UNKNOWN = 0, SPRT_ACCEPT, SPRT_REJECT } sprt_result_t;

sprt_result_t sprt(int n_wins, int n_loss, int n_draw, double elo0, double elo1, double a, double b);
void print_sprt(FILE *file, int n_wins, int n_loss, int n_draw, double elo0, double elo1, double a, double b);

#endif
