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
