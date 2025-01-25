#include "move.h"
#include "movelist.h"

const char *move_to_short_string(move_t move, const movelist_t *movelist = NULL, char *buffer = NULL, bool san_castle = true);
