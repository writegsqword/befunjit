#pragma once

#include "typedefs.h"
#include "externs.h"

class G {
    public:
    inline static byte static_memory[N_ROWS_X][N_ROWS_Y];
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

