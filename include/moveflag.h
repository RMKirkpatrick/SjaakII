#ifndef MOVEFLAG_H
#define MOVEFLAG_H

#include <stdint.h>
#include "assert.h"
#include "bool.h"
typedef uint32_t move_flag_t;

#define MF_SLIDER_H     0x00000001
#define MF_SLIDER_V     0x00000002
#define MF_SLIDER_D     0x00000004
#define MF_SLIDER_A     0x00000008
#define MF_SLIDER       0x0000000F

#define MF_HOPPER_H     0x00000010
#define MF_HOPPER_V     0x00000020
#define MF_HOPPER_D     0x00000040
#define MF_HOPPER_A     0x00000080
#define MF_HOPPER       0x000000F0

#define MF_HOPSLIDE     (MF_HOPPER | MF_SLIDER)

#define MF_STEPPER      0x00000F00  /* This is an index in an array with repeat counts */

#define MF_RIDER        0x0000F000  /* This is an index in an array with repeat counts */

#define MF_LEAPER       0x000F0000  /* This is an index in an array */
#define MF_LEAPER2      0x00F00000  /* Second index, for compound leapers */
#define MF_LEAPER_MASK  0x0F000000  /* Mask index, for compound leapers */
#define MF_IS_LEAPER    0x10000000  /* Whether this is actually a leaper */
#define MF_LEAPER_HAVE2 0x20000000  /* Leaper is a compound leaper */
#define MF_LEAPER_HAVEM 0x40000000  /* Leaper has a mask (only makes sense for compound leapers) */
#define MF_LEAPER_ASYMM 0x80000000  /* Leaper is asymmetric */
#define MF_LEAPER_FLAGS 0xFFFF0000  /* All leaper flags */

#define MF_HOPSLIDELEAP (MF_HOPSLIDE | MF_IS_LEAPER | MF_RIDER)

/* Stepper repeat count masks (3 bits per direction) */
#define MF_STEPPER_N    (0xf << 0)
#define MF_STEPPER_NE   (0xf << 4)
#define MF_STEPPER_E    (0xf << 8)
#define MF_STEPPER_SE   (0xf << 12)
#define MF_STEPPER_S    (0xf << 16)
#define MF_STEPPER_SW   (0xf << 20)
#define MF_STEPPER_W    (0xf << 24)
#define MF_STEPPER_NW   (0xf << 28)

/* TODO: repeated leapers = riders, needs extension of the move_flag type.
 * Intended input file syntax:
 * Ride: (x,y)
 * (similar to asymmetric leaper)
 */

static inline bool is_slider(move_flag_t mf)
{
   return (mf & MF_SLIDER) != 0;
}

static inline bool is_hopper(move_flag_t mf)
{
   return (mf & MF_HOPPER) != 0;
}

static inline bool is_rider(move_flag_t mf)
{
   return (mf & MF_RIDER) != 0;
}

static inline bool is_leaper(move_flag_t mf)
{
   return (mf & MF_IS_LEAPER) != 0;
}

static inline bool is_aleaper(move_flag_t mf)
{
   const move_flag_t flags = MF_LEAPER_ASYMM;// | MF_IS_LEAPER;
   return (mf & flags) == flags;
}

static inline bool is_simple_leaper(move_flag_t mf)
{
   const move_flag_t flags = MF_LEAPER_HAVE2 | MF_LEAPER_HAVEM;
   return (mf & MF_IS_LEAPER) && !(mf & flags);
}

static inline bool is_double_leaper(move_flag_t mf)
{
   const move_flag_t flags = MF_LEAPER_HAVE2;// | MF_IS_LEAPER;
   return (mf & flags) == flags;
}

static inline bool is_masked_leaper(move_flag_t mf)
{
   const move_flag_t flags = MF_LEAPER_HAVEM;// | MF_IS_LEAPER;
   return (mf & flags) == flags;
}

static inline int get_leaper_index(move_flag_t mf)
{
   return (mf & MF_LEAPER) >> 16;
}

static inline int get_leaper_index2(move_flag_t mf)
{
   return (mf & MF_LEAPER2) >> 20;
}

static inline int get_leaper_indexm(move_flag_t mf)
{
   return (mf & MF_LEAPER_MASK) >> 24;
}

static inline bool is_stepper(move_flag_t mf)
{
   return (mf & MF_STEPPER) != 0;
}

static inline int get_stepper_index(move_flag_t mf)
{
   return (mf & MF_STEPPER) >> 8;
}

static inline int make_stepper_index(int id)
{
   assert(id < 16);
   return (id << 8);
}

static inline int get_rider_index(move_flag_t mf)
{
   return (mf & MF_RIDER) >> 12;
}

static inline int make_rider_index(int id)
{
   assert(id < 16);
   return (id << 12);
}


#endif
