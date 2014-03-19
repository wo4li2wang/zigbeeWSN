#ifndef _MEMALLOC_H_
#define _MEMALLOC_H_

void MemInit (void);
UINT8 * MemAlloc (UINT16 size);
void MemFree(UINT8 *ptr);


#ifdef LRWPAN_COMPILER_NO_RECURSION//编译器没有递归方式，有什么分别。

UINT8 * ISRMemAlloc (UINT16 size);
void ISRMemFree(UINT8 *ptr);

#else

#define ISRMemAlloc(x)   MemAlloc(x)
#define ISRMemFree(x)   MemFree(x)

#endif


#endif
