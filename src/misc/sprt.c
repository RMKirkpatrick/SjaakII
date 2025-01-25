#include <math.h>
#include "assert.h"
#include "sprt.h"

double elo_to_winprob(double elo, double draw_elo)
{
   return 1. / (1. + pow(10., (-elo + draw_elo) / 400.));
}

double elo_to_lossprob(double elo, double draw_elo)
{
   return elo_to_winprob(-elo, draw_elo);
}

double probability_to_elo(double p_win, double p_loss)
{
   assert(p_win  >= 0. && p_win  < 1.);
   assert(p_loss >= 0. && p_loss < 1.);

   return  200. * log10(p_win/p_loss * (1.-p_loss)/(1.-p_win));
}

double probability_to_drawelo(double p_win, double p_loss)
{
   assert(p_win  >= 0. && p_win  < 1.);
   assert(p_loss >= 0. && p_loss < 1.);

   return  200. * log10((1. - p_loss)/p_loss * (1.-p_win)/(1.-p_win));
}

sprt_result_t sprt(int n_wins, int n_loss, int n_draw, double elo0, double elo1, double a, double b)
{
   double elo, draw_elo;
   int n = n_wins + n_loss + n_draw;

   double p_win  = (double)n_wins / n;
   double p_loss = (double)n_loss / n;
   double p_draw = (double)n_draw / n;
   if (n_wins > 0 && n_loss > 0 && n_draw > 0) {
      elo      = probability_to_elo(p_win, p_loss);
      draw_elo = probability_to_drawelo(p_win, p_loss);
   } else {
      return SPRT_UNKNOWN;
   }

   double p_win0  = elo_to_winprob (elo0, draw_elo);
   double p_loss0 = elo_to_lossprob(elo0, draw_elo);
   double p_draw0 = 1. - p_win0 - p_loss0;
   double p_win1  = elo_to_winprob (elo1, draw_elo);
   double p_loss1 = elo_to_lossprob(elo1, draw_elo);
   double p_draw1 = 1. - p_win1 - p_loss1;

   double lower = log(b / (1. - a));
   double upper = log((1. - b) / a);
   double llr   = n_wins * log(p_win1  / p_win0 ) +
                  n_loss * log(p_loss1 / p_loss0) +
                  n_draw * log(p_draw1 / p_draw0);

   if (llr < lower) return SPRT_REJECT;
   if (llr > upper) return SPRT_ACCEPT;

   return SPRT_UNKNOWN;
}


void print_sprt(FILE *file, int n_wins, int n_loss, int n_draw, double elo0, double elo1, double a, double b)
{
   double elo, draw_elo;
   int n = n_wins + n_loss + n_draw;

   double p_win  = (double)n_wins / n;
   double p_loss = (double)n_loss / n;
   double p_draw = (double)n_draw / n;
   if (n_wins > 0 && n_loss > 0 && n_draw > 0) {
      elo      = probability_to_elo(p_win, p_loss);
      draw_elo = probability_to_drawelo(p_win, p_loss);
   } else {
      return;
   }

   double p_win0  = elo_to_winprob (elo0, draw_elo);
   double p_loss0 = elo_to_lossprob(elo0, draw_elo);
   double p_draw0 = 1. - p_win0 - p_loss0;
   double p_win1  = elo_to_winprob (elo1, draw_elo);
   double p_loss1 = elo_to_lossprob(elo1, draw_elo);
   double p_draw1 = 1. - p_win1 - p_loss1;

   double lower = log(b / (1. - a));
   double upper = log((1. - b) / a);
   double llr   = n_wins * log(p_win1  / p_win0 ) +
                  n_loss * log(p_loss1 / p_loss0) +
                  n_draw * log(p_draw1 / p_draw0);

   fprintf(file, "SPRT: %g [%g %g], a=%g b=%g elo0=%g elo1=%g, drawelo=%g\n",
          llr, lower, upper, a, b, elo0, elo1, draw_elo);
   if (llr < lower) fprintf(file, "SPRT: rejected\n");
   if (llr > upper) fprintf(file, "SPRT: accepted\n");
}

