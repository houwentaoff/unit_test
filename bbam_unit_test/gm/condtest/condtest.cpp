
#include <pthread.h>
#include <assert.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>

#define	DEBUG_PRINT(...) do{fprintf(stderr,"[%-30s][%5d]:\t",__FILE__,__LINE__);fprintf(stderr,__VA_ARGS__);}while(0)


typedef bool BOOL;
class CConditionWait
{
public:
    CConditionWait ( int timeout );
    ~CConditionWait();
    BOOL Signal();
    BOOL Wait();
private:
    int m_Timeout;
    int m_Count;
    pthread_mutex_t m_CondMutex;
    pthread_mutex_t m_CountMutex;
    pthread_cond_t m_Cond;
};

CConditionWait::CConditionWait ( int timeout ) : m_Timeout ( timeout ), m_Count ( 0 )
{
    pthread_mutex_init ( & ( this->m_CondMutex ) , NULL );
    pthread_cond_init ( & ( this->m_Cond ), NULL );
    pthread_mutex_init ( & ( this->m_CountMutex ), NULL );
}

CConditionWait::~CConditionWait()
{
    pthread_cond_destroy ( & ( this->m_Cond ) );
    pthread_mutex_destroy ( & ( this->m_CountMutex ) );
    pthread_mutex_destroy ( & ( this->m_CondMutex ) );
}

BOOL CConditionWait::Signal()
{
    int ret = -1;
    int signal = 0, count;

    pthread_mutex_lock ( & ( this->m_CondMutex ) );
    pthread_mutex_lock ( & ( this->m_CountMutex ) );
    count = this->m_Count;
    if ( count == -1 )
    {
        signal = 1;
    }
    else if ( count == 1 )
    {
    }
    else if ( count == 0 )
    {
        this->m_Count = 1;
    }
    else
    {
        assert ( count == 0 );
    }
    pthread_mutex_unlock ( & ( this->m_CountMutex ) );
    if ( signal )
    {
        ret = pthread_cond_signal ( & ( this->m_Cond ) );
    }
    pthread_mutex_unlock ( & ( this->m_CondMutex ) );
    return ret == 0 ? true : false;
}

BOOL CConditionWait::Wait()
{
    timespec tm;
    BOOL bret = false;
    int ret;
    int wait = 0;
    int count;

    pthread_mutex_lock ( & ( this->m_CondMutex ) );
    pthread_mutex_lock ( & ( this->m_CountMutex ) );
    count = this->m_Count;
    if ( count == -1 )
    {
        ;
    }
    else if ( count == 1 )
    {
        this->m_Count = 0;
    }
    else if ( count == 0 )
    {
        /*if we have not set for the count ,so we should make the count -1
        	and wait for it*/
        wait = 1;
        this->m_Count = -1;
    }
    else
    {
        assert ( count == 0 );
    }
    pthread_mutex_unlock ( & ( this->m_CountMutex ) );

    if ( wait )
    {
        tm.tv_sec = time ( NULL ) + this->m_Timeout;
        tm.tv_nsec = 0;

        ret = pthread_cond_timedwait ( & ( this->m_Cond ), & ( this->m_CondMutex ), &tm );
        if ( ret == 0 )
        {
            bret = true;
        }
    }

    pthread_mutex_lock ( & ( this->m_CountMutex ) );
    count = this->m_Count;
    if ( count == -1 )
    {
        this->m_Count = 0;
    }
    else if ( count == 1 )
    {
        this->m_Count = 0;
    }
    else if ( count == 0 )
    {
        ;
    }
    else
    {
        assert ( count == 0 );
    }
    pthread_mutex_unlock ( & ( this->m_CountMutex ) );

    pthread_mutex_unlock ( & ( this->m_CondMutex ) );

    return bret;
}


static int st_WaitTime = 10;
static int st_RunLoop = 1;
void Sighandler ( int signo )
{
    st_RunLoop = 0;
}

void* ChildThread ( void* arg )
{
    CConditionWait* pWait = ( CConditionWait* ) arg;

    while ( st_RunLoop )
    {
        fprintf ( stderr, "*" );
        fflush ( stderr );
        pWait->Wait();
        fprintf ( stderr, "/" );
        fflush ( stderr );
    }

    return ( void* ) 0;
}

int main ( int argc, char*argv[] )
{
    int ret, res;
    pthread_t chldthd = 0;
    void* pRetVal;
    CConditionWait* pCond = NULL;

    if ( argc > 1 )
    {
        st_WaitTime = atoi ( argv[1] );
    }

	signal(SIGINT,Sighandler);
	signal(SIGTERM,Sighandler);

    pCond = new CConditionWait ( st_WaitTime );
    ret = pthread_create ( &chldthd, NULL, ChildThread, pCond );
    if ( ret < 0 )
    {
        chldthd = 0;
        goto out;
    }

    while ( st_RunLoop )
    {
        sleep ( 12 );
        fprintf ( stderr, "+" );
        fflush ( stderr );
        pCond->Signal();
        fprintf ( stderr, "-" );
        fflush ( stderr );
    }

    ret = 0;

out:
    if ( chldthd )
    {
        res = pthread_join ( chldthd, &pRetVal );
        if ( ret >= 0 && res < 0 )
        {
            ret = res;
        }
    }
    chldthd = 0;

    if ( pCond )
    {
        delete pCond;
    }
    pCond = NULL;

    return ret;
}

