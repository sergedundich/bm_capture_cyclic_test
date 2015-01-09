#include <utils.h>
#include <map>

//#define DISABLE_CUSTOM_ALLOCATOR
//#define DISABLE_INPUT_CALLBACK
//#define DISABLE_SIGNAL_STOP_DETECTION
BMDDisplayMode  g_display_mode = bmdModeHD720p5994;

//=====================================================================================================================
class CInputCallback : public IDeckLinkInputCallback
{
    volatile int32_t  ref_count;
    volatile int32_t  frame_count, signal_frame_count;

public:
    volatile BMDDisplayMode  display_mode;

public:
    CInputCallback() : ref_count(0), frame_count(0), signal_frame_count(0), display_mode(bmdModeUnknown)  {}

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
HRESULT STDMETHODCALLTYPE CInputCallback::VideoInputFormatChanged(
                                                            BMDVideoInputFormatChangedEvents events,
                                                            IDeckLinkDisplayMode* displayMode,
                                                            BMDDetectedVideoInputFormatFlags flags
                                                            )
{
    BMDDisplayMode displayModeId = displayMode->GetDisplayMode();
    printf( "CInputCallback::VideoInputFormatChanged: changed=[ %s%s%s], display_mode=%s, flags=[ %s%s%s]\n",
                                            ( events & bmdVideoInputDisplayModeChanged ? "DISP_MODE " : "" ),
                                            ( events & bmdVideoInputFieldDominanceChanged ? "FIELD_DOMINANCE " : "" ),
                                            ( events & bmdVideoInputColorspaceChanged ? "COLORSPACE " : "" ),
                                            DisplayModeName(displayModeId),
                                            ( flags & bmdDetectedVideoInputYCbCr422 ? "YUV422 " : "" ),
                                            ( flags & bmdDetectedVideoInputRGB444 ? "RGB " : "" ),
                                            ( flags & bmdDetectedVideoInputDualStream3D ? "3D " : "" )
                                            );

    if( display_mode != bmdModeUnknown )
    {
        display_mode = displayModeId;
    }

    return S_OK;
}

//---------------------------------------------------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE CInputCallback::VideoInputFrameArrived(
                                                            IDeckLinkVideoInputFrame* videoFrame,
                                                            IDeckLinkAudioInputPacket* audioPacket
                                                            )
{
    if( videoFrame != 0 )
    {
        Int32AtomicAdd( &frame_count, 1 );

        if( ( videoFrame->GetFlags() & bmdFrameHasNoInputSource ) == 0 )
        {
            if( Int32AtomicAdd( &signal_frame_count, 1 ) == 0 )
            {
                BMDTimeValue video_time, d;
                videoFrame->GetStreamTime( &video_time, &d, 240000 );

                if( audioPacket != 0 )
                {
                    BMDTimeValue audio_time;
                    audioPacket->GetPacketTime( &audio_time, 240000 );

                    printf(  "CInputCallback::VideoInputFrameArrived: signal started - video_time=%lld/240000, "
                                        "audio_time=%lld/240000\n",  (long long)video_time,  (long long)audio_time  );
                }
                else
                {
                    printf( "CInputCallback::VideoInputFrameArrived: signal started - video_time=%lld/240000\n",
                                                                                            (long long)video_time  );
                }
            }
        }
#ifndef DISABLE_SIGNAL_STOP_DETECTION
        else if(  signal_frame_count > 0  &&  g_display_mode == display_mode  )
        {
            display_mode = bmdModeUnknown;
        }
#endif
    }

    return S_OK;
}

//---------------------------------------------------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE CInputCallback::QueryInterface( REFIID riid, void** pp )
{
    if( IsEqualGUID( riid, IID_IDeckLinkInputCallback ) )
    {
        long cnt = Int32AtomicAdd( &ref_count, 1 ) + 1;
        printf( "CInputCallback::QueryInterface(IDeckLinkInputCallback) - new_ref_count=%ld\n", cnt );
        *pp = static_cast<IDeckLinkInputCallback*>(this);
        return S_OK;
    }

    if( IsEqualGUID( riid, IID_IUnknown ) )
    {
        long cnt = Int32AtomicAdd( &ref_count, 1 ) + 1;
        printf( "CInputCallback::QueryInterface(IUnknown) - new_ref_count=%ld\n", cnt );
        *pp = static_cast<IUnknown*>(this);
        return S_OK;
    }

    return E_NOINTERFACE;
}

//---------------------------------------------------------------------------------------------------------------------
ULONG STDMETHODCALLTYPE CInputCallback::AddRef(void)
{
    long cnt = Int32AtomicAdd( &ref_count, 1 ) + 1;
    printf( "CInputCallback::AddRef - new_ref_count=%ld\n", cnt );
    return cnt;
}

//---------------------------------------------------------------------------------------------------------------------
ULONG STDMETHODCALLTYPE CInputCallback::Release(void)
{
    long cnt = Int32AtomicAdd( &ref_count, -1 ) - 1;

    if( cnt <= 0 )
    {
        printf( "CInputCallback::Release - new_ref_count=%ld, total_frame_count=%ld, signal_frame_count=%ld\n",
                                                            cnt, (long)frame_count, (long)signal_frame_count );
        frame_count = 0;  signal_frame_count = 0;
    }
    else
    {
        printf( "CInputCallback::Release - new_ref_count=%ld\n", cnt );
    }

    return cnt;
}

//=====================================================================================================================
class CMemAlloc: public IDeckLinkMemoryAllocator
{
    volatile int32_t  ref_count;
    CMutex  buffers_lock;
    std::multimap<BM_UINT32,char*>  free_buffers;
    std::map<char*,BM_UINT32>  alloc_buffers;

public:
    CMemAlloc(): ref_count(0)  {}
    bool Reset();

    virtual ULONG STDMETHODCALLTYPE AddRef();
    virtual ULONG STDMETHODCALLTYPE Release();
    virtual HRESULT STDMETHODCALLTYPE QueryInterface( REFIID riid, void** pp );

    virtual HRESULT STDMETHODCALLTYPE AllocateBuffer( BM_UINT32 buf_size, void** pBuffer );
    virtual HRESULT STDMETHODCALLTYPE ReleaseBuffer( void* buffer );

    virtual HRESULT STDMETHODCALLTYPE Commit(void);
    virtual HRESULT STDMETHODCALLTYPE Decommit(void);
};

//---------------------------------------------------------------------------------------------------------------------
static const uint32_t g_magic_base = 0xf4aeac59U;
static const uint32_t g_magic_factor = 0xaa39456bU;
static const uint32_t g_magic_init = 0x155c96f9U;

void FillMagicData( void* ptr, size_t sz )
{
    uint32_t x = g_magic_init;
    uint32_t* p = (uint32_t*)ptr;
    uint32_t* p1 = p + sz/sizeof(uint32_t);

    for( ; p < p1; ++p, x = (uint32_t)( (uint64_t)x*g_magic_factor % g_magic_base ) )
    {
        *p = x;
    }
}

#if UINTPTR_MAX > 0xffffffffUL
#define PRINTF_PTR_SIZE "16"
#else
#define PRINTF_PTR_SIZE "8"
#endif

bool VerifyMagicData( const void* ptr, size_t sz )
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

    for(  ;  ++p < p1  &&  *p == x1;  x = (uint32_t)( (uint64_t)x*g_magic_factor % g_magic_base )  );

    printf(  "ALERT!!! Buffer verification failed: ptr=0x%0" PRINTF_PTR_SIZE
                                                    "llx, total_size=%lu, valid_size=%lu, content=%lu*{0x%08lx}...\n",
                (unsigned long long)ptr,  (unsigned long)sz,  (unsigned long)( (const char*)p2 - (const char*)ptr ),
                                            (unsigned long)( (const char*)p - (const char*)p2 ),  (unsigned long)x1  );
    return false;
}

//---------------------------------------------------------------------------------------------------------------------
bool CMemAlloc::Reset()
{
    bool ok = true;
    CMutexLockGuard lock_guard(buffers_lock);

    for(  std::multimap<BM_UINT32,char*>::const_iterator it = free_buffers.begin();  it != free_buffers.end();  ++it  )
    {
        ok &= VerifyMagicData( it->second, it->first );
        delete [] it->second;
    }

    free_buffers.clear();
    return ok;
}

//---------------------------------------------------------------------------------------------------------------------
ULONG STDMETHODCALLTYPE CMemAlloc::AddRef()
{
    long cnt = Int32AtomicAdd( &ref_count, 1 ) + 1;
    printf( "CMemAlloc::AddRef - new_ref_count=%ld\n", cnt );
    return cnt;
}

//---------------------------------------------------------------------------------------------------------------------
ULONG STDMETHODCALLTYPE CMemAlloc::Release()
{
    long cnt = Int32AtomicAdd( &ref_count, -1 ) - 1;
    printf( "CMemAlloc::Release - new_ref_count=%ld\n", cnt );

    if( cnt <= 0 )
    {
        CMutexLockGuard lock_guard(buffers_lock);

        assert( alloc_buffers.empty() );

        for(  std::multimap<BM_UINT32,char*>::const_iterator it = free_buffers.begin();  it != free_buffers.end();  ++it  )
        {
            FillMagicData( it->second, it->first );
        }

#if 0 // corrupt one of the buffers
        std::multimap<BM_UINT32,char*>::const_iterator it = free_buffers.begin();
        if(  it != free_buffers.end()  &&  it->first > 80  )
        {
            memset( (char*)it->second + it->first/2, 0x80, 40 );
        }
#endif
    }

    return cnt;
}

//---------------------------------------------------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE CMemAlloc::QueryInterface( REFIID riid, void** pp )
{
    if( IsEqualGUID( riid, IID_IDeckLinkMemoryAllocator ) )
    {
        long cnt = Int32AtomicAdd( &ref_count, 1 ) + 1;
        printf( "CMemAlloc::QueryInterface(IDeckLinkInputCallback) - new_ref_count=%ld\n", cnt );
        *pp = static_cast<IDeckLinkMemoryAllocator*>(this);
        return S_OK;
    }

    if( IsEqualGUID( riid, IID_IUnknown ) )
    {
        long cnt = Int32AtomicAdd( &ref_count, 1 ) + 1;
        printf( "CMemAlloc::QueryInterface(IUnknown) - new_ref_count=%ld\n", cnt );
        *pp = static_cast<IUnknown*>(this);
        return S_OK;
    }

    return E_NOINTERFACE;
}

//---------------------------------------------------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE CMemAlloc::AllocateBuffer( BM_UINT32 buf_size, void** pBuffer )
{
    if( buf_size >= 0x80000000UL )
    {
        printf( "CMemAlloc::AllocateBuffer: buf_size=0x%08lx is not a sane value.\n", (unsigned long)buf_size );
        return E_OUTOFMEMORY;
    }

    char* ptr;

    try
    {
        CMutexLockGuard lock_guard(buffers_lock);
        std::multimap<BM_UINT32,char*>::iterator it = free_buffers.lower_bound(buf_size);

        if( it != free_buffers.end() )
        {
            ptr = it->second;
            buf_size = it->first;
            free_buffers.erase(it);
        }
        else
        for(;;)
        {
            ptr = new char[buf_size];

            if( (uintptr_t)ptr % 16 == 0 )
            {
                break;
            }

            delete[] ptr;
        }

        alloc_buffers[ptr] = buf_size;
    }
    catch(...)
    {
        printf( "CMemAlloc::AllocateBuffer: allocation failed (buf_size=%lu).\n", (unsigned long)buf_size );
        return E_OUTOFMEMORY;
    }

    *pBuffer = ptr;
    return S_OK;
}

//---------------------------------------------------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE CMemAlloc::ReleaseBuffer( void* buffer )
{
    {
        CMutexLockGuard lock_guard(buffers_lock);
        std::map<char*,BM_UINT32>::iterator it = alloc_buffers.find( (char*)buffer );

        assert( it != alloc_buffers.end() );
        if( it != alloc_buffers.end() )
        {
            free_buffers.insert( std::multimap<BM_UINT32,char*>::value_type( it->second, it->first ) );
            alloc_buffers.erase(it);
            return S_OK;
        }
    }

    delete[] buffer;
    return S_OK;
}

//---------------------------------------------------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE CMemAlloc::Commit(void)
{
    printf("CMemAlloc::Commit\n");
    return S_OK;
}

//---------------------------------------------------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE CMemAlloc::Decommit(void)
{
    printf("CMemAlloc::Decommit\n");
    return S_OK;
}

//=====================================================================================================================
static CMemAlloc  g_alloc;
static CInputCallback  g_callback;

//---------------------------------------------------------------------------------------------------------------------
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
    printf("input->SetVideoInputFrameMemoryAllocator...\n");
    hr = input->SetVideoInputFrameMemoryAllocator(&g_alloc);
    if( FAILED(hr) )
    {
        fprintf( stderr, "input->SetVideoInputFrameMemoryAllocator(obj) failed\n" );
    }
    else
    {
#endif
        printf("input->EnableVideoInput display_mode=%s\n", DisplayModeName(g_display_mode) );
        hr = input->EnableVideoInput( g_display_mode, bmdFormat8BitYUV, bmdVideoInputEnableFormatDetection );
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
                g_callback.display_mode = g_display_mode;
                printf("input->SetCallback(obj)...\n");
                hr = input->SetCallback(&g_callback);
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
                        for(  unsigned n = 20; n  &&  g_callback.display_mode == g_display_mode;  --n  )
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
#endif
    printf("input->Release...\n");
    input->Release();

    printf( "Video+Audio Capture Testing Iteration #%lu finished!\n\n", j );

    if( g_callback.display_mode != bmdModeUnknown )
    {
        g_display_mode = g_callback.display_mode;
    }
}

//=====================================================================================================================
int main( int argc, char* argv[] )
{
    if( !InitCom() )
    {
        return 1;
    }

    IDeckLinkIterator*  deckLinkIterator = CreateDeckLinkIteratorInstance();
    if( deckLinkIterator == NULL )
    {
        fprintf( stderr, "A DeckLink iterator could not be created. The DeckLink drivers may not be installed.\n" );
        return 1;
    }

    IDeckLink*  deckLink;
    int  numDevices = 0;

    {
        // We can get the version of the API like this:
        IDeckLinkAPIInformation* deckLinkAPIInformation;
        HRESULT hr = deckLinkIterator->QueryInterface( IID_IDeckLinkAPIInformation, (void**)&deckLinkAPIInformation );
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

    HRESULT hr = deckLinkIterator->Next(&deckLink); // first device
    if( FAILED(hr) )
    {
        fprintf( stderr, "No Blackmagic Design devices were found.\n" );
        deckLinkIterator->Release();
        return 1;
    }

    for( unsigned long j = 0; j < 0x80000000UL; ++j )
    {
        test_iteration( deckLink, j+1 );
        printf("\nWaiting 1 sec...\n");
        WaitSec(1);

        if( !g_alloc.Reset() )
        {
            printf("\nPress ENTER to continue...\n");
            getc(stdin);
        }
    }

    deckLink->Release();
    deckLinkIterator->Release();
    return 0;
}
