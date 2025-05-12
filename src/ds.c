#define DS_DA_IMPLEMENTATION
#define DS_SB_IMPLEMENTATION
#define DS_HM_IMPLEMENTATION
#ifdef __wasm__
#define DS_LIST_ALLOCATOR_IMPLEMENTATION
#else
#define DS_AP_IMPLEMENTATION
#endif
#include "ds.h"
