#pragma once

#include "typedefs.h"
#include "thunk.h"
#include "globals.h"
#include <cassert>
#include <utility>

#include <cstdlib>
#include <sys/mman.h>

#include "vec.h"
inline int prot_rwx(void* ptr, size_t size) {
    uint64 addr = (uint64)ptr;
    return mprotect((void*)(addr & (~0x0fff)), size + (addr & ~0x0fff), PROT_READ | PROT_WRITE | PROT_EXEC);
}
inline void* alloc_rwx(size_t size) {
    void* ptr = mmap(NULL, size, PROT_EXEC | PROT_WRITE | PROT_READ, MAP_ANON | MAP_PRIVATE, 0, 0);
    
    return ptr;

}



inline Vec2 idx_to_pos(uint64 idx) {
    return Vec2(idx / N_ROWS_Y, idx % N_ROWS_Y);
}
// inline void assert_pos(code_pos_t p) {
//     assert(p.first < 4);
//     assert(p.second.first < I_N_ROWS_X);
//     assert(p.second.second < I_N_ROWS_Y);
//     assert(p.second.first >= 0);
//     assert(p.second.second >= 0);
// }
inline code_pos_t offset_to_codepos(uint64 offset) {

    assert(offset <= STUB_TABLE_SIZE);
    uint64 table_offset = offset / THUNK_ENTRY_SIZE;
    DirType row_idx = (DirType)(table_offset / N_ROWS_XY);
    
    code_pos_t r(row_idx, idx_to_pos(table_offset % N_ROWS_XY));

    return r;

}

inline uint64 codepos_to_offset(const code_pos_t& code_pos) {
    return (code_pos.first * N_ROWS_XY + code_pos.second.x * N_ROWS_Y + code_pos.second.y) * THUNK_ENTRY_SIZE;
}





inline Vec2 vec_from_dir(DirType d) {
    switch (d)
    {
    case DIR_UP:
        return {0, -1};
    case DIR_DOWN:
        return {0, 1};
    case DIR_LEFT:
        return {-1, 0};
    case DIR_RIGHT:
        return {1, 0};
    
    default:
        throw std::runtime_error("Invalid direction");    
        }
}

inline DirType dir_from_ascii(char c) {
    switch (c)
    {
    case '^':
        return DIR_UP;
    case 'v':
        return DIR_DOWN;
    case '<':
        return DIR_LEFT;
    case '>':
        return DIR_RIGHT;
    
    default:
        throw std::runtime_error("Invalid direction");    
    }
}


inline void print_mem() {
    for(int y = 0; y < N_ROWS_Y; y++) {
        for(int x = 0; x < N_ROWS_X; x++) {
            std::cerr << G::static_memory[x][y];
        }
        std::cerr << std::endl;
    }
}