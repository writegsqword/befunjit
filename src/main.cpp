#include "typedefs.h"
#include "utils.h"
#include <cstdlib>
#include <sys/mman.h>
#include <unistd.h>

#include <vector>
#include <string>
#include "globals.h"
#include <iostream>
#include <fstream>

#include <chrono>

void placeholder_func() {};
using void_fn_ptr = decltype(&placeholder_func);

struct __attribute__((packed)) tramp_asm  {
    byte pop0 = instructions::__pop_rax;
    instructions::_jump_to jt;

};

tramp_asm* tasm;

void trampoline_code() {
    byte buf[1024 * 4] = {0};
    ((void_fn_ptr)(tasm))();
    

}
int main(int argc, char** argv) {

    if(argc < 2) {
        std::cerr << "No filename provided. Exiting...\n";
        return -1;
    };
    std::string filename = argv[1];
    std::fstream fin(filename);
    
    auto now = std::chrono::system_clock::now();

    // Convert the current time to time since epoch
    auto duration = now.time_since_epoch();

    // Convert duration to milliseconds
    auto milliseconds
        = std::chrono::duration_cast<std::chrono::milliseconds>(
              duration)
              .count();
    srand(milliseconds);

    std::vector<std::string> ins;
    // {
    //     "\"!dlroW olleH\",,,,,,,,,,,,@",
    //     ">5-3*v"
    // };
    std::string line;

    while (std::getline(fin, line))
    {
        ins.push_back(line);
    }

    assert(ins.size() <= 25);
    for(int i = 0; i < ins.size(); i++) {

        std::string& s = ins[i];
        for(int j = 0; j < s.size(); j++ ) {
            G::static_memory[j][i] = s[j];
        };
    }

    print_mem();
    ThunkManager* mgr = new ThunkManager();
    std::cerr << "extern printint " << std::hex << G::p_extern_printint;
    std::cerr << "base@"  << std::hex << mgr->GetBase() << std::endl;
    std::cerr << "resolve callback@" << std::hex << &mgr->st.get()->f_dispatch << std::endl;
    tasm = (tramp_asm*)alloc_rwx(sizeof(tramp_asm));
    *tasm = tramp_asm();
    std::cerr << "tasm@ " << tasm << std::endl;
    tasm->jt.addr = mgr->GetAddress(code_pos_t(DIR_RIGHT, Vec2(0,0)));
    trampoline_code();

    

    return 0;
}