#include "thunk.h"
#include "utils.h"
#include <compile.h>
#include <unistd.h>
#include <cstdlib>
#include <sys/mman.h>
#include <cassert>

#include <iostream>





CodeManager::CodeManager() {
    stub_table* st_ptr = (stub_table*)alloc_rwx(sizeof(stub_table));
    st_ptr->_stub_table();
    std::cerr << errno <<  "hi" << std::hex << st_ptr << std::endl;
    st = std::unique_ptr<stub_table>(st_ptr);
    //std::cerr << "mprof res: " << res << "@" << st.get() << "errno " << errno << "\n";
    st.get()->f_dispatch.resolve_thunk_ptr = reinterpret_cast<uint64>(&resolve_thunk);

}



code_pos_t CodeManager::GetCodePos(uint64 address) const {
    uint64 base = this->GetBase();
    uint64 offset = address - base;
    return offset_to_codepos(offset);
    
}


void CodeManager::ResolvePos(uint64 jump_dst, const code_pos_t& pos) {
    st.get()->entries[codepos_to_offset(pos) / sizeof(thunk_entry)].resolve(jump_dst);
     
}
void CodeManager::InvalidatePos(const code_pos_t& pos) {
    //forward map: x, y -> code_pos
    auto& depends = code_depends[pos.x][pos.y];
    //TODO: iterate through all depends, mark corresponding entries as invalid
    
    depends.clear();
    // TODO: implement a reverse lookup map: for each given code_pos, identify all other x, y that this depends on
    // and remove all references to this code_pos(since it will no longer exist)
    //auto& ref = st.get()->entries[codepos_to_offset(pos) / sizeof(thunk_entry)];
    //ref.init((uint64)&st.get()->f_dispatch);
}
    
uint64 CodeManager::GetAddress(const code_pos_t& code_pos) {
        return GetBase() + codepos_to_offset(code_pos);
}