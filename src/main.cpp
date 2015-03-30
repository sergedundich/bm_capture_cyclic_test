#include <utils.h>
#include <MemUtils.h>
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
    int index;
    BMDDisplayMode  display_mode;
    CWaitableCondition  need_restart;

public:
    CInputCallback(): ref_count(0), frame_count(0), signal_frame_count(0), index(-1), display_mode(bmdModeHD720p60)  {}

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
    printf( "[%d] CInputCallback::VideoInputFormatChanged: changed=[ %s%s%s], display_mode=%s, flags=[ %s%s%s]\n",
                                            index,
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

                    printf(  "[%d] CInputCallback::VideoInputFrameArrived: signal started - video_time=%lld/240000, "
                                "audio_time=%lld/240000\n",  index,  (long long)video_time,  (long long)audio_time  );
                    fflush(stdout);
                }
                else
                {
                    printf( "[%d] CInputCallback::VideoInputFrameArrived: signal started - video_time=%lld/240000\n",
                                                                                    index,  (long long)video_time  );
                    fflush(stdout);
                }
            }
        }
#ifndef DISABLE_SIGNAL_STOP_DETECTION
        else if(  signal_frame_count > 0  &&  !need_restart.Value()  )
        {
            printf( "[%d] CInputCallback::VideoInputFrameArrived: signal stopped - video_time=%lld/240000\n",
                                                                                    index,  (long long)video_time  );
//            display_mode = bmdModeHD720p60;
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
        printf( "[%d] CInputCallback::QueryInterface(IDeckLinkInputCallback) - new_ref_count=%ld\n", index, cnt );
        fflush(stdout);
        *pp = static_cast<IDeckLinkInputCallback*>(this);
        return S_OK;
    }

    if( IsEqualGUID( riid, IID_IUnknown ) )
    {
        long cnt = Int32AtomicAdd( &ref_count, 1 ) + 1;
        printf( "[%d] CInputCallback::QueryInterface(IUnknown) - new_ref_count=%ld\n", index, cnt );
        fflush(stdout);
        *pp = static_cast<IUnknown*>(this);
        return S_OK;
    }

    return E_NOINTERFACE;
}

//---------------------------------------------------------------------------------------------------------------------
ULONG STDMETHODCALLTYPE CInputCallback::AddRef(void)
{
    long cnt = Int32AtomicAdd( &ref_count, 1 ) + 1;
    printf( "[%d] CInputCallback::AddRef - new_ref_count=%ld\n", index, cnt );
    fflush(stdout);
    return cnt;
}

//---------------------------------------------------------------------------------------------------------------------
ULONG STDMETHODCALLTYPE CInputCallback::Release(void)
{
    long cnt = Int32AtomicAdd( &ref_count, -1 ) - 1;

    if( cnt <= 0 )
    {
        printf(  "[%d] CInputCallback::Release - new_ref_count=%ld, total_frame_count=%ld, signal_frame_count=%ld\n",
                                                            index, cnt, (long)frame_count, (long)signal_frame_count  );
        frame_count = 0;  signal_frame_count = 0;
    }
    else
    {
        printf( "[%d] CInputCallback::Release - new_ref_count=%ld\n", index, cnt );
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
    int index;

public:
    CMemAlloc(): ref_count(0), index(-1)  {}
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
bool CMemAlloc::Reset()
{
    bool ok = true;
    CMutexLockGuard lock_guard(buffers_lock);

    for(  std::multimap<BM_UINT32,char*>::const_iterator it = free_buffers.begin();  it != free_buffers.end();  ++it  )
    {
        ok &= MemUnprotect( index, it->second, it->first );
        MemFree(it->second);
    }

    free_buffers.clear();
    return ok;
}

//---------------------------------------------------------------------------------------------------------------------
ULONG STDMETHODCALLTYPE CMemAlloc::AddRef()
{
    long cnt = Int32AtomicAdd( &ref_count, 1 ) + 1;
    printf( "[%d] CMemAlloc::AddRef - new_ref_count=%ld\n", index, cnt );
    fflush(stdout);
    return cnt;
}

//---------------------------------------------------------------------------------------------------------------------
ULONG STDMETHODCALLTYPE CMemAlloc::Release()
{
    long cnt = Int32AtomicAdd( &ref_count, -1 ) - 1;
    printf( "[%d] CMemAlloc::Release - new_ref_count=%ld\n", index, cnt );

    if( cnt <= 0 )
    {
        CMutexLockGuard lock_guard(buffers_lock);

        assert( alloc_buffers.empty() );

        for(  std::multimap<BM_UINT32,char*>::const_iterator it = free_buffers.begin();  it != free_buffers.end();  ++it  )
        {
            MemProtect( index, it->second, it->first );
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
        printf( "[%d] CMemAlloc::QueryInterface(IDeckLinkInputCallback) - new_ref_count=%ld\n", index, cnt );
        *pp = static_cast<IDeckLinkMemoryAllocator*>(this);
        return S_OK;
    }

    if( IsEqualGUID( riid, IID_IUnknown ) )
    {
        long cnt = Int32AtomicAdd( &ref_count, 1 ) + 1;
        printf( "[%d] CMemAlloc::QueryInterface(IUnknown) - new_ref_count=%ld\n", index, cnt );
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
        printf( "[%d] CMemAlloc::AllocateBuffer: buf_size=0x%08lx is not a sane value.\n", index, (unsigned long)buf_size );
        fflush(stdout);
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
        {
            ptr = (char*)MemAlloc(buf_size);
        }

        alloc_buffers[ptr] = buf_size;
    }
    catch(...)
    {
        printf( "[%d] CMemAlloc::AllocateBuffer: allocation failed (buf_size=%lu).\n", index, (unsigned long)buf_size );
        fflush(stdout);
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

    MemFree(buffer);
    return S_OK;
}

//---------------------------------------------------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE CMemAlloc::Commit(void)
{
    printf( "[%d] CMemAlloc::Commit\n", index );
    return S_OK;
}

//---------------------------------------------------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE CMemAlloc::Decommit(void)
{
    printf( "[%d] CMemAlloc::Decommit\n", index );
    return S_OK;
}

//=====================================================================================================================
class CDeviceItem
{
public:
    IDeckLink* deck_link;
    CMemAlloc  alloc;
    CInputCallback  callback;

    CDeviceItem(): deck_link(NULL)  {}
    ~CDeviceItem()  {  if( deck_link != NULL)  deck_link->Release();  }

    void SetIndex( int j )  { alloc.index = j; callback.index = j; }
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

    printf( "[%d] IDeckLink::QueryInterface(IID_IDeckLinkConfiguration)...\n", item.callback.index );
    hr = item.deck_link->QueryInterface( IID_IDeckLinkConfiguration, (void**)&conf );
    if( FAILED(hr) )
    {
        printf( "[%d] IDeckLink::QueryInterface(IID_IDeckLinkConfiguration) failed.\n", item.callback.index  );
        assert( conf == NULL );
        conf = NULL;
    }
    else
    {
        printf( "[%d] IDeckLinkConfiguration::"
                "SetInt( bmdDeckLinkConfigVideoInputConnection, bmdVideoConnectionSDI )...\n", item.callback.index  );
        hr = conf->SetInt( bmdDeckLinkConfigVideoInputConnection, bmdVideoConnectionSDI );

        if( FAILED(hr) )
        {
            printf( "[%d] IDeckLinkConfiguration::SetInt( "
                    "bmdDeckLinkConfigVideoInputConnection, bmdVideoConnectionSDI ) failed.\n", item.callback.index  );
        }
    }

    fflush(stdout);
#endif

    Int32AtomicAdd( &g_thread_count, 1 );

    while( g_thread_count > VALIDATION_RESERVE )
    {
        printf( "\n[%d] Starting Video+Audio Capture...\n", item.callback.index );

        IDeckLinkInput* input;
        printf( "[%d] IDeckLink::QueryInterface(IID_IDeckLinkInput)...\n", item.callback.index );
        fflush(stdout);
        hr = item.deck_link->QueryInterface( IID_IDeckLinkInput, (void**)&input );
        if( FAILED(hr) )
        {
            printf( "[%d] IDeckLink::QueryInterface(IID_IDeckLinkInput) failed.\n", item.callback.index  );
            fflush(stdout);
            break;
        }

#ifndef DISABLE_CUSTOM_ALLOCATOR
        printf( "[%d] IDeckLinkInput::SetVideoInputFrameMemoryAllocator...\n", item.callback.index );
        fflush(stdout);
        hr = input->SetVideoInputFrameMemoryAllocator(&item.alloc);
        if( FAILED(hr) )
        {
            printf( "[%d] IDeckLinkInput::SetVideoInputFrameMemoryAllocator(obj) failed.\n", item.callback.index );
            fflush(stdout);
        }
        else
        {
#endif
            printf( "[%d] IDeckLinkInput::EnableVideoInput display_mode=%s\n", item.callback.index,
                                                                        DisplayModeName(item.callback.display_mode) );
            fflush(stdout);
            hr = input->EnableVideoInput(
                                    item.callback.display_mode, bmdFormat8BitYUV, bmdVideoInputEnableFormatDetection );

            if( FAILED(hr) )
            {
                printf( "[%d] IDeckLinkInput::EnableVideoInput failed.\n", item.callback.index );
                fflush(stdout);
            }
            else
            {
                printf( "[%d] IDeckLinkInput::EnableAudioInput...\n", item.callback.index );
                fflush(stdout);
                hr = input->EnableAudioInput( bmdAudioSampleRate48kHz, bmdAudioSampleType32bitInteger, 16 );
                if( FAILED(hr) )
                {
                    printf( "[%d] IDeckLinkInput::EnableAudioInput failed.\n", item.callback.index );
                    fflush(stdout);
                }
                else
                {
                    printf( "[%d] IDeckLinkInput::SetCallback(obj)...\n", item.callback.index );
                    fflush(stdout);
                    hr = input->SetCallback(&item.callback);
                    if( FAILED(hr) )
                    {
                        printf( "[%d] IDeckLinkInput::SetCallback failed.\n", item.callback.index );
                        fflush(stdout);
                    }
                    else
                    {
                        printf( "[%d] IDeckLinkInput::StartStreams...\n", item.callback.index );
                        fflush(stdout);
                        hr = input->StartStreams();
                        if( FAILED(hr) )
                        {
                            printf( "[%d] IDeckLinkInput::StartStreams failed.\n", item.callback.index );
                            fflush(stdout);
                        }
                        else
                        {
                            item.callback.need_restart.Wait();

                            printf("[%d] IDeckLinkInput::StopStreams...\n", item.callback.index);
                            hr = input->StopStreams();
                            if( FAILED(hr) )
                            {
                                printf( "[%d] IDeckLinkInput::StopStreams failed.\n", item.callback.index );
                            }
                        }

                        printf( "[%d] IDeckLinkInput::SetCallback(NULL)...\n", item.callback.index);
                        hr = input->SetCallback(NULL);
                        if( FAILED(hr) )
                        {
                            printf( "[%d] IDeckLinkInput::SetCallback failed.\n", item.callback.index );
                        }
                    }

                    printf( "[%d] IDeckLinkInput::DisableAudioInput...\n", item.callback.index );
                    hr = input->DisableAudioInput();
                    if( FAILED(hr) )
                    {
                        printf( "[%d] IDeckLinkInput::DisableAudioInput failed.\n", item.callback.index );
                    }
                }

                printf( "[%d] IDeckLinkInput::DisableVideoInput...\n", item.callback.index );
                hr = input->DisableVideoInput();
                if( FAILED(hr) )
                {
                    printf( "[%d] IDeckLinkInput::DisableVideoInput failed.\n", item.callback.index );
                }
            }

#ifndef DISABLE_CUSTOM_ALLOCATOR
#if 0
            hr = input->SetVideoInputFrameMemoryAllocator(NULL);
            if( FAILED(hr) )
            {
                printf("[%d] IDeckLinkInput::SetVideoInputFrameMemoryAllocator(NULL) failed.\n",item.callback.index);
            }
#endif
        }
#endif
        printf( "[%d] IDeckLinkInput::Release...\n", item.callback.index );
        input->Release();

        printf( "[%d] Stopped Video+Audio Capture.\n\n", item.callback.index );
        item.callback.need_restart.SetFalse();
        fflush(stdout);

        if( g_thread_count < VALIDATION_RESERVE )
        {
            break;
        }

        printf( "[%d] Waiting 1 sec...\n\n", item.callback.index );
        fflush(stdout);
        WaitSec(1);

        if( !item.alloc.Reset() )
        {
            fflush(stdout);
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
            fflush(stdout);

            deckLinkAPIInformation->Release();
        }
    }

    fprintf( stderr, "\nRunning video+audio capture tests...\n" );

    IDeckLink*  deck_link;

    for(  int j = 0;  j < g_items_count  &&  deckLinkIterator->Next(&deck_link) == S_OK;  ++j  )
    {
        CDeviceItem& item = g_items[j];
        item.SetIndex(j);
        item.deck_link = deck_link;
        StartThread( &ThreadFunc, &item );
    }

    g_test_finished.Wait();
    fprintf( stderr, "\n!!!VALIDATION FAILED!!!\nPress ENTER to exit...\n" );
    getc(stdin);

    deckLinkIterator->Release();
    return 0;
}
