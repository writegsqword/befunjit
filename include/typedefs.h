#pragma once


#include <cstddef>
#include <cstdint>
#include <utility>




using byte = unsigned char;
using uint32 = unsigned int32_t;
using int32 = int32_t;
using uint64 = unsigned long long;
using int64 = long long;
static_assert(sizeof(uint32) == 4);


enum DirType { 

    DIR_UP,
    DIR_DOWN,
    DIR_LEFT,
    DIR_RIGHT
};



constexpr size_t N_ROWS_X = 80, N_ROWS_Y = 25, N_DIRS = 4;
constexpr size_t N_ROWS_XY = N_ROWS_X * N_ROWS_Y;
constexpr size_t N_ROWS_TOT = N_ROWS_XY * N_DIRS;

constexpr int I_N_ROWS_X = N_ROWS_X, I_N_ROWS_Y = N_ROWS_Y;


