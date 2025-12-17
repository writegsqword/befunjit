#include "compile.h"
#include "thunk.h"
#include "typedefs.h"
#include "utils.h"
#include "snippets.h"
#include "vec.h"
#include "xbyak.h"
#include "tlsf.h"
#include "Zydis.h"
#include <cassert>
#include <cstddef>
#include <cstring>
#include <iostream>


#define DEF_INSTRUCTION(ins_name) instructions::_##ins_name i_##ins_name = instructions::_##ins_name();
#define WRITE_INSTRUCTION(ins_name) c = (byte*) &i_##ins_name; size = sizeof(i_##ins_name);
#define DEF_WRITE(ins_name) DEF_INSTRUCTION(ins_name); WRITE_INSTRUCTION(ins_name)
#define DEF_WRITE_BREAK(ins_name) { DEF_WRITE(ins_name) break; }

uint64 __attribute_noinline__ resolve_thunk(uint64 address) {

    /*
    0:  48 b8 00 10 00 00 00    movabs rax,0x1000
    7:  00 00 00
    a:  ff d0                   call    rax
    */
    //decrement by the offset caused by the entire thing
    address -= 0xc;

    //write address to stub entry
    //
    //returns ABSOLUTE address
    std::cerr << "called from address: " << std::hex << address << std::endl;
    code_pos_t codepos = G::code_manager.GetCodePos(address);
    void* res_addr = compile(codepos);
    G::code_manager.ResolvePos((uint64)res_addr, codepos);
    return address;
}

static void __DebugDumpCode(void* code, size_t len) {
    // The runtime address (instruction pointer) was chosen arbitrarily here in order to better 
    // visualize relative addressing. In your actual program, set this to e.g. the memory address 
    // that the code being disassembled was read from. 
    ZyanU64 runtime_address = (uint64)code; 
    unsigned char* data = (unsigned char*)code;
    
    // Loop over the instructions in our buffer. 
    ZyanUSize offset = 0; 
    ZydisDisassembledInstruction instruction; 
    while (ZYAN_SUCCESS(ZydisDisassembleIntel( 
        /* machine_mode:    */ ZYDIS_MACHINE_MODE_LONG_64, 
        /* runtime_address: */ runtime_address, 
        /* buffer:          */ data + offset, 
        /* length:          */ len - offset, 
        /* instruction:     */ &instruction 
    ))) { 
        std::cerr << runtime_address << ": " << instruction.text << std::endl;
        offset += instruction.info.length; 
        runtime_address += instruction.info.length; 
    } 
    //The assert will fail when disassembling jumptable
    //assert(offset == len);
}


struct _CodeGen : Xbyak::CodeGenerator {
    char tmp_buf[0x1000];
    _CodeGen() : Xbyak::CodeGenerator(sizeof(tmp_buf), tmp_buf){};
};
static _CodeGen code_generator;


tlsf_t tlsf_alloc;
// probably need to move this inside codemanager since we need to free it there
//TODO: handle oom
static void* _rwx_malloc(size_t size) {
    return tlsf_malloc(tlsf_alloc, size);
}

static void _rwx_free(void* ptr) {
    tlsf_free(tlsf_alloc, ptr);
}

constexpr size_t RWX_SIZE = 0x4000;
void compiler_init() {
    void* mem = alloc_rwx(RWX_SIZE);
    tlsf_alloc = tlsf_create_with_pool(mem, RWX_SIZE);

}

void* poc_compiler(const code_pos_t& code_pos) {
    using namespace Xbyak::util;
    //reset temporary code generator
    code_generator.reset();

    //walk until control-flow changes or we wrap back to the start
    code_pos_t start_pos = code_pos;
    coord_t start_coord = start_pos.to_coord();
    code_pos_t cur_pos = code_pos;

    auto advance_pos = [&](code_pos_t pos, Dir::DirType dir, int steps = 1) {
        code_pos_t delta = codepos_from_dir(dir);
        return code_pos_t(pos.x + delta.x * steps, pos.y + delta.y * steps, dir);
    };

    uint64 jump_target = 0;
    bool emitted_jump = false;

    auto call_extern = [&](uint64 addr) {
        code_generator.mov(rax, addr);
        code_generator.mov(rbx, rsp);
        code_generator.and_(rbx, 8);
        code_generator.sub(rsp, rbx);
        code_generator.call(rax);
        code_generator.add(rsp, rbx);
    };
    std::cerr << "start of compilation of thunk: dir: " << code_pos.dir << " x: " << code_pos.x << " y: " << code_pos.y << std::endl;
    while (true) {
        // Track that this compiled function depends on the current cell.
        G::code_manager.AddDependency(cur_pos.to_coord(), start_pos);
        

        const unsigned char instr = G::static_memory[cur_pos.x][cur_pos.y];
        std::cerr << "compiling command char: " << instr << std::endl;
        switch (instr) {
        case '+':// Preserve stack alignment for external C calls.
            code_generator.pop(rax);
            code_generator.pop(rbx);
            code_generator.add(rax, rbx);
            code_generator.push(rax);
            break;
        case '-':
            code_generator.pop(rax);
            code_generator.pop(rbx);
            code_generator.sub(rbx, rax);
            code_generator.push(rbx);
            break;
        case '*':
            code_generator.pop(rax);
            code_generator.pop(rbx);
            code_generator.mul(rbx);
            code_generator.push(rax);
            break;
        case '/':
            code_generator.pop(rbx);
            code_generator.pop(rax);
            code_generator.xor_(rdx, rdx);
            code_generator.div(rbx);
            code_generator.push(rax);
            break;
        case '%':
            code_generator.pop(rbx);
            code_generator.pop(rax);
            code_generator.xor_(rdx, rdx);
            code_generator.div(rbx);
            code_generator.push(rdx);
            break;
        case ':':
            code_generator.pop(rax);
            code_generator.push(rax);
            code_generator.push(rax);
            break;
        case '\\':
            code_generator.pop(rax);
            code_generator.pop(rbx);
            code_generator.push(rax);
            code_generator.push(rbx);
            break;
        case '$':
            code_generator.pop(rax);
            break;
        case '!':
            code_generator.pop(rax);
            code_generator.xor_(rcx, rcx);
            code_generator.cmp(rax, 0);
            code_generator.sete(cl);
            code_generator.push(rcx);
            break;
        case '`':
            code_generator.pop(rax);
            code_generator.pop(rbx);
            code_generator.xor_(rcx, rcx);
            code_generator.cmp(rbx, rax);
            code_generator.setg(cl);
            code_generator.push(rcx);
            break;
        case '&':
            call_extern(G::p_extern_getint);
            code_generator.push(rax);
            break;
        case '~':
            call_extern(G::p_extern_getchar);
            code_generator.push(rax);
            break;
        case '.':
            code_generator.pop(rdi);
            call_extern(G::p_extern_printint);
            break;
        case ',':
            code_generator.pop(rdi);
            call_extern(G::p_extern_printchar);
            break;
        case '@':
            call_extern(G::p_extern_exit);
            code_generator.ud2(); // should not return
            emitted_jump = true;
            break;
        case 'g':
            code_generator.pop(rdi); // y
            code_generator.pop(rsi); // x
            call_extern(G::p_extern_read_val);
            code_generator.push(rax);
            break;
        case 'p':
            code_generator.pop(rdi); // y
            code_generator.pop(rsi); // x
            code_generator.pop(rdx); // v
            call_extern(G::p_extern_write_val);
            break;
        case '>':
            jump_target = G::code_manager.GetThunkAddress(advance_pos(cur_pos, Dir::RIGHT));
            break;
        case '<':
            jump_target = G::code_manager.GetThunkAddress(advance_pos(cur_pos, Dir::LEFT));
            break;
        case '^':
            jump_target = G::code_manager.GetThunkAddress(advance_pos(cur_pos, Dir::UP));
            break;
        case 'v':
            jump_target = G::code_manager.GetThunkAddress(advance_pos(cur_pos, Dir::DOWN));
            break;
        case '#':
            jump_target = G::code_manager.GetThunkAddress(advance_pos(cur_pos, cur_pos.dir, 2));
            break;
        case '_': {
            code_pos_t right = advance_pos(cur_pos, Dir::RIGHT);
            code_pos_t left = advance_pos(cur_pos, Dir::LEFT);
            uint64 right_addr = G::code_manager.GetThunkAddress(right);
            uint64 left_addr = G::code_manager.GetThunkAddress(left);
            code_generator.pop(rax);
            code_generator.cmp(rax, 0);
            code_generator.mov(rax, right_addr);
            Xbyak::Label after;
            code_generator.je(after);
            code_generator.mov(rax, left_addr);
            code_generator.L(after);
            code_generator.jmp(rax);
            emitted_jump = true;
            break;
        }
        case '|': {
            code_pos_t down = advance_pos(cur_pos, Dir::DOWN);
            code_pos_t up = advance_pos(cur_pos, Dir::UP);
            uint64 down_addr = G::code_manager.GetThunkAddress(down);
            uint64 up_addr = G::code_manager.GetThunkAddress(up);
            code_generator.pop(rax);
            code_generator.cmp(rax, 0);
            code_generator.mov(rax, down_addr);
            Xbyak::Label after;
            code_generator.je(after);
            code_generator.mov(rax, up_addr);
            code_generator.L(after);
            code_generator.jmp(rax);
            emitted_jump = true;
            break;
        }
        case '?':
        {
            uint64 addr_up = G::code_manager.GetThunkAddress(advance_pos(cur_pos, Dir::UP));
            uint64 addr_down = G::code_manager.GetThunkAddress(advance_pos(cur_pos, Dir::DOWN));
            uint64 addr_left = G::code_manager.GetThunkAddress(advance_pos(cur_pos, Dir::LEFT));
            uint64 addr_right = G::code_manager.GetThunkAddress(advance_pos(cur_pos, Dir::RIGHT));

            Xbyak::Label jpt;
            call_extern(G::p_extern_rand);
            code_generator.and_(rax, 0x3);
            code_generator.lea(rbx, ptr[rip + jpt]);
            code_generator.mov(rax, ptr[rbx + rax * 8]);
            code_generator.jmp(rax);
            code_generator.L(jpt);
            code_generator.dq(addr_up);
            code_generator.dq(addr_down);
            code_generator.dq(addr_left);
            code_generator.dq(addr_right);
            emitted_jump = true;
            break;
        }
        case '"':
        {
            code_pos_t scan_pos = advance_pos(cur_pos, cur_pos.dir);
            // Walk forward, pushing each character until the next quote. Stop if we wrap back to the start to avoid infinite loops.
            while (true) {
                G::code_manager.AddDependency(scan_pos.to_coord(), start_pos);
                unsigned char c = G::static_memory[scan_pos.x][scan_pos.y];
                if (c == '\"') {
                    code_pos_t after_quote = advance_pos(scan_pos, scan_pos.dir);
                    jump_target = G::code_manager.GetThunkAddress(after_quote);
                    break;
                }
                code_generator.push(static_cast<int>(c));
                scan_pos = advance_pos(scan_pos, scan_pos.dir);
                if (scan_pos == start_pos) {
                    // Unterminated string; jump back to start to avoid infinite compilation walk.
                    jump_target = G::code_manager.GetThunkAddress(start_pos);
                    break;
                }
            }
            break;
        }
        default:
            if (instr >= '0' && instr <= '9') {
                code_generator.push(static_cast<int>(instr - '0'));
            }
            break;
        }

        if (jump_target || emitted_jump) {
            break;
        }

        cur_pos = advance_pos(cur_pos, cur_pos.dir);

        // If we wrapped all the way around, emit a jump back to the start thunk.
        if (cur_pos.to_coord() == start_coord) {
            jump_target = G::code_manager.GetThunkAddress(start_pos);
            break;
        }
    }

    if (!emitted_jump) {
        // Jump to the thunk for the resolved destination.
        code_generator.mov(rax, jump_target);
        code_generator.jmp(rax);
    }
    
    size_t code_size = code_generator.getSize();
    void* code_mem = _rwx_malloc(code_size);
    assert(code_mem != nullptr);
    std::cerr << "tmp buf " << std::hex << (uint64)code_generator.tmp_buf << std::endl;
    std::cerr << "end of compilation " << std::endl;
    std::memcpy(code_mem, code_generator.tmp_buf, code_size);
    __DebugDumpCode(code_mem, code_size);

    return code_mem;

}

void* __attribute_noinline__ compile (const code_pos_t& code_pos) {
    return poc_compiler(code_pos);
}
