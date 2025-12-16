



#include "externs.h"
#include "globals.h"
#include <iostream>
#include <cstdlib> 
#include "thunk.h"
#include "typedefs.h"
#include "vec.h"

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
    int res = (rand() % 4) + Dir::UP;
    std::cerr << "rand res " << res << std::endl;
    return res;
}
uint64 extern_read_val(uint64 y, uint64 x) {
    if(x >= N_COLS || y >= N_ROWS)
        return 0;
    return (uint64)G::static_memory[x][y];
}


void extern_write_val(uint64 y, uint64 x, uint64 v) {
    if(x >= N_COLS || y >= N_ROWS)
        return;
    G::static_memory[x][y] = v;
    
    //im sorry ok
    G::code_manager.InvalidatePos(code_pos_t(x, y, Dir::INVALID));

}