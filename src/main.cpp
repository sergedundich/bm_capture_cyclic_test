#include <comutil.h>
#include "DeckLinkAPI.h"

#define DISABLE_CUSTOM_ALLOCATOR
//#define DISABLE_INPUT_CALLBACK
const BMDDisplayMode g_mode = bmdModePAL;

#if defined(_WIN32)
//=====================================================================================================================
#include <windows.h>

const long SystemTimeSec = 10000000;

inline long long GetSystemTime()
{
    LARGE_INTEGER sysTime;
    GetSystemTimeAsFileTime( (LPFILETIME)&sysTime );
    return sysTime.QuadPart;
}

inline void WaitSec( unsigned duration_sec )
{
    Sleep( duration_sec*1000 );
}

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

#elif defined(__APPLE__)
//=====================================================================================================================
#define STDMETHODCALLTYPE

//---------------------------------------------------------------------------------------------------------------------
#include <string.h>
#include <sys/time.h>

const long SystemTimeSec = 1000000;

inline long long GetSystemTime()
{
    struct timeval tv;
    gettimeofday( &tv, NULL );
    return tv.tv_sec*SystemTimeSec + tv.tv_usec*(SystemTimeSec/1000000);
}

inline void WaitSec( unsigned duration_sec )
{
    sleep(duration_sec);
}

//---------------------------------------------------------------------------------------------------------------------
const REFIID IID_IUnknown = CFUUIDGetUUIDBytes(IUnknownUUID);

//---------------------------------------------------------------------------------------------------------------------
inline bool IsEqualGUID( const CFUUIDBytes& a, const CFUUIDBytes& b )
{
    return memcmp( &a, &b, sizeof(CFUUIDBytes) ) == 0;
}

#elif defined(__linux__)
#include <string.h>
#include <time.h>

//=====================================================================================================================
#define STDMETHODCALLTYPE

//---------------------------------------------------------------------------------------------------------------------
inline bool IsEqualGUID( const REFIID& a, const REFIID& b )
{
    return memcmp( &a, &b, sizeof(REFIID) ) == 0;
}

//---------------------------------------------------------------------------------------------------------------------
const long SystemTimeSec = 10000000;

inline long long GetSystemTime()
{
    struct timespec tms;
    clock_gettime( CLOCK_REALTIME, &tms );
    return tms.tv_sec*SystemTimeSec + tms.tv_nsec/(1000000000/SystemTimeSec);
}

inline void WaitSec( unsigned duration_sec )
{
    sleep(duration_sec);
}

#else
#error "Unsupported OS"
#endif

//=====================================================================================================================
class InputCallback : public IDeckLinkInputCallback
{
    LONG volatile ref_count;
    LONG volatile frame_count, signal_frame_count;

public:
    InputCallback() : ref_count(1), frame_count(0), signal_frame_count(0)  {}

    ~InputCallback()
    {
        printf( "InputCallback::~InputCallback - ref_count=%ld, frame_count=%ld(%ld)\n",
                                                        (long)ref_count, (long)frame_count, (long)signal_frame_count );
    }

    // overrides from IDeckLinkInputCallback
    virtual HRESULT STDMETHODCALLTYPE VideoInputFormatChanged(
                                                        BMDVideoInputFormatChangedEvents notificationEvents,
                                                        IDeckLinkDisplayMode* newDisplayMode,
                                                        BMDDetectedVideoInputFormatFlags detectedSignalFlags
                                                        );

    virtual HRESULT STDMETHODCALLTYPE VideoInputFrameArrived(
                                                        IDeckLinkVideoInputFrame* videoFrame,
                                                        IDeckLinkAudioInputPacket* audioPacket
                                                        );

    // overrides from IUnknown
    virtual HRESULT STDMETHODCALLTYPE QueryInterface( REFIID riid, void** pp );

    virtual ULONG STDMETHODCALLTYPE AddRef(void);
    virtual ULONG STDMETHODCALLTYPE Release(void);
};

//---------------------------------------------------------------------------------------------------------------------
const char* DisplayModeName( BMDDisplayMode displayModeId )
{
    switch(displayModeId)
    {
    case bmdModeNTSC:
        return "NTSC";
    case bmdModeNTSC2398:
        return "NTSC2398";
    case bmdModePAL:
        return "PAL";
    case bmdModeNTSCp:
        return "NTSCp";
    case bmdModePALp:
        return "PALp";
    case bmdModeHD1080p2398:
        return "HD1080p2398";
    case bmdModeHD1080p24:
        return "HD1080p24";
    case bmdModeHD1080p25:
        return "HD1080p25";
    case bmdModeHD1080p2997:
        return "HD1080p2997";
    case bmdModeHD1080p30:
        return "HD1080p30";
    case bmdModeHD1080i50:
        return "HD1080i50";
    case bmdModeHD1080i5994:
        return "HD1080i5994";
    case bmdModeHD1080i6000:
        return "HD1080i6000";
    case bmdModeHD1080p50:
        return "HD1080p50";
    case bmdModeHD1080p5994:
        return "HD1080p5994";
    case bmdModeHD1080p6000:
        return "HD1080p6000";
    case bmdModeHD720p50:
        return "HD720p50";
    case bmdModeHD720p5994:
        return "HD720p5994";
    case bmdModeHD720p60:
        return "HD720p60";
    case bmdMode2k2398:
        return "2k2398";
    case bmdMode2k24:
        return "2k24";
    case bmdMode2k25:
        return "2k25";
    case bmdMode2kDCI2398:
        return "2kDCI2398";
    case bmdMode2kDCI24:
        return "2kDCI24";
    case bmdMode2kDCI25:
        return "2kDCI25";
    case bmdMode4K2160p2398:
        return "4K2160p2398";
    case bmdMode4K2160p24:
        return "4K2160p24";
    case bmdMode4K2160p25:
        return "4K2160p25";
    case bmdMode4K2160p2997:
        return "4K2160p2997";
    case bmdMode4K2160p30:
        return "4K2160p30";
    case bmdMode4K2160p50:
        return "4K2160p50";
    case bmdMode4K2160p5994:
        return "4K2160p5994";
    case bmdMode4K2160p60:
        return "4K2160p60";
    case bmdMode4kDCI2398:
        return "4kDCI2398";
    case bmdMode4kDCI24:
        return "4kDCI24";
    case bmdMode4kDCI25:
        return "4kDCI25";
    case bmdModeUnknown:
        return "Unknown";
    default:
        return "UNRECOGNIZED";
    }
}

//---------------------------------------------------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE InputCallback::VideoInputFormatChanged(
                                                            BMDVideoInputFormatChangedEvents events,
                                                            IDeckLinkDisplayMode* displayMode,
                                                            BMDDetectedVideoInputFormatFlags flags
                                                            )
{
    printf( "InputCallback::VideoInputFormatChanged - changed=[ %s%s%s], display_mode=%s, flags=[ %s%s%s]\n",
                        ( events & bmdVideoInputDisplayModeChanged ? "DISP_MODE " : "" ),
                        ( events & bmdVideoInputFieldDominanceChanged ? "FIELD_DOMINANCE " : "" ),
                        ( events & bmdVideoInputColorspaceChanged ? "COLORSPACE " : "" ),
                        DisplayModeName( displayMode->GetDisplayMode() ),
                        ( flags & bmdDetectedVideoInputYCbCr422 ? "YUV422 " : "" ),
                        ( flags & bmdDetectedVideoInputRGB444 ? "RGB " : "" ),
                        ( flags & bmdDetectedVideoInputDualStream3D ? "3D " : "" )
                        );

    return S_OK;
}

//---------------------------------------------------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE InputCallback::VideoInputFrameArrived(
                                                            IDeckLinkVideoInputFrame* videoFrame,
                                                            IDeckLinkAudioInputPacket* audioPacket
                                                            )
{
    if( videoFrame != 0 )
    {
        InterlockedIncrement( &frame_count );

        if( ( videoFrame->GetFlags() & bmdFrameHasNoInputSource ) == 0 )
        {
            InterlockedIncrement( &signal_frame_count );
        }
    }

    return S_OK;
}

//---------------------------------------------------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE InputCallback::QueryInterface( REFIID riid, void** pp )
{
    if( IsEqualGUID( riid, IID_IDeckLinkInputCallback ) )
    {
        int cnt = InterlockedIncrement( &ref_count );
        printf( "InputCallback::QueryInterface(IDeckLinkInputCallback) - ref_count=%d\n", cnt );
        *pp = static_cast<IDeckLinkInputCallback*>(this);
        return S_OK;
    }

    if( IsEqualGUID( riid, IID_IUnknown ) )
    {
        int cnt = InterlockedIncrement( &ref_count );
        printf( "InputCallback::QueryInterface(IUnknown) - ref_count=%d\n", cnt );
        *pp = static_cast<IUnknown*>(this);
        return S_OK;
    }

    return E_NOINTERFACE;
}

//---------------------------------------------------------------------------------------------------------------------
ULONG STDMETHODCALLTYPE InputCallback::AddRef(void)
{
    int cnt = InterlockedIncrement( &ref_count );
    printf( "InputCallback::AddRef (ref_count=%d)\n", cnt );
    return cnt;
}

//---------------------------------------------------------------------------------------------------------------------
ULONG STDMETHODCALLTYPE InputCallback::Release(void)
{
    int cnt = InterlockedDecrement( &ref_count );
    printf( "InputCallback::Release (ref_count=%d)\n", cnt );
    return cnt;
}

//=====================================================================================================================
class Alloc: public IDeckLinkMemoryAllocator
{
    LONG volatile ref_count;
    LONG volatile buf_count;

    Alloc(): ref_count(1), buf_count(0)
    {
        printf("Alloc::Alloc (ref_count=1)\n");
    }

    ~Alloc()  { printf( "Alloc::~Alloc current buf count %ld\n", buf_count ); }

public:
    static IDeckLinkMemoryAllocator* CreateInstance()  { return new Alloc(); }

    virtual ULONG STDMETHODCALLTYPE AddRef()
    {
        int cnt = InterlockedIncrement( &ref_count );
        printf( "Alloc::AddRef (ref_count=%d)\n", cnt );
        return cnt;
    }

    virtual ULONG STDMETHODCALLTYPE Release()
    {
        int cnt = InterlockedDecrement( &ref_count );
        printf( "Alloc::Release (ref_count=%d)\n", cnt );

        if( cnt <= 0 )
        {
            delete this;
        }

        return cnt;
    }

    virtual HRESULT STDMETHODCALLTYPE QueryInterface( REFIID riid, void** pp )
    {
        if( IsEqualGUID( riid, IID_IDeckLinkMemoryAllocator ) )
        {
            int cnt = InterlockedIncrement( &ref_count );
            printf( "Alloc::QueryInterface(IDeckLinkInputCallback) - ref_count=%d\n", cnt );
            *pp = static_cast<IDeckLinkMemoryAllocator*>(this);
            return S_OK;
        }

        if( IsEqualGUID( riid, IID_IUnknown ) )
        {
            int cnt = InterlockedIncrement( &ref_count );
            printf( "Alloc::QueryInterface(IUnknown) - ref_count=%d\n", cnt );
            *pp = static_cast<IUnknown*>(this);
            return S_OK;
        }

        return E_NOINTERFACE;
    }

    virtual HRESULT STDMETHODCALLTYPE AllocateBuffer( unsigned long buf_size, void** pBuffer )
    {
        if( buf_size >= 0x80000000UL )
        {
            printf( "Alloc::AllocateBuffer: buf_size=0x%08lx is not a sane value.\n", buf_size );
            return E_OUTOFMEMORY;
        }

        char* ptr;

        try
        {
            for(;;)
            {
                ptr = new char[buf_size];

                if( (uintptr_t)ptr % 16 == 0 )
                {
                    break;
                }

                delete[] ptr;
            }
        }
        catch(...)
        {
            printf( "Alloc::AllocateBuffer: allocation failed (buf_size=%lu).\n", buf_size );
            return E_OUTOFMEMORY;
        }

        long cnt = InterlockedIncrement(&buf_count);

//        printf( "Alloc::AllocateBuffer: ptr=0x%016llx, size=%lu, total_count=%ld\n",
//                                                            (unsigned long long)*pBuffer, buf_size, cnt+1 );
        *pBuffer = ptr;
        return S_OK;
    }

    virtual HRESULT STDMETHODCALLTYPE ReleaseBuffer( void* buffer )
    {
        long cnt = InterlockedDecrement(&buf_count);
        //printf( "Alloc::ReleaseBuffer: ptr=0x%016llx, total_count=%ld\n", (unsigned long long)buffer, cnt );
        delete[] buffer;
        return S_OK;
    }

    virtual HRESULT STDMETHODCALLTYPE Commit(void)
    {
        printf("Alloc::Commit\n");
        return S_OK;
    }

    virtual HRESULT STDMETHODCALLTYPE Decommit(void)
    {
        printf("Alloc::Decommit\n");
        return S_OK;
    }
};

//=====================================================================================================================
void test_iteration( IDeckLink* deckLink, unsigned long j )
{
    printf( "\nVideo+Audio Capture Testing Iteration #%lu started!\n", j );

    IDeckLinkInput* input;
    printf("deckLink->QueryInterface(IID_IDeckLinkInput)...\n");
    HRESULT hr = deckLink->QueryInterface( IID_IDeckLinkInput, (void**)&input );
    if( FAILED(hr) )
    {
        fprintf( stderr, "deckLink->QueryInterface(IID_IDeckLinkInput) failed\n" );
        return;
    }

#ifndef DISABLE_CUSTOM_ALLOCATOR
    IDeckLinkMemoryAllocator* alloc = Alloc::CreateInstance();
    printf("input->SetVideoInputFrameMemoryAllocator...\n");
    hr = input->SetVideoInputFrameMemoryAllocator(alloc);
    if( FAILED(hr) )
    {
        fprintf( stderr, "input->SetVideoInputFrameMemoryAllocator(obj) failed\n" );
    }
    else
    {
#endif
        printf("input->EnableVideoInput...\n");
        hr = input->EnableVideoInput( g_mode, bmdFormat8BitYUV, bmdVideoInputEnableFormatDetection );
        if( FAILED(hr) )
        {
            fprintf( stderr, "input->EnableVideoInput failed\n" );
        }
        else
        {
            printf("input->EnableAudioInput...\n");
            hr = input->EnableAudioInput( bmdAudioSampleRate48kHz, bmdAudioSampleType32bitInteger, 16 );
            if( FAILED(hr) )
            {
                fprintf( stderr, "input->EnableAudioInput failed\n" );
            }
            else
            {
#ifndef DISABLE_INPUT_CALLBACK
                InputCallback callback;
                printf("input->SetCallback(obj)...\n");
                hr = input->SetCallback(&callback);
                if( FAILED(hr) )
                {
                    fprintf( stderr, "input->SetCallback failed\n" );
                }
                else
                {
#endif
                    printf("input->StartStreams...\n");
                    hr = input->StartStreams();
                    if( FAILED(hr) )
                    {
                        fprintf( stderr, "input->StartStreams failed\n" );
                    }
                    else
                    {
                        for( unsigned n = 20; n; --n )
                        {
                            printf( "Video+Audio Capture remaining %u sec...\n", n );
                            WaitSec(1);
                        }

                        printf("input->StopStreams...\n");
                        hr = input->StopStreams();
                        if( FAILED(hr) )
                        {
                            fprintf( stderr, "input->StopStreams failed\n" );
                        }
                    }

#ifndef DISABLE_INPUT_CALLBACK
                    printf("input->SetCallback(NULL)...\n");
                    hr = input->SetCallback(NULL);
                    if( FAILED(hr) )
                    {
                        fprintf( stderr, "input->SetCallback failed\n" );
                    }
                }
#endif

                printf("input->DisableAudioInput...\n");
                hr = input->DisableAudioInput();
                if( FAILED(hr) )
                {
                    fprintf( stderr, "input->DisableAudioInput failed\n" );
                }
            }

            printf("input->DisableVideoInput...\n");
            hr = input->DisableVideoInput();
            if( FAILED(hr) )
            {
                fprintf( stderr, "input->DisableVideoInput failed\n" );
            }
        }

#ifndef DISABLE_CUSTOM_ALLOCATOR
#if 0
        hr = input->SetVideoInputFrameMemoryAllocator(NULL);
        if( FAILED(hr) )
        {
            fprintf( stderr, "input->SetVideoInputFrameMemoryAllocator(NULL) failed\n" );
        }
#endif
    }

    printf("input->Release...\n");
    input->Release();

    alloc->Release();
#else
    printf("input->Release...\n");
    input->Release();
#endif

    printf( "Video+Audio Capture Stopped.\n"
            "Allocating+Reading+Deallocating 50 memory buffers of 1920x1080x4 bytes each.\n" );

    unsigned memory_sum = 0;

    for( unsigned k = 0; k < 50; ++k )
    {
        // memory allocating+reading+deallocating iteration
        unsigned* buf = new unsigned[1920*1080];

        for( size_t j = 0; j < 1920*1080; ++j )
        {
            memory_sum += buf[j];
        }

        delete[] buf;
    }

    printf( "Video+Audio Capture Testing Iteration #%lu finished (memory_sum=0x%x)!\n\n", j, memory_sum );
}

//=====================================================================================================================
int main( int argc, char* argv[] )
{
#ifdef _WIN32
    //  Initialize COM on this thread
    HRESULT hr = CoInitialize(NULL);
    if( FAILED(hr) )
    {
        fprintf( stderr, "Initialization of COM failed - hr = %08x.\n", hr );
        return 1;
    }
#endif

    IDeckLinkIterator*  deckLinkIterator = CreateDeckLinkIteratorInstance();
    if( deckLinkIterator == NULL )
    {
        fprintf( stderr, "A DeckLink iterator could not be created. The DeckLink drivers may not be installed.\n" );
        CoUninitialize();
        return 1;
    }

    IDeckLink*  deckLink;
    int  numDevices = 0;

    {
        // We can get the version of the API like this:
        IDeckLinkAPIInformation* deckLinkAPIInformation;
        hr = deckLinkIterator->QueryInterface( IID_IDeckLinkAPIInformation, (void**)&deckLinkAPIInformation );
        if( hr == S_OK )
        {
            LONGLONG  deckLinkVersion;
            int  dlVerMajor, dlVerMinor, dlVerPoint;

            // We can also use the BMDDeckLinkAPIVersion flag with GetString
            deckLinkAPIInformation->GetInt( BMDDeckLinkAPIVersion, &deckLinkVersion );

            dlVerMajor = (deckLinkVersion & 0xFF000000) >> 24;
            dlVerMinor = (deckLinkVersion & 0x00FF0000) >> 16;
            dlVerPoint = (deckLinkVersion & 0x0000FF00) >> 8;

            printf( "DeckLink API version: %d.%d.%d\n", dlVerMajor, dlVerMinor, dlVerPoint );

            deckLinkAPIInformation->Release();
        }
    }

    hr = deckLinkIterator->Next(&deckLink); // first device
    if( FAILED(hr) )
    {
        fprintf( stderr, "No Blackmagic Design devices were found.\n" );
        deckLinkIterator->Release();
        CoUninitialize();
        return 1;
    }

    for( unsigned long j = 0; j < 0x80000000UL; ++j )
    {
        test_iteration( deckLink, j+1 );
        printf("\nWaiting 2 sec...\n");
        WaitSec(2);
    }

    deckLink->Release();
    deckLinkIterator->Release();

#ifdef _WIN32
    CoUninitialize();
#endif
    return 0;
}
