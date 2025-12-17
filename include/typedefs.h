#pragma once


#include <cstddef>
#include <cstdint>





using byte = unsigned char;
using uint16 = unsigned short;
using uint32 = unsigned int;
using int32 = int32_t;
using uint64 = unsigned long long;
using int64 = long long;
static_assert(sizeof(uint32) == 4);

namespace Dir {
    enum DirType { 
        UP,
        DOWN,
        LEFT,
        RIGHT,
        INVALID,
    };
    static const DirType All[] = { UP, DOWN, LEFT, RIGHT };
}



constexpr size_t N_ROWS = 25, N_COLS = 80, N_DIRS = 4;
constexpr size_t N_ROWS_COLS = N_ROWS * N_COLS;
constexpr size_t N_ROWS_COLS_DIRS = N_ROWS_COLS * N_DIRS;

constexpr int I_N_ROWS = N_ROWS, I_N_COLS = N_COLS;


