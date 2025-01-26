# befunjit
 jit compiler for meme language

# how work???  
program starts off with a call table that all points to the jit compiler function(invalid entries)    

when execution starts, the compiler function takes advantage of the fact that call pushes return address on stack,
which is used to resolve the location of the code to be compiled  

after compilation, all compiled positions will have their entries in the table replaced by a jump instruction that jumps to
the newly compiled code  

when a control flow instruction is executed, it jumps to the corresponding position in the table where it either jumps to compiled code(valid entries)
or calls the compiler function, where the code then gets compiled dynamically  

when the p instruction is invoked, all affected positions are invalidated, with their corresponding entries in the jump table reverted to a call to 
the compiler, where it can then be recompiled if needed    

# todo 
put different code segments into the same page  
clean up pages properly (leaks memory rn)  
smarter invalidation  
inter-instruction optimization
