#include "compile.h"
#include "thunk.h"
#include "typedefs.h"
#include "utils.h"
#include "snippets.h"
#include "vec.h"
#include "xbyak.h"
#include "tlsf.h"
#include <cassert>
#include <cstddef>
#include <cstring>

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
    code_pos_t cur_pos = code_pos;

    auto advance_pos = [&](code_pos_t pos, Dir::DirType dir, int steps = 1) {
        code_pos_t delta = codepos_from_dir(dir);
        return code_pos_t(pos.x + delta.x * steps, pos.y + delta.y * steps, dir);
    };

    uint64 jump_target = 0;
    bool emitted_jump = false;

    while (true) {
        // Track that this compiled function depends on the current cell.
        G::code_manager.AddDependency(cur_pos.to_coord(), start_pos);

        const unsigned char instr = G::static_memory[cur_pos.x][cur_pos.y];
        switch (instr) {
        case '+':
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
            code_generator.setne(cl);
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
            code_generator.mov(rax, G::p_extern_getint);
            code_generator.call(rax);
            code_generator.push(rax);
            break;
        case '~':
            code_generator.mov(rax, G::p_extern_getchar);
            code_generator.call(rax);
            code_generator.push(rax);
            break;
        case '.':
            code_generator.pop(rdi);
            code_generator.mov(rax, G::p_extern_printint);
            code_generator.call(rax);
            break;
        case ',':
            code_generator.pop(rdi);
            code_generator.mov(rax, G::p_extern_printchar);
            code_generator.call(rax);
            break;
        case '@':
            code_generator.mov(rax, G::p_extern_exit);
            code_generator.call(rax);
            code_generator.ud2(); // should not return
            emitted_jump = true;
            break;
        case 'g':
            code_generator.pop(rdi); // y
            code_generator.pop(rsi); // x
            code_generator.mov(rax, G::p_extern_read_val);
            code_generator.call(rax);
            code_generator.push(rax);
            break;
        case 'p':
            code_generator.pop(rdx); // v
            code_generator.pop(rdi); // y
            code_generator.pop(rsi); // x
            code_generator.mov(rax, G::p_extern_write_val);
            code_generator.call(rax);
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
            code_generator.mov(rax, G::p_extern_rand);
            code_generator.call(rax);
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
        if (cur_pos == start_pos) {
            jump_target = G::code_manager.GetThunkAddress(start_pos);
            break;
        }
    }

    if (!emitted_jump) {
        // Jump to the thunk for the resolved destination.
        code_generator.mov(rax, jump_target);
        code_generator.jmp(rax);
    }
    
    
    void* code_mem = _rwx_malloc(code_generator.getSize());
    assert(code_mem != nullptr);
    std::cerr << "tmp buf " << std::hex << (uint64)code_generator.tmp_buf << std::endl;
    std::memcpy(code_mem, code_generator.tmp_buf, code_generator.getSize());
    return code_mem;

}

void* __attribute_noinline__ compile (const code_pos_t& code_pos) {
    return poc_compiler(code_pos);
}


//jumping in the middle is kinda cool but also kinda awful for like a million reasons 
// void* __attribute_noinline__ __compile (const code_pos_t& code_pos) {

//     //recompile for current 
//     Vec2 rpos = code_pos.second, inc = vec_from_dir(code_pos.first);
//     Vec2 start_pos = rpos;
//     std::vector<byte> code_res;
//     std::vector<uint64> offsets;
//     //im lazy sorry
//     std::vector<Vec2> seen_poses;
//     bool stop = false;
//     bool started = false;
//     std::cerr << "dir type: " << code_pos.first << std::endl;
//     while(!stop) {
//         if(start_pos == rpos)
//             if(started)
//                 break;
//         started = true;
        
//         byte b = G::static_memory[rpos.x][rpos.y];
//         seen_poses.push_back(rpos);
//         offsets.push_back(code_res.size());
        
//         byte* c = nullptr;
//         int size = 0;
//         std::cerr << "char: " << b << "    pos: x: " <<  rpos.x << ", y: " <<  rpos.y << std::endl;
//         switch(b) {
//             case '+': 
//                 DEF_WRITE_BREAK(add);
//             case '-':
//                 DEF_WRITE_BREAK(sub);
//             case '*':
//                 DEF_WRITE_BREAK(mul);
//             case '/':
//                 DEF_WRITE_BREAK(div);
//             case '%':
//                 DEF_WRITE_BREAK(mod);
//             case ':':
//                 DEF_WRITE_BREAK(dup_top);
//             case '\\':
//                 DEF_WRITE_BREAK(swap_top);
//             case '$':
//                 DEF_WRITE_BREAK(pop_top);
//             case '!':
//                 DEF_WRITE_BREAK(not);
//             case '`':
//                 DEF_WRITE_BREAK(greater);
//             case '&':
//                 DEF_WRITE_BREAK(push_get_int);
//             case '~':
//                 DEF_WRITE_BREAK(push_get_char);
//             case '.':
//                 DEF_WRITE_BREAK(pop_print_int);
//             case ',':
//                 DEF_WRITE_BREAK(pop_print_char);
//             case '@':
//                 DEF_WRITE_BREAK(exit);
//             case 'g':
//                 DEF_WRITE_BREAK(read_val);
//             case 'p':
//             {
//                 //in case self gets modified
//                 Dir::DirType next_dir = code_pos.dir;
//                 DEF_INSTRUCTION(write_val);
//                 i_write_val.jump.addr = G::code_manager->GetAddress(code_pos_t(next_dir, rpos + vec_from_dir(next_dir)));
//                 //comment out if issues
//                 //stop = true;
//                 WRITE_INSTRUCTION(write_val);
//                 break;
//             }
                
//             //pop and discard
            
//             case '>':
//             case '<':
//             case '^':
//             case 'v': 
//             {

            
//                 DEF_INSTRUCTION(jump_to);
//                 Dir::DirType next_dir = Dir::from_ascii(b);
//                 i_jump_to.addr = G::code_manager.GetAddress(code_pos_t(next_dir, rpos + vec_from_dir(next_dir)));
//                 stop = true;
//                 WRITE_INSTRUCTION(jump_to);
//                 break;
//             }
//             case '|': 
//             {
//                 DEF_INSTRUCTION(jump_if);
//                 i_jump_if.jmp.movabs_rbx.val = G::code_manager.GetAddress(code_pos_t(Dir::DOWN, rpos + vec_from_dir(Dir::Dir::DOWN)));
//                 i_jump_if.jmp.movabs_rcx.val = G::code_manager.GetAddress(code_pos_t(Dir::UP, rpos + vec_from_dir(Dir::UP)));
//                 stop = true;
//                 WRITE_INSTRUCTION(jump_if);
//                 break;
//             }
//             case '_':
//             {
//                 DEF_INSTRUCTION(jump_if);
//                 i_jump_if.jmp.movabs_rbx.val = G::code_manager.GetAddress(code_pos_t(Dir::RIGHT, rpos + vec_from_dir(Dir::RIGHT)));
//                 i_jump_if.jmp.movabs_rcx.val = G::code_manager.GetAddress(code_pos_t(Dir::LEFT, rpos + vec_from_dir(Dir::LEFT)));
//                 stop = true;
//                 WRITE_INSTRUCTION(jump_if);
//                 break;
//             }
//             case '#':
//             {
//                 DEF_INSTRUCTION(jump_to);
//                 Dir::DirType next_dir = code_pos.dir;
//                 i_jump_to.addr = G::code_manager.GetAddress(code_pos_t(next_dir, rpos + (vec_from_dir(next_dir) * 2)));
//                 stop = true;
//                 WRITE_INSTRUCTION(jump_to);
//                 break;
//             }
//             case '?': 
//             {
//                 DEF_INSTRUCTION(jump_rand);
//                 i_jump_rand.movabs_0.val = G::code_manager.GetAddress(code_pos_t(Dir::UP, rpos + vec_from_dir(Dir::UP)));
//                 i_jump_rand.movabs_1.val = G::code_manager.GetAddress(code_pos_t(Dir::DOWN, rpos + vec_from_dir(Dir::DOWN)));
//                 i_jump_rand.movabs_2.val = G::code_manager.GetAddress(code_pos_t(Dir::LEFT, rpos + vec_from_dir(Dir::LEFT)));
//                 i_jump_rand.movabs_3.val = G::code_manager.GetAddress(code_pos_t(Dir::RIGHT, rpos + vec_from_dir(Dir::RIGHT)));
//                 stop = true;
//                 WRITE_INSTRUCTION(jump_rand);
//                 break;
//             }
//             case '"': 
//             {
//                 byte ascii_frame[200] = { 0 };
//                 size_t ctr = 0;
//                 Vec2 cur_pos = rpos;
//                 cur_pos += inc;
//                 char nc = G::static_memory[cur_pos.x][cur_pos.y];
//                 //start scanning ahead
//                 while(nc != '"') {
//                     //push imm8
//                     ascii_frame[ctr] = 0x6a;
//                     ctr++;
//                     ascii_frame[ctr] = nc;
//                     ctr++;
//                     cur_pos += inc;
//                     nc = G::static_memory[cur_pos.x][cur_pos.y];
//                 }
//                 //at this point, another " has just been hit
//                 //however we should not stop here, instead
//                 //we need to jump to the char that comes after "
//                 cur_pos += inc;
//                 DEF_INSTRUCTION(jump_to);
//                 //maintain dir
//                 DirType next_dir = code_pos.first;
//                 i_jump_to.addr = G::code_manager.GetAddress(code_pos_t(next_dir, cur_pos));
//                 stop = true;
//                 for(size_t s = 0; s < sizeof(i_jump_to); s++) {
//                     ascii_frame[ctr] = *(((byte*)&i_jump_to) + s);
//                     ctr++;
//                 }
//                 //maybe error here
//                 c = (byte*)&ascii_frame;
//                 size = ctr;
//                 stop = true;
//                 break;
//             }
            

            
//             case '0':
//             case '1':
//             case '2':
//             case '3':
//             case '4':
//             case '5':
//             case '6':
//             case '7':
//             case '8':
//             case '9':
//             {
//                 DEF_INSTRUCTION(push_num);
//                 i_push_num.num = b - '0';
//                 WRITE_INSTRUCTION(push_num);
//                 break;
//             }
            
//             default:
//                 break;
//         };
//         for(int i = 0; i < size; i++) {
//             code_res.push_back(*(c + i));
//         }
//         rpos += inc;
//         // assert(rpos.first >= 0);
//         // assert(rpos.second >= 0);
        
//     }
//     assert(seen_poses.size() == offsets.size());
//     //MEMORY LEAK!!
//     byte* ptr = (byte*)alloc_rwx(code_res.size());

//     std::cerr << std::dec << "code size: " << code_res.size() << "\nrwx code pointer: " << std::hex << (void*)ptr <<std::endl;
//     std::copy(code_res.begin(), code_res.end(), ptr);
//     for(int i = 0; i < seen_poses.size(); i++) {
//         G::code_manager.ResolvePos((uint64)ptr + offsets[i], code_pos_t(code_pos.first, seen_poses[i]));
//     };
    

//     return ptr;



// }
