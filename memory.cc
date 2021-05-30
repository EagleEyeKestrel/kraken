#include "memory.h"

void Memory::HandleRequest(uint64_t addr, int bytes, int read,
                          char *content, int ifPrefetch, int ifWriteDirty) {
    if(ifPrefetch) return;
    stats_.access_counter++;
}

