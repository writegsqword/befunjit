#include "thunk.h"
#include "utils.h"
#include "vec.h"
#include <compile.h>
#include <unistd.h>
#include <cstdlib>
#include <sys/mman.h>
#include <cassert>

#include <iostream>





CodeManager::CodeManager() {
    stub_table* st_ptr = (stub_table*)alloc_rwx(sizeof(stub_table));
    st_ptr->_stub_table();
    //std::cerr << errno <<  " hi " << std::hex << st_ptr << std::endl;
    st = st_ptr;
    //std::cerr << "mprof res: " << res << "@" << st.get() << "errno " << errno << "\n";
    st->f_dispatch.resolve_thunk_ptr = reinterpret_cast<uint64>(&resolve_thunk);

}

CodeManager::~CodeManager() {
    munmap(st, sizeof(stub_table));
}


code_pos_t CodeManager::GetCodePos(uint64 address) const {
    uint64 base = this->GetThunkBase();
    uint64 offset = address - base;
    std::cerr << "calculated offset " << offset << " base" << base << std::endl; 
    return offset_to_codepos(offset);
}

// dir is ignored here
void CodeManager::AddDependency(coord_t coords, code_pos_t pos) {
    _code_depends[coords.x][coords.y].insert(pos);
    _code_depends_reverse[pos.dir][pos.x][pos.y].push_back(coords);
}


void CodeManager::ResolvePos(uint64 jump_dst, const code_pos_t& pos) {
    st->entries[codepos_to_offset(pos) / sizeof(thunk_entry)].resolve(jump_dst);
}

//honestly it doesnt even matter if im passing by const ref since its a 64 bit value
void CodeManager::InvalidateCoords(coord_t coords) {
    //forward map: x, y -> code_pos
    auto& depends = _code_depends[coords.x][coords.y];
    std::cerr << " invalidating coords " << "x: " << coords.x << " y: " << coords.y << std::endl;
    __DebugDumpDependencies();
    for(code_pos_t depend : depends){
    
        // The problem: code_pos depends on multiple coords
        // Solution: keep track of a reverse map(code_pos to coords)
        // Once a code_pos is invalidated, remove all references of the code_pos out of all depends
        // TODO: free the code
        // probably needs some refactoring
        thunk_entry& entry = GetThunkEntry(depend);
        // free entry.addr
        // Mark as unresolved
        entry.unresolve((uint64)&st->f_dispatch);

        auto& reverse_depends = _code_depends_reverse[depend.dir][depend.x][depend.y];
        // Remove all other refs
        for(coord_t reverse_dep : reverse_depends) {
            // dont mess with the container that we are iterating over
            // we will get to it later by clearing everything
            if(reverse_dep == coords)
                continue;
            _code_depends[reverse_dep.x][reverse_dep.y].erase(depend);
        }
        reverse_depends.clear();
    }
    depends.clear();
    
}

thunk_entry& CodeManager::GetThunkEntry(const code_pos_t& code_pos)  {
    return st->entries[codepos_to_offset(code_pos) / sizeof(thunk_entry)];
}

    
uint64 CodeManager::GetThunkAddress(const code_pos_t& code_pos) {
    //return GetThunkBase() + codepos_to_offset(code_pos);
    assert(GetThunkBase() + codepos_to_offset(code_pos) == (uint64)(&CodeManager::GetThunkEntry(code_pos)));
    return (uint64)(&CodeManager::GetThunkEntry(code_pos));
}

void CodeManager::__DebugDumpDependencies() const {
    std::cerr << "=== Forward dependencies (coord -> code_pos) ===\n";
    for (size_t x = 0; x < N_COLS; ++x) {
        for (size_t y = 0; y < N_ROWS; ++y) {
            const auto& set = _code_depends[x][y];
            if (set.empty()) continue;
            std::cerr << "coord(" << x << "," << y << "): ";
            for (const auto& cp : set) {
                std::cerr << "(" << cp.x << "," << cp.y << "," << cp.dir << ") ";
            }
            std::cerr << "\n";
        }
    }
    std::cerr << "=== Reverse dependencies (code_pos -> coord list) ===\n";
    for (size_t d = 0; d < N_DIRS; ++d) {
        for (size_t x = 0; x < N_COLS; ++x) {
            for (size_t y = 0; y < N_ROWS; ++y) {
                const auto& vec = _code_depends_reverse[d][x][y];
                if (vec.empty()) continue;
                std::cerr << "code_pos(" << x << "," << y << "," << d << "): ";
                for (const auto& c : vec) {
                    std::cerr << "(" << c.x << "," << c.y << "," << c.dir << ") ";
                }
                std::cerr << "\n";
            }
        }
    }
}
