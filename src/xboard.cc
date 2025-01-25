/*  Sjaak, a program for playing chess
 *  Copyright (C) 2011, 2014  Evert Glebbeek
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#define __STDC_FORMAT_MACROS 1
#include "compilerdef.h"
#include <cstdio>
#include <cstdlib>
#include <cstdarg>
#include <ctype.h>
#include <ctime>
#if defined _MSC_VER
#   include <vector>
#endif

#include "sjaak.h"
#include "xstring.h"
#include "keypressed.h"
#include "cfgpath.h"
#include "test_suite.h"

#ifdef __APPLE__
#define __unix__
#endif
#ifdef __unix__
#include <unistd.h>
#include <signal.h>
#endif

#ifdef HAVE_READLINE
#include <readline/readline.h>
#include <readline/history.h>
#endif

#if defined __LP64__ || defined _WIN64
#define ARCHSTR "(x86_64)"
#elif defined __i386__ || defined _WIN32
#define ARCHSTR "(i386)"
#elif defined POWERPC
#define ARCHSTR "(powerpc)"
#else
#define ARCHSTR "(unknown)"
#endif 

#ifndef SJAAKIIVERSION
#define SJAAKIIVERSION "(version unknown) "
#endif
#define VERSIONSTR SJAAKIIVERSION

#ifdef DEBUGMODE
#undef VERSIONSTR
#define VERSIONSTR SJAAKIIVERSION " (debug)"
#endif

#define PROGNAME "Sjaak II"

#define TIME_BUFFER  100

typedef struct help_topic_t {
   const char *topic;
   const char *cmd;
   const char *text;
} help_topic_t;

typedef struct position_signature_t {
   const char *fen;
   int depth;
   uint64_t nodes;
} position_signature_t;

static position_signature_t perftests[] = {
   // Martin Sedlak's test positions
   // (http://www.talkchess.com/forum/viewtopic.php?t=47318)
   // avoid illegal ep
   { "3k4/3p4/8/K1P4r/8/8/8/8 b - - 0 1",         6, 1134888 },
   { "8/8/8/8/k1p4R/8/3P4/3K4 w - - 0 1",         6, 1134888 },
   { "8/8/4k3/8/2p5/8/B2P2K1/8 w - - 0 1",         6, 1015133 },
   { "8/b2p2k1/8/2P5/8/4K3/8/8 b - - 0 1",         6, 1015133 },
   // en passant capture checks opponent: 
   { "8/8/1k6/2b5/2pP4/8/5K2/8 b - d3 0 1",         6, 1440467 },
   { "8/5k2/8/2Pp4/2B5/1K6/8/8 w - d6 0 1",         6, 1440467 },
   // short castling gives check: 
   { "5k2/8/8/8/8/8/8/4K2R w K - 0 1",            6, 661072 },
   { "4k2r/8/8/8/8/8/8/5K2 b k - 0 1",            6, 661072 },
   // long castling gives check: 
   { "3k4/8/8/8/8/8/8/R3K3 w Q - 0 1",            6, 803711 },
   { "r3k3/8/8/8/8/8/8/3K4 b q - 0 1",            6, 803711 },
   // castling (including losing cr due to rook capture): 
   { "r3k2r/1b4bq/8/8/8/8/7B/R3K2R w KQkq - 0 1",   4, 1274206 },
   { "r3k2r/7b/8/8/8/8/1B4BQ/R3K2R b KQkq - 0 1",    4, 1274206 },
   // castling prevented: 
   { "r3k2r/8/3Q4/8/8/5q2/8/R3K2R b KQkq - 0 1",   4, 1720476 },
   { "r3k2r/8/5Q2/8/8/3q4/8/R3K2R w KQkq - 0 1",   4, 1720476 },
   // promote out of check: 
   { "2K2r2/4P3/8/8/8/8/8/3k4 w - - 0 1",         6, 3821001 },
   { "3K4/8/8/8/8/8/4p3/2k2R2 b - - 0 1",         6, 3821001 },
   // discovered check: 
   { "8/8/1P2K3/8/2n5/1q6/8/5k2 b - - 0 1",         5, 1004658 },
   { "5K2/8/1Q6/2N5/8/1p2k3/8/8 w - - 0 1",         5, 1004658 },
   // promote to give check: 
   { "4k3/1P6/8/8/8/8/K7/8 w - - 0 1",            6, 217342 },
   { "8/k7/8/8/8/8/1p6/4K3 b - - 0 1",            6, 217342 },
   // underpromote to check: 
   { "8/P1k5/K7/8/8/8/8/8 w - - 0 1",            6, 92683 },
   { "8/8/8/8/8/k7/p1K5/8 b - - 0 1",            6, 92683 },
   // self stalemate: 
   { "K1k5/8/P7/8/8/8/8/8 w - - 0 1",            6, 2217 },
   { "8/8/8/8/8/p7/8/k1K5 b - - 0 1",            6, 2217 },
   // stalemate/checkmate: 
   { "8/k1P5/8/1K6/8/8/8/8 w - - 0 1",            7, 567584 },
   { "8/8/8/8/1k6/8/K1p5/8 b - - 0 1",            7, 567584 },
   // double check: 
   { "8/8/2k5/5q2/5n2/8/5K2/8 b - - 0 1",         4, 23527 },
   { "8/5k2/8/5N2/5Q2/2K5/8/8 w - - 0 1",         4, 23527 },

   // short castling impossible although the rook never moved away from its corner 
   { "1k6/1b6/8/8/7R/8/8/4K2R b K - 0 1", 5, 1063513 },
   { "4k2r/8/8/7r/8/8/1B6/1K6 w k - 0 1", 5, 1063513 },

   // long castling impossible although the rook never moved away from its corner 
   { "1k6/8/8/8/R7/1n6/8/R3K3 b Q - 0 1", 5, 346695 },
   { "r3k3/8/1N6/r7/8/8/8/1K6 w q - 0 1", 5, 346695 },

   // From the Wiki
   { "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -", 4, 4085603 },
   { "rnbqkb1r/pp1p1ppp/2p5/4P3/2B5/8/PPP1NnPP/RNBQK2R w KQkq - 0 6", 3, 53392 },

   // Shortened form of the third position below
   { "8/7p/p5pb/4k3/P1pPn3/8/P5PP/1rB2RK1 b - d3 0 28", 4, 67197 },

   // Some FRC postions by Reinhard Scharnagl
   // (http://www.talkchess.com/forum/viewtopic.php?t=55274)
   // We have each of them twice, to get the number of moves at the root
   // correct too.
   { "r1k1r2q/p1ppp1pp/8/8/8/8/P1PPP1PP/R1K1R2Q w KQkq - 0 1", 1, 23 },
   { "r1k2r1q/p1ppp1pp/8/8/8/8/P1PPP1PP/R1K2R1Q w KQkq - 0 1", 1, 28 },
   { "8/8/8/4B2b/6nN/8/5P2/2R1K2k w Q - 0 1", 1, 34 },
   { "2r5/8/8/8/8/8/6PP/k2KR3 w K - 0 1", 1, 17 },
   { "4r3/3k4/8/8/8/8/6PP/qR1K1R2 w KQ - 0 1", 1, 19 },

   { "r1k1r2q/p1ppp1pp/8/8/8/8/P1PPP1PP/R1K1R2Q w KQkq - 0 1", 2, 522 },
   { "r1k2r1q/p1ppp1pp/8/8/8/8/P1PPP1PP/R1K2R1Q w KQkq - 0 1", 2, 738 },
   { "8/8/8/4B2b/6nN/8/5P2/2R1K2k w Q - 0 1", 2, 318 },
   { "2r5/8/8/8/8/8/6PP/k2KR3 w K - 0 1", 2, 242 },
   { "4r3/3k4/8/8/8/8/6PP/qR1K1R2 w KQ - 0 1", 2, 628 },

   { "r1k1r2q/p1ppp1pp/8/8/8/8/P1PPP1PP/R1K1R2Q w KQkq - 0 1", 5, 7096972 }, 
   { "r1k2r1q/p1ppp1pp/8/8/8/8/P1PPP1PP/R1K2R1Q w KQkq - 0 1", 5, 15194841 }, 
   { "8/8/8/4B2b/6nN/8/5P2/2R1K2k w Q - 0 1", 5, 3223406 }, 
   { "2r5/8/8/8/8/8/6PP/k2KR3 w K - 0 1", 5, 985298 }, 
   { "4r3/3k4/8/8/8/8/6PP/qR1K1R2 w KQ - 0 1", 5, 8992652 },

   // John Merlino's test positions, some of these take a long time, only do them
   // in debug mode.
#ifdef DEBUGMODE
   { "r3k2r/8/8/8/3pPp2/8/8/R3K1RR b KQkq e3 0 1", 6, 485647607 },
   { "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1", 6, 706045033 },
   { "8/7p/p5pb/4k3/P1pPn3/8/P5PP/1rB2RK1 b - d3 0 28", 6, 38633283 },
   { "8/3K4/2p5/p2b2r1/5k2/8/8/1q6 b - - 1 67", 7, 493407574 },
   { "rnbqkb1r/ppppp1pp/7n/4Pp2/8/8/PPPP1PPP/RNBQKBNR w KQkq f6 0 3", 6, 244063299 },
   { "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -", 5, 193690690 },
   { "8/p7/8/1P6/K1k3p1/6P1/7P/8 w - -", 8, 8103790 },
   { "n1n5/PPPk4/8/8/8/8/4Kppp/5N1N b - -", 6, 71179139 },
   { "r3k2r/p6p/8/B7/1pp1p3/3b4/P6P/R3K2R w KQkq -", 6, 77054993 },

   { "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - -", 7, 178633661 },
   { "8/5p2/8/2k3P1/p3K3/8/1P6/8 b - -", 8, 64451405 },
   { "r3k2r/pb3p2/5npp/n2p4/1p1PPB2/6P1/P2N1PBP/R3K2R w KQkq -", 5, 29179893 },
#endif

   { NULL, 0, 0 }
};

/* Test positions, for Spartan chess */
static position_signature_t spartan_perftests[] = {
   { "5Q1k/8/7K/8/8/8/5h2/8 b - -", 5, 53727 },
   { "k7/g6k/1c6/7K/2B1Q3/8/8/1R6 b - - 34 75", 5, 6293168 },
   { "lgkcckw1/hhhhhhhh/8/8/3l4/5P2/PPPPPKPP/RNBQ1BNR w KQ - 3 3", 5, 1190857 },
   { "1R1k1w2/h5kh/h1g1B2h/3c3h/3Q1PN1/8/2P3PP/R6K b - - 0 1", 5, 23939987 },
   { "3k4/2h1k2h/5Q2/2hR4/3h4/8/PP3P1P/3w1K2 b  - 0 7", 5, 3195587 },
   { "4k2R/4hk2/2c2hh1/h3gh2/3h4/1B3P2/PP3P1P/2R2K2 b  - 0 7", 5, 6440892 },
   { "3k4/2h2k1R/5Q2/g4h2/3h4/8/PP3P1P/3w1K2 b  - 1 6", 5, 8066536 },

   { NULL, 0, 0 }
};

/* Test positions, for Shogi */
static position_signature_t shogi_perftests[] = {
   { "8l/1l+R2P3/p2pBG1pp/kps1p4/Nn1P2G2/P1P1P2PP/1PS6/1KSG3+r1/LN2+p3L [Sbgnppp] b 0 62", 3, 2552846 },
   { "lnsgkgsnl/1r5b1/pppppp1pp/6p2/9/2P6/PP1PPPPPP/1B5R1/LNSGKGSNL [-] w 0 1", 4, 2000286 },
   { "l6+Rl/3s1s3/p4p1pp/1G1G2p2/1p3kBN1/4P1P2/PP+p2R1PP/4KS3/1+b6L[PPPNNSppnlgg] b 1 47", 1, 5 },

   { NULL, 0, 0 }
};

/* Test positions, for Xiangqi */
static position_signature_t xiangqi_perftests[] = {
   { "rheakaehr/9/1c7/p1p1p1p1p/7c1/4C4/P1P1P1P1P/1C7/9/RHEAKAEHR b - 4 2", 4, 411539 },
   { "2e1ka1h1/2P1h1HC1/9/p3pc3/8p/9/4P2c1/4E4/9/2EAKA3 b 2 26", 2, 60 },
   { "rheakaehr/9/1c5c1/p1p1p1p1p/9/9/P1P1P1P1P/1C5C1/9/RHEAKAEHR w - - 0 1", 5, 133312995 },

   { NULL, 0, 0 }
};

/* Test positions, for Seirawan
 */
static position_signature_t seirawan_perftests[] = {
   { "Q3k2r/2qnbppp/1ppppn2/5bB1/3P4/2N1PN2/PP2BPPP/R3K2R [HEhe] b KQBCFGkcdfg - 0 2", 1, 5 },
   { "rQbqkbnr/p1pppppp/8/8/2p5/8/PP1PPPPP/RNB1KBNR [HEhe] b KQBCFGkqcdfg - 0 3", 1, 30 },
   { "6k1/5ppp/1p6/1P6/4e3/8/rb1BKPbP/1R6 [-] w - - 8 1", 4, 97954 },
   { "1h1qkber/rQ1bp1pp/3p1n2/2pP2N1/p7/P3B3/1PP2PPP/RE2KBHR [-] w KQk - 1 16", 4, 1966909 },

   { NULL, 0, 0 }
};

/* Test positions, for Sittuyin
 */
static position_signature_t sittuyin_perftests[] = {
   { "8/6k1/6p1/3s2P1/3npR2/2r5/p2N2F1/3K4[-] b 0 49", 4, 395063 },
   { "8/5R2/2P5/5S2/3sFs2/P3k3/2K5/8[-] w 3 69", 4, 62239 },

   { NULL, 0, 0 }
};

/* Test positions, for Crazyhouse
 */
static position_signature_t crazyhouse_perftests[] = {
   { "2kr1b1r/p1p1ppp1/2q1b1pn/2p1P3/2PpN3/3P2NP/PP3PP1/R1BQ1RK1[Bn] b - c3 0 16 ", 4, 12608213 },
   { "rnb1kbnr/Ppp1pppp/p7/3q4/8/8/PPPP1PPP/RNBQKBNR[p] w KQkq - 0 4", 4, 5225501 },
   { "rQ~b1kbnr/1pp1pppp/p7/3q4/8/8/PPPP1PPP/RNBQKBNR[Np] b KQkq - 0 4", 4, 12471091 },
   { NULL, 0, 0 }
};

/* Benchmark positions. For now we just use the Stockfish ones */
static position_signature_t benchtests[] = {
  { "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", 0, 0 },
  { "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -", 0, 0 },
  { "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - -", 0, 0 },
  { "4rrk1/pp1n3p/3q2pQ/2p1pb2/2PP4/2P3N1/P2B2PP/4RRK1 b - - 7 19", 0, 0 },
  { "rq3rk1/ppp2ppp/1bnpb3/3N2B1/3NP3/7P/PPPQ1PP1/2KR3R w - - 7 14", 0, 0 },
  { "r1bq1r1k/1pp1n1pp/1p1p4/4p2Q/4Pp2/1BNP4/PPP2PPP/3R1RK1 w - - 2 14", 0, 0 },
  { "r3r1k1/2p2ppp/p1p1bn2/8/1q2P3/2NPQN2/PPP3PP/R4RK1 b - - 2 15", 0, 0 },
  { "r1bbk1nr/pp3p1p/2n5/1N4p1/2Np1B2/8/PPP2PPP/2KR1B1R w kq - 0 13", 0, 0 },
  { "r1bq1rk1/ppp1nppp/4n3/3p3Q/3P4/1BP1B3/PP1N2PP/R4RK1 w - - 1 16", 0, 0 },
  { "4r1k1/r1q2ppp/ppp2n2/4P3/5Rb1/1N1BQ3/PPP3PP/R5K1 w - - 1 17", 0, 0 },
  { "2rqkb1r/ppp2p2/2npb1p1/1N1Nn2p/2P1PP2/8/PP2B1PP/R1BQK2R b KQ - 0 11", 0, 0 },
  { "r1bq1r1k/b1p1npp1/p2p3p/1p6/3PP3/1B2NN2/PP3PPP/R2Q1RK1 w - - 1 16", 0, 0 },
  { "3r1rk1/p5pp/bpp1pp2/8/q1PP1P2/b3P3/P2NQRPP/1R2B1K1 b - - 6 22", 0, 0 },
  { "r1q2rk1/2p1bppp/2Pp4/p6b/Q1PNp3/4B3/PP1R1PPP/2K4R w - - 2 18", 0, 0 },
  { "4k2r/1pb2ppp/1p2p3/1R1p4/3P4/2r1PN2/P4PPP/1R4K1 b - - 3 22", 0, 0 },
  { "3q2k1/pb3p1p/4pbp1/2r5/PpN2N2/1P2P2P/5PP1/Q2R2K1 b - - 4 26", 0, 0 },

  { NULL, 0, 0 }
};

static help_topic_t help_topic[] = {
   /* .........|.........|.........|.........|.........|.........|.........|.........| */
   { "all", NULL, NULL },

   { "analyse", "analyse, analyze",
     "  Analyse the current position.\n" },

   { "bench", "test benchmark [depth]",
     "  Perform a benchmark test to the specified depth. The returned node-count\n"
     "  can be used as a validation that the program is working correctly while\n"
     "  the reported time can be used as a performance measure on the current\n"
     "  system.\n" },

   { "board", "board [on|off]",
     "  Print the current board position, or toggles automatic printing of the\n"
     "  current position on and off.\n" },

   { "book", "book [filename|off]",
     "  Set the name of the (polyglot) opening book file to use.\n"
     "  'book off' switches off the opening book.\n" },

#ifdef SMP
   { "cores", "cores N, threads N",
     "  Use N cores/threads for the search.\n" },
#endif

   { "fen", NULL,
     "  Print the FEN representation of the current board position.\n" },

   { "force", NULL,
     "  Switch to 'force' mode: Sjaak will play neither side in the game until\n"
     "  it receives a 'go' command\n" },

   { "go", NULL,
     "  Switch off force mode, tell Sjaak to start playing from the current position.\n" },

   { "help", "help [topic]",
     "  Display a list of help topics, or detailed information about a particular\n"
     "  topic.\n" },

   { "hint", NULL,
     "  Print the move that Sjaak is pondering on. If there is no ponder move, no\n"
     "  hint is displayed.\n" },

   { "history", NULL,
     "  Print the game history to the current point.\n" },

   { "level", "level moves time inc",
     "  Set time control options: number of moves per session, time per session\n"
     "  (either in minutes or minutes:seconds) and time increment (in seconds)\n" },

   { "load", "load filename",
     "  Load variant descriptions from the specified file\n" },

   { "maxnodes", "maxnodes n",
     "  Set the maximum number of nodes that can be searched before a move is\n"
     "  returned. Setting maxnodes to 0 disables it and restores normal time\n"
     "  control.\n" },

   { "memory", "memory [MB]",
     "  Set the (approximate) amount of memory the program can use, in MB. The\n"
     "  actual amount of memory used will be different from this and may be\n"
     "  slightly larger or smaller.\n" },

   { "multipv", "multipv n",
     "  Set the number of full evaluation/variations to find in analysis mode.\n"},

   { "new", NULL,
     "  Start a new game from the starting position.\n"
     "  In xboard mode the current variant is reset to 'normal'.\n" },

   { "perft", "perft [depth] [print depth]",
     "  Perform a 'perft' (performance test) on the current position: count all\n"
     "  positions that can result from this position to the specified depth.\n"
     "  If 'print depth' is specified then the total will be sub-divided per move\n"
     "  upto 'print depth'\n" },

   { "ponder", "ponder [on|off]",
     "  Switches ponder mode on or off (Sjaak will think while it is not on move)\n" }, 

   { "post", "(no)post, post off",
     "  Display thinking output (post) or not (nopost/post off)\n" },

   { "prompt", "prompt [on|off]",
     "  Switch the prompt on or off\n" },

   { "quit", "quit, exit, bye",
     "  Exit Sjaak\n" },

   { "random", "random [on|off]",
     "  Switch randomisation of opening moves on or off.\n" },

   { "readepd", "readepd filename",
     "  Read (and setup) a position from an EPD file.\n" },

   { "rules", NULL,
     "  Summarise the rules of the game and the movement of the pieces in human-radable text.\n" },

   { "setboard", "setboard FEN",
     "  Setup a position on the board from a FEN string.\n" },

   { "sd", "sd depth",
     "  Specify the maximum search depth\n" },

   { "settings", NULL,
     "   Show current settings\n" },

   { "shell", "!command",
     "  Run a shell command.\n" },

   { "skill", "skill level",
     "  Set the skill level for engine play.\n"},

   { "st", "st time",
     "  Specify the maximum time (in seconds) to think on a single move\n" }, 

   { "takeback", "takeback, remove",
     "  Reverses the last two moves in the game, if any.\n" }, 

   { "test", "test [movegen|benchmark [depth]|legal movegen|chase|see <move>|wac|sts]",
     "  Perform tests on the move generator, the search or various evaluation\n"
     "  components. Can also run a number of build-in test suites.\n" },

   { "time", "time csec",
     "  Set the remaining time on the engine's clock, in centi-seconds.\n" },

   { "ucci", "ucci [on|off]",
     "  Toggle UCCI mode on or off. In UCCI mode commands from the UCCI protocol\n"
     "  are recognised in addition to the standard commands described here.\n"
     "  'ucci' is equivalent to 'ucci on'\n"},

   { "uci", "uci [on|off]",
     "  Toggle UCI mode on or off. In UCI mode commands from the UCI protocol\n"
     "  are recognised in addition to the standard commands described here.\n"
     "  'uci' is equivalent to 'uci on'\n"},

   { "undo", NULL,
     "  Unmakes the last move in the game, if any. If it is Sjaak's turn, it will\n"
     "  start thinking again immediately.\n" },

   { "unload", "unload filename",
     "  Unload variant descriptions from the specified file\n" },

   { "usi", "usi [on|off]",
     "  Toggle USI mode on or off. In USI mode commands from the USI protocol\n"
     "  are recognised in addition to the standard commands described here.\n"
     "  'usi' is equivalent to 'usi on'\n"},

   { "variant", "variant name",
     "  Set the name of the variant that will be selected when a new game is started.\n"
     "  If no name is specified a list of known variants is displayed.\n" },

   { "variants", NULL,
     "  List all known variants.\n" },

   { "xboard", "xboard [on|off]",
     "  Switch to xboard mode: prompt off, don't display board, don't print SAN\n"
     "  moves, don't trap interrupts (ctrl-C).\n"
     "  'xboard' is equivalent to 'xboard on'\n" },

   { "MOVE", NULL,
     "  Play a move on the board. Moves can be entered as 'long algebraic'\n"
     "  (e2e4, g1f3) or SAN (e4, Nf3).\n" },

   { NULL, NULL, NULL }
};

#ifdef HAVE_READLINE
static const char *xcmd_list[] = {
   "moves", "longmoves", "pseudomoves", "pieces", "pieceinfo",
   NULL,
};
#endif

static int  option_ms = MATE_SEARCH_ENABLE_DROP;
static int  draw_count = 0;
static int  resign_count = 0;
static int  random_ply_count = 10;
static int  multipv = 1;
static eval_t random_amplitude = 20;
static eval_t draw_threshold = 0;
static eval_t resign_threshold = -500;
static bool user_variants_first = true;
static bool mask_dark_squares = false;
static bool send_piece_descriptions = true;
static bool report_fail_low = false;
static bool report_fail_high = false;
static bool repetition_claim = true;
static bool castle_oo = true;
static bool remember_eval_file = false;
static bool prompt = true;
static bool show_board = true;
static bool san = true;
static bool trapint = true;
static int  tc_moves = 40;
static int  tc_time  = 60000;
static int  tc_inc   = 0;
static char configfile[1024] = { 0 };
static char *fairy_file = NULL;
static char *eval_file = NULL;
static char deferred[256];
static int  lift_sqr = 0;
static int  skill_level = LEVEL_NORMAL;

static struct {
   const char *label, *name;
   bool *opt;
   bool def;
   bool neg;
   bool resend_options;
} boolean_settings[] = {
   { "Send 'piece' descriptions",                           "send_piece_descriptions",  &send_piece_descriptions,  send_piece_descriptions,  false, false },
   { "Mark holes in board",                                 "mask_dark_squares",        &mask_dark_squares,        mask_dark_squares,        true,  false },
   { "List user-defined variants before buildin variants",  "user_variants_first",      &user_variants_first, user_variants_first,           false, true  },
   { "Report fail low",                                     "report_fail_low",          &report_fail_low,          report_fail_low,          false, false },
   { "Report fail high",                                    "report_fail_high",         &report_fail_high,         report_fail_high,         false, false },
   { "Claim repetitions",                                   "repetition_claim",         &repetition_claim,         repetition_claim,         false, false },
   { "Send O-O/O-O-O for castling",                         "castle_oo",                &castle_oo,                castle_oo,                false, false },
   { "Remember evaluation parameter file",                  "remember_eval_file",       &remember_eval_file,       remember_eval_file,       false, false },
   { NULL, NULL, NULL, false, false, false },
};

static struct {
   const char *label;
   int value;
   int *var;
} combo_skill[] = {
   { "Clueless", LEVEL_RANDOM, &skill_level },
   { "Random",   LEVEL_BEAL,   &skill_level },
   { "Static",   LEVEL_STATIC, &skill_level },
   { "Normal",   LEVEL_NORMAL, &skill_level },
};


#ifdef __APPLE__
#define __unix__
#endif
#ifdef __unix__
static sig_t old_signal_handler;

void interrupt_computer(int i)
{
   abort_search = true;
   if (old_signal_handler)
      old_signal_handler(i);
}
#endif


#ifdef HAVE_READLINE
static bool stdin_is_terminal(void)
{
#ifdef __unix__
   return isatty(fileno(stdin));
#else
   return true;
#endif
}
#endif

static FILE *f = NULL;
static char *buf;

static bool may_ponder = false;
static bool in_play = true;

static void log_engine_output(const char *msg, ...) ATTRIBUTE_UNUSED;
static void log_engine_output(const char *msg, ...)
{
   va_list ap;
   va_start(ap, msg);
   vsnprintf(buf, 65535, msg, ap);
   va_end(ap);

   if (f) {
      fprintf(f, "  %s", buf);
      fflush(f);
   }
}

static void log_xboard_output(const char *msg, ...)
{
   va_list ap;
   static bool newline = true;
   va_start(ap, msg);
   vsnprintf(buf, 65535, msg, ap);
   va_end(ap);

   if (f) {
      if (newline)
         fprintf(f, "> ");
      newline = false;
      fprintf(f, "%s", buf);
      if (strstr(buf, "\n"))
         newline = true;
   }

   printf("%s", buf);
}

struct variant_file_list_t {
   const char *filename;
   const char *shortname;
   const char *longname;
   int files, ranks;
   size_t line_number;
};
static variant_file_list_t *custom_variants = NULL;
static int num_custom_variants = 0;
static int max_custom_variants = 0;

static variant_file_list_t *new_variant_file_entry(void)
{
   variant_file_list_t *file = NULL;

   if (num_custom_variants >= max_custom_variants) {
      max_custom_variants = max_custom_variants + 8;
      custom_variants = (variant_file_list_t *)realloc(custom_variants, max_custom_variants*sizeof *custom_variants);
   }

   file = custom_variants + num_custom_variants;
   num_custom_variants++;

   return file;
}

static void invalidate_variant_file(const char *filename)
{
   if (!filename) return;

   /* Skip if the file has already been indexed.
    * TODO: we need a way to invalidate a previously scanned file.
    */
   for (int n=0; n<num_custom_variants && num_custom_variants > 0; n++) {
      if (!streq(custom_variants[n].filename, filename)) continue;

      free((void *)custom_variants[n].filename);
      free((void *)custom_variants[n].shortname);
      free((void *)custom_variants[n].longname);

      custom_variants[n].filename = NULL;
      custom_variants[n].shortname = NULL;
      custom_variants[n].longname = NULL;

      custom_variants[n] = custom_variants[num_custom_variants-1];

      n--;
      num_custom_variants--;
   }
}

static void scan_variant_file(const char *filename)
{
   /* Skip if the file has already been indexed.
    * TODO: we need a way to invalidate a previously scanned file.
    */
   for (int n=0; n<num_custom_variants; n++) {
      if (streq(custom_variants[n].filename, filename)) return;
   }

   FILE *f = fopen(filename, "r");
   if (!f) return;

   size_t line_number = 0;
   char line[4096];
   const char *name = NULL;
   while (!feof(f)) {
      if (fgets(line, sizeof line, f) == 0)
         continue;
      /* Strip away comments */
      char *s = strstr(line, "#");
      if (s) s[0] = '\0';

      /* Snip end-of-line */
      s = strstr(line, "\n");
      if (s) s[0] = '\0';

      /* Strip trailing space */
      s = line+strlen(line)-1;
      while (s > line && isspace(s[0])) { s[0] = '\0'; s--; }

      /* New variant */
      if (strstr(line, "Variant:") == line) {
         s = line + 8;
         while (isspace(*s)) s++;
         free((void *)name);
         name = strdup(s);
         continue;
      }

      if (strstr(line, "Board:") == line) {
         variant_file_list_t *file = new_variant_file_entry();
         int files, ranks;
         sscanf(line + 6, "%dx%d", &files, &ranks);

         file->filename = strdup(filename);
         file->longname = strdup(name);
         file->files = files;
         file->ranks = ranks;
         file->line_number = line_number;
         char *s = strdup(name);
         file->shortname = s;
         /* Truncate the long name into something XBoard can handle */
         while (*s) {
            *s = tolower(*s);
            if (isspace(*s)) *s = '_';
            if (!isalnum(s[1])) {
               if (!isspace(s[1]) && s[1] != '-')
                  s[1] = 0;
               if (*s == '_') *s = 0;
            }
            s++;
         }
      }
      line_number++;
   }
   free((void *)name);

   //for (int n = 0; n<num_custom_variants; n++) {
   //   printf("'%s' '%s' (%dx%d) %llu\n",
   //      custom_variants[n].longname, custom_variants[n].shortname, 
   //      custom_variants[n].files, custom_variants[n].ranks, 
   //      (unsigned long long)custom_variants[n].line_number);
   //}

   fclose(f);
}

struct variant_t {
   int files, ranks, holdings;
   const char *name;
   new_variant_game_t create;
};

int num_games = 0;
const char *variant_name = NULL;
static variant_t standard_variants[] = {
   {  8,  8, 0, "chess",         create_standard_game },
   {  8,  8, 0, "seirawan",      create_seirawan_game },
   {  8,  8, 0, "shatar",        create_shatar_game },
   {  8,  8, 0, "makruk",        create_makruk_game },
   {  8,  8, 0, "shatranj",      create_shatranj_game },
   {  8,  8, 6, "sittuyin",      create_sittuyin_game },

   {  8,  8, 6, "crazyhouse",    create_crazyhouse_game },
   {  8,  8, 6, "chessgi",       create_chessgi_game },
   //{  8,  8, 6, "twilight",      create_twilight_game },

   {  8,  8, 0, "asean",         create_asean_game },
   {  8,  8, 0, "ai-wok",        create_aiwok_game },

   {  8,  8, 0, "super",         create_super_game },

   {  8,  8, 0, "spartan",       create_spartan_game },
   {  8,  8, 1, "pocketknight",  create_pocketknight_game },
   {  8,  8, 0, "kingofthehill", create_kingofthehill_game },
   {  8,  8, 0, "knightmate",    create_knightmate_game },
   {  8,  8, 0, "berolina",      create_berolina_game },

   {  6,  6, 0, "losalamos",     create_losalamos_game },
   {  5,  5, 0, "micro",         create_micro_game },

   { 10,  8, 0, "capablanca",    create_capablanca_game },
   { 10,  8, 0, "gothic",        create_gothic_game },
   { 10,  8, 0, "embassy",       create_embassy_game },

   { 10,  8, 0, "greatshatranj", create_greatshatranj_game },

   { 12,  8, 0, "courier",       create_courier_game },

   { 10, 10, 0, "grand",         create_grand_game },
   { 10, 10, 0, "opulent",       create_opulent_game },

   { 12, 12, 0, "omega",         create_omega_game },

   {  5,  5, 5, "minishogi",     create_minishogi_game },
   {  9,  9, 0, "shoshogi",      create_shoshogi_game },
   {  9,  9, 8, "shogi",         create_shogi_game },
   {  7,  7, 6, "torishogi",     create_tori_game },

   {  9, 10, 0, "xiangqi",       create_chinese_game },
};
static const int num_standard_variants = sizeof standard_variants/sizeof *standard_variants;

static char fairy_alias[512] = { 0 };
static char normal_alias[512] = { 0 };
static struct name_alias_t {
   const char *name, *alias;
} aliases[] = {
   { normal_alias,   "normal" },
   { "chess",        "chess960" },
   { "chess",        "fischerandom" },
   { "chess",        "fischerrandom" },
   { "chess",        "nocastle" },
   { "chess",        "wildcastle" },
   { "greatshatranj","great" },
   { "capablanca",   "caparandom" },
   { "minishogi",    "minisho" },
   { "minishogi",    "5x5+5_shogi" },
   { "torishogi",    "tori" },
   { "torishogi",    "7x7+6_shogi" },
   { "shoshogi",     "sho" },
   { "shoshogi",     "9x9+0_shogi" },
   { "kingofthehill","king-of-the-hill" },
   { fairy_alias,    "fairy" },
};
static char *user_alias = NULL;

const char *find_real_variant_name(const char *variant_name, int recurse = 0)
{
   for (int n=0; n<num_standard_variants; n++) {
      if (streq(variant_name, standard_variants[n].name))
         return standard_variants[n].name;
   }

   int num_alias = sizeof aliases / sizeof *aliases;
   if (recurse < 2)
   for (int n = 0; n<num_alias; n++) {
      if (streq(variant_name, aliases[n].alias))
         return find_real_variant_name(aliases[n].name, recurse+1);
   }
   return NULL;
}


game_t *create_variant_game(const char *variant_name, int recurse = 0)
{
   for (int n = 0; n<num_custom_variants; n++) {
      if (streq(variant_name, custom_variants[n].shortname)) {
         return create_game_from_file(custom_variants[n].filename, custom_variants[n].longname);
      }
   }

   for (int n=0; n<num_standard_variants; n++) {
      if (streq(variant_name, standard_variants[n].name))
         return standard_variants[n].create(standard_variants[n].name);
   }

   int num_alias = sizeof aliases / sizeof *aliases;
   if (recurse < 2)
   for (int n = 0; n<num_alias; n++) {
      if (streq(variant_name, aliases[n].alias))
         return create_variant_game(aliases[n].name, recurse+1);
   }
   if (streq(variant_name, "test"))
      return create_test_game("test");
   return NULL;
}

/* Play a sequence of moves from the initial position */
bool input_move(game_t *game, char *move_str)
{
   char *s;

   if (!game)
      return false;

   s = move_str;
   if(s && *s) {
      while (*s == ' ') s++;
      move_t move = game->move_string_to_move(s);
      if (move == 0) {
         //fprintf(stderr, "\n");
         if (f) {
            fprintf(f, "Report illegal move %s in position:\n", move_str);
            //print_bitboards_file(f, game->board);
         }
         return false;
      }
      game->playmove(move);
   } else {
      return false;
   }

   return true;
}

static void send_legal_move_targets(game_t *game, int from, const movelist_t *external_movelist = NULL)
{
   int move_board[256];
   char fen[4096];

   if (!game) return;

   memset(move_board, 0, sizeof move_board);

   movelist_t movelist;

   /* First, generate the list of moves for this position */
   if (external_movelist) {
      movelist.clear();
      for(int n = 0; n<external_movelist->num_moves; n++)
         movelist.push(external_movelist->move[n]);
   } else {
      game->generate_legal_moves(&movelist);
   }

   move_t move;
   bool set = false;
   while ((move = movelist.next_move())) {
      if (is_drop_move(move)) continue;
      if (get_move_from(move) != from) continue;
      int s = game->bit_to_square[get_move_to(move)];
      if (s < 0) continue;

      int p = get_move_piece(move);
      int piece = piece_symbol_string[p];
      bool multi = is_double_capture_move(move) || (piece != ' ' && is_capture_move(move) && get_move_capture_square(move)!=get_move_to(move));

      if (multi) {
         int n = get_move_pickups(move);
         for (int c=0; c<n; c++) {
            uint16_t p  = get_move_pickup(move, c);
            int square  = decode_pickup_square(p);
            if (square != get_move_to(move))
               move_board[game->bit_to_square[square]] = 4;
         }
      }

      if (is_castle_move(move) && abs(get_move_from(move) - get_move_to(move)) == 1) {
         int square = get_castle_move_from2(move);
         move_board[game->bit_to_square[square]] = 4;
      }

      if (move_board[s] == 5)
         move_board[s] = 3;

      if (move_board[s] == 4) continue;
      if (move_board[s] == 3) continue;
      if (is_promotion_move(move))
         move_board[s] = 5;
      else if (move_board[s] == 0 && is_capture_move(move))
         move_board[s] = 2;
      else
         move_board[s] = 1;
      set = set || move_board[s];
   }
   if (!set) { /* Second part in a multi-step move */
      movelist.rewind();
      while ((move = movelist.next_move())) {
         int p = get_move_piece(move);
         int piece = piece_symbol_string[p];
         bool multi = is_double_capture_move(move) || (piece != ' ' && is_capture_move(move) && get_move_capture_square(move)!=get_move_to(move));
         if (get_move_to(move) == from) {
            int s = game->bit_to_square[from];
            if (s>=0) 
               move_board[s] = 1;
         }
         if (!multi) continue;

         int n = get_move_pickups(move);
         bool mark = false;
         for (int c=0; c<n && !mark; c++) {
            uint16_t p  = get_move_pickup(move, c);
            int square  = decode_pickup_square(p);
            if (square == from)
               mark = true;
         }
         if (!mark) continue;

         for (int c=0; c<n; c++) {
            uint16_t p  = get_move_pickup(move, c);
            int square  = decode_pickup_square(p);

            int s = game->bit_to_square[square];
            if (is_double_capture_move(move))
               move_board[s] = 2;
            else
               move_board[s] = 1;
         }
         int s = game->bit_to_square[get_move_to(move)];
         if (is_double_capture_move(move))
            move_board[s] = 2;
         else
            move_board[s] = 1;
      }
   }
   //log_xboard_output("# %s\n# Set: %d\n", game->make_fen_string(), set);

   int n = 0;
   int ranks = (game->virtual_ranks > 0) ? game->virtual_ranks : game->ranks;
   int files = (game->virtual_files > 0) ? game->virtual_files : game->files;
   for (int r = ranks-1; r>=0; r--) {
      int count = 0;
      for (int f = 0; f < files; f++) {
         int s = f + r * files;

         /* Empty? */
         if (move_board[s] == 0) {
            count++;
            continue;
         }

         /* Not empty, do we have a count? */
         if (count) n += snprintf(fen+n, 4096 - n, "%d", count);
         count = 0;

         const char *colour_string = " YRMCB";
         n += snprintf(fen+n, 4096-n, "%c", colour_string[move_board[s]]);
      }
      if (count) n += snprintf(fen+n, 4096 - n, "%d", count);
      if (r) n += snprintf(fen+n, 4096 - n, "/");
   }

   log_xboard_output("highlight %s\n", fen);
}

/* Play a sequence of moves from the initial position */
void replay_game(game_t *game, char *moves)
{
   movelist_t movelist;
   char *s;
   /* First rewind to the beginning if not there yet */
   while (game->moves_played)
      game->takeback();

   s = moves;
   while(s && *s) {
      char move_str[10] = { 0 };
      while (*s == ' ') s++;
      for (int n = 0; n<10; n++) {
         if (*s == ' ' || *s == 0) break;
         move_str[n] = *s;
         s++;
      }

      bool result = input_move(game, move_str);

      if (!result) {
         fprintf(stderr, "Bad move %s in movelist!\n", move_str);
         break;
      }
   }
}

static void set_time_from_string(game_t *game, const char *timestr)
{
   float milliseconds;
   sscanf(timestr, "%g", &milliseconds);
   milliseconds *= 10;

   /* Reserve a bit of time for when we reach the time control, so we don't get penalised by a time
    * loss. We don't need this for a Fischer clock.
    */
   if (milliseconds > TIME_BUFFER)
      milliseconds -= TIME_BUFFER;

   game->clock.time_left = int(milliseconds);
   game->move_clock[game->moves_played] = game->clock.time_left;
   set_time_for_game(&game->clock);
}

static void set_timecontrol_from_string(game_t *game, const char *tcstr)
{
   int moves, milliseconds;
   float minutes, seconds, inc;
   /* Set defaults */
   set_time_per_move(&game->clock, 5000);
   game->clock.movestotc = 0;
   game->clock.movestogo = 0;
   game->clock.time_inc = 0;

   if (strstr(tcstr, ":")) {
      sscanf(tcstr, "%d %g:%g %g", &moves, &minutes, &seconds, &inc);
   } else {
      sscanf(tcstr, "%d %g %g", &moves, &minutes, &inc);
      seconds = 0;
   }
   seconds += minutes*60;
   milliseconds = int(seconds * 1000);

   /* Reserve a bit of time for when we reach the time control, so we don't get penalised by a time
    * loss. We don't need this for a Fischer clock.
    */
   if (inc == 0 && milliseconds > TIME_BUFFER)
      milliseconds -= TIME_BUFFER;

   game->clock.movestotc = moves;
   game->clock.movestogo = moves;
   game->clock.time_inc = int(inc*1000);
   game->clock.time_left = milliseconds;
   game->move_clock[game->moves_played] = game->clock.time_left;
   set_time_for_game(&game->clock);

   tc_moves = game->clock.movestotc;
   tc_time  = game->clock.time_left;
   tc_inc   = game->clock.time_inc;
}

static bool interrupt_ponder(game_t *)
{
   return keyboard_input_waiting();
}

static bool keyboard_input_on_move(game_t *game)
{
   if (deferred[0])  return true;
   if (abort_search) return true;
   static char ponder_input[65536];
   bool input_waiting = keyboard_input_waiting();
   bool read_input    = input_waiting && fgets(ponder_input, sizeof ponder_input, stdin);
   if (read_input) {
      chomp(ponder_input);
      trim(ponder_input);
      if (strstr(ponder_input, "random")) {
         sgenrand((unsigned int)time(NULL));
         game->random_ok = true;
         game->random_key = genrandui();
         game->random_amplitude = random_amplitude;
         game->random_ply_count = random_ply_count;
      } else if (strstr(ponder_input, "easy")) {
         may_ponder = false;
      } else if (strstr(ponder_input, "hard")) {
         may_ponder = true;
      } else if (strstr(ponder_input, "nopost") || strstr(ponder_input, "post off") == ponder_input) {
         game->set_xboard_output_function(NULL);
      } else if (strstr(ponder_input, "post")) {
         game->set_xboard_output_function(log_xboard_output);
      } else if (strstr(ponder_input, "?") == ponder_input) {
         return true;
      } else if (strstr(ponder_input, "stop") == ponder_input) {
         return true;
      } else if (strstr(ponder_input, "draw")) {
         if (game->draw_count > draw_count)
            log_xboard_output("offer draw\n");
      } else if (strstr(ponder_input, "new")) {
         snprintf(deferred, sizeof deferred, "%s", ponder_input);
         return true;
      } else if (strstr(ponder_input, "otim")) {
      } else if (strstr(ponder_input, "ping")) {
         snprintf(deferred, sizeof deferred, "%s", ponder_input);
         return true;
      } else if (strstr(ponder_input, "result")) {
         return true;
      } else if (strstr(ponder_input, "time")) {
         set_time_from_string(game, ponder_input+5);
      } else if (strstr(ponder_input, "level") == ponder_input) {
         set_timecontrol_from_string(game, ponder_input+6);
      } else if (strstr(ponder_input, "st ")) {
         float tpm = 5;
         sscanf(ponder_input+3, "%g", &tpm);
         int time_per_move = int(tpm * 1000);
         set_time_per_move(&game->clock, time_per_move);
      } else if (strstr(ponder_input, "force") == ponder_input) {
         in_play = false;
      } else if (strstr(ponder_input, "pause")) {
         uint64_t start_pause = get_timer();
         /* Sleep until keyboard input */
         while(!fgets(ponder_input, sizeof ponder_input, stdin) || !strstr(ponder_input, "resume"));
         uint64_t stop_pause = get_timer();
         game->clock.start_time += stop_pause - start_pause;
      } else if (streq(ponder_input, "quit")) {
         if (game)
            delete game;
         free(buf);
         exit(0);
      } else {
         snprintf(deferred, sizeof deferred, "%s", ponder_input);
      }
   }
   return false;
}

static bool (*uci_clock_handler)(const struct chess_clock_t *clock);
static bool uci_keyboard_input_on_ponder(game_t *game)
{
   if (!game->pondering) return true;
   static char ponder_input[65536];
   if (keyboard_input_waiting() && fgets(ponder_input, sizeof ponder_input, stdin)) {
      if (f) {
         fprintf(f, "< %s\n", ponder_input);
         fflush(f);
      }
      if (strstr(ponder_input, "ponderhit")) {
         game->clock.check_clock = uci_clock_handler;
         game->pondering = false;
         game->check_keyboard = NULL;
         return false;
      } else if (strstr(ponder_input, "isready") == ponder_input) {
         log_xboard_output("readyok\n");
         return false;
      } else if (strstr(ponder_input, "stop") == ponder_input) {
         game->pondering = false;
         return true;
      } else if (streq(ponder_input, "quit")) {
         if (game)
            delete game;
         free(buf);
         exit(0);
      }
   }
   return false;
}

static bool keyboard_input_analyse(game_t *game)
{
   static char ponder_input[65536];
   bool restart = false;
   if (game->analyse_move != 0) return true;
   if (!game->analysing) return false;

   if (keyboard_input_waiting() && fgets(ponder_input, sizeof ponder_input, stdin)) {
      chomp(ponder_input);
      trim(ponder_input);
      if (strstr(ponder_input, "undo")) {
         if (game->analyse_moves_played) {
            game->analyse_undo++;
            restart = true;
         }
      } else if (strstr(ponder_input, "new")) {
         game->analyse_new = true;
         restart = true;
      } else if (strstr(ponder_input, "exit")) {
         game->analysing = false;
         restart = true;
      } else if (strstr(ponder_input, "setboard")) {
         char *p = strstr(ponder_input, " ");
         while (p && *p && p[0]==' ') p++;
         free((void *)game->analyse_fen);
         game->analyse_fen = strdup(p);
         restart = true;
      } else if (strstr(ponder_input, ".") == ponder_input) {
      } else if (strstr(ponder_input, "hint")) {
      } else if (strstr(ponder_input, "bk")) {
      } else if (streq(ponder_input, "quit")) {
         if (game)
            delete game;
         free(buf);
         exit(0);
      } else if (strstr(ponder_input, "lift")) {
         for (int square = 0; square < game->ranks*game->files; square++) {
            if (streq(ponder_input+5, square_names[square])) {
               send_legal_move_targets(game, square, &game->analyse_movelist);
               lift_sqr = square;
               break;
            }
         }
      } else if (strstr(ponder_input, "put")) {
         /* Send promotion options, for promotion moves */
         char choice_str[256];
         int  cs = 0;
         const char *keep = "";

         movelist_t movelist;
         movelist.clear();
         for(int n = 0; n<game->analyse_movelist.num_moves; n++)
            movelist.push(game->analyse_movelist.move[n]);

         move_t move;
         while ((move = movelist.next_move())) {
            if (get_move_from(move) != lift_sqr) continue;
            for (int square = 0; square < game->ranks*game->files; square++) {
               if (streq(ponder_input+4, square_names[square])) {
                  if (get_move_to(move) != square) continue;

                  if (!is_promotion_move(move)) {
                     int p = get_move_piece(move);
                     keep = game->get_piece_notation(p);
                     break;
                  }
                  int p = get_move_promotion_piece(move);
                  cs += snprintf(choice_str+cs, sizeof(choice_str) - cs, "%s", game->get_piece_notation(p));

                  break;
               }
            }
         }
         if (cs) {
            cs+=snprintf(choice_str+cs, sizeof(choice_str) - cs, "%s", keep);
            for (int n=0; n<cs; n++)
               choice_str[n] = toupper(choice_str[n]);
            log_xboard_output("choice %s\n", choice_str);
         }
      } else if (strstr(ponder_input, "hover")) {
         /* TODO */
      } else {
         char *s = ponder_input;
         if(s && *s) {
            move_t move = game->move_string_to_move(s, &game->analyse_movelist);
            game->analyse_move = move;
            if (move == 0)
               log_xboard_output("Illegal move: %s\n", ponder_input);
            else
               restart = true;
         }
      }
   }
   return restart;
}

static uint64_t perft(game_t *game, int depth, int root, bool legal = false)
{
   movelist_t *movelist = game->movelist + depth;
   side_t me = game->get_side_to_move();
   uint64_t nodes = 0;
   int n;

   if (depth == 0) return 1;

   /* Check if previous move left the player in check */
   if (legal)
      game->generate_legal_moves(movelist);
   else
      game->generate_moves(movelist);

   for (n=0; n<movelist->num_moves; n++) {
      uint64_t count = 0;
      game->playmove(movelist->move[n]);
      if (legal || !game->player_in_check(me)) {   /* Don't count illegal moves */
         game->test_move_game_check();
         count = perft(game, depth-1, root - 1);
      }
      nodes += count;
      if (root > 0)
         printf("%8s %10" PRIu64 " %10" PRIu64 "\n", move_to_string(movelist->move[n], NULL), count, nodes);
      game->takeback();
      if (abort_search) break;
   }
   return nodes;
}

static bool run_movegen_test(const char *name, const char *variant, const position_signature_t *suite, bool legal)
{
   game_t *game = NULL;
   int n = 0;
   while (suite[n].fen) {
      uint64_t nodes;
      game = create_variant_game(variant);
      game->start_new_game();

      game->setup_fen_position(suite[n].fen);
      printf(".");
      fflush(stdout);
      nodes = perft(game, suite[n].depth, 0, legal);
      if (nodes != suite[n].nodes) {
         printf("\n");
         printf("*** Failed at %s position %d (%s):\n", name, n, suite[n].fen);
         printf("    Expected %" PRIu64 " nodes at depth %d, got %" PRIu64 " nodes\n", suite[n].nodes, suite[n].depth, nodes);
         return false;
      }

      delete game;
      n++;
   }

   return true;
}

static void test_movegen(bool legal = false)
{
   if (!run_movegen_test("Chess",      "chess",      perftests,            legal)) return;
   if (!run_movegen_test("Spartan",    "spartan",    spartan_perftests,    legal)) return;
   if (!run_movegen_test("Shogi",      "shogi",      shogi_perftests,      legal)) return;
   if (!run_movegen_test("XiangQi",    "xiangqi",    xiangqi_perftests,    legal)) return;
   if (!run_movegen_test("Seirawan",   "seirawan",   seirawan_perftests,   legal)) return;
   if (!run_movegen_test("Sittuyin",   "sittuyin",   sittuyin_perftests,   legal)) return;
   if (!run_movegen_test("Crazyhouse", "crazyhouse", crazyhouse_perftests, legal)) return;
   printf("\nOk.\n");
}

static uint64_t test_benchmark(int depth)
{
   game_t *game = NULL;
   int n = 0;
   uint64_t nodes = 0;
   uint64_t t = get_timer();
   unsigned long long int nodes_searched = 0;
#ifdef TRACK_PRUNING_STATISTICS
   memset(branches_pruned_by_move, 0, sizeof branches_pruned_by_move);
#endif

   while (benchtests[n].fen) {
      game = create_variant_game("chess");
      game->start_new_game();
      game->random_ok = false;
      game->output_iteration = NULL;
      game->uci_output = NULL;
      game->xboard_output = NULL;

      //printf("%s\n", benchtests[n].fen);
      game->setup_fen_position(benchtests[n].fen);
      printf(".");
      fflush(stdout);

#ifdef __unix__
      if (trapint) old_signal_handler = signal(SIGINT, interrupt_computer);
#endif
      game->think(depth);
#ifdef __unix__
      if (trapint) signal(SIGINT, old_signal_handler);
#endif
      nodes_searched = game->clock.nodes_searched;
      delete game;

      if (abort_search) {
         printf("\n*** Aborted");
         break;
      }

      //printf("%"PRIu64"\n", nodes);
      nodes += nodes_searched;
      n++;
   }

   uint64_t tt = get_timer();

   printf("\n");
   printf("%" PRIu64 " nodes searched\n", nodes);
   printf("Elapsed time %" PRIu64 " ms\n", (tt - t) / 1000);
   printf("%g nodes / s\n", 1.0e6*nodes / (tt - t));

   return (tt - t);
}

#ifdef SMP
/* Test the SMP search.
 * Test position is the first from Hyatt (1994), after White's 9th move in a game
 * Mchess Pro - Cray Blitz.
 */
static void test_smp(int cores, int depth)
{
   game_t *game = NULL;
   int n = get_number_of_threads();
   uint64_t t_start[2];
   uint64_t t_end[2];
   unsigned long long int moves_searched[2];
   char *fen = "r2qkbnr/ppp2p1p/2n5/3P4/2BP1pb1/2N2p2/PPPQ2PP/R1B2RK1/ b - - 2 9";//benchtests[1].fen;

   init_threads(1);
   game = create_variant_game("chess")
   game->start_new_game();
   game->random_ok = false;
   setup_fen_position(game, fen);
   print_board(game->board);
   printf("Single core analysis:\n");
   t_start[0] = get_timer();
   computer_play(game, depth);
   t_end[0] = get_timer();
   moves_searched[0] = game->moves_searched;
   end_game(game);

   init_threads(cores);
   printf("Analysis using %d cores:\n", cores);
   game = create_variant_game("chess")
   game->start_new_game();
   game->random_ok = false;
   setup_fen_position(game, fen);
   t_start[1] = get_timer();
   computer_play(game, depth);
   t_end[1] = get_timer();
   moves_searched[1] = game->moves_searched;
   end_game(game);

   init_threads(n);

   printf("\n");
   printf("Single core:     %-10"PRIu64" nodes searched\n", moves_searched[0]);
   printf("Multi-core:      %-10"PRIu64" nodes searched\n", moves_searched[1]);
   printf("Single core time %"PRIu64" ms\n", (t_end[0]-t_start[0]) / 1000);
   printf("Multi-core time  %"PRIu64" ms\n", (t_end[1]-t_start[1]) / 1000);
   printf("Relative node counts (multi/single): %.2f\n", (float)moves_searched[1] / moves_searched[0]);
   printf("Parallel speed-up (time-to-depth):   %.2f\n", (float)(t_end[0]-t_start[0]) / (t_end[1]-t_start[1]));

}
#endif

static void run_test_suite(uint64_t time, const char *tests[])
{
   movelist_t legal_moves;
   movelist_t best_moves;
   int n = 0;
   int position_searched = 0;
   int position_correct = 0;
   int score = 0;
   int max_positions = 0;
   while (tests[max_positions]) max_positions++;
   while (tests[n]) {
      const char *best_move_string = strstr(tests[n], "bm ");
      if (!best_move_string) { n++; continue; }
      best_move_string += 3;

      position_searched++;
      printf("(%d/%d) %s:\n", position_searched, max_positions, tests[n]);

      game_t *game = create_variant_game("chess");
      game->start_new_game();

      game->setup_fen_position(tests[n]);

      game->generate_legal_moves(&legal_moves);
      best_moves.clear();

      const char *move_score_string = strstr(tests[n], "c0");
      if (move_score_string == NULL || strstr(move_score_string, "=") == NULL) {
         const char *s = best_move_string;
         while (*s && *s != ';') {
            char move_str[32] = { 0 };
            char *p = move_str;
            
            while (*s && isspace(*s)) s++;

            while (*s && !isspace(*s) && *s != ';') {
               if (*s != '+') { *p = *s; p++; }
               s++;
            }

            move_t move = game->move_string_to_move(move_str, &legal_moves);

            if (move) {
               best_moves.push(move);
               best_moves.score[best_moves.num_moves-1] = 10;
            }
         }
      } else {
         const char *s = move_score_string + 4;
         while (*s && *s != ';') {
            char move_str[32] = { 0 };
            int score = 10;
            char *p = move_str;
            
            while (*s && isspace(*s)) s++;

            while (*s && !isspace(*s) && *s != '=') {
               if (*s != '+') { *p = *s; p++; }
               s++;
            }
            if (*s == '=') {
               s++;
               sscanf(s, "%d", &score);
               while (*s && !isspace(*s)) s++;
            }

            move_t move = game->move_string_to_move(move_str, &legal_moves);

            if (move) {
               best_moves.push(move);
               best_moves.score[best_moves.num_moves-1] = score;
            }
         }
      }

      set_time_per_move(&game->clock, (int)time);
      game->think(MAX_SEARCH_DEPTH);
      move_t move = game->get_last_move();
      //printf("%s\n", move_to_string(move));
      //best_moves.print();

      bool correct = false;
      for (int k=0; k<best_moves.num_moves; k++)
         if (move == best_moves.move[k]) {
            score += best_moves.score[k];
            position_correct++;
            correct = true;
            break;
         }
      printf("Position %d/%d: move %s (%s) (score %d, %d correct)\n\n", position_searched, max_positions, move_to_short_string(move, &legal_moves), correct? "correct" : "wrong", score, position_correct);

      delete game;
      n++;
   }

   printf("\n*** Finished test suite ***\n");
   printf("Score: %d / %d (%d/%d correct)\n", score, 10*max_positions, position_correct, max_positions);
}

static void print_help(const char *topic)
{
   if (!topic) topic = "help";
   while (isspace(*topic)) topic++;
   if(!strlen(topic)) topic = "help";

   int n = 0;
   if (streq(topic, "all")) {
   while (help_topic[n].topic) {
      if (help_topic[n].text) {
         const char *s = help_topic[n].cmd;
         if (!s) s = help_topic[n].topic;
         printf("%s\n%s\n", s, help_topic[n].text);
      }
      n++;
   }
      return;
   }
   while (help_topic[n].topic) {
      if (streq(help_topic[n].topic, topic)) {
         const char *s = help_topic[n].cmd;
         if (!s) s = help_topic[n].topic;
         printf("%s\n%s\n", s, help_topic[n].text);
         if (streq(topic, "help")) break;
         if (streq(topic, "skill")) {
            printf("  Skill levels:\n");
            for (int n=0; n<int(sizeof combo_skill/sizeof *combo_skill); n++)
               printf("   %s\n", combo_skill[n].label);
         }
         return;
      }
      n++;
   }

   printf("Help topics: \n");
   n = 0;
   while (help_topic[n].topic) {
      if (n) printf(", ");
      printf("%s", help_topic[n].topic);
      n++;
   }
   printf("\n");
}

static void report_game_status(game_t *game, play_state_t status, const char *input)
{
   if (!repetition_claim && status == SEARCH_GAME_ENDED_REPEAT) return;

   switch (status) {
      case SEARCH_OK:
         break;

      case SEARCH_GAME_ENDED_50_MOVE:
         log_xboard_output("1/2-1/2 {50-move rule}\n");
         break;

      case SEARCH_GAME_ENDED_REPEAT:
         if (game->rep_score == LEGALDRAW)
            log_xboard_output("1/2-1/2 {%d-fold repetition}\n", game->repeat_claim+1);
         else {
            side_t me = game->get_side_to_move();
            if (game->get_rules() & RF_USE_CHASERULE) {
               if (game->player_in_check(me)) {
                  if (game->rep_score < 0) {
                     if (game->get_side_to_move() == BLACK)
                        log_xboard_output("0-1 {White chases}\n");
                     else
                        log_xboard_output("1-0 {Black chases}\n");
                  } else {
                     if (game->get_side_to_move() == BLACK)
                        log_xboard_output("1-0 {White chases}\n");
                     else
                        log_xboard_output("0-1 {Black chases}\n");
                  }
               } else {
                  if (game->rep_score < 0) {
                     if (game->get_side_to_move() == WHITE)
                        log_xboard_output("0-1 {Black chases}\n");
                     else
                        log_xboard_output("1-0 {White chases}\n");
                  } else {
                     if (game->get_side_to_move() == WHITE)
                        log_xboard_output("1-0 {Black chases}\n");
                     else
                        log_xboard_output("0-1 {White chases}\n");
                  }
               }
            } else {
               if (game->rep_score < 0) {
                  if (game->get_side_to_move() == BLACK)
                     log_xboard_output("0-1 {White repeats}\n");
                  else
                     log_xboard_output("1-0 {Black repeats}\n");
               } else {
                  if (game->get_side_to_move() == BLACK)
                     log_xboard_output("1-0 {White repeats}\n");
                  else
                     log_xboard_output("0-1 {Black repeats}\n");
               }
            }
         }
         break;

      case SEARCH_GAME_ENDED_MATE:
         if (game->get_side_to_move() == WHITE)
            log_xboard_output("0-1 {Black mates}\n");
         else
            log_xboard_output("1-0 {White mates}\n");
         break;

      case SEARCH_GAME_ENDED_CHECK_COUNT:
         if (game->get_side_to_move() == WHITE)
            log_xboard_output("0-1 {Black checks %d times}\n", game->check_limit);
         else
            log_xboard_output("1-0 {White checks %d times}\n", game->check_limit);
         break;

      case SEARCH_GAME_ENDED_LOSEBARE:
         if (game->get_side_to_move() == WHITE)
            log_xboard_output("0-1 {Bare king}\n");
         else
            log_xboard_output("1-0 {Bare king}\n");
         break;

      case SEARCH_GAME_ENDED_WINBARE:
         if (game->get_side_to_move() == WHITE)
            log_xboard_output("1-0 {Bare king}\n");
         else
            log_xboard_output("0-1 {Bare king}\n");
         break;

      case SEARCH_GAME_ENDED_STALEMATE:
         if (game->stale_score == 0)
            log_xboard_output("1/2-1/2 {Stalemate}\n");
         else if (game->stale_score < 0) {
            if (game->get_side_to_move() == WHITE)
               log_xboard_output("0-1 {Black mates}\n");
            else
               log_xboard_output("1-0 {White mates}\n");
         } else {
            if (game->get_side_to_move() == WHITE)
               log_xboard_output("1-0 {Stalemate}\n");
            else
               log_xboard_output("0-1 {Stalemate}\n");
         }
         break;

      case SEARCH_GAME_ENDED_INSUFFICIENT:
         log_xboard_output("1/2-1/2 {insufficient material}\n");
         break;

      case SEARCH_GAME_ENDED_FORFEIT:
         log_xboard_output("Illegal move (may not be used to give mate): %s\n", input);
         game->takeback();
         //if (game->get_side_to_move() == WHITE)
         //   log_xboard_output("1-0 {Black forfeits}\n");
         //else
         //   log_xboard_output("0-1 {White forfeits}\n");
         break;

      case SEARCH_GAME_ENDED_INADEQUATEMATE:
         log_xboard_output("1/2-1/2 {Mate, but no shak}\n");
         break;

      case SEARCH_GAME_ENDED_FLAG_CAPTURED:
         if (game->flag_score == 0)
            log_xboard_output("1/2-1/2 {Flag captured}\n");
         else if (game->flag_score < 0) {
            if (game->side_captured_flag(BLACK))
               log_xboard_output("0-1 {Black captures the flag}\n");
            else
               log_xboard_output("1-0 {White captures the flag}\n");
         } else {
            if (game->side_captured_flag(BLACK))
               log_xboard_output("1-0 {Black captures the flag}\n");
            else
               log_xboard_output("0-1 {White captures the flag}\n");
         }
         break;

      case SEARCH_GAME_ENDED_NOPIECES:
         if (game->no_piece_score == 0) {
            log_xboard_output("1/2-1/2 {No pieces remaining}\n");
         } else if (game->no_piece_score < 0) {
            if (game->get_side_to_move() == WHITE)
               log_xboard_output("0-1 {No white pieces remaining}\n");
            else
               log_xboard_output("1-0 {No black pieces remaining}\n");
         } else {
            if (game->get_side_to_move() == WHITE)
               log_xboard_output("1-0 {No white pieces remaining}\n");
            else
               log_xboard_output("0-1 {No black pieces remaining}\n");
         }
         break;

      case SEARCH_GAME_ENDED:
         log_xboard_output("telluser game ended (unknown reason)\n");
         break;
   }
}

static void read_config_file(FILE *cf)
{
   static char buf[65536];

   while (!feof(cf)) {
      if (fgets(buf, sizeof buf, cf) == 0)
         continue;

      char *eol = strstr(buf, "\n");
      if (eol) *eol = 0;

      if (strstr(buf, "user_alias") == buf) {
         char *s = strchr(buf, '=');
         if (s) {
            s++;
            while (*s && isspace(*s)) s++;
            if (*s) {
               free(user_alias);
               user_alias = strdup(s);
            }
         }
      } else if (strstr(buf, "fairy_file") == buf) {
         char *s = strchr(buf, '=');
         if (s) {
            s++;
            while (*s && isspace(*s)) s++;
            if (*s) {
               free(fairy_file);
               fairy_file = strdup(s);
            }
         }
      } else if (strstr(buf, "eval_file") == buf) {
         char *s = strchr(buf, '=');
         if (s) {
            s++;
            while (*s && isspace(*s)) s++;
            if (*s) {
               free(eval_file);
               eval_file = strdup(s);
            }
         }
      } else if (strstr(buf, "option_mate_search") == buf) {
         char *s = strchr(buf, '=');
         if (s) {
            int i = 1;
            s++;
            sscanf(s, "%d", &i);
            if (i < MATE_SEARCH_DISABLED) i = MATE_SEARCH_DISABLED;
            if (i > MATE_SEARCH_ENABLED) i = MATE_SEARCH_ENABLED;
            option_ms = i;
         }
      } else if (strstr(buf, "skill_level") == buf) {
         char *s = strchr(buf, '=');
         if (s) {
            int i = 1;
            s++;
            sscanf(s, "%d", &i);
            if (i < LEVEL_RANDOM) i = LEVEL_RANDOM;
            if (i >= LEVEL_NUM_LEVELS) i = LEVEL_NORMAL;
            skill_level = (level_t)i;
         }
      } else if (strstr(buf, "draw_count") == buf) {
         char *s = strchr(buf, '=');
         if (s) {
            int i = 0;
            s++;
            sscanf(s, "%d", &i);
            draw_count = i;
         }
      } else if (strstr(buf, "resign_count") == buf) {
         char *s = strchr(buf, '=');
         if (s) {
            int i = 0;
            s++;
            sscanf(s, "%d", &i);
            resign_count = i;
         }
      } else if (strstr(buf, "random_ply_count") == buf) {
         char *s = strchr(buf, '=');
         if (s) {
            int i = 0;
            s++;
            sscanf(s, "%d", &i);
            random_ply_count = i;
         }
      } else if (strstr(buf, "random_amplitude") == buf) {
         char *s = strchr(buf, '=');
         if (s) {
            int i = 0;
            s++;
            sscanf(s, "%d", &i);
            random_amplitude = i;
         }
      } else if (strstr(buf, "multipv") == buf) {
         char *s = strchr(buf, '=');
         if (s) {
            int i = 0;
            s++;
            sscanf(s, "%d", &i);
            multipv = abs(i);
            if (multipv < 1) multipv = 1;
         }
      } else if (strstr(buf, "draw_threshold") == buf) {
         char *s = strchr(buf, '=');
         if (s) {
            int i = 0;
            s++;
            sscanf(s, "%d", &i);
            draw_threshold = abs(i);
         }
      } else if (strstr(buf, "resign_threshold") == buf) {
         char *s = strchr(buf, '=');
         if (s) {
            int i = LEGALWIN;
            s++;
            sscanf(s, "%d", &i);
            resign_threshold = -abs(i);
         }
      } else {
         for (int n = 0; boolean_settings[n].name; n++) {
            if (strstr(buf, boolean_settings[n].name) == buf) {
               char *s = strchr(buf, '=');
               if (s) {
                  int i = boolean_settings[n].def;
                  s++;
                  sscanf(s, "%d", &i);
                  *boolean_settings[n].opt = i != 0;
               }
            }
         }
      }
   }
}

static void write_config_file(void)
{
   FILE *cf = fopen(configfile, "w");

   if (!cf) return;

   if (user_alias && strlen(user_alias))
      fprintf(cf, "user_alias = %s\n", user_alias);

   if (fairy_file && strlen(fairy_file))
      fprintf(cf, "fairy_file = %s\n", fairy_file);

   if (eval_file && strlen(eval_file) && remember_eval_file)
      fprintf(cf, "eval_file = %s\n", eval_file);

   if (option_ms != MATE_SEARCH_ENABLE_DROP)
      fprintf(cf, "option_mate_search = %d\n", option_ms);

   if (skill_level != LEVEL_NORMAL)
      fprintf(cf, "skill_level = %d\n", skill_level);

   for (int n = 0; boolean_settings[n].name; n++) {
      if (*boolean_settings[n].opt != boolean_settings[n].def)
         fprintf(cf, "%s = %d\n", boolean_settings[n].name, *boolean_settings[n].opt);
   }

   fprintf(cf, "multipv = %d\n", multipv);
   fprintf(cf, "draw_count = %d\n", draw_count);
   fprintf(cf, "draw_threshold = %d\n", draw_threshold);
   fprintf(cf, "resign_count = %d\n", resign_count);
   fprintf(cf, "resign_threshold = %d\n", resign_threshold);
   fprintf(cf, "random_ply_count = %d\n", random_ply_count);
   fprintf(cf, "random_amplitude = %d\n", random_amplitude);

   fclose(cf);
}

#ifdef HAVE_READLINE

/* Generator function for command completion.  STATE lets us know whether
 * to start from scratch; without any state (i.e. STATE == 0), then we
 * start at the top of the list.
 */
char *command_generator (const char *text, int state)
{
   static int list_index, len, ind;
   const char *name = NULL;

   /* If this is a new word to complete, initialize now.  This includes
      saving the length of TEXT for efficiency, and initializing the index
      variable to 0. */
   if (!state) {
      list_index = 0;
      ind = 0;
      len = strlen (text);
   }

   /* Return the next name which partially matches from the command list. */
   while ((name = help_topic[list_index].topic)) {
      list_index++;

      if (strncmp (name, text, len) == 0)
         return strdup(name);
   }

   while ((name = xcmd_list[ind])) {
      ind++;

      if (strncmp (name, text, len) == 0)
         return strdup(name);
   }

   /* If no names matched, then return NULL. */
   return (char *)NULL;
}

/* Generator function for command completion.  STATE lets us know whether
 * to start from scratch; without any state (i.e. STATE == 0), then we
 * start at the top of the list.
 */
char *variant_generator (const char *text, int state)
{
   static int list_index, len;
   const char *name = NULL;

   /* If this is a new word to complete, initialize now.  This includes
      saving the length of TEXT for efficiency, and initializing the index
      variable to 0. */
   if (!state) {
      list_index = 0;
      len = strlen (text);
   }

   /* Return the next name which partially matches from the command list. */
   while (list_index < num_standard_variants && (name = standard_variants[list_index].name)) {
      list_index++;

      if (strncmp (name, text, len) == 0)
         return strdup(name);
   }

   if (list_index >= num_standard_variants) {
      int n_alias = sizeof aliases / sizeof *aliases;
      int index = list_index - num_standard_variants;
      while (index < n_alias && (name = aliases[index].alias)) {
         list_index++;
         index++;

         if (strncmp (name, text, len) == 0)
            return strdup(name);
      }

      if (index >= n_alias) {
         index -= n_alias;
         while (index<num_custom_variants) {
            name = custom_variants[index].shortname;
            list_index++;
            index++;

            if (strncmp (name, text, len) == 0)
               return strdup(name);
         }
      }
   }

   /* If no names matched, then return NULL. */
   return (char *)NULL;
}

char *unload_generator (const char *text, int state)
{
   static int list_index, len;
   const char *name = NULL;

   /* If this is a new word to complete, initialize now.  This includes
      saving the length of TEXT for efficiency, and initializing the index
      variable to 0. */
   if (!state) {
      list_index = 0;
      len = strlen (text);
   }

   /* Return the next name which partially matches from the command list. */
   while (list_index < num_custom_variants && (name = custom_variants[list_index].filename)) {
      list_index++;

      if (strncmp (name, text, len) == 0)
         return strdup(name);
   }

   /* If no names matched, then return NULL. */
   return (char *)NULL;
}

static char **complete_sjaakii_command(const char *text, int start, int /* end */)
{
   char **matches = (char **)NULL;
   char *rlline=(char *)rl_line_buffer;

   /* If this word is at the start of the line, then it is a command
    * to complete.  Otherwise it may be the name of a variant to play.
    */
   if (start == 0) {
      matches = rl_completion_matches(text, command_generator);
   } else if (strstr(rlline, "variant ") == rlline) {
      matches = rl_completion_matches(text, variant_generator);
   } else if (strstr(rlline, "unload ") == rlline) {
      matches = rl_completion_matches(text, unload_generator);
   }

   return matches;
}

#endif

static const char *filter_dark_squares_fen(const char *fen)
{
   static char buffer[1024];
   char *s = buffer;
   buffer[0] = 0;

   int len = 0;
   int count = 0;
   for (const char *p = fen; *p; p++) {
      if (*p == '*') {
         count++;
         continue;
      }
      if (isdigit(*p)) {
         int c = 0;
         int k = 0;
         sscanf(p, "%d %n", &c, &k);
         p+=k-1;
         count+=c;
         continue;
      }
      if (count) {
         int l = snprintf(s, sizeof(buffer) - len, "%d", count);
         s   += l;
         len += l;
         count = 0;
      }
      *s = *p;
      s++;
      *s = 0;
      len++;
   }

   return buffer;
}

static void send_fairy_menu_to_xboard(void)
{
   int num_aliases = sizeof aliases / sizeof *aliases;

   /* List options as "fairy" variants, for older XBoard */
   log_xboard_output("feature option=\"Variant fairy selects -combo ");
   for (int n=0; n<num_standard_variants; n++) {
      if (n) log_xboard_output(" /// ");
      log_xboard_output("%s (%dx%d+%x)", standard_variants[n].name, standard_variants[n].files, standard_variants[n].ranks, standard_variants[n].holdings);
   }
   for (int n = 0; n<num_custom_variants; n++)
      log_xboard_output(" /// %s (%dx%d)", custom_variants[n].shortname, custom_variants[n].files, custom_variants[n].ranks);
   num_aliases = sizeof aliases / sizeof *aliases;
   for (int n = 0; n<num_aliases-1; n++) {
      if (streq(aliases[n].alias, "normal")) continue;
      log_xboard_output(" /// %s (= %s)", aliases[n].alias, aliases[n].name);
   }
   log_xboard_output("\"\n");
}

static void send_variants_to_xboard(void)
{
         log_xboard_output("feature setboard=1"
//                                       " smp=1"
                                      " time=1"
                                    " sigint=0"
                                    " colors=0"
                                 " highlight=1"
                                      " ping=1"
                                    " memory=1"
                                   " analyze=1"
                                     " pause=1"
                                       " nps=1"
                                      " sjef=1"
                                    " myname=\"%s %s\""
                                 " myversion=\"[%s]\"", PROGNAME, VERSIONSTR, VERSIONSTR " " ARCHSTR);
   log_xboard_output(" variants=\"");

   if (user_variants_first) {
      for (int n = 0; n<num_custom_variants; n++)
         log_xboard_output("%s,", custom_variants[n].shortname);
      if (user_alias && strstr(user_alias, "=")) {
         char *p = strstr(user_alias, "=")+1;
         log_xboard_output("%s,", p);
         p[-1] = 0;
         log_xboard_output("%s,", user_alias);
         p[-1] = '=';
      }
      for (int n = 0; n<num_standard_variants; n++) {
         if (streq(standard_variants[n].name, "chess")) continue;
         log_xboard_output("%s,", standard_variants[n].name);
      }
   } else {
      for (int n = 0; n<num_standard_variants; n++) {
         if (streq(standard_variants[n].name, "chess")) continue;
         log_xboard_output("%s,", standard_variants[n].name);
      }
      if (user_alias && strstr(user_alias, "=")) {
         char *p = strstr(user_alias, "=")+1;
         log_xboard_output("%s,", p);
      }
      for (int n = 0; n<num_custom_variants; n++)
         log_xboard_output("%s,", custom_variants[n].shortname);
   }
   int num_aliases = sizeof aliases / sizeof *aliases;
   for (int n = 0; n<num_aliases; n++) {
      if (streq(aliases[n].alias, "normal")) continue;
      log_xboard_output("%s,", aliases[n].alias);
   }
   /* Variant fairy on different board sizes */
   char fairy_size_string[1024] = { 0 };
   for (int n = 0; n<num_standard_variants; n++) {
      char s[32];
      snprintf(s, sizeof s, "%dx%d+%d", standard_variants[n].files, standard_variants[n].ranks, standard_variants[n].holdings);
      if (!strstr(fairy_size_string, s)) {
         size_t l = strlen(fairy_size_string);
         snprintf(fairy_size_string + l, sizeof fairy_size_string - l, " %s ", s);
         log_xboard_output("%s_fairy,", s);
      }
   }
   log_xboard_output("normal\"");
   log_xboard_output("\n");
   //log_xboard_output("feature option=\"Opening book (polyglot) -file %s\"\n", pgbook_file?pgbook_file:"");
}

void send_options_to_xboard(void)
{
   send_variants_to_xboard();
   send_fairy_menu_to_xboard();
   log_xboard_output("feature option=\"Mate search -combo %sDisabled /// %sEnabled for drop games /// %sEnabled\"\n",
      (option_ms == MATE_SEARCH_DISABLED)    ? "*" : "",
      (option_ms == MATE_SEARCH_ENABLE_DROP) ? "*" : "",
      (option_ms == MATE_SEARCH_ENABLED)     ? "*" : "");

   log_xboard_output("feature option=\"Level -combo");
   for (int n=0; n<int(sizeof combo_skill/sizeof *combo_skill); n++) {
      log_xboard_output(" %s%s%s", n?"/// ":"",
                                   (combo_skill[n].var[0] == combo_skill[n].value) ? "*" : "",
                                   combo_skill[n].label);
   }
   log_xboard_output("\"\n");

   log_xboard_output("feature option=\"MultiPV -spin %d 1 256\"\n", multipv);
   log_xboard_output("feature option=\"Draw offer threshold -spin %d 0 1000\"\n", draw_threshold);
   log_xboard_output("feature option=\"Moves before draw offer (0 to disable) -spin %d 0 1000\"\n", draw_count);
   log_xboard_output("feature option=\"Resign threshold -spin %d 100 %d\"\n", abs(resign_threshold), LEGALWIN);
   log_xboard_output("feature option=\"Moves before resigning (0 to disable) -spin %d 0 1000\"\n", resign_count);
   log_xboard_output("feature option=\"Randomise opening moves -spin %d 0 40\"\n", random_ply_count);
   log_xboard_output("feature option=\"Random amplitude (0 to disable) -spin %d 0 100\"\n", random_amplitude);

   for (int n = 0; boolean_settings[n].label; n++) {
      bool val = *boolean_settings[n].opt;
      if (boolean_settings[n].neg) val = !val;
      log_xboard_output("feature option=\"%s -check %d\"\n", boolean_settings[n].label, val);
   }

   log_xboard_output("feature option=\"Variant 'normal' is -string %s\"\n", normal_alias);
   log_xboard_output("feature option=\"Set variant alias -string %s\"\n", user_alias?user_alias:"");
   log_xboard_output("feature option=\"Variant configuration file -file %s\"\n", fairy_file?fairy_file:"");
   log_xboard_output("feature option=\"Evaluation parameter file -file %s\"\n", eval_file?eval_file:"");
}


static void load_evaluation_parameters(game_t *game, const char *fname)
{
   if (game && fname) {
      FILE *inf = fopen(fname, "r");
      if (inf) {
         game->load_eval_parameters(inf);
         fclose(inf);
      }
   }
}


int main(int argc, char **argv)
{
   bool input_interrupt_search = true;
   bool xboard_mode = false;
   bool uci_mode = false;
   bool uci_kxr = true;
   char uci_dialect = 'c';
   int  uci_timeunit = 1;
   side_t my_colour = BLACK;
   game_t *game = NULL;
   char input[65536];
   char prompt_str[256];
   int depth = MAX_SEARCH_DEPTH;
   size_t hash_size = HASH_TABLE_SIZE;
   char *pgbook_file = NULL;
   int time_per_move = 0;
   int nps = -1;

   get_user_config_file(configfile, sizeof configfile, "sjaakii");

#if defined SMP && defined _WIN32
   pthread_win32_process_attach_np();
   pthread_win32_thread_attach_np();
#endif

   snprintf(normal_alias, sizeof(normal_alias), "chess");
   bool load_variant_file = true;
   for (int n = 1; n<argc; n++) {
      if (strstr(argv[n], "-book")) {
         if (n+1 >= argc) {
            fprintf(stderr, "error: no book specified\n");
            exit(0);
         }
         n++;
         pgbook_file = strdup(argv[n]);
         printf("Using opening book %s\n", pgbook_file);
         //pgbook_file = strdup("bigbook.bin");
      } else if (strstr(argv[n], "-eval")) {
         if (n+1 >= argc) {
            fprintf(stderr, "error: no evaluation parameter file specified\n");
            exit(0);
         }
         n++;
         eval_file = strdup(argv[n]);
         printf("Using eval parameters from %s\n", eval_file);
      } else if (strstr(argv[n], "-log") || strstr(argv[n], "-newlog")) {
         const char *logfile = "sjaak.log";
         if (n+1 < argc && argv[n+1][0] != '-') {
            logfile = argv[n+1];
            n++;
         }
         const char *mode = "a";
         if (strstr(argv[n], "-newlog")) mode = "w";
         if (!f) f = fopen(logfile, mode);
      } else if (strstr(argv[n], "-xboard")) {
         xboard_mode = true;
         uci_mode = false;
      } else if (strstr(argv[n], "-uci")) {
         xboard_mode = false;
         uci_mode = true;
         uci_dialect = 'c';
         free((void *)variant_name);
         variant_name = strdup("chess");
      } else if (strstr(argv[n], "-usi")) {
         xboard_mode = false;
         uci_mode = true;
         uci_dialect = 's';
         free((void *)variant_name);
         variant_name = strdup("shogi");
      } else if (strstr(argv[n], "-ucci")) {
         xboard_mode = false;
         uci_mode = true;
         uci_dialect = 'C';
         free((void *)variant_name);
         variant_name = strdup("xiangqi");
      } else if (strstr(argv[n], "-no_user_variants")) {
         load_variant_file = false;
      } else if (strstr(argv[n], "-variant")) {
         if (n+1 < argc) {
            variant_name = strdup(argv[n+1]);
            n++;
         }
      } else if (strstr(argv[n], "-normal")) {
         if (n+1 < argc) {
            snprintf(normal_alias, sizeof(normal_alias), "%s", argv[n+1]);
            n++;
         }
      } else if (strstr(argv[n], "-") == argv[n]) {
         fprintf(stderr, "Unknown option: %s\n", argv[n]);
         exit(0);
      } else {
         scan_variant_file(argv[n]);
      }
   }

#ifdef DATADIR
   if (load_variant_file)
      scan_variant_file(DATADIR"/variants.txt");
#endif

   buf = (char *)malloc(65536);

   printf("%s version %s\n", PROGNAME, VERSIONSTR " " ARCHSTR);
   printf("Type 'help' for a list of commands and help topics\n");
   initialise_hash_keys();

#ifdef SMP
   int ncore = get_number_of_cores();
   printf("Machine has %d core%s\n", ncore, (ncore > 1)? "s" : "");
   atexit(kill_threads);

   printf("Locks are implemented as %s\n", LOCK_DESCRIPTION);
#endif

#ifdef HAVE_READLINE
   if (stdin_is_terminal()) {
      rl_initialize();
      //rl_read_init_file(readline_path);
      using_history();
      stifle_history(256);
      //read_history(history_path);
      //atexit(write_history_file_on_exit);

      rl_attempted_completion_function = complete_sjaakii_command;
   }
#endif

   /* Turn off buffering for stdout and stdin */
   setvbuf(stdout, NULL, _IONBF, 0);
   setvbuf(stdin, NULL, _IONBF, 0);

   if (xboard_mode) { free((void *)variant_name); variant_name = NULL; }
   snprintf(fairy_alias, sizeof fairy_alias, "chess");
   if (!variant_name) variant_name = strdup("chess");

   /* Read persistent settings */
   FILE *cf = fopen(configfile, "r");
   if (cf) {
      if (f) fprintf(f, "Reading config file '%s'\n", configfile);
      read_config_file(cf);
      fclose(cf);
   } else {
      if (f) fprintf(f, "Cannot read config file '%s'\n", configfile);
   }
   atexit(write_config_file);

   /* Load variant description file */
   if (fairy_file)
      scan_variant_file(fairy_file);

   game = create_variant_game(variant_name);
   if (game == NULL) {
      printf("Failed to start variant '%s', defaulting to 'normal'\n", variant_name);
      free((void *)variant_name);
      variant_name = strdup("normal");
      game = create_variant_game(variant_name);
   }
   game->start_new_game();
   load_evaluation_parameters(game, eval_file);

   game->set_default_output_function(log_xboard_output);
   game->set_xboard_output_function(NULL);
   game->set_uci_output_function(NULL);

   if (xboard_mode) {
      uci_mode = false;
      prompt = false;
      show_board = false;
      san = false;
      trapint = false;
      if (game && xboard_mode) rank_offset = (game->ranks != 10);
      relabel_chess_square_names();
      game->set_xboard_output_function(log_xboard_output);
      game->set_uci_output_function(NULL);
      game->set_default_output_function(NULL);
      log_xboard_output("\n");
   }

   if (uci_mode) {
      prompt = false;
      show_board = false;
      san = false;
      trapint = false;
      in_play = false;
      rank_offset = 1;

      uci_timeunit = 1;
      if (uci_dialect == 'C')
         uci_timeunit = 1000;

      if (game)
         delete game;
      game = create_variant_game(variant_name);
      game->set_transposition_table_size(hash_size);
      game->start_new_game();
      load_evaluation_parameters(game, eval_file);
      if (uci_dialect == 's') relabel_shogi_square_names();

      game->set_xboard_output_function(NULL);
      game->set_default_output_function(NULL);
      game->set_uci_output_function(log_xboard_output);
   }

   while (true) {
      input[0] = '\0';
      prompt_str[0] = '\0';
      if (show_board && game)
         game->print_board();
      if (prompt) {
         int n = 0;
         n += snprintf(prompt_str+n, (sizeof prompt_str) - n, "#");
         if (game) {
            n += snprintf(prompt_str+n, (sizeof prompt_str) - n, "[%s] ", game->get_name());
            n += snprintf(prompt_str+n, (sizeof prompt_str) - n, "%d", (int)game->get_moves_played());
            n += snprintf(prompt_str+n, (sizeof prompt_str) - n, "%s", (game->get_side_to_move() == WHITE)?"w":"b");
            if (!in_play) n += snprintf(prompt_str+n, (sizeof prompt_str) - n, " (f)");
            if (game->player_in_check(game->get_side_to_move())) n += snprintf(prompt_str+n, (sizeof prompt_str) - n, "+");
         } else {
            n += snprintf(prompt_str+n, (sizeof prompt_str) - n, "[-]");
         }
         n += snprintf(prompt_str+n, (sizeof prompt_str) - n, ">");

#ifdef HAVE_READLINE
         if (!stdin_is_terminal())
#endif
         printf("%s", prompt_str);
      }
      //if (!(may_ponder && game && game->ponder_move)|| keyboard_input_waiting()) {
      if (deferred[0]) {
         snprintf(input, sizeof input, "%s", deferred);
         deferred[0] = 0;
      } else if (keyboard_input_waiting() || (!may_ponder && !(game && game->analysing))) {
#ifdef HAVE_READLINE
         if (stdin_is_terminal()) {
            char *readline_input = readline(prompt_str);
            if (readline_input && strlen(readline_input))
               add_history(readline_input);
            snprintf(input, sizeof input, "%s", readline_input);
            free(readline_input);
         } else
#endif
         if (!fgets(input, sizeof input, stdin))
            break;
      }
      chomp(input);
      trim(input);
      /* Strip trailing whitespace */
      char *e = input + strlen(input)-1;
      while (e > input && isspace(*e)) {
         *(e--) = '\0';
      }
#ifdef HAVE_READLINE
      if (!stdin_is_terminal()) {
         char *s = strchr(input, '#');
         if (s) *s = '\0';
      }
#endif
      if (f) {
         fprintf(f, "< %s\n", input);
         fflush(f);
      }
      if (streq(input, "xboard") || streq(input, "xboard on")) {
         uci_mode = false;
         xboard_mode = true;
         prompt = false;
         show_board = false;
         san = false;
         trapint = false;
         if (game && xboard_mode) rank_offset = (game->ranks != 10);
         relabel_chess_square_names();
         game->set_xboard_output_function(log_xboard_output);
         game->set_uci_output_function(NULL);
         game->set_default_output_function(NULL);
         log_xboard_output("\n");
      } else if (streq(input, "xboard off")) {
         xboard_mode = false;
         prompt = true;
         show_board = true;
         san = true;
         trapint = true;
         rank_offset = 1;
         relabel_chess_square_names();
         log_xboard_output("\n");
         game->set_xboard_output_function(NULL);
         game->set_default_output_function(log_xboard_output);
      } else if (streq(input, "interrupt off")) {
         input_interrupt_search = false;
      } else if (streq(input, "interrupt on")) {
         input_interrupt_search = true;
      } else if (streq(input, "uci") || streq(input, "uci on") || streq(input, "ucci") || streq(input, "ucci on") || streq(input, "usi") || streq(input, "usi on")) {
         uci_mode = true;
         uci_dialect = input[1];
         prompt = false;
         show_board = false;
         san = false;
         trapint = false;
         in_play = false;
         rank_offset = 1;
         if (input[2] == 'c')
            uci_dialect = 'C';

         uci_timeunit = 1;
         if (uci_dialect == 'C')
            uci_timeunit = 1000;

         free((void *)variant_name);
         switch (uci_dialect) {
            case 's':
               variant_name = strdup("shogi");
               break;
            case 'C':
               variant_name = strdup("xiangqi");
               break;
            default:
               variant_name = strdup("chess");
         }
         if (game)
            delete game;
         game = create_variant_game(variant_name);
         game->set_transposition_table_size(hash_size);
         game->start_new_game();
         load_evaluation_parameters(game, eval_file);
         if (input[1] == 's') relabel_shogi_square_names();

         game->set_xboard_output_function(NULL);
         game->set_default_output_function(NULL);
         game->set_uci_output_function(log_xboard_output);

         log_xboard_output("\n");
         log_xboard_output("id name %s %s %s\n", PROGNAME, VERSIONSTR, ARCHSTR);
         log_xboard_output("id author Evert Glebbeek\n");
         //log_xboard_output("option name OwnBook type check default true\n");
         /* Hash size: 1MB - 1 GB */
         const char *option_name = " name";
         if (uci_dialect == 'C') option_name = "";
         log_xboard_output("option%s Hash type spin default 48 min 1 max 4096\n", option_name);
#ifdef SMP
         log_xboard_output("option%s Cores type spin default 1 min 1 max %d\n", option_name, MAX_THREADS);
#endif
         log_xboard_output("option%s Ponder type check default true\n", option_name);
         log_xboard_output("option%s UCI_Variant type combo default %s", option_name, variant_name);
         log_xboard_output(" var %s var chess960", standard_variants[0].name);
         for (int n = 1; n<num_standard_variants; n++) {
            log_xboard_output(" var %s", standard_variants[n].name);
         }
         log_xboard_output("\n");
         if (uci_dialect == 'c')
            log_xboard_output("option name UCI_Chess960 type check default true\n");
         if (uci_dialect == 'C')
            log_xboard_output("option usemillisec type check default true\nucciok\n");
         else
            log_xboard_output("u%ciok\n", uci_dialect);
      } else if (streq(input, "uci off") || streq(input, "ucci off") || streq(input, "usi off")) {
         uci_mode = true;
         game->set_default_output_function(log_xboard_output);
         game->set_uci_output_function(NULL);
         relabel_chess_square_names();
      } else if (streq(input, "isready") && uci_mode) {
         log_xboard_output("readyok\n");
      } else if ((strstr(input, "setoption usemillisec") == input) && uci_mode) {
         uci_timeunit = 1000;
         if (strstr(input, "true")) uci_timeunit = 1;
      } else if ((strstr(input, "setoption name Hash") == input || strstr(input, "setoption Hash") == input) && uci_mode) {
         /* TODO: set the size of the hash table */
         unsigned long long memory_size = hash_size * sizeof(hash_table_entry_t);
         char *s = strstr(input, "value");
         if (s) {
            sscanf(s+6, "%llu", &memory_size);

            /* Convert to bytes */
            memory_size <<= 20;

            /* Reserve default */
            if (memory_size > 10<<20)
               memory_size -= 5<<20;
         } else {
            sscanf(input+13, "%llu", &memory_size);

            /* Convert to bytes */
            memory_size <<= 20;

            /* Reserve default */
            if (memory_size > 10<<20)
               memory_size -= 5<<20;
         }
         hash_size = size_t(memory_size / sizeof(hash_table_entry_t));
         game->set_transposition_table_size(hash_size);
      } else if ((strstr(input, "setoption name UCI_Variant") == input || strstr(input, "setoption UCI_Variant") == input) && uci_mode) {
         char *s = strstr(input, "value");
         if (s) {
            s += 6;
            while (*s && isspace(*s)) s++;
            if (s[0]) {
               /* Trim trailing spaces */
               char *p = s + strlen(s)-1;
               while (*p == ' ') {*p = 0; p--; }
               free((void *)variant_name);
               variant_name = strdup(s);

               if (game)
                  delete game;
               game = create_variant_game(variant_name);
               game->set_transposition_table_size(hash_size);
               game->start_new_game();
               load_evaluation_parameters(game, eval_file);

               if (uci_dialect == 's') relabel_shogi_square_names();
               uci_kxr = false;
               if (streq(variant_name, "chess960")) uci_kxr = true;
            }
         }  else {
            /* TODO: select variants in ucci mode */
         }
      } else if (strstr(input, "setoption name UCI_Chess960") == input && uci_mode) {
         char *s = strstr(input, "value");
         if (s) {
            s += 6;
            uci_kxr = false;
            if (strstr(s, "true") == s) uci_kxr = true;
         }
      } else if (strstr(input, "setoption name Ponder") == input && uci_mode) {
#ifdef SMP
      } else if (strstr(input, "setoption name Cores") == input && uci_mode) {
         int threads = 0;
         char *s = strstr(input, "value");
         if (s) {
            s += 6;
            sscanf(s, "%d", &threads);
            kill_threads();
            if (threads > 0)
               init_threads(threads);
            if (show_board) printf("Started %d threads\n", get_number_of_threads());
         }
#endif
      } else if ((streq(input, "ucinewgame") || streq(input, "uccinewgame") || streq(input, "usinewgame")) && uci_mode) {
         if (game)
            delete game;
         game = create_variant_game(variant_name);
         game->set_transposition_table_size(hash_size);
         game->start_new_game();
         load_evaluation_parameters(game, eval_file);
         if (uci_dialect == 's') relabel_shogi_square_names();
         num_games++;
         if (f) fprintf(f, "# game %d\n", num_games);
      } else if (strstr(input, "banmoves") && uci_mode) {
      } else if (strstr(input, "position") && uci_mode) {
         char *p = strstr(input, " ");
         while (p && *p && p[0]==' ') p++;

         assert(game);

         if (strstr(p, "startpos") == p) {   /* Game from start position */
            /* We need to return to the first node in the game struct and
             * then replay all the moves from the beginning to the current
             * position.
             */
            p = strstr(p, "moves");
            if (p && *p) p+=6;
            replay_game(game, p);
         } else if (strstr(p, "fen") == p) { /* Game from FEN string */
            p+=4;
            game->setup_fen_position(p);

            p = strstr(p, "moves");
            if (p && *p) p+=6;
            replay_game(game, p);
         } else if (strstr(p, "sfen") == p) { /* Game from sFEN string */
            /* Translate SFEN to FEN */
            char fen[256] = { 0 };
            int n = 0;
            p += 5;

            /* Skip space */
            while (*p && isspace(*p)) p++;

            /* Copy position verbatim */
            while (*p && !isspace(*p)) {
               fen[n++] = *p;
               p++;
            }

            while (*p && isspace(*p)) p++;
            char side = *p;
            while (*p && !isspace(*p)) p++;
            while (*p && isspace(*p)) p++;

            /* Flip white and black in side to move */
            side = (side == 'w') ? 'b' : 'w';

            /* Copy pieces in holdings */
            n += snprintf(fen+n, (sizeof fen) - n, " [");
            if (*p != '-') {
               while (*p && !isspace(*p)) {
                  int c = 1;
                  if (isdigit(*p)) { sscanf(p, "%d", &c); p++; }
                  while (c>0) {
                     fen[n++] = *p;
                     c--;
                  }
                  p++;
               }
            }
            n += snprintf(fen+n, (sizeof fen) - n, "] %c", side);

            int ply = 1;
            sscanf(p, "%d", &ply);
            n += snprintf(fen+n, (sizeof fen) - n, " 0 %d", (ply+1)/2);

            game->setup_fen_position(fen);

            p = strstr(p, "moves");
            if (p && *p) p+=6;
            replay_game(game, p);
         }
      } else if (streq(input, "stop") && uci_mode) {
      } else if (strstr(input, "go") == input && uci_mode) {
         const char *p = "";
         int depth = MAX_SEARCH_DEPTH;
         char *s;

         /* Set defaults */
         set_time_per_move(&game->clock, 5000);
         game->clock.movestogo = 0;
         game->clock.time_inc = 0;

         /* parse options */
         if ((s = strstr(input, "movetime"))) {
            sscanf(s+9, "%d", &time_per_move);
            set_time_per_move(&game->clock, time_per_move * uci_timeunit);
         }
         if ((s = strstr(input, " movestogo"))) {
            sscanf(s+10, "%d", &game->clock.movestogo);
         }
         if ((s = strstr(input, "infinite"))) {
            set_infinite_time(&game->clock);
         }
         if ((s = strstr(input, " time"))) {
            int time_left;
            sscanf(s+5, "%d", &time_left);
            game->clock.time_left = time_left * uci_timeunit;
            set_time_for_game(&game->clock);
         }
         if ((s = strstr(input, "wtime"))) {
            int time_left;
            sscanf(s+6, "%d", &time_left);
            if (my_colour == WHITE || (uci_dialect == 's' && my_colour == BLACK)) {
               game->clock.time_left = time_left * uci_timeunit;
               set_time_for_game(&game->clock);
            }
         }
         if ((s = strstr(input, "btime"))) {
            int time_left;
            sscanf(s+6, "%d", &time_left);
            if (my_colour == BLACK || (uci_dialect == 's' && my_colour == WHITE)) {
               game->clock.time_left = time_left * uci_timeunit;
               set_time_for_game(&game->clock);
            }
         }
         if ((s = strstr(input, " increment"))) {
            int inc;
            sscanf(s+5, "%d", &inc);
            game->clock.time_inc = inc * uci_timeunit;
         }
         if ((s = strstr(input, "winc"))) {
            int inc;
            sscanf(s+5, "%d", &inc);
            if (my_colour == WHITE || (uci_dialect == 's' && my_colour == BLACK)) {
               game->clock.time_inc = inc * uci_timeunit;
            }
         }
         if ((s = strstr(input, "binc"))) {
            int inc;
            sscanf(s+5, "%d", &inc);
            if (my_colour == BLACK || (uci_dialect == 's' && my_colour == WHITE)) {
               game->clock.time_inc = inc * uci_timeunit;
            }
         }
         if ((s = strstr(input, "mate"))) {  /* Just do a normal N-ply search instead */
            if (uci_dialect == 's') {
               /* Tsume search, not implemented */
               log_xboard_output("checkmate notimplemented\n");
               continue;
            } else {
               sscanf(s+5, "%d", &depth);
               set_infinite_time(&game->clock);
            }
         }
         if ((s = strstr(input, "depth"))) {
            sscanf(s+6, "%d", &depth);
            set_infinite_time(&game->clock);
         }
         if ((s = strstr(input, "byoyomi"))) {  /* Traditional Shogi time-control (USI) */
            int bt = 0;
            sscanf(s+7, "%d", &bt);
            bt *= uci_timeunit;
            if (bt > game->clock.time_left) {
               game->clock.time_inc = bt;
               set_time_for_game(&game->clock);
            } else {
               set_time_per_move(&game->clock, bt);
               game->clock.movestogo = 1;
            }
         }

         uci_clock_handler = game->clock.check_clock;
         game->check_keyboard = NULL;
         game->pondering = false;
         game->ponder_move = 0;
         if ((s = strstr(input, "ponder"))) {
            game->check_keyboard = uci_keyboard_input_on_ponder;
            game->ponder_move = game->move_list[game->moves_played-1];
            game->pondering = true;
            game->clock.check_clock = NULL;
         }

         if (game->think(depth) == SEARCH_OK)  {
            move_t move = game->get_last_move();
            log_xboard_output("bestmove %s", move_to_lan_string(move, false, uci_kxr));
            if (game->ponder_move)
               log_xboard_output(" ponder %s", move_to_lan_string(game->ponder_move, false, uci_kxr));
            log_xboard_output("\n");
         }
         game->clock.check_clock = uci_clock_handler;
         game->pondering = false;
      } else if (strstr(input, "ponderhit") && uci_mode) {  /* Nothing special yet... */
      } else if (strstr(input, "gameover") && uci_mode) {
      } else if (strstr(input, "help") == input) {
         print_help(input+4);
      } else if (strstr(input, "#") == input) {
      } else if (strstr(input, "echo ") == input) {
         log_xboard_output("%s", input+5);
      } else if (strstr(input, "protover") == input) {
         send_options_to_xboard();
         log_xboard_output("feature done=1\n");
      } else if (strstr(input, "accepted") == input) {
      } else if (strstr(input, "rejected") == input) {
      } else if (streq(input, "variant") || streq(input, "variants")) {
         printf("Known variants:\n");
         size_t max_len = 0;
         for (int n = 0; n<num_standard_variants; n++) {
            size_t l = strlen(standard_variants[n].name);
            if (l > max_len) max_len = l;
         }
         for (int n = 0; n<num_custom_variants; n++) {
            size_t l = strlen(custom_variants[n].shortname);
            if (l > max_len) max_len = l;
         }

         for (int n = 0; n<num_standard_variants; n++) {
            int c = (int)(max_len - strlen(standard_variants[n].name));
            printf(" '%s' ", standard_variants[n].name);
            for(; c>0; c--) printf(" ");
            printf("(%dx%d", standard_variants[n].files, standard_variants[n].ranks);
            if (standard_variants[n].holdings) printf("+%d", standard_variants[n].holdings);
            printf(")\n");
         }
         for (int n = 0; n<num_custom_variants; n++) {
            int c = (int)(max_len - strlen(custom_variants[n].shortname));
            printf(" '%s' ", custom_variants[n].shortname);
            for(; c>0; c--) printf(" ");
            printf("(%dx%d", custom_variants[n].files, custom_variants[n].ranks);
            printf(")\t[%s]\n", custom_variants[n].filename);
         }
      } else if (strstr(input, "load") == input) {
         char *s = input+5;
         char *p;
         while (isspace(*s)) s++;
         p = s + strlen(s)-1;
         while (p > s && isspace(*p)) {
            *p = '\0';
            p--;
         }
         invalidate_variant_file(s);
         scan_variant_file(s);
         for (int n = 0; n<num_custom_variants; n++) {
            if (streq(custom_variants[n].filename, s))
               printf("Loaded '%s' (%dx%d)\n", custom_variants[n].shortname, custom_variants[n].files, custom_variants[n].ranks);
         }
      } else if (strstr(input, "unload") == input) {
         char *s = input+7;
         char *p;
         while (isspace(*s)) s++;
         p = s + strlen(s)-1;
         while (p > s && isspace(*p)) {
            *p = '\0';
            p--;
         }
         invalidate_variant_file(s);
      } else if (strstr(input, "variant ") == input || streq(input, "new")) {
         bool override_fairy = false;
         if (strstr(input, "variant")) {
            char *s = input + 7;
            while (*s && isspace(*s)) s++;
            if (s[0]) {
               /* Trim trailing spaces */
               char *p = s + strlen(s)-1;
               while (*p == ' ') {*p = 0; p--; }
               free((void *)variant_name);
               variant_name = NULL;
               if (user_alias && strstr(user_alias, s) == user_alias) {
                  char *p = strstr(user_alias, "=");
                  if (p) {
                     p++;
                     variant_name = trim(strdup(p));
                     override_fairy = true;
                  }
               }
               if (!variant_name)
                  variant_name = strdup(s);

               /* Remove size-override for variant fairy */
               s = strstr((char *)variant_name, "fairy");
               if (s && s != variant_name && s[-1] == '_') {
                  snprintf((char *)variant_name, strlen(variant_name), "fairy");
               }
            }
         } else {
            /* "new" should switch to variant "normal" in XBoard mode. */
            if (xboard_mode) {
               free((void *)variant_name);
               variant_name = strdup("normal");
            }
         }

         /* Backup randomisation settings */
         bool         rok = game->random_ok;
         unsigned int rk  = game->random_key;
         eval_t       ra  = game->random_amplitude;
         size_t       rpc = game->random_ply_count;

         if (game)
            delete game;
         game = create_variant_game(variant_name);
         if (!game) {
            log_xboard_output("Error (cannot start variant game): '%s'\n", variant_name);
         } else {
            if (xboard_mode)
               log_xboard_output("# New game '%s'\n", game->get_name());
            //game->book = open_opening_book(pgbook_file);
            game->set_transposition_table_size(hash_size);
            game->start_new_game();
            if (xboard_mode && (game->xb_setup || streq(variant_name, "fairy") || override_fairy)) {
               const char *fen = game->start_fen;
               if (mask_dark_squares)
                  fen = filter_dark_squares_fen(game->start_fen);
               if ((game->xb_setup == NULL || game->xb_setup[0] == 0) && (streq(variant_name, "fairy") || override_fairy)) {
                  const char *parent = find_real_variant_name(variant_name);
                  if (streq(parent, "chess")) parent = "fairy";
                  log_xboard_output("setup () %dx%d+%d_%s %s\n",
                     game->files, game->ranks, game->holdsize, parent, fen);
               } else {
                  if (game->xb_setup && game->xb_setup[strlen(game->xb_setup)-1] == ')') {
                     const char *parent_name = "fairy";
                     if (game->xb_parent && game->xb_parent[0]) parent_name = game->xb_parent;
                     log_xboard_output("setup %s %dx%d+%d_%s %s\n", game->xb_setup ?  game->xb_setup : "()", game->files, game->ranks, game->holdsize, parent_name, fen);
                  } else
                     log_xboard_output("setup %s %s\n", game->xb_setup ?  game->xb_setup : "()", fen);
               }
               if (send_piece_descriptions) game->write_piece_descriptions(true);
            }

            /* Restore random settings */
            game->random_ok = rok;
            game->random_key = rk;
            game->random_amplitude = ra;
            game->random_ply_count = rpc;

            if (strstr(input, "new")) {
               my_colour = BLACK;
               depth = MAX_SEARCH_DEPTH;
               num_games++;
               in_play = true;
               if (f) fprintf(f, "# game %d\n", num_games);
               game->random_ok = false;
            }
         }

         if (game && xboard_mode) rank_offset = (game->ranks != 10);
         relabel_chess_square_names();
         load_evaluation_parameters(game, eval_file);
      } else if (strstr(input, "option") == input) {
#if 0
         if (strstr(input+7, "Opening book (polyglot)")) {
            char *s = strstr(input, "=");
            char *eol = input + strlen(input)-1;
            if (s) s++;

            /* Strip leading and trailing spaces */
            while (*s && isspace(*s)) s++;
            while (isspace(*eol)) { *eol='\0'; eol--; }

            log_xboard_output("# %s\n", s);

            free(pgbook_file);
            pgbook_file = strdup(s);

            if (game) {
               close_opening_book(game->book);
               game->book = open_opening_book(pgbook_file);
            }
         }
#endif
#if 0
      } else if (strstr(input, "book") == input) {
            char *s = input+4;
            char *eol = input + strlen(input)-1;
            if (s) s++;

            /* Strip leading and trailing spaces */
            while (*s && isspace(*s)) s++;
            while (isspace(*eol)) { *eol='\0'; eol--; }

            if (streq(s, "off")) {
               free(pgbook_file);
               pgbook_file = NULL;
               if (game) {
                  close_opening_book(game->book);
                  game->book = NULL;
               }
            } else {
               free(pgbook_file);
               pgbook_file = strdup(s);

               if (game) {
                  close_opening_book(game->book);
                  game->book = open_opening_book(pgbook_file);
               }
            }
#endif
         if (strstr(input+7, "MultiPV")) {
            char *s = strstr(input, "=");
            if (s) {
               int i = multipv;
               sscanf(s+1, "%d", &i);
               multipv = i;
            }
         }
         if (strstr(input+7, "Draw offer threshold")) {
            char *s = strstr(input, "=");
            if (s) {
               int i = draw_threshold;
               sscanf(s+1, "%d", &i);
               draw_threshold = i;
            }
         }
         if (strstr(input+7, "Moves before draw offer (0 to disable)")) {
            char *s = strstr(input, "=");
            if (s) {
               int i = draw_count;
               sscanf(s+1, "%d", &i);
               draw_count = i;
            }
         }
         if (strstr(input+7, "Resign threshold")) {
            char *s = strstr(input, "=");
            if (s) {
               int i = resign_threshold;
               sscanf(s+1, "%d", &i);
               resign_threshold = -abs(i);
            }
         }
         if (strstr(input+7, "Moves before resigning (0 to disable)")) {
            char *s = strstr(input, "=");
            if (s) {
               int i = resign_count;
               sscanf(s+1, "%d", &i);
               resign_count = i;
            }
         }
         if (strstr(input+7, "Randomise opening moves")) {
            char *s = strstr(input, "=");
            if (s) {
               int i = random_ply_count;
               sscanf(s+1, "%d", &i);
               random_ply_count = i;
            }
         }
         if (strstr(input+7, "Random amplitude (0 to disable)")) {
            char *s = strstr(input, "=");
            if (s) {
               int i = random_amplitude;
               sscanf(s+1, "%d", &i);
               random_amplitude = i;
            }
         }
         if (strstr(input+7, "Mate search")) {
            char *s = strstr(input, "=");
            if (s) {
               s++;
               if (strstr(s, "Disabled") == s) {
                  option_ms = MATE_SEARCH_DISABLED;
               } else if (strstr(s, "Enabled for drop games") == s) {
                  option_ms = MATE_SEARCH_ENABLE_DROP;
               } else if (strstr(s, "Enabled") == s) {
                  option_ms = MATE_SEARCH_ENABLED;
               }
            }
         }
         if (strstr(input+7, "Level")) {
            char *s = strstr(input, "=");
            if (s) {
               s++;
               skill_level = LEVEL_NORMAL;
               for (int n=0; n<int(sizeof combo_skill/sizeof *combo_skill); n++) {
                  if (strstr(s, combo_skill[n].label) == s)
                     combo_skill[n].var[0] = combo_skill[n].value;
               }
            }
         }
         if (strstr(input+7, "Variant fairy selects")) {
            char *s = strstr(input, "=");
            if (s) {
               snprintf(fairy_alias, sizeof fairy_alias, "%s", s+1);
               s = strstr(fairy_alias, " ");
               if (s) *s = 0;
            }
         }
         
         for (int n = 0; boolean_settings[n].label; n++) {
            if (!strstr(input+7, boolean_settings[n].label)) continue;
            char *s = strstr(input, "=");

            if (s) {
               s++;
               int i = *boolean_settings[n].opt;
               if (boolean_settings[n].neg) i = !i;
               sscanf(s, "%d", &i);
               if (boolean_settings[n].neg) i = !i;
               *boolean_settings[n].opt = i != 0;

               if (boolean_settings[n].resend_options) {
                  log_xboard_output("feature done=0\n");
                  send_variants_to_xboard();
                  log_xboard_output("feature done=1\n");
               }
            }
         }

         if (strstr(input+7, "Send O-O/O-O-O for castling")) {
            if (!castle_oo)
               log_xboard_output("telluser Warning: disabling O-O/O-O-O notation for castling breaks shuffle variants!\n");
         }
         if (strstr(input+7, "Set variant alias")) {
            char *s = strstr(input, "=");
            if (s) {
               s++;
               free(user_alias);
               user_alias = trim(strdup(s));

               log_xboard_output("feature done=0\n");
               send_variants_to_xboard();
               log_xboard_output("feature done=1\n");
            }
         }
         if (strstr(input+7, "Variant 'normal' is")) {
            char *s = strstr(input, "=");
            if (s) {
               s++;
               snprintf(normal_alias, sizeof(normal_alias), "%s", s);
            }
         }
         if (strstr(input+7, "Variant configuration file")) {
            char *s = input + 34;
            char *eol = input + strlen(input)-1;

            /* Strip leading and trailing spaces */
            while (*s && isspace(*s)) s++;
            while (isspace(*eol)) { *eol='\0'; eol--; }

            invalidate_variant_file(fairy_file);
            free(fairy_file);
            fairy_file = strdup(s);
            scan_variant_file(fairy_file);

            /* Try to work around a problem with XBoard rejecting the "reset" option by sending our
             * configuration anyway. This seems to be the best we can do in this situation.
             */
            log_xboard_output("feature done=0\n");
            send_variants_to_xboard();
            send_fairy_menu_to_xboard();
            log_xboard_output("feature done=1\n");
         }
         if (strstr(input+7, "Evaluation parameter file")) {
            char *s = input + 33;
            char *eol = input + strlen(input)-1;

            /* Strip leading and trailing spaces */
            while (*s && isspace(*s)) s++;
            while (isspace(*eol)) { *eol='\0'; eol--; }

            free(eval_file);
            eval_file = strdup(s);
            load_evaluation_parameters(game, eval_file);
         }

         write_config_file();
      } else if (strstr(input, "skill") == input) {
            char *s = strstr(input, " ");
            if (s) {
               s++;
               skill_level = LEVEL_NUM_LEVELS;
               for (int n=0; n<int(sizeof combo_skill/sizeof *combo_skill); n++) {
                  if (strstr(s, combo_skill[n].label) == s)
                     combo_skill[n].var[0] = combo_skill[n].value;
               }
               if (skill_level == LEVEL_NUM_LEVELS) {
                  skill_level = LEVEL_NORMAL;
                  printf("Unknown skill level '%s', defaulting to 'Normal'\n", s);
               }
            } else {
               for (int n=0; n<int(sizeof combo_skill/sizeof *combo_skill); n++) {
                  if (combo_skill[n].var[0] == combo_skill[n].value)
                     printf("Current skill level '%s'\n", combo_skill[n].label);
               }
            }
      } else if (strstr(input, "memory") == input) {
         unsigned long int memory_size;
         sscanf(input+7, "%lu", &memory_size);

         /* Convert to bytes */
         memory_size <<= 20;

         size_t nelem = memory_size / sizeof(hash_table_entry_t);
         /* Round to the next-lowest power of 2 */
         nelem |= (nelem >> 1);
         nelem |= (nelem >> 2);
         nelem |= (nelem >> 4);
         nelem |= (nelem >> 8);
         nelem |= (nelem >> 16);
         nelem |= (nelem >> 32*(sizeof(size_t)>4));
         nelem >>= 1;
         nelem++;

         hash_size = nelem;
         game->set_transposition_table_size(hash_size);
      } else if (strstr(input, "analyze") || strstr(input, "analyse")) {
         if (game) {
            in_play = false;
            //game->set_xboard_output_function(log_xboard_output);
            game->analysing = true;
         }
      } else if (strstr(input, "multipv")) {
         sscanf(input+7, "%d", &multipv);
         if (multipv < 1) multipv = 1;
      } else if (strstr(input, "force setboard")) {
         char *p = strstr(input+7, " ");
         while (p && *p && p[0]==' ') p++;

         if (game) game->setup_fen_position(p);
         in_play = false;
      } else if (strstr(input, "force") == input) {
         game->draw_count = 0;
         game->resign_count = 0;
         in_play = false;
      } else if (strstr(input, "undo") == input) {
         if (game->draw_count) game->draw_count--;
         if (game->resign_count) game->resign_count--;
         game->takeback();
         game->clock.time_left = game->move_clock[game->moves_played];
      } else if (strstr(input, "rules") == input) {
         if (game) game->print_rules();
      } else if (strstr(input, "wikirules") == input) {
         if (game) game->print_wiki_rules();
      } else if (strstr(input, "pieceinfo") == input) {
         game->print_pieces();
      } else if (strstr(input, "remove") || strstr(input, "takeback")) {
         if (game->draw_count) game->draw_count--;
         if (game->resign_count) game->resign_count--;
         game->takeback();
         game->clock.time_left = game->move_clock[game->moves_played];
         game->takeback();
         game->clock.time_left = game->move_clock[game->moves_played];
      } else if (strstr(input, "setboard") == input) {
         char *p = strstr(input, " ");
         while (p && *p && p[0]==' ') p++;

         if (game) game->setup_fen_position(p);
         if (!xboard_mode) in_play = false;
      } else if (strstr(input, "perft") == input) {
         if (game) {
            int depth = 6;
            int root = 0;
            char *s = input + 5;
            while (*s && isspace(*s)) s++;
            if (*s) {
               sscanf(s, "%d", &depth);
               while(*s && isdigit(*s)) s++;
            }
            while (*s && isspace(*s)) s++;
            if (*s) {
               sscanf(s, "%d", &root);
            }
#ifdef __unix__
            if (trapint) old_signal_handler = signal(SIGINT, interrupt_computer);
#endif
            abort_search = false;
            uint64_t t = get_timer();
            for (int n = 1; n<depth+1; n++) {
               uint64_t nodes = perft(game, n, root);
               uint64_t tt = get_timer();

               if (tt == t) tt++;

               if (abort_search) break;
               printf("%2d %10lld %5.2f %12.2fnps\n", n, (long long int)nodes,
                     (double)((tt - t)/1000000.0), (double)(nodes*1.0e6/(tt-t)));

               t = tt;
            }
#ifdef __unix__
            if (trapint) signal(SIGINT, old_signal_handler);
#endif
         }
      } else if (strstr(input, "lperft") == input) {
         if (game) {
            int depth = 6;
            int root = 0;
            char *s = input + 6;
            while (*s && isspace(*s)) s++;
            if (*s) {
               sscanf(s, "%d", &depth);
               while(*s && isdigit(*s)) s++;
            }
            while (*s && isspace(*s)) s++;
            if (*s) {
               sscanf(s, "%d", &root);
            }
#ifdef __unix__
            if (trapint) old_signal_handler = signal(SIGINT, interrupt_computer);
#endif
            abort_search = false;
            uint64_t t = get_timer();
            for (int n = 1; n<depth+1; n++) {
               uint64_t nodes = perft(game, n, root, true);
               uint64_t tt = get_timer();

               if (tt == t) tt++;

               if (abort_search) break;
               printf("%2d %10lld %5.2f %12.2fnps\n", n, (long long int)nodes,
                     (double)((tt - t)/1000000.0), (double)(nodes*1.0e6/(tt-t)));

               t = tt;
            }
#ifdef __unix__
            if (trapint) signal(SIGINT, old_signal_handler);
#endif
         }
      } else if (strstr(input, "test movegen") == input) {
         test_movegen();
      } else if (strstr(input, "test legal movegen") == input) {
         test_movegen(true);
      } else if (strstr(input, "test benchmark") == input) {
         int depth = 10;
         char *s = input + 15;
         while (*s && isspace(*s)) s++;
         if (*s) sscanf(s, "%d", &depth);
         //printf("Benchmark %d\n", depth);
         test_benchmark(depth);
      } else if (strstr(input, "test ebf") == input) {
         int depth = 10;
         char *s = input + 8;
         while (*s && isspace(*s)) s++;
         if (*s) sscanf(s, "%d", &depth);
         printf("Benchmark %d\n", depth);
         uint64_t t1 = test_benchmark(depth);
         if (!abort_search) {
            printf("Benchmark %d\n", depth+1);
            uint64_t t2 = test_benchmark(depth+1);
            if (!abort_search) {
               printf("EBF = %.3f\n", (double)t2 / t1);
            }
         }
      } else if (strstr(input, "test fine70") == input) {
         game_t *game = create_variant_game("chess");
         game->start_new_game();
         load_evaluation_parameters(game, eval_file);
         game->setup_fen_position("8/k7/3p4/p2P1p2/P2P1P2/8/8/K7 w - - bm Kb1");
         play_state_t status = game->think(depth);

         delete game;
      } else if (strstr(input, "test wac") == input) {
         run_test_suite(1000, wac_test);
      } else if (strstr(input, "test sts") == input) {
         run_test_suite(1000, sts_test);
      } else if (strstr(input, "test static_qs") == input) {
         printf("%d\n", game->static_qsearch(LEGALWIN));
      } else if (strstr(input, "test see") == input) {
         move_t move = game->move_string_to_move(input+8);
         printf("%d\n", game->see(move));
      } else if (strstr(input, "test smp") == input) {
#ifdef SMP
         int cores = ncore/2;
         int depth = 12;
         char *s = input + 8;
         while (*s && isspace(*s)) s++;
         if (*s == '-') {
            if (*s) sscanf(s, "- %d", &depth);
         } else {
            if (*s) sscanf(s, "%d %d", &cores, &depth);
         }
         test_smp(cores, depth);
#endif
      } else if (strstr(input, "test chase") == input) {
         chase_state_t state = game->test_chase();
         switch (state) {
            case NO_CHASE:
               printf("No chase\n");
               break;

            case DRAW_CHASE:
               printf("Legal chase (draw)\n");
               break;

            case LOSE_CHASE:
               printf("Illegal chase (lose)\n");
               break;

            case WIN_CHASE:
               printf("Illegal chase (win)\n");
               break;
         }
      } else if (strstr(input, "test") == input) {
         printf("Unknown test: %s\n", input+4);
      } else if (strstr(input, "sd inf")) {
         depth = MAX_SEARCH_DEPTH;
      } else if (strstr(input, "sd ") == input) {
         sscanf(input, "sd %d", &depth);
      } else if (strstr(input, "threads") || strstr(input, "cores")) {
#ifdef SMP
         int threads = 0;
         char *s = input;
         while (*s && isalpha(*s)) s++;
         sscanf(s, "%d", &threads);
         kill_threads();
         if (threads > 0)
            init_threads(threads);

         if (show_board) printf("Started %d threads\n", get_number_of_threads());
#endif
      } else if (streq(input, "playother")) {
         if (game) {
            my_colour = next_side[game->get_side_to_move()];
            in_play = true;

            /* Correctly set the number of moves to go */
            if (game->clock.movestotc) {
               size_t moves_played = game->get_moves_played() / 2;
               if (my_colour == WHITE)
                  moves_played += (game->get_moves_played() % 2);
               moves_played %= game->clock.movestotc;
               game->clock.movestogo = game->clock.movestotc - (int)moves_played - game->start_move_count/2;
            }
         }
      } else if (streq(input, "go")) {
         if (game) {
            my_colour = game->get_side_to_move();
            in_play = true;

            /* Correctly set the number of moves to go */
            if (game->clock.movestotc) {
               size_t moves_played = game->get_moves_played() / 2;
               if (my_colour == WHITE)
                  moves_played += (game->get_moves_played() % 2);
               moves_played %= game->clock.movestotc;
               game->clock.movestogo = game->clock.movestotc - (int)moves_played - game->start_move_count/2;
            }
         }
      } else if (strstr(input, "st inf") == input) {
         if (game) set_infinite_time(&game->clock);
      } else if (strstr(input, "st ") == input) {
         float tpm = 5;
         sscanf(input+3, "%g", &tpm);
         time_per_move = int(tpm * 1000);
         set_time_per_move(&game->clock, time_per_move);
         if (nps > 0) game->clock.max_nodes = nps * time_per_move;
      } else if (strstr(input, "computer") == input) {
      } else if (strstr(input, "name") == input) {
      } else if (strstr(input, "rating") == input) {
      } else if (strstr(input, "random") == input) {
         sgenrand((unsigned int)time(NULL));
         if (strstr(input, "on"))
            game->random_ok = true;
         else if (strstr(input, "off"))
            game->random_ok = false;
         else
            game->random_ok = !game->random_ok;
         game->random_key = genrandui();
         game->random_amplitude = random_amplitude;
         game->random_ply_count = random_ply_count;
      } else if (strstr(input, "easy") || streq(input, "ponder off")) {
         may_ponder = false;
      } else if (strstr(input, "hard") || streq(input, "ponder on")) {
         may_ponder = true;
      } else if (strstr(input, "nopost") == input || strstr(input, "post off") == input) {
         game->set_xboard_output_function(NULL);
      } else if (strstr(input, "post") == input) {
         game->set_xboard_output_function(log_xboard_output);
      } else if (strstr(input, "trace on")) {
         game->trace = true;
      } else if (strstr(input, "trace off")) {
         game->trace = false;
      } else if (strstr(input, "?") == input) {
      } else if (strstr(input, "hint") == input) {
         if (game && game->ponder_move)
            log_xboard_output("Hint: %s\n", move_to_lan_string(game->ponder_move));
      } else if (strstr(input, "otim") == input) {
      } else if (strstr(input, "ping") == input) {
         log_xboard_output("pong %s\n", input+5);
      } else if (strstr(input, "draw") == input) {
         /* Process draw offer */
         if (game && game->draw_count > draw_count)
            log_xboard_output("offer draw\n");
      } else if (strstr(input, "result") == input) {
      } else if (strstr(input, "time") == input) {
         set_time_from_string(game, input+5);
      } else if (strstr(input, "nps")) {
         /* This is a convoluted mess... */
         sscanf(input+4, "%d", &nps);
         /* If we have a fixed time per move, then we can set a maximum number of nodes.
          * Otherwise... things get complicated.
          */
         game->clock.max_nodes = 0;
         if (time_per_move > 0 && nps > 0) game->clock.max_nodes = nps * time_per_move;
      } else if (strstr(input, "maxnodes")) {
         unsigned int nodes = 0;
         sscanf(input+9, "%u", &nodes);
         game->clock.max_nodes = nodes;
      } else if (strstr(input, "level") == input) {
         set_timecontrol_from_string(game, input+6);
      } else if (streq(input, "quit") || streq(input, "exit") || streq(input, "bye")) {
         exit(0);
      } else if (streq(input, "eval")) {
         if (game) {
            printf("Static evaluation: %d\n", game->eval());
         }
      } else if (strstr(input, "prompt on") == input) {
         prompt = true;
      } else if (strstr(input, "prompt off") == input) {
         printf("\n");
         prompt = false;
      } else if (strstr(input, "board on") == input) {
         show_board = true;
      } else if (strstr(input, "board off") == input) {
         show_board = false;
      } else if (strstr(input, "san on") == input) {
         san = true;
      } else if (strstr(input, "san off") == input) {
         san = false;
      } else if (streq(input, "pst")) {
         if (game)
            game->print_pst();
      } else if (streq(input, "bitboards")) {
         if (game)
            game->print_bitboards();
      } else if (streq(input, "board")) {
         if (game)
            game->print_board();
      } else if (streq(input, "settings")) {
         printf("Xboard mode     : %s\n",   xboard_mode ? "on" : "off");
         printf("UCI mode        : %s\n",   uci_mode    ? "on" : "off");
         printf("UCI dialect     : %c\n",   uci_dialect);
         printf("UCI timeunit    : %dms\n", uci_timeunit);
         printf("\n");
         printf("Prompt          : %s\n",   prompt     ? "on" : "off");
         printf("Board           : %s\n",   show_board ? "on" : "off");
         printf("SAN notation    : %s\n",   san        ? "on" : "off");
         printf("Trap SIGINT     : %s\n",   trapint    ? "on" : "off");
         printf("\n");
         printf("Time control    : %d moves in %ds + %dms\n", tc_moves, tc_time, tc_inc);
         printf("Time per move   : %ds\n", time_per_move);
         printf("Max depth       : %d\n", depth);
         printf("Max nodes       : %llu\n", (unsigned long long) (game ?  game->clock.max_nodes : 0));
         printf("MultiPV         : %d\n", multipv);
      } else if (streq(input, "pieces")) {
         if (game) game->write_piece_descriptions();
      } else if (strstr(input, "lep") == input) {
         const char *fname = input + strlen("lep");
         while (*fname && isspace(*fname)) fname++;
         load_evaluation_parameters(game, fname);
      } else if (streq(input, "show eval parameters")) {
         if (game)
            game->print_eval_parameters();
      } else if (strstr(input, "show attackers ") == input) {
         const char *s = input + 15;
         while (*s && isspace(*s)) s++;

         int file, rank;
         int n = 0;
         file = *s - 'a'; s++;
         sscanf(s, "%d %n", &rank, &n); s += n;
         rank -= rank_offset; /* Xboard counts ranks from 0 for large boards */
         int square = game->pack_rank_file(rank, file);
         game->print_attacker_bitboard(square);

      } else if (strstr(input, "show attacks ") == input) {
         const char *s = input + 13;
         while (*s && isspace(*s)) s++;

         int file, rank;
         int n = 0;
         file = *s - 'a'; s++;
         sscanf(s, "%d %n", &rank, &n); s += n;
         rank -= rank_offset; /* Xboard counts ranks from 0 for large boards */
         int square = game->pack_rank_file(rank, file);
         game->print_attack_bitboard(square);

      } else if (strstr(input, "san ") == input) {
         char *movestr = input+4;
         while (*movestr && isspace(*movestr)) movestr++;

         if (game && *movestr) {
            movelist_t movelist;
            move_t move = 0;

            game->generate_legal_moves(&movelist);
            for (int k=0; k<movelist.num_moves; k++) {
               if (streq(move_to_lan_string(movelist.move[k], game->castle_san_ok, false), movestr) ||
                   streq(move_to_lan_string(movelist.move[k], false, false), movestr) ||
                   streq(move_to_lan_string(movelist.move[k], false, true ), movestr)) {
                  move = movelist.move[k];
                  log_xboard_output("%s", move_to_short_string(move, &movelist, NULL, game->castle_san_ok));
                  break;
               }
            }

            if (move) {
               game->playmove(move);
               if (!strchr(piece_symbol_string, '+') && game->player_in_check(game->get_side_to_move())) log_xboard_output("+");
               log_xboard_output("\n");
               game->takeback();
            } else {
               log_xboard_output("%s\n", movestr);
            }
         }
      } else if (streq(input, "fen")) {
         if (game)
            printf("%s\n", game->make_fen_string());
      } else if (streq(input, "moves")) {
         if (game) {
            movelist_t movelist;
            game->generate_legal_moves(&movelist);
            int k;
            printf("%d moves\n", movelist.num_moves);
            for (k=0; k<movelist.num_moves; k++)
               printf("%s ", move_to_string(movelist.move[k], NULL));
         }
         printf("\n");
      } else if (streq(input, "longmoves")) {
         if (game) {
            movelist_t movelist;
            game->generate_legal_moves(&movelist);
            int k;
            printf("%d moves\n", movelist.num_moves);
            for (k=0; k<movelist.num_moves; k++) {
               printf("%3d/%3d ", k+1, movelist.num_moves);
               printf("%-10s ",   move_to_string(movelist.move[k]));
               printf("%-10s ",   move_to_short_string(movelist.move[k], &movelist, NULL, game->castle_san_ok));
               printf("%-10s ",   move_to_lan_string(movelist.move[k], false, false));
               printf("%-10s\n",  move_to_lan_string(movelist.move[k], false, true));
            }
         }
         printf("\n");
      } else if (streq(input, "pseudomoves")) {
         if (game) {
            movelist_t movelist;
            game->generate_moves(&movelist);
            int k;
            printf("%d moves\n", movelist.num_moves);
            for (k=0; k<movelist.num_moves; k++)
               printf("%s ", move_to_string(movelist.move[k], NULL));
         }
         printf("\n");
      } else if (strstr(input, "history") == input) {
         if (game) {
            int nm = game->moves_played;
            move_t *history = new move_t[nm];
            for (int n=0; n<nm; n++)
               history[n] = game->move_list[n];

            while (game->moves_played)
               game->takeback();

            for (int n=0; n<nm; n++) {
               movelist_t movelist;
               game->generate_legal_moves(&movelist);

               if (n % 2 == 0) printf("%d. ", 1+n/2);
               printf("%s ", move_to_short_string(game->move_list[n], &movelist));
               game->playmove(history[n]);
            }
            printf("\n");
            delete[] history;
         }
      } else if (strstr(input, "lift")) {
         lift_sqr = -1;

         movelist_t movelist;
         game->generate_legal_moves(&movelist);
         for (int square = 0; square < game->ranks*game->files; square++) {
            if (streq(input+5, square_names[square])) {
               send_legal_move_targets(game, square, &movelist);
               lift_sqr = square;
               break;
            }
         }
         /* Send "choice" string if all moves are promotion moves with the
          * same set of options.
          */
         movelist.rewind();
         move_t move;
         uint32_t pmask = 0;
         movelist.rewind();
         while ((move = movelist.next_move())) {
            if (get_move_from(move) != lift_sqr) continue;
            if (!is_promotion_move(move)) continue;

            pmask |= 1 << get_move_promotion_piece(move); 
         }

         for (int to_sqr = 0; pmask && to_sqr < game->files*game->ranks; to_sqr++) {
            movelist.rewind();
            uint32_t mask = 0;
            while ((move = movelist.next_move())) {
               if (get_move_to(move) != to_sqr) continue;
               if (get_move_from(move) != lift_sqr) continue;
               if (!is_promotion_move(move)) continue;

               mask |= 1 << get_move_promotion_piece(move); 
            }
            if (!mask) continue;
            if (mask != pmask) pmask= 0;
         }
         if (pmask) {
            char choice_str[256];
            int  cs = 0;
            while (pmask) {
               int bit = game->get_most_valuable_piece_id(pmask);
               if (bit < 0) break;

               pmask ^= 1<<bit;

               cs += snprintf(choice_str+cs, sizeof(choice_str) - cs, "%s", game->get_piece_notation(bit));
            }
            for (int n=0; n<cs; n++)
               choice_str[n] = toupper(choice_str[n]);
            log_xboard_output("choice %s\n", choice_str);
         }
      } else if (strstr(input, "put")) {
         /* Send promotion options, for promotion moves */
         char choice_str[256];
         int  cs = 0;
         const char *keep = "";

         movelist_t movelist;
         game->generate_legal_moves(&movelist);

         move_t move;
         while ((move = movelist.next_move())) {
            if (get_move_from(move) != lift_sqr) continue;
            for (int square = 0; square < game->ranks*game->files; square++) {
               if (streq(input+4, square_names[square])) {
                  if (get_move_to(move) != square) continue;

                  if (!is_promotion_move(move)) {
                     int p = get_move_piece(move);
                     keep = game->get_piece_notation(p);
                     if (isspace(keep[0]))
                        keep = game->get_piece_abbreviation(WHITE, p);
                     break;
                  }
                  int p = get_move_promotion_piece(move);
                  cs += snprintf(choice_str+cs, sizeof(choice_str) - cs, "%s", game->get_piece_notation(p));

                  break;
               }
            }
         }
         if (cs) {
            cs+=snprintf(choice_str+cs, sizeof(choice_str) - cs, "%s", keep);
            for (int n=0; n<cs; n++)
               choice_str[n] = toupper(choice_str[n]);
            log_xboard_output("choice %s\n", choice_str);
         }
      } else if (strstr(input, "hover")) {
         /* TODO */
      } else if (strstr(input, "readfen") || strstr(input, "readepd")) {
         FILE *f = fopen(input+8, "r");
         if (f) {
            char s[4096];
            if (fgets(s, sizeof s, f)) {
               if (game)  {
                  game->setup_fen_position(s);
                  game->print_board();
               }
            }
            fclose(f);
         } else {
            printf("Can't open file: %s\n", input+8);
         }
      } else if (prompt && input[0] == '!') {
         if (system(input+1) == -1)
            printf("Cannot execute command %s\n", input+1);
      } else if (input[0]) {
         /* Can the input be interpreted as a move? Or is it an unknown
          * command?
          */
         if (!input_move(game, input)) {
            log_xboard_output("Error (Illegal move or unknown command): %s\n", input);
            if (f)
               fprintf(f, "In position '%s'\n", game->make_fen_string());
         } else {
            if (!uci_mode) report_game_status(game, game->get_game_end_state(), input);
         }
      }

      /* Should the computer play a move? */
      if (game && in_play && !game->analysing && !uci_mode) {
         memset(deferred, 0, sizeof deferred);
         game->check_keyboard = keyboard_input_on_move;
         if (!input_interrupt_search)
            game->check_keyboard = NULL;
         if (game->get_side_to_move() == my_colour) {
            /* Check whether NPS time control is sane. */
            if (nps>-1 && !time_per_move) {
               log_xboard_output("telluser Warning: specifying NPS without fixed time per move is not implemented\n");
               game->clock.max_nodes = nps;
            }

            game->clock.pondering = false;
            //log_engine_output("Searching position %s\n", make_fen_string(game, NULL));

            movelist_t legal_moves;
            if (san) game->generate_legal_moves(&legal_moves);

#ifdef __unix__
            if (trapint) old_signal_handler = signal(SIGINT, interrupt_computer);
#endif
            game->move_clock[game->moves_played] = game->clock.time_left;
            game->show_fail_low    = report_fail_low;
            game->show_fail_high   = report_fail_high;
            game->repetition_claim = repetition_claim;
            game->resign_threshold = resign_threshold;
            game->draw_threshold   = draw_threshold;
            game->multipv          = 1;
            game->option_ms        = option_ms;
            game->level            = level_t(skill_level);

            play_state_t status = game->think(depth);
#ifdef __unix__
            if (trapint) signal(SIGINT, old_signal_handler);
#endif
            game->clock.time_left -= peek_timer(&game->clock);
            game->clock.time_left += tc_inc;
            game->move_clock[game->moves_played] = game->clock.time_left;
            if (!xboard_mode && game->clock.check_clock) {
               int  time = std::max(0, game->clock.time_left);
               int   min = time / 60000;
               float sec = float((time - min * 60000) / 1000.);
               log_xboard_output("Time remaining %d:%.2f\n", min, sec);
            }
            if (status == SEARCH_OK) {
               move_t move = game->get_last_move();

               /* Offer draw of resign, as appropriate */
               if (draw_count>0 && game->draw_count > draw_count) {
                  game->draw_count /= 2;   /* Wait a while before offering again */
                  log_xboard_output("offer draw\n");
               } else if (resign_count>0 && game->resign_count > resign_count)
                  log_xboard_output("resign\n");

               if (san) {
                  log_xboard_output("move %s\n", move_to_short_string(move, &legal_moves, NULL, game->castle_san_ok));
               } else {
                  char *s = strdup(move_to_lan_string(move, castle_oo && game->castle_san_ok, false));
                  char *p = strstr(s, ",");
                  if (p) {
                     /* Multi-leg moves should be sent in two goes. */
                     *p = 0;
                     log_xboard_output("move %s,\n", s);
                     log_xboard_output("move %s\n", p+1);
                  } else {
                     log_xboard_output("move %s\n", s);
                  }
                  free(s);
               }
               if (game->clock.movestogo) {
                  game->clock.movestogo--;
                  if (!game->clock.movestogo) {
                     game->clock.movestogo = game->clock.movestotc;
                     game->clock.time_left += tc_time;
                  }
               }
               report_game_status(game, game->get_game_end_state(), input);
            } else {
               report_game_status(game, game->get_game_end_state(), input);
               in_play = false;
            }
         } else if (may_ponder && game->ponder_move) {
            /* Ponder */
            bool (*old_keyboard_handler)(struct game_t *game) = game->check_keyboard;
            bool (*old_clock_handler)(const struct chess_clock_t *clock) = game->clock.check_clock;
            game->check_keyboard = interrupt_ponder;
            game->clock.check_clock = NULL;
            game->ponder();
            game->check_keyboard = old_keyboard_handler;
            game->clock.check_clock = old_clock_handler;
         }
      }

      if (game && game->analysing) {
         bool (*old_keyboard_handler)(struct game_t *game) = game->check_keyboard;
         bool (*old_clock_handler)(const struct chess_clock_t *clock) = game->clock.check_clock;

         game->analyse_fen = NULL;
         game->analyse_new = false;
         game->analyse_undo = 0;
         game->analyse_moves_played = int(game->moves_played);
         game->multipv = multipv;
         game->level   = LEVEL_NORMAL;

         game->check_keyboard = keyboard_input_analyse;
         game->clock.check_clock = NULL;
         game->analyse();
         game->check_keyboard = old_keyboard_handler;
         game->clock.check_clock = old_clock_handler;

         if (game->analyse_new)
            game->start_new_game();
         for (int n=0; n<game->analyse_undo; n++)
            game->takeback();
         if (game->analyse_fen) {
            game->setup_fen_position(game->analyse_fen);
            free((void *)game->analyse_fen);
            game->analyse_fen = NULL;
         }
      }
   }

   if (prompt)
      printf("\n");

   return 0;
}

