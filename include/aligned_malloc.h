#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

void *aligned_malloc(size_t size, size_t align);
void aligned_free(void *ptr);

#ifdef __cplusplus
}
#endif
