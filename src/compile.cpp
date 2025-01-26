#include "compile.h"
#include "thunk.h"
#include "utils.h"
#include "snippets.h"


#define DEF_INSTRUCTION(ins_name) instructions::_##ins_name i_##ins_name = instructions::_##ins_name();
#define WRITE_INSTRUCTION(ins_name) c = (byte*) &i_##ins_name; size = sizeof(i_##ins_name);
#define DEF_WRITE(ins_name) DEF_INSTRUCTION(ins_name); WRITE_INSTRUCTION(ins_name)
#define DEF_WRITE_BREAK(ins_name) { DEF_WRITE(ins_name) break; }

uint64 __attribute_noinline__ resolve_thunk(uint64 address) {

    //decrement by the offset caused by call instruction
    address -= 5;
    //write address to stub entry
    //
    //returns ABSOLUTE address
    std::cerr << "called from address: " << std::hex << address << std::endl;
    void* res_addr = compile(ThunkManager::_instance->GetCodePos(address));
    return address;
}



std::vector<std::vector<byte>> memory_leaker; 
void* __attribute_noinline__ compile (const code_pos_t& code_pos) {

    //recompile for current 
    Vec2 rpos = code_pos.second, inc = vec_from_dir(code_pos.first);
    Vec2 start_pos = rpos;
    std::vector<byte> code_res;
    std::vector<uint64> offsets;
    //im lazy sorry
    std::vector<Vec2> seen_poses;
    bool stop = false;
    bool started = false;
    std::cerr << "dir type: " << code_pos.first << std::endl;
    while(!stop) {
        if(start_pos == rpos)
            if(started)
                break;
        started = true;
        
        byte b = G::static_memory[rpos.x][rpos.y];
        seen_poses.push_back(rpos);
        offsets.push_back(code_res.size());
        
        byte* c = nullptr;
        int size = 0;
        std::cerr << "char: " << b << "    pos: x: " <<  rpos.x << ", y: " <<  rpos.y << std::endl;
        switch(b) {
            case '+': 
                DEF_WRITE_BREAK(add);
            case '-':
                DEF_WRITE_BREAK(sub);
            case '*':
                DEF_WRITE_BREAK(mul);
            case '/':
                DEF_WRITE_BREAK(div);
            case '%':
                DEF_WRITE_BREAK(mod);
            case ':':
                DEF_WRITE_BREAK(dup_top);
            case '\\':
                DEF_WRITE_BREAK(swap_top);
            case '$':
                DEF_WRITE_BREAK(pop_top);
            case '!':
                DEF_WRITE_BREAK(not);
            case '`':
                DEF_WRITE_BREAK(greater);
            case '&':
                DEF_WRITE_BREAK(push_get_int);
            case '~':
                DEF_WRITE_BREAK(push_get_char);
            case '.':
                DEF_WRITE_BREAK(pop_print_int);
            case ',':
                DEF_WRITE_BREAK(pop_print_char);
            case '@':
                DEF_WRITE_BREAK(exit);
            case 'g':
                DEF_WRITE_BREAK(read_val);
            case 'p':
            {
                //in case self gets modified
                DirType next_dir = code_pos.first;
                DEF_INSTRUCTION(write_val);
                i_write_val.jump.addr = ThunkManager::_instance->GetAddress(code_pos_t(next_dir, rpos + vec_from_dir(next_dir)));
                //comment out if issues
                //stop = true;
                WRITE_INSTRUCTION(write_val);
                break;
            }
                
            //pop and discard
            
            case '>':
            case '<':
            case '^':
            case 'v': 
            {

            
                DEF_INSTRUCTION(jump_to);
                DirType next_dir = dir_from_ascii(b);
                i_jump_to.addr = ThunkManager::_instance->GetAddress(code_pos_t(next_dir, rpos + vec_from_dir(next_dir)));
                stop = true;
                WRITE_INSTRUCTION(jump_to);
                break;
            }
            case '|': 
            {
                DEF_INSTRUCTION(jump_if);
                i_jump_if.jmp.movabs_rbx.val = ThunkManager::_instance->GetAddress(code_pos_t(DIR_DOWN, rpos + vec_from_dir(DIR_DOWN)));
                i_jump_if.jmp.movabs_rcx.val = ThunkManager::_instance->GetAddress(code_pos_t(DIR_UP, rpos + vec_from_dir(DIR_UP)));
                stop = true;
                WRITE_INSTRUCTION(jump_if);
                break;
            }
            case '_':
            {
                DEF_INSTRUCTION(jump_if);
                i_jump_if.jmp.movabs_rbx.val = ThunkManager::_instance->GetAddress(code_pos_t(DIR_RIGHT, rpos + vec_from_dir(DIR_RIGHT)));
                i_jump_if.jmp.movabs_rcx.val = ThunkManager::_instance->GetAddress(code_pos_t(DIR_LEFT, rpos + vec_from_dir(DIR_LEFT)));
                stop = true;
                WRITE_INSTRUCTION(jump_if);
                break;
            }
            case '#':
            {
                DEF_INSTRUCTION(jump_to);
                DirType next_dir = code_pos.first;
                i_jump_to.addr = ThunkManager::_instance->GetAddress(code_pos_t(next_dir, rpos + (vec_from_dir(next_dir) * 2)));
                stop = true;
                WRITE_INSTRUCTION(jump_to);
                break;
            }
            case '?': 
            {
                DEF_INSTRUCTION(jump_rand);
                i_jump_rand.movabs_0.val = ThunkManager::_instance->GetAddress(code_pos_t(DIR_UP, rpos + vec_from_dir(DIR_UP)));
                i_jump_rand.movabs_1.val = ThunkManager::_instance->GetAddress(code_pos_t(DIR_DOWN, rpos + vec_from_dir(DIR_DOWN)));
                i_jump_rand.movabs_2.val = ThunkManager::_instance->GetAddress(code_pos_t(DIR_LEFT, rpos + vec_from_dir(DIR_LEFT)));
                i_jump_rand.movabs_3.val = ThunkManager::_instance->GetAddress(code_pos_t(DIR_RIGHT, rpos + vec_from_dir(DIR_RIGHT)));
                stop = true;
                WRITE_INSTRUCTION(jump_rand);
                break;
            }
            case '"': 
            {
                byte ascii_frame[200] = { 0 };
                size_t ctr = 0;
                Vec2 cur_pos = rpos;
                cur_pos += inc;
                char nc = G::static_memory[cur_pos.x][cur_pos.y];
                //start scanning ahead
                while(nc != '"') {
                    //push imm8
                    ascii_frame[ctr] = 0x6a;
                    ctr++;
                    ascii_frame[ctr] = nc;
                    ctr++;
                    cur_pos += inc;
                    nc = G::static_memory[cur_pos.x][cur_pos.y];
                }
                //at this point, another " has just been hit
                //however we should not stop here, instead
                //we need to jump to the char that comes after "
                cur_pos += inc;
                DEF_INSTRUCTION(jump_to);
                //maintain dir
                DirType next_dir = code_pos.first;
                i_jump_to.addr = ThunkManager::_instance->GetAddress(code_pos_t(next_dir, cur_pos));
                stop = true;
                for(size_t s = 0; s < sizeof(i_jump_to); s++) {
                    ascii_frame[ctr] = *(((byte*)&i_jump_to) + s);
                    ctr++;
                }
                //maybe error here
                c = (byte*)&ascii_frame;
                size = ctr;
                stop = true;
                break;
            }
            

            
            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':
            {
                DEF_INSTRUCTION(push_num);
                i_push_num.num = b - '0';
                WRITE_INSTRUCTION(push_num);
                break;
            }
            
            default:
                break;
        };
        for(int i = 0; i < size; i++) {
            code_res.push_back(*(c + i));
        }
        rpos += inc;
        // assert(rpos.first >= 0);
        // assert(rpos.second >= 0);
        
    }
    assert(seen_poses.size() == offsets.size());
    //MEMORY LEAK!!
    byte* ptr = (byte*)alloc_rwx(code_res.size());

    std::cerr << std::dec << "code size: " << code_res.size() << "\nrwx code pointer: " << std::hex << (void*)ptr <<std::endl;
    std::copy(code_res.begin(), code_res.end(), ptr);
    for(int i = 0; i < seen_poses.size(); i++) {
        ThunkManager::_instance->ResolvePos((uint64)ptr + offsets[i], code_pos_t(code_pos.first, seen_poses[i]));
    };
    

    return ptr;



}
