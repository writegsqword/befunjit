#pragma once

#include "typedefs.h"
#include "compile.h"
#include "vec.h"
#include "snippets.h"
#include <memory>
#include <iostream>
#include <set>
#include <unordered_map>
#include <unordered_set>
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

    inline void unresolve(uint64 stub_resolver_addr)
    {
        __movabs_rax_prefix = 0xb848;
        ins_prefix = INS_CALL;
        addr = stub_resolver_addr;
    }

    inline void resolve(uint64 target_addr)
    {
        __movabs_rax_prefix = 0xb848;
        ins_prefix = INS_JUMP;
        addr = target_addr;
    }

    thunk_entry(uint64 stub_resolver_addr)
    {
        __movabs_rax_prefix = 0xb848;
        unresolve(stub_resolver_addr);
    }
    thunk_entry()
    {
        __movabs_rax_prefix = 0xb848;
        addr = 0xcccccccc;
        ins_prefix = INS_JUMP;
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
        std::cerr << std::hex << "thunk resolver @" << f_dispatch.resolve_thunk_ptr << ", thunk @ " << (uint64)this << std::endl;
        for (auto &e : entries)
        {
            e.unresolve((uint64)&f_dispatch);
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
    // key : coord_t, value set of code_pos
    std::unordered_set<code_pos_t> _code_depends[N_COLS][N_ROWS];
    // For a code_pos, the set of coords that code_pos depends on
    // key: code_pos_t, value: set of coord_t
    // the reason why this isn't unordered is because this only exists to remove other refs in code_depends
    std::vector<coord_t> _code_depends_reverse[N_DIRS][N_COLS][N_ROWS];

    
public:
    //std::unique_ptr<stub_table> st;

    //dont use unique ptr here since it uses glibc malloc/free, st is mmaped
    stub_table* st;
    CodeManager();
    ~CodeManager();
    code_pos_t GetCodePos(uint64 address) const;
    inline uint64 GetThunkBase() const
    {
        return (uint64) &(this->st->entries);
    }

    //Add a dependency on a coord, given a code_pos
    void AddDependency(coord_t coords, code_pos_t code_pos);
    // returns the thunk address(jump target) of a code_pos
    uint64 GetThunkAddress(const code_pos_t& code_pos);
    thunk_entry& GetThunkEntry(const code_pos_t& code_pos);

    void ResolvePos(uint64 jump_dst, const code_pos_t &pos);

    // invalidate all code associated with x, y
    // I will come up with better names when I come up with better names
    void InvalidateCoords(coord_t coords);
};

constexpr uint64 STUB_TABLE_SIZE = sizeof(stub_table::entries), THUNK_ENTRY_SIZE = sizeof(thunk_entry);