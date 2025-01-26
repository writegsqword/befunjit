



#include "externs.h"
#include "globals.h"
#include <iostream>
#include <cstdlib> 
#include "thunk.h"

void extern_put(uint64 x, uint64 y, uint64 v) {

}

uint64 extern_getchar() {
    char c;
    std::cin >> c;
    std::cerr << "\nTO STDIN char:" << c << std::endl;
    return (uint64)c;
}
uint64 extern_getint() {
    uint64 i;
    std::cin >> i;
    std::cerr << "\nTO STDIN int:" << i << std::endl;
    return i;
}

void extern_printint(uint64 v) {
    std::cerr << "\nFROM STDOUT int:";
    std::cout << (int64)v << " ";
}

void extern_printchar(uint64 v) {
    std::cerr << "\nFROM STDOUT char:";
    std::cout << (char)v;
    
}

uint64 extern_exit() {
    exit(0);
}

uint64 extern_rand() {
    
    int res = rand() % 4;
    std::cerr << "rand res " << res << std::endl;
    return res;
}
uint64 extern_read_val(uint64 y, uint64 x) {
    if(x >= N_ROWS_X || y >= N_ROWS_Y)
        return 0;
    return (uint64)G::static_memory[x][y];
}


void extern_write_val(uint64 y, uint64 x, uint64 v) {
    if(x >= N_ROWS_X || y >= N_ROWS_Y)
        return;
    G::static_memory[x][y] = v;
    //im sorry ok
    for(int yi = 0; yi < N_ROWS_Y; yi++) {
        Vec2 p(x, yi);
        ThunkManager::_instance->InvalidatePos(code_pos_t(DIR_UP, p));
        ThunkManager::_instance->InvalidatePos(code_pos_t(DIR_DOWN, p));
        ThunkManager::_instance->InvalidatePos(code_pos_t(DIR_LEFT, p));
        ThunkManager::_instance->InvalidatePos(code_pos_t(DIR_RIGHT, p));
    }
    for(int xi = 0; xi < N_ROWS_X; xi++) {
        Vec2 p(xi, y);
        ThunkManager::_instance->InvalidatePos(code_pos_t(DIR_UP, p));
        ThunkManager::_instance->InvalidatePos(code_pos_t(DIR_DOWN, p));
        ThunkManager::_instance->InvalidatePos(code_pos_t(DIR_LEFT, p));
        ThunkManager::_instance->InvalidatePos(code_pos_t(DIR_RIGHT, p));
    }

}