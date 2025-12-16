#pragma once

#include "typedefs.h"
#include "compile.h"
#include "vec.h"
#include "snippets.h"
#include <memory>
#include <iostream>
#include <set>
#include <unordered_map>
#include <vector>

/*
a:  ff e0                   jmp    rax
c:  ff d0                   call   rax
*/
constexpr uint16 INS_CALL = 0xd0ff, INS_JUMP = 0xe0ff;

struct __attribute__((packed)) thunk_entry
{

    //please dont optimize out this field...
    uint16 __movabs_rax_prefix = 0xb848;

    uint64 addr = 0;
    uint16 ins_prefix = INS_CALL;
    inline bool is_resolved()
    {
        return ins_prefix == INS_JUMP;
    }

    inline void init(uint64 stub_resolver_addr)
    {
        ins_prefix = INS_CALL;
        addr = stub_resolver_addr;
    }

    inline void resolve(uint64 target_addr)
    {
        ins_prefix = INS_JUMP;
        addr = target_addr;
    }

    thunk_entry(uint64 stub_resolver_addr)
    {
        init(stub_resolver_addr);
    }
    thunk_entry()
    {
        ins_prefix = 0;
    }
};
static_assert(sizeof(thunk_entry) == 0xc);


struct __attribute__((packed)) stub_table
{
    thunk_entry entries[N_ROWS_COLS_DIRS];
    // 3 dims
    // dim 0: row type(4)
    // dim 1: x(80)
    // dim 2: y(25)
    instructions::compiler_dispatch f_dispatch;

    void _stub_table()
    {
        f_dispatch = instructions::compiler_dispatch();
        f_dispatch.resolve_thunk_ptr = (uint64)&resolve_thunk;
        std::cerr << std::hex << "thunk resolve " << f_dispatch.resolve_thunk_ptr << std::endl;
        for (auto &e : entries)
        {
            e.init((uint64)&f_dispatch);
        }
    };
    stub_table()
    {
        _stub_table();
    }
} __attribute__((packed));

// should only have one instance
class CodeManager
{

    // Set of functions(each code_pos_t uniquely identifies a function) that depends on the value at x,y
    std::set<code_pos_t> code_depends[N_COLS][N_ROWS];
    std::unordered_map<code_pos_t, code_pos_t> code_depends_reverse;
public:
    std::unique_ptr<stub_table> st;
    
    CodeManager();
    code_pos_t GetCodePos(uint64 address) const;
    inline uint64 GetBase() const
    {
        return (uint64) & (this->st.get()->entries);
    }
    uint64 GetAddress(const code_pos_t& code_pos);
    void ResolvePos(uint64 jump_dst, const code_pos_t &pos);
    void InvalidatePos(const code_pos_t &pos);
};

constexpr uint64 STUB_TABLE_SIZE = sizeof(stub_table::entries), THUNK_ENTRY_SIZE = sizeof(thunk_entry);