#pragma once
#include "typedefs.h"


void extern_put(uint64 x, uint64 y, uint64 v);

uint64 extern_getchar();
uint64 extern_getint();

void extern_printint(uint64 v);

void extern_printchar(uint64 v);
uint64 extern_rand();
uint64 extern_read_val(uint64 y, uint64 x);
void extern_write_val(uint64 y, uint64 x, uint64 v);
uint64 extern_exit();