
#include <pthread.h>
#include <semaphore.h>
#include <mw_uploadwarn.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <Pdu.h>
#include <client_camera_protocol.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "./timesock.cpp"

#ifndef DEBUG_PRINT
#define	DEBUG_PRINT(...) do{fprintf(stdout,"[%-30s][%5d]:\t",__FILE__,__LINE__);fprintf(stdout,__VA_ARGS__);}while(0)
#endif

#ifndef ERROR_PRINT
#define ERROR_PRINT(...) do{{fprintf(stderr,"[%-30s][%5d][ERROR]:\t",__FILE__,__LINE__);fprintf(stderr,__VA_ARGS__);}}while(0)
#endif


static int st_RunLoop = 1;

void sigstop ( int signo )
{
    if ( signo == SIGINT || signo == SIGTERM )
    {
		DEBUG_PRINT("Get Signal %d\n",signo);
        st_RunLoop = 0;
    }
    else if ( signo == SIGCHLD )
    {
        int status;
		pid_t pid;
		do
		{
	        pid = waitpid ( -1, &status, WNOHANG );
		}while(pid > 0);
    }

    return ;
}

void ParsePdu ( CGmPdu& pdu )
{
    DWORD code = 0;
    CGmPdu ePdu, oPdu;
    BYTE* pBuffer;
    int len;
    std::string valstr;

    pdu >> code ;
    fprintf ( stdout, "code is %ld\n", code );
    pdu >> ePdu;

    pBuffer = ( LPBYTE ) pdu;
    len = pdu.Length();
    pBuffer += 12; /*skip the code*/
    pBuffer += 8;  /* skip the eBuffer Code*/
    len -= 20;

    oPdu.Assign ( pBuffer, len );
    oPdu >> code ;
    fprintf ( stdout, "cmd 0x%08lx\n", code );
    oPdu >> ePdu;

    ePdu >> code;
    fprintf ( stdout, "iHeight 0x%08lx\n", code );
    ePdu >> code;
    fprintf ( stdout, "iLow 0x%08lx\n", code );
    ePdu >> code;
    fprintf ( stdout, "dwType 0x%08lx\n", code );
    ePdu >> code;
    fprintf ( stdout, "iLevel 0x%08lx\n", code );
    ePdu >> code;
    fprintf ( stdout, "iState 0x%08lx\n", code );

    ePdu >> valstr;
    fprintf ( stdout, "name %s\n", valstr.c_str() );
    ePdu >> valstr;
    fprintf ( stdout, "timestamp %s\n", valstr.c_str() );
    ePdu >> valstr;
    fprintf ( stdout, "description %s\n", valstr.c_str() );
    return ;
}

int ServerHandle ( int sock )
{
    std::auto_ptr<CTimeSocket> pAccSock2 ( new CTimeSocket ( 0 ) );
    CTimeSocket* pAccSock = pAccSock2.get();
    int maxlen = 512;
    std::auto_ptr<char> pChar2 ( new char[maxlen] );
    char* pChar = pChar2.get();
    BOOL bret;
    CGmPdu pdu;
	
    pAccSock->InitAcceptSocket ( sock );

    while ( st_RunLoop )
    {
        unsigned char ch[5];
        int len;
        bret = pAccSock->Read ( ch, 5 );
        if ( !bret )
        {
            ERROR_PRINT (  "server read[5] error %d\n", pAccSock->GetError() );
            return -2;
        }
        len = ch[2] << 8;
        len += ch[3];


        if ( len > maxlen )
        {
            maxlen = len;
            pChar2.reset ( new char[maxlen] );
            pChar = pChar2.get();
        }

		DEBUG_PRINT("want read len %d\n",len-1);
        bret = pAccSock->Read ( pChar, len - 1 );
        if ( !bret )
        {
            ERROR_PRINT ( "server read[%d] error %d\n", len - 1, pAccSock->GetError() );
            return -3;
        }

        DEBUG_PRINT (  "read len %d\n", len - 1 );

        /*now to parse the pdu*/
        pdu.Clear();
        pdu.Assign ( ( LPBYTE ) pChar, len - 1 );
        ParsePdu ( pdu );
    }
	DEBUG_PRINT("\n");
    return 0;
}

int main ( int argc, char* argv[] )
{
    int ret = -1;
    int port;
    BOOL bret;
    std::auto_ptr<CTimeSocket> pServerSock2 ( new CTimeSocket ( 0 ) );
    CTimeSocket* pServerSock = pServerSock2.get();
    struct sockaddr_in sockaddr;
    int socklen;
    int accsock = -1;
    pid_t pid;

    if ( argc < 2 )
    {
        ERROR_PRINT( "%s port\n", argv[0] );
        exit ( 3 );
    }

    port = atoi ( argv[1] );

    signal ( SIGINT, sigstop );
    signal ( SIGTERM, sigstop );
    signal ( SIGPIPE, sigstop );
    signal ( SIGCHLD, sigstop );

    bret = pServerSock->Bind ( port, 1 );
    if ( !bret )
    {
        ret = -1;
        ERROR_PRINT( "bind %d failed\n", port );
        goto out;
    }

    while ( st_RunLoop )
    {
        accsock = -1;
        socklen = sizeof ( sockaddr );
		DEBUG_PRINT("\n");
        bret = pServerSock->AcceptSocket ( accsock, &sockaddr, &socklen, false );
        if ( !bret )
        {
            continue;
        }		

        DEBUG_PRINT (  "accept socket %d\n", accsock );

        pid = fork();
        if ( pid < 0 )
        {

            ret = -errno;
            goto out;
        }
        else if ( pid == 0 )
        {
            pServerSock->Close();
            ret = ServerHandle ( accsock );
			
            exit ( ret );
        }
        close ( accsock );
        accsock = -1;

    }


    ret = 0;
out:
    if ( accsock >= 0 )
    {
        close ( accsock );
    }
    accsock = -1;
    return ret;
}

