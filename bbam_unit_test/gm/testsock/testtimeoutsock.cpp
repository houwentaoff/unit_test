
#include <stdio.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>


typedef int BOOL;

#define	DEBUG_PRINT(...) do{fprintf(stderr,"[%-30s][%5d]:\t",__FILE__,__LINE__);fprintf(stderr,__VA_ARGS__);}while(0)


typedef unsigned int u32;
#include "./timesock.cpp"

static int st_RunLoop = 1;
static int st_Timeout = 10;
static char st_Buffer[64];

int ServerHandle ( int accsock )
{
    BOOL bret;
    std::auto_ptr<CTimeSocket> pSocket2 ( new CTimeSocket ( st_Timeout ) );
    CTimeSocket* pSocket = pSocket2.get();
    bret = pSocket->InitAcceptSocket ( accsock );
    if ( !bret )
    {
        DEBUG_PRINT ( "can not init socket %d\n", accsock );
    }
    while ( st_RunLoop )
    {
        bret = pSocket->Read ( st_Buffer, sizeof ( st_Buffer ) );
        if ( !bret )
        {
            DEBUG_PRINT ( "\n" );
            return -1;
        }

        bret = pSocket->Write ( st_Buffer, sizeof ( st_Buffer ) );
        if ( !bret )
        {
            DEBUG_PRINT ( "\n" );
            return -1;
        }
    }

    return 0;
}

void SigHandler ( int signo )
{
    int status;
    pid_t chldpid;
    switch ( signo )
    {
    case SIGCHLD:
        do
        {
            chldpid = waitpid ( -1, &status, WNOHANG );
            DEBUG_PRINT ( "\n" );
        }
        while ( chldpid >= 0 );
        break;
    case SIGPIPE:
        DEBUG_PRINT ( "\n" );
        break;
    default:
        st_RunLoop = 0;
        DEBUG_PRINT ( "\n" );
        break;
    }
    return ;
}

int TimeServer ( char * portstr, char * timeoutstr )
{
    int port = atoi ( portstr );
    int timeout = atoi ( timeoutstr );
    std::auto_ptr<CTimeSocket> pSocket2 ( new CTimeSocket ( timeout ) );
    CTimeSocket *pSocket = pSocket2.get();
    int accsock = -1;
    struct sockaddr_in saddr;
    int ssize = 0;
    pid_t chldpid = -1;

    BOOL bret;

    st_Timeout = timeout;
    bret = pSocket->Bind ( port, 1 );
    if ( !bret )
    {
        DEBUG_PRINT ( "\n" );
        return -1;
    }

    while ( st_RunLoop )
    {
        ssize = sizeof ( saddr );
        bret = pSocket->AcceptSocket ( accsock, &saddr, &ssize, false );
        if ( !bret  )
        {
            if ( pSocket->GetError() != EINTR )
            {
                DEBUG_PRINT ( "\n" );
                return -1;
            }
            continue;
        }

        DEBUG_PRINT ( "accsock %d\n", accsock );

        chldpid = fork();
        if ( chldpid < 0 )
        {
            DEBUG_PRINT ( "\n" );
            return -1;
        }
        else if ( chldpid == 0 )
        {
            pSocket->Close();
            ServerHandle ( accsock );
            exit ( 0 );
        }

        close ( accsock );
        accsock = -1;
        DEBUG_PRINT ( "\n" );

    }

    return 0;

}

static char st_CmpBuffer[sizeof ( st_Buffer )];

int TimeClient ( char * ip, char * portstr, char * timeoutstr )
{
    int timeout = atoi ( timeoutstr );
    int port = atoi ( portstr );
    std::auto_ptr<CTimeSocket> pSocket2 ( new CTimeSocket ( timeout ) );
    CTimeSocket* pSocket = pSocket2.get();
    BOOL bret;
    char ch;

    bret = pSocket->Connect ( ip, port );
    if ( !bret )
    {
        DEBUG_PRINT ( "\n" );
        return -1;
    }

    ch = 'a';
    while ( st_RunLoop )
    {
        int gch;
        fprintf ( stdout, "Please enter a char into send:" );
        fflush ( stdout );
        while ( 1 )
        {
            gch = fgetc ( stdin );
            if ( gch != EOF || st_RunLoop == 0)
            {
                break;
            }
			DEBUG_PRINT("gch = %d\n",gch);
        }
        if ( ch > 'z'  || ch <  'a' )
        {
            ch = 'a';
        }
        memset ( st_Buffer, 0, sizeof ( st_Buffer ) );
        memset ( st_Buffer, ch, sizeof ( st_Buffer ) - 1 );

        bret = pSocket->Write ( st_Buffer, sizeof ( st_Buffer ) );
        if ( !bret )
        {
            DEBUG_PRINT ( "\n" );
            return -1;
        }

        bret = pSocket->Read ( st_CmpBuffer, sizeof ( st_CmpBuffer ) );
        if ( !bret )
        {
            DEBUG_PRINT ( "\n" );
            return -1;
        }

        if ( memcmp ( st_CmpBuffer, st_Buffer, sizeof ( st_Buffer ) ) )
        {
            DEBUG_PRINT ( "\n" );
            return -1;
        }
        DEBUG_PRINT ( "write and read %c ok\n", ch );
        ch ++;
    }

    return 0;

}

int main ( int argc, char * argv[] )
{
    int ret;

    signal ( SIGINT, SigHandler );
    signal ( SIGTERM, SigHandler );
    signal ( SIGCHLD, SigHandler );
    signal ( SIGPIPE, SigHandler );
    if ( argc < 3 )
    {
        fprintf ( stderr, "%s port timeout || ipaddr port timeout\n", argv[0] );
        exit ( 3 );
    }

    if ( argc == 3 )
    {
        ret = TimeServer ( argv[1], argv[2] );
    }
    else
    {
        ret = TimeClient ( argv[1], argv[2], argv[3] );
    }
    return ret;
}

