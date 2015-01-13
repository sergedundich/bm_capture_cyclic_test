#ifndef MEM_UTILS__H__
#define MEM_UTILS__H__
#include <stddef.h>

void* MemAlloc( size_t sz );
void MemFree( void* ptr );

void MemProtect( int index, void* ptr, size_t sz );
bool MemUnprotect( int index, void* ptr, size_t sz );

#endif // !defined(MEM_UTILS__H__)
