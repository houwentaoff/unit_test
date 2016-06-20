
#include <config.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <mw_uploadwarn_internal.h>
#include <uploadwarn.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <execinfo.h>

#define	DEBUG_PRINT(...) do{fprintf(stderr,"[%-30s][%5d]:\t",__FILE__,__LINE__);fprintf(stderr,__VA_ARGS__);}while(0)


static void BackTrace_Symbol()
{
    void *array[30];
    size_t size;
    int i;

    char **strings;
    size = backtrace ( array, 30 );
    strings = backtrace_symbols ( array, size );
    for ( i = 0; i < ( int ) size; i++ )
    {
        fprintf ( stderr, "%s\n", strings[i] );
    }
    return ;
}



static int st_RunLoop = 1;
void Sighandler ( int signo )
{
	DEBUG_PRINT("caught signo %d SIGPIPE %d\n",signo,SIGPIPE);
    if ( signo == SIGCHLD )
    {
        int status;
        waitpid ( -1, &status, 0 );
    }
    else if ( signo == SIGSEGV )
    {
        BackTrace_Symbol();
        exit ( 3 );
    }
    else if (signo == SIGINT || signo == SIGTERM)
    {	
        st_RunLoop = 0;
    }

    return ;
}

int main ( int argc, char* argv[] )
{
    CUploadWarn* pUploadWarn = NULL;
    int ret = -1;
    BOOL bret;
    key_t key = 0;
    char* pConfigfile = NULL;
	int i;

	for (i=0;i<SIGRTMAX;i++)
	{
		signal ( i, Sighandler );
	}
	key = key;

    if ( argc < 3 )
    {
        fprintf ( stderr, "%s key cfgfile\n", argv[0] );
        exit ( 3 );
    }
    key = atoi ( argv[1] );
    pConfigfile = argv[2];
    pUploadWarn = new CUploadWarn ( GMI_MSG_UPLOADWARN_KEY, pConfigfile );
    bret = pUploadWarn->Start();
    if ( !bret )
    {
        ret = -1;
        goto out;
    }

    /*now to give the relax*/
    while ( st_RunLoop )
    {
        sleep ( 100 );
		DEBUG_PRINT("\n");
    }
	DEBUG_PRINT("\n");

out:
    if ( pUploadWarn )
    {
		DEBUG_PRINT("\n");
        delete pUploadWarn;
    }
    pUploadWarn = NULL;
    return ret;
}



