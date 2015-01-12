#include <utils.h>
#include <stdio.h>
#include <map>

//#define DISABLE_CUSTOM_ALLOCATOR
//#define DISABLE_SELECT_SDI
//#define DISABLE_SIGNAL_STOP_DETECTION

//=====================================================================================================================
class CInputCallback : public IDeckLinkInputCallback
{
    volatile int32_t  ref_count;
    volatile int32_t  frame_count, signal_frame_count;

public:
    BMDDisplayMode  display_mode;
    CWaitableCondition  need_restart;

public:
    CInputCallback() : ref_count(0), frame_count(0), signal_frame_count(0), display_mode(bmdModeHD720p5994)  {}

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

    if( !need_restart.Value() )
    {
        display_mode = displayModeId;
        need_restart.SetTrue();
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
        BMDTimeValue video_time, d;
        videoFrame->GetStreamTime( &video_time, &d, 240000 );

        Int32AtomicAdd( &frame_count, 1 );

        if( ( videoFrame->GetFlags() & bmdFrameHasNoInputSource ) == 0 )
        {
            if( Int32AtomicAdd( &signal_frame_count, 1 ) == 0 )
            {
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
        else if(  signal_frame_count > 0  &&  !need_restart.Value()  )
        {
            printf( "CInputCallback::VideoInputFrameArrived: signal stopped - video_time=%lld/240000\n",
                                                                                            (long long)video_time  );
            need_restart.SetTrue();
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

    while(  ++p < p1  &&  *p == x1  );

    printf(  "\nALERT!!! Buffer verification failed: ptr=0x%0" PRINTF_PTR_SIZE
                                                "llx, total_size=%lu, valid_size=%lu, content=%lu*{0x%08lx}...\n\n",
                (unsigned long long)ptr,  (unsigned long)sz,  (unsigned long)( (const char*)p2 - (const char*)ptr ),
                                                                    (unsigned long)( p - p2 ),  (unsigned long)x1  );
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
class CDeviceItem
{
public:
    int  index;
    IDeckLink* deck_link;
    CMemAlloc  alloc;
    CInputCallback  callback;

    CDeviceItem(): index(-1), deck_link(NULL)  {}
    ~CDeviceItem()  {  if( deck_link != NULL)  deck_link->Release();  }
};

#define VALIDATION_RESERVE  0x40000000L
static volatile int32_t g_thread_count = VALIDATION_RESERVE;
static CWaitableCondition  g_test_finished;
static const size_t g_items_count = 16;
static CDeviceItem g_items[g_items_count];

//---------------------------------------------------------------------------------------------------------------------
static void ThreadFunc( void* ctx )
{
    CDeviceItem& item = *(CDeviceItem*)ctx;
    HRESULT hr;
    assert( item.deck_link != NULL );

#ifndef DISABLE_SELECT_SDI
    IDeckLinkConfiguration* conf = NULL;

    printf( "[%d] IDeckLink::QueryInterface(IID_IDeckLinkConfiguration)...\n", item.index );
    hr = item.deck_link->QueryInterface( IID_IDeckLinkConfiguration, (void**)&conf );
    if( FAILED(hr) )
    {
        printf( "[%d] IDeckLink::QueryInterface(IID_IDeckLinkConfiguration) failed\n", item.index  );
        assert( conf == NULL );
        conf = NULL;
    }
    else
    {
        conf->SetInt( bmdDeckLinkConfigVideoInputConnection, bmdVideoConnectionSDI );
    }
#endif

    Int32AtomicAdd( &g_thread_count, 1 );

    while( g_thread_count > VALIDATION_RESERVE )
    {
        printf( "\n[%d] Starting Video+Audio Capture...\n", item.index );

        IDeckLinkInput* input;
        printf( "[%d] IDeckLink::QueryInterface(IID_IDeckLinkInput)...\n", item.index );
        hr = item.deck_link->QueryInterface( IID_IDeckLinkInput, (void**)&input );
        if( FAILED(hr) )
        {
            printf( "[%d] IDeckLink::QueryInterface(IID_IDeckLinkInput) failed\n", item.index  );
            break;
        }

#ifndef DISABLE_CUSTOM_ALLOCATOR
        printf( "[%d] IDeckLinkInput::SetVideoInputFrameMemoryAllocator...\n", item.index );
        hr = input->SetVideoInputFrameMemoryAllocator(&item.alloc);
        if( FAILED(hr) )
        {
            printf( "[%d] IDeckLinkInput::SetVideoInputFrameMemoryAllocator(obj) failed\n", item.index );
        }
        else
        {
#endif
            printf( "[%d] IDeckLinkInput::EnableVideoInput display_mode=%s\n", item.index,
                                                                        DisplayModeName(item.callback.display_mode) );
            hr = input->EnableVideoInput(
                                    item.callback.display_mode, bmdFormat8BitYUV, bmdVideoInputEnableFormatDetection );

            if( FAILED(hr) )
            {
                printf( "[%d] IDeckLinkInput::EnableVideoInput failed\n", item.index );
            }
            else
            {
                printf( "[%d] IDeckLinkInput::EnableAudioInput...\n", item.index );
                hr = input->EnableAudioInput( bmdAudioSampleRate48kHz, bmdAudioSampleType32bitInteger, 16 );
                if( FAILED(hr) )
                {
                    printf( "[%d] IDeckLinkInput::EnableAudioInput failed\n", item.index );
                }
                else
                {
                    printf("[%d] IDeckLinkInput::SetCallback(obj)...\n");
                    hr = input->SetCallback(&item.callback);
                    if( FAILED(hr) )
                    {
                        printf( "[%d] IDeckLinkInput::SetCallback failed\n", item.index );
                    }
                    else
                    {
                        printf( "[%d] IDeckLinkInput::StartStreams...\n", item.index );
                        hr = input->StartStreams();
                        if( FAILED(hr) )
                        {
                            printf( "[%d] IDeckLinkInput::StartStreams failed\n", item.index );
                        }
                        else
                        {
                            item.callback.need_restart.Wait();

                            printf("[%d] IDeckLinkInput::StopStreams...\n", item.index);
                            hr = input->StopStreams();
                            if( FAILED(hr) )
                            {
                                printf( "[%d] IDeckLinkInput::StopStreams failed\n", item.index );
                            }
                        }

                        printf( "[%d] IDeckLinkInput::SetCallback(NULL)...\n", item.index);
                        hr = input->SetCallback(NULL);
                        if( FAILED(hr) )
                        {
                            printf( "[%d] IDeckLinkInput::SetCallback failed\n", item.index );
                        }
                    }

                    printf( "[%d] IDeckLinkInput::DisableAudioInput...\n", item.index );
                    hr = input->DisableAudioInput();
                    if( FAILED(hr) )
                    {
                        printf( "[%d] IDeckLinkInput::DisableAudioInput failed\n", item.index );
                    }
                }

                printf( "[%d] IDeckLinkInput::DisableVideoInput...\n", item.index );
                hr = input->DisableVideoInput();
                if( FAILED(hr) )
                {
                    printf( "[%d] IDeckLinkInput::DisableVideoInput failed\n", item.index );
                }
            }

#ifndef DISABLE_CUSTOM_ALLOCATOR
#if 0
            hr = input->SetVideoInputFrameMemoryAllocator(NULL);
            if( FAILED(hr) )
            {
                printf( "[%d] IDeckLinkInput::SetVideoInputFrameMemoryAllocator(NULL) failed\n", item.index );
            }
#endif
        }
#endif
        printf( "[%d] IDeckLinkInput::Release...\n", item.index );
        input->Release();

        printf( "[%d] Stopped Video+Audio Capture.\n\n", item.index );
        item.callback.need_restart.SetFalse();

        if( g_thread_count < VALIDATION_RESERVE )
        {
            break;
        }

        printf( "[%d] Waiting 1 sec...\n\n", item.index );
        WaitSec(1);

        if( !item.alloc.Reset() )
        {
            Int32AtomicAdd( &g_thread_count, -VALIDATION_RESERVE );

            for( size_t j = 0; j < g_items_count; ++j )
            {
                g_items[j].callback.need_restart.SetTrue();
            }

            break;
        }
    }

    if( Int32AtomicAdd( &g_thread_count, -1 ) <= 1 )
    {
        g_test_finished.SetTrue();
    }

#ifndef DISABLE_SELECT_SDI
    if( conf != NULL )
    {
        conf->Release();
    }
#endif
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
        printf( "A DeckLink iterator could not be created. Probably DeckLink drivers not installed.\n" );
        return 1;
    }

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

    fprintf( stderr, "\nRunning video+audio capture tests...\n" );

    IDeckLink*  deck_link;

    for(  int j = 0;  j < g_items_count  &&  deckLinkIterator->Next(&deck_link) == S_OK;  ++j  )
    {
        g_items[j].index = j;
        g_items[j].deck_link = deck_link;
        StartThread( &ThreadFunc, &g_items[j] );
    }

    g_test_finished.Wait();
    fprintf( stderr, "\n!!!VALIDATION FAILED!!!\nPress ENTER to exit...\n" );
    getc(stdin);

    deckLinkIterator->Release();
    return 0;
}
