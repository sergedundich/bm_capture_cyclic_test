#include <memory>
#include <utils.h>

//=====================================================================================================================
namespace {
//-----------------------------------------------------------------------------
struct SParam
{
    FTaskAction Func;
    void* Ctx;

    SParam( FTaskAction func, void* ctx ) : Func(func), Ctx(ctx)  {}
};

//-----------------------------------------------------------------------------
static void* ThreadProc( void* param )
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

    return NULL;
}

} //unnamed namespace
//=====================================================================================================================
void ThreadStart( FTaskAction func, void* ctx )
{
    std::auto_ptr<SParam> paParam( new SParam(func,ctx) );
	pthread_t thr;
	int err = pthread_create( &thr, NULL, &ThreadProc, paParam.get() );

	if( err != 0 )
	{
		fprintf( stderr, "StartThread: pthread_create failed.\n" );
        return;
	}

    pthread_detach(thr);
    paParam.release();
}
