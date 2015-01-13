#include <stdint.h>
#include <stdio.h>
#include <new>
#include <MemUtils.h>

//=====================================================================================================================
void* MemAlloc( size_t sz )
{
    void* ptr;

    for(;;)
    {
        ptr = operator new(sz);

        if( (uintptr_t)ptr % 16 == 0 )
        {
            return ptr;
        }

        operator delete(ptr);
    }
}

//---------------------------------------------------------------------------------------------------------------------
void MemFree( void* ptr )
{
    operator delete(ptr);
}

//=====================================================================================================================
static const uint32_t g_magic_base = 0xf4aeac59U;
static const uint32_t g_magic_factor = 0xaa39456bU;
static const uint32_t g_magic_init = 0x155c96f9U;

//---------------------------------------------------------------------------------------------------------------------
void MemProtect( int /*index*/, void* ptr, size_t sz )
{
    uint32_t x = g_magic_init;
    uint32_t* p = (uint32_t*)ptr;
    uint32_t* p1 = p + sz/sizeof(uint32_t);

    for( ; p < p1; ++p, x = (uint32_t)( (uint64_t)x*g_magic_factor % g_magic_base ) )
    {
        *p = x;
    }
}

//---------------------------------------------------------------------------------------------------------------------
#if UINTPTR_MAX > 0xffffffffUL
#define PRINTF_PTR_SIZE "16"
#else
#define PRINTF_PTR_SIZE "8"
#endif

//---------------------------------------------------------------------------------------------------------------------
bool MemUnprotect( int index, void* ptr, size_t sz )
{
    uint32_t x = g_magic_init;
    const uint32_t* p = (const uint32_t*)ptr;
    const uint32_t* p1 = p + sz/sizeof(uint32_t);

    for(  ;;  ++p,  x = (uint32_t)( (uint64_t)x*g_magic_factor % g_magic_base )  )
    {
        if( p >= p1 )
        {
            return true;
        }

        if( *p != x )
        {
            break;
        }
    }

    uint32_t x1 = *p;
    const uint32_t* p2 = p;

    while(  ++p < p1  &&  *p == x1  );

    printf(  "\n[%d] ALERT!!! Buffer verification failed: ptr=0x%0" PRINTF_PTR_SIZE
                                                "llx, total_size=%lu, valid_size=%lu, content=%lu*{0x%08lx}...\n\n",
        index,  (unsigned long long)ptr,  (unsigned long)sz,  (unsigned long)( (const char*)p2 - (const char*)ptr ),
                                                                    (unsigned long)( p - p2 ),  (unsigned long)x1  );
    return false;
}
