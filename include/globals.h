#pragma once

#include "thunk.h"
#include "typedefs.h"
#include "externs.h"
#include "vec.h"
#include <vector>

class G {
    public:
    //actual globals
    inline static byte static_memory[N_COLS][N_ROWS];
    inline static CodeManager code_manager;
    // not too sure what these are
    inline static uint64 p_extern_put = (uint64)&extern_put;

    inline static uint64 p_extern_getchar = (uint64)&extern_getchar;
    inline static uint64 p_extern_getint = (uint64)&extern_getint;

    inline static uint64 p_extern_printint = (uint64)&extern_printint;

    inline static uint64 p_extern_printchar = (uint64)&extern_printchar;
    inline static uint64 p_extern_rand = (uint64)&extern_rand;
    inline static uint64 p_extern_exit = (uint64)&extern_exit;
    inline static uint64 p_extern_read_val = (uint64)&extern_read_val;
    inline static uint64 p_extern_write_val = (uint64)&extern_write_val;
};

