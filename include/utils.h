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
class CWaitableCondition
{
    HANDLE  m_h;

public:
    CWaitableCondition( bool value = false ) : m_h( ::CreateEvent( NULL, FALSE, value, NULL ) )
    {
        if( m_h == NULL )
        {
            throw std::runtime_error("CWaitableCondition: CreateEvent failed");
        }
    }

    ~CWaitableCondition()  { ::CloseHandle(m_h); }

    void Wait()  { ::WaitForSingleObject( m_h, INFINITE ); }
    bool Value() const  { return ::WaitForSingleObject( m_h, 0 ) != WAIT_TIMEOUT; }
    void SetTrue()  { ::SetEvent(m_h); }
    void SetFalse()  { ::ResetEvent(m_h); }
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

    ~CMutex()  { pthread_mutex_destroy(&m_mutex); }

    void Lock()  { pthread_mutex_lock(&m_mutex); }
    void Unlock()  { pthread_mutex_unlock(&m_mutex); }

    pthread_mutex_t* Ptr()  { return &m_mutex; }
};

//-----------------------------------------------------------------------------
class CWaitableCondition
{
    CMutex  m_mutex;
    pthread_cond_t  m_cond;
    bool  m_value;

public:
    CWaitableCondition( bool value = false ) : m_value(value)  {}

    void Wait()
    {
        m_mutex.Lock();

        if( !m_value )
        {
            pthread_cond_wait( &m_cond, m_mutex.Ptr() );
        }

        m_mutex.Unlock();
    }

    bool Value() const  {  return  m_value;  }

    void SetTrue()
    {
        m_mutex.Lock();

        m_value = true;
        pthread_cond_broadcast(&m_cond);
        m_mutex.Unlock();
    }

    void SetFalse()
    {
        m_mutex.Lock();
        m_value = false;
        m_mutex.Unlock();
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
