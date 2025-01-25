#ifndef SCORE_H
#define SCORE_H

#define LEGALWIN  16000
#define LEGALLOSS (-LEGALWIN)
#define LEGALDRAW     0
#define ILLEGAL   (LEGALWIN+500)

static inline bool is_mate_score(int score)
{
   static const int mate = LEGALWIN - 1000;
   if ( score > mate || score < -mate )
      return true;

   return false;
}

static inline int score_to_hashtable(int score, int depth)
{
   if (is_mate_score(score)) {
      assert( (score>0 && score + depth <= LEGALWIN) ||
              (score<0 && score - depth >= -LEGALWIN) );
      if (score > 0)
         return score + depth;
      else
         return score - depth;
   }

   return score;
}

static inline int score_from_hashtable(int score, int depth)
{
   if (is_mate_score(score)) {
      assert( (score>0 && (score - depth <=  LEGALWIN)) ||
              (score<0 && (score + depth >= -LEGALWIN)) );
      if (score > 0)
         return score - depth;
      else
         return score + depth;
   }

   return score;
}


#endif
