#pragma once

#include "typedefs.h"
#include "externs.h"
#include <array>
#include <iostream>




namespace instructions {




    //WHY IS THIS A THING
    constexpr byte __push_rax = 0x50;
    constexpr byte __push_rbx = 0x53;
    constexpr byte __push_rcx = 0x51;
    constexpr byte __push_rdx = 0x52;
    
    constexpr byte __pop_rax = 0x58;
    constexpr byte __pop_rbx = 0x5b;
    constexpr byte __pop_rdx = 0x5a;
    constexpr byte __pop_rdi = 0x5f;
    constexpr byte __pop_rsi = 0x5e;
    constexpr byte __ret = 0xc3;

    struct __attribute__((packed)) __movabs_rax  {
        byte movabs[2] = { 0x48, 0xb8 };
        uint64 val = 0;
    };
    struct __attribute__((packed)) __movabs_rbx  {
        byte movabs[2] = { 0x48, 0xbb };
        uint64 val = 0;
    };
    struct __attribute__((packed)) __movabs_rcx  {
        byte movabs[2] = { 0x48, 0xb9 };
        uint64 val = 0;
    };
    
    struct __attribute__((packed)) __movabs_rdx  {
        byte movabs[2] = { 0x48, 0xba };
        uint64 val = 0;
    };
    


    struct __attribute__((packed)) __common_pop  {
        byte pop_rax = __pop_rax, pop_rbx = __pop_rbx;
    };

    struct __attribute__((packed)) __align_rsp {
        byte mov_rbx_rsp[3] = {0x48, 0x89, 0xe3};
        //byte shr_rbx_3[4] = {0x48, 0xc1, 0xeb, 0x03};
        byte and_rbx_8[4] = {0x48, 0x83, 0xe3, 0x08};
        //byte xor_rbx_8[4] = {0x48, 0x83, 0xf3, 0x08};
        byte sub_rsp_rbx[3] = {0x48, 0x29, 0xdc};
        //byte cc = 0xcc;
    };

    struct __attribute__((packed)) __restore_rsp {
        byte add_rsp_rbx[3] = {0x48, 0x01, 0xdc};
    };

    struct __attribute__((packed)) __common_call { 
        
        byte movabs[2] = { 0x48, 0xb8 };
        uint64 addr = 0;

        __align_rsp align;

        byte call_rax[2] = {0xff, 0xd0};
        
        __restore_rsp restore;
    } __attribute__((packed));

    

    //+
    struct __attribute__((packed)) _add {
        __common_pop __pop;
        byte __add[3] = {0x48, 0x01, 0xd8};
        byte __push = __push_rax;
    };

    //TODO: fix incorrect instructions
    //(e.g. b-a instead of a-b)
    struct __attribute__((packed)) _sub {
        __common_pop __pop;
        byte __sub[3] = {0x48, 0x29, 0xc3};
        byte __push = __push_rbx;
    };

    struct __attribute__((packed)) _mul {
        __common_pop __pop;
        //mul rbx(implicit rax)
        byte __mul[3] = {0x48, 0xf7, 0xe3};
        byte __push = __push_rax;
    } __attribute__((packed));;

    struct __attribute__((packed)) _div {
        //__common_pop __pop;
        byte pop_rbx = __pop_rbx;
        byte pop_rax = __pop_rax;
        //IMAGINE
        byte clear_rdx[3] = {0x48, 0x31, 0xd2}; //                xor    rdx,rdx 
        //div rbx
        byte __div[3] = {0x48, 0xf7, 0xf3};
        byte __push = __push_rax;
    } __attribute__((packed));;
    struct __attribute__((packed)) _mod {
        //__common_pop __pop;
        byte pop_rbx = __pop_rbx;
        byte pop_rax = __pop_rax;
        byte clear_rdx[3] = {0x48, 0x31, 0xd2}; //                xor    rdx,rdx 
        byte __div[3] = {0x48, 0xf7, 0xf3};
        byte __push = __push_rdx;

    } __attribute__((packed));

    struct __attribute__((packed)) _not {
        //cmp rax, 0
        /*
        0:  58                      pop    rax
        1:  48 31 c9                xor    rcx,rcx
        4:  48 83 f8 00             cmpdiv    rax,0x0
        8:  0f 95 c1                setne  cl
        b:  51                      push   rcx 
        */
       byte __instr[12] = {
            0x58,
            0x48, 0x31, 0xc9,
            0x48, 0x83, 0xf8, 0x00,
            0x0f, 0x95, 0xc1, 
            0x51,
       };

    } __attribute__((packed));

    /*
    0:  58                      pop    rax
    1:  5b                      pop    rbx
    2:  48 31 c9                xor    rcx,rcx
    5:  48 39 c3                cmp    rbx,rax
    8:  0f 9f c1                setg   cl
    b:  51                      push   rcx 

    */
    struct __attribute__((packed)) _greater {
        __common_pop __pop;
        byte __instr[10] = {
            0x48, 0x31, 0xc9,
            0x48, 0x39, 0xc3,
            0x0f, 0x9f, 0xc1,
            __push_rcx,
        };

    } __attribute__((packed));


    struct __attribute__((packed)) _jump_to {
        byte movabs_rax[2] = {
        0x48, 0xb8};
        //movabs rax, addr
        uint64 addr = 0;
        byte __push = __push_rax;
        byte ret = __ret;

    };
    struct __attribute__((packed)) __jump_check_zf {
        __movabs_rbx movabs_rbx;
        __movabs_rcx movabs_rcx;
        byte cmove_rbx[4] = { 0x48, 0x0f, 0x44, 0xc3 };
        byte cmovne_rcx[4] = { 0x48, 0x0f, 0x45, 0xc1 };
        //push rax
        byte __push = __push_rax;
        //ret
        byte ret = __ret;

    };
    struct __attribute__((packed)) _jump_if {

        byte pop_rax = __pop_rax;
        //byte cmp_rax_0[3] = { 0x48, 0x39, 0xc3 };
        byte cmp_rax_0[4] = { 0x48, 0x83, 0xf8, 0x00 };
        //cmp rax, 0
        __jump_check_zf jmp;

    };

    


    struct __attribute__((packed))  _dup_top {
        byte pop = __pop_rax;
        byte pushes[2] = {
            __push_rax,
            __push_rax
        };
    };

    struct __attribute__((packed)) _swap_top {
        byte pops[2] = {
            __pop_rax,
            __pop_rbx
        };
        byte pushes[2] = {
            __push_rax,
            __push_rbx
        };
    };

    struct __attribute__((packed)) _pop_top {
        byte pop = __pop_rax;
    };


    struct __attribute__((packed)) __pop_call {
        byte __pop = __pop_rdi;
        __common_call call;
    } __attribute__((packed));

    struct __attribute__((packed)) _pop_print_int {

        __pop_call call;
        inline _pop_print_int() {
            
            //call.call.addr = G::p_extern_printint;
        }

    };

    struct __attribute__((packed)) _pop_print_char {
        __pop_call call;
        inline _pop_print_char() {
            //call.call.addr = G::p_extern_printchar;
        }
    };
    struct __attribute__((packed)) __write_val_intern {
        byte __pop_a0 = __pop_rdi;
        byte __pop_a1 = __pop_rsi;
        byte __pop_a2 = __pop_rdx;
        __common_call call;
        inline __write_val_intern() {
            //call.addr = G::p_extern_write_val;
        }
    };
    struct __attribute__((packed)) _write_val {
        __write_val_intern __write;
        //jump to itself
        _jump_to jump;
    };


    struct __attribute__((packed)) __push_get_from_call { 
        __common_call call;
        byte push = __push_rax;
    };

    struct __attribute__((packed)) _jump_rand {
        __push_get_from_call push;
        byte pop_rsi = __pop_rsi;
        __movabs_rax movabs_0;
        __movabs_rbx movabs_1;
        __movabs_rcx movabs_2;
        __movabs_rdx movabs_3;
        byte cmp_rsi_0[4] = { 0x48, 0x83, 0xfe, 0x00 };
        byte cmove_rax[4] = { 0x48, 0x0f, 0x44, 0xf0 };
        byte cmp_rsi_1[4] = { 0x48, 0x83, 0xfe, 0x01 };
        byte cmove_rbx[4] = { 0x48, 0x0f, 0x44, 0xf3 };
        byte cmp_rsi_2[4] = { 0x48, 0x83, 0xfe, 0x02 };
        byte cmove_rcx[4] = { 0x48, 0x0f, 0x44, 0xf1 };
        byte cmp_rsi_3[4] = { 0x48, 0x83, 0xfe, 0x03 };
        byte cmove_rdx[4] = { 0x48, 0x0f, 0x44, 0xf2 };
        byte push_rsi = 0x56;
        byte ret = __ret;
        inline _jump_rand() {
            //push.call.addr = G::p_extern_rand;
        }  
    };


    struct __attribute__((packed)) _push_get_int {
        __push_get_from_call push;
        inline _push_get_int() {
            //push.call.addr = G::p_extern_getint;
        }  
    };

    struct __attribute__((packed)) _push_get_char {
        __push_get_from_call push;
        inline _push_get_char() {
            //push.call.addr = G::p_extern_getchar;
        }  
    };
    struct __attribute__((packed)) _read_val {
        byte __pop_a0 = __pop_rdi;
        byte __pop_a1 = __pop_rsi;
        __common_call call;
        byte push_rax = __push_rax;
        inline _read_val() {
            //call.addr = G::p_extern_read_val;
        }
    };

    struct __attribute__((packed)) _exit {
        __common_call call;
        inline _exit() {
            //call.addr = G::p_extern_exit;
        }
    };

    struct __attribute__((packed)) _push_num {
        byte __push = 0x6a;
        byte num = 0;
    };

    struct __attribute__((packed)) compiler_dispatch
    {

        // pop rdi(get caller address as arg)
        byte pop_rdi = 0x5f;

        byte movabs_rax[2] = {
            0x48, 0xb8};
        uint64 resolve_thunk_ptr = 0;
        //align 
        instructions::__align_rsp align;

        byte call_rax[2] = {
            0xff, 0xd0
        };
        
        instructions::__restore_rsp restore;
        byte jmp_rax[2] = {0xff, 0xe0}; 

    };
}