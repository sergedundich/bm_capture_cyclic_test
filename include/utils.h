#ifndef UTILS__H__
#define UTILS__H__
#include <assert.h>
#include <stdint.h>
#include <stdexcept>
#include <DeckLinkAPI.h>

#if defined(_WIN32)
//=====================================================================================================================
#include <comutil.h>
#include <windows.h>

//---------------------------------------------------------------------------------------------------------------------
inline int32_t Int32AtomicAdd( volatile int32_t* p, int32_t x )
{
    return InterlockedExchangeAdd( (volatile LONG*)p, x );
}

//---------------------------------------------------------------------------------------------------------------------
inline void WaitSec( unsigned duration_sec )
{
    Sleep( duration_sec*1000 );
}

//---------------------------------------------------------------------------------------------------------------------
inline bool InitCom()
{
    //  Initialize COM on this thread
    HRESULT hr = CoInitialize(NULL);
    if( FAILED(hr) )
    {
        fprintf( stderr, "Initialization of COM failed - hr = %08x.\n", hr );
        return false;
    }

    return true;
}

//---------------------------------------------------------------------------------------------------------------------
class CMutex
{
    CRITICAL_SECTION  m_obj;

public:
    CMutex()
    {
        ::InitializeCriticalSection( &m_obj );
    }

    ~CMutex()
    {
        ::DeleteCriticalSection(&m_obj);
    }

    void Lock()  { ::EnterCriticalSection(&m_obj); }
    void Unlock()  { ::LeaveCriticalSection(&m_obj); }
};

//---------------------------------------------------------------------------------------------------------------------
inline IDeckLinkIterator* CreateDeckLinkIteratorInstance()
{
    LPVOID  p = NULL;
    HRESULT  hr = CoCreateInstance(
                                    CLSID_CDeckLinkIterator,  NULL,  CLSCTX_ALL,
                                    IID_IDeckLinkIterator,  &p
                                    );

    if ( FAILED(hr) )
    {
        return NULL;
    }

    assert( p != NULL );
    return  static_cast<IDeckLinkIterator*>(p);
}

typedef unsigned long BM_UINT32;

#else // !defined(_WIN32)
//=====================================================================================================================
#include <string.h>
#include <pthread.h>

//---------------------------------------------------------------------------------------------------------------------
#if defined(__APPLE__)
#include <sys/time.h>

const REFIID IID_IUnknown = CFUUIDGetUUIDBytes(IUnknownUUID);

#elif defined(__linux__)
#include <time.h>
#else
#error "Unsupported OS"
#endif

//---------------------------------------------------------------------------------------------------------------------
inline void WaitSec( unsigned duration_sec )
{
    sleep(duration_sec);
}

//---------------------------------------------------------------------------------------------------------------------
#if defined(__i386__) || defined(__amd64__)

inline int32_t Int32AtomicAdd( volatile int32_t* p, int32_t x )
{
    __asm__ __volatile__( "lock xaddl %0, %1" : "=r"(x) : "m"(*p), "0"(x) );
    return x;
}

#else
#error "Unsupported CPU architecture"
#endif

//---------------------------------------------------------------------------------------------------------------------
inline bool InitCom()  { return true; }

//---------------------------------------------------------------------------------------------------------------------
class CMutex
{
    pthread_mutex_t m_mutex;

public:
    CMutex()
    {
        int err = pthread_mutex_init( &m_mutex, NULL );

        if( err != 0 )
        {
            throw  std::runtime_error("pthread_mutex_init failed");
        }
    }

    ~CMutex()
    {
        pthread_mutex_destroy(&m_mutex);
    }

    void Lock()
    {
        int err = pthread_mutex_lock(pMutex);

        if( err != 0 )
        {
            throw std::runtime_error("pthread_mutex_lock failed");
        }
    }

    void Unlock()
    {
        int err = pthread_mutex_unlock(pMutex);

        if( err != 0 )
        {
            throw std::runtime_error("pthread_mutex_unlock failed");
        }
    }
};

//---------------------------------------------------------------------------------------------------------------------
#define STDMETHODCALLTYPE

inline bool IsEqualGUID( const REFIID& a, const REFIID& b )
{
    return memcmp( &a, &b, sizeof(REFIID) ) == 0;
}

typedef uint32_t BM_UINT32;

#endif // defined(_WIN32) || !defined(_WIN32)

//=====================================================================================================================
class CMutexLockGuard
{
    CMutex&  m_obj;

public:
    CMutexLockGuard( CMutex& obj ) : m_obj(obj)  { m_obj.Lock(); }
    ~CMutexLockGuard()  { m_obj.Unlock(); }
};

//=====================================================================================================================
typedef void (*FTaskAction)( void* ctx );

void StartThread( FTaskAction func, void* ctx );

#endif // !defined(UTILS__H__)
