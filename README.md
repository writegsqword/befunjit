# befunjit
 jit compiler for meme language 
 **NATIVE** btw 😎
# how work???  
## 1. thunk table  
this is a jump table every entry is a snippet of code:
```asm
movabs rax, addr
call/jmp rax
```
addr initially points to the compile function(it knows which position to compile from the return address)
after compilation, the compiled position will have their entries in the table replaced by the jump instruction
and its addr replaced with the code pos

this table also serves as a table for info(i know its bad practice but come on its really kinda cool)
namely, it is used for: 
a. detecting if this entry has been jit compiled(check if the last bit of code is a jump or a call) and
b. freeing the allocated code based on checking addr

when a control flow instruction is executed, it jumps to the corresponding position in the table where it either jumps to compiled code(valid entries)
or calls the compiler function, where the code then gets compiled dynamically  

when the p instruction is invoked, all affected positions are invalidated, with their corresponding entries in the jump table reverted to a call to 
the compiler, where it can then be recompiled if needed    

# todo 
underflow protection
