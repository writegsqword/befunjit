#include "thunk.h"
#include "utils.h"
#include <compile.h>
#include <unistd.h>
#include <cstdlib>
#include <sys/mman.h>
#include <cassert>

#include <iostream>





ThunkManager::ThunkManager() {
    stub_table* st_ptr = (stub_table*)alloc_rwx(sizeof(stub_table));
    st_ptr->_stub_table();
    std::cerr << errno <<  "hi" << std::hex << st_ptr << std::endl;
    st = std::unique_ptr<stub_table>(st_ptr);
    //std::cerr << "mprof res: " << res << "@" << st.get() << "errno " << errno << "\n";
    st.get()->f_dispatch.resolve_thunk_ptr = reinterpret_cast<uint64>(&resolve_thunk);
    _instance = this;

}



code_pos_t ThunkManager::GetCodePos(uint64 address) const {
    uint64 base = this->GetBase();
    uint64 offset = address - base;
    return offset_to_codepos(offset);
    
}


void ThunkManager::ResolvePos(uint64 jump_dst, const code_pos_t& pos) {
    st.get()->entries[codepos_to_offset(pos) / sizeof(thunk_entry)].resolve(jump_dst);
     
}
void ThunkManager::InvalidatePos(const code_pos_t& pos) {
    auto& ref = st.get()->entries[codepos_to_offset(pos) / sizeof(thunk_entry)];
    ref.init((uint64)&st.get()->f_dispatch);


}
    
uint64 ThunkManager::GetAddress(const code_pos_t& code_pos) {
        return GetBase() + codepos_to_offset(code_pos);
}