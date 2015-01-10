#include <memory>
#include <utils.h>
#include <stdio.h>

//=====================================================================================================================
namespace {
//---------------------------------------------------------------------------------------------------------------------
struct SParam
{
    FTaskAction Func;
    void* Ctx;

    SParam( FTaskAction func, void* ctx ) : Func(func), Ctx(ctx)  {}
};

//---------------------------------------------------------------------------------------------------------------------
static DWORD WINAPI ThreadProc( LPVOID param )
{
    SParam* p = (SParam*)param;
    FTaskAction func = p->Func;
    void* ctx = p->Ctx;

    delete p;

    InitCom();

    try
    {
        (*func)(ctx);
    }
    catch( const std::exception& ex )
    {
        fprintf( stderr, "ThreadProc: %s\n", ex.what() );
    }
    catch(...)
    {
        fprintf( stderr, "ThreadProc: UNKNOWN ERROR\n" );
    }

    return NO_ERROR;
}

} //unnamed namespace

//=====================================================================================================================
void StartThread( FTaskAction func, void* ctx )
{
    std::auto_ptr<SParam> paParam( new SParam(func,ctx) );
    HANDLE h = ::CreateThread( NULL, 0, &ThreadProc, paParam.get(), 0, NULL );

    if( h == NULL )
    {
        fprintf( stderr, "StartThread: CreateThread failed.\n" );
        return;
    }

    ::CloseHandle(h);
    paParam.release();
}
