#pragma once
#include "typedefs.h"
#include "vec.h"

#include <vector>



uint64 __attribute_noinline__ resolve_thunk(uint64 address);
void* __attribute_noinline__ compile (const code_pos_t& pos);

