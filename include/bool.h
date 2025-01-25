#if defined(_MSC_VER)
#   include <stdint.h>
#   if !defined __cplusplus
      // FIXME: hack
#      define inline
      typedef int bool;
#      define true  1
#      define false 0
#   endif
#else
#   include <stdbool.h>
#endif
