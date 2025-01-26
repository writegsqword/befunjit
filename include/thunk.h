#pragma once

#include "typedefs.h"
#include "compile.h"
#include "snippets.h"
#include <memory>
#include <iostream>

constexpr byte INS_CALL = 0xe8, INS_JUMP = 0xe9;

struct __attribute__((packed)) thunk_entry
{
    byte ins_prefix = INS_CALL;
    uint32 addr = 0;
    inline bool is_resolved()
    {
        return ins_prefix == INS_JUMP;
    }

    inline void init(uint64 stub_resolver_addr)
    {
        ins_prefix = INS_CALL;
        // need to bet the offset is less than 32 bits(lol)
        // if something went wrong its probably here
        addr = (int64)stub_resolver_addr - reinterpret_cast<int64>(this) - sizeof(*this);
    }

    inline void resolve(uint64 target_addr)
    {
        ins_prefix = INS_JUMP;
        addr = (int64)target_addr - reinterpret_cast<int64>(this) - sizeof(*this);
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
static_assert(sizeof(thunk_entry) == 5);




struct __attribute__((packed)) stub_table
{
    thunk_entry entries[N_ROWS_TOT];
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
class ThunkManager
{

public:
    std::unique_ptr<stub_table> st;

    inline static ThunkManager *_instance;

    ThunkManager();
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