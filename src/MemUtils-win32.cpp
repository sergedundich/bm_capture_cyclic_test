#if 1
#include "MemAlloc-generic.cpp"
#include "MemProtect-generic.cpp"
#else
//=====================================================================================================================
#include <stdint.h>
#include <stdio.h>
#include <new>
#include <MemUtils.h>
#include <windows.h>

//=====================================================================================================================
#if 0
#include "MemProtect-generic.cpp"
#else
//---------------------------------------------------------------------------------------------------------------------
#if UINTPTR_MAX > 0xffffffffUL
#define PRINTF_PTR_SIZE "16"
#else
#define PRINTF_PTR_SIZE "8"
#endif

//---------------------------------------------------------------------------------------------------------------------
void MemProtect( int index, void* ptr, size_t sz )
{
    DWORD old_protect = PAGE_NOACCESS;

    if( !::VirtualProtect( ptr, sz, PAGE_NOACCESS, &old_protect ) )
    {
        DWORD err = ::GetLastError();
        printf(  "[%d] VirtualProtect( 0x0" PRINTF_PTR_SIZE "llx, %lu, PAGE_NOACCESS,...) failed.\n",
                                                                index, (unsigned long long)ptr, (unsigned long)sz  );
    }

    if( old_protect != PAGE_READWRITE )
    {
        printf(  "\n[%d] Warning: buffer protection flags has been changed: "
                    "ptr=0x0" PRINTF_PTR_SIZE "llx, size=%lu\n",  index, (unsigned long long)ptr, (unsigned long)sz  );
    }
}

//---------------------------------------------------------------------------------------------------------------------
bool MemUnprotect( int index, void* ptr, size_t sz )
{
    DWORD old_protect = PAGE_READWRITE;

    if( !::VirtualProtect( ptr, sz, PAGE_READWRITE, &old_protect ) )
    {
        printf(  "\n[%d] VirtualProtect( 0x0" PRINTF_PTR_SIZE "llx, %lu, PAGE_READWRITE,...) failed.\n",
                                                                index, (unsigned long long)ptr, (unsigned long)sz  );
        return false;
    }

    if( old_protect != PAGE_NOACCESS )
    {
        printf(  "\n[%d] ALERT!!! Buffer protection flags has been changed when it is already released: "
                    "ptr=0x0" PRINTF_PTR_SIZE "llx, size=%lu\n",  index, (unsigned long long)ptr, (unsigned long)sz  );
        return false;
    }

    return true;
}

#endif
//=====================================================================================================================
void* MemAlloc( size_t sz )
{
    void* ptr = ::VirtualAlloc( NULL, sz, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE );

    if( ptr == NULL )
    {
        throw std::bad_alloc();
    }

    return ptr;
}

//---------------------------------------------------------------------------------------------------------------------
void MemFree( void* ptr )
{
    ::VirtualFree( ptr, 0, MEM_RELEASE );
}

#endif
