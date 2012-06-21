#ifndef __TEBAHPLA_H__
#define __TEBAHPLA_H__

#define NULL_CHAR ((char)-1)
#include "common_defs.h"
//#include "stdbool.h"

#define bool _Bool
#define false 0
#define true 1
CHARACTER getCHAR(char ch,bool selected);
void * my_memcpy (void *destaddr, void const *srcaddr, int len);
#endif
