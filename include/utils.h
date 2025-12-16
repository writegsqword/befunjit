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



inline code_pos_t idx_to_pos(uint64 idx, Dir::DirType d) {
    return code_pos_t(idx / N_ROWS, idx % N_ROWS, d);
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
    Dir::DirType row_idx = (Dir::DirType)(table_offset / N_ROWS_COLS);
    
    code_pos_t r(idx_to_pos(table_offset % N_ROWS_COLS, row_idx));

    return r;

}

inline uint64 codepos_to_offset(const code_pos_t& code_pos) {
    return (code_pos.dir * N_ROWS_COLS + code_pos.x * N_COLS + code_pos.y) * THUNK_ENTRY_SIZE;
}





inline code_pos_t vec_from_dir(Dir::DirType d) {
    switch (d)
    {
    case Dir::UP:
        return {0, -1, d};
    case Dir::DOWN:
        return {0, 1, d};
    case Dir::LEFT:
        return {-1, 0, d};
    case Dir::RIGHT:
        return {1, 0, d};
    
    default:
        throw std::runtime_error("Invalid direction");    
    }
}

inline Dir::DirType dir_from_ascii(char c) {
    switch (c)
    {
    case '^':
        return Dir::UP;
    case 'v':
        return Dir::DOWN;
    case '<':
        return Dir::LEFT;
    case '>':
        return Dir::RIGHT;
    
    default:
        throw std::runtime_error("Invalid direction");    
    }
}


inline void print_mem() {
    for(int y = 0; y < N_ROWS; y++) {
        for(int x = 0; x < N_COLS; x++) {
            std::cerr << G::static_memory[x][y];
        }
        std::cerr << std::endl;
    }
}