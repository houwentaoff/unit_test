
#include <mw_uploadwarn.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <memory>
#include <signal.h>
#include <assert.h>
#include <unistd.h>

#ifndef ERROR_PRINT
#define ERROR_PRINT(...) do{{fprintf(stderr,"[%-30s][%5d][ERROR]:\t",__FILE__,__LINE__);fprintf(stderr,__VA_ARGS__);}}while(0)
#endif

#ifndef DEBUG_PRINT
#define DEBUG_PRINT(...) do{{fprintf(stdout,"[%-30s][%5d][INFO]:\t",__FILE__,__LINE__);fprintf(stdout,__VA_ARGS__); fflush(stdout);}}while(0)
#endif
char* SkipSpace ( char* pChar )
{
    char* pCur = pChar;
    if ( pCur )
    {
        while ( isspace ( *pCur ) )
        {
            pCur ++;
        }
    }
    return pCur;
}

int Atoi ( char* pChar, char**ppRetChar )
{
    int num = 0;
    char* pCurPtr = pChar;
    *ppRetChar = pChar;
    if ( pChar )
    {
        num = atoi ( pChar );
        while ( isdigit ( *pCurPtr ) )
        {
            pCurPtr ++;
        }
        *ppRetChar = pCurPtr;
    }

    return num;
}

char* CopyString ( char* pBuffer, int size, char*pChar )
{
    char* pCurChar = pChar;
    int inquote = 0;
    int len = 0;

    if ( *pCurChar == '"' || *pCurChar == '\'' )
    {
        inquote = *pCurChar;
        pCurChar ++;
    }

    while ( 1 )
    {
        if ( inquote )
        {
            if ( *pCurChar == inquote )
            {
                pCurChar ++;
                break;
            }
        }
        else if ( isspace ( *pCurChar ) || ( *pCurChar == '\r' || *pCurChar == '\n' || *pCurChar == '\0' ) )
        {
            pCurChar ++;
        }

        if ( len < ( size - 1 ) )
        {
            pBuffer[len] = *pCurChar;
        }

        len ++;
        pCurChar ++;
    }
	if (len < (size - 1))
	{
		pBuffer[len]='\0';
	}
	else
	{
		pBuffer[size-1]='\0';
	}

    return pCurChar;
}

/****************************************************
FILE line like this
emergency level state "name" type "description"
0 2 1 "ptz" 3 "ptz alarm"
****************************************************/
#define MUST_HAS_SKIPSPACE()\
do\
{\
	pLastPtr = pCurPtr;\
	pCurPtr = SkipSpace(pLastPtr);\
	if (pCurPtr == pLastPtr)\
	{\
		ret = -EINVAL;\
		goto out;\
	}\
	}while(0)

#define MUST_HAS_ATOI(val)\
do\
{\
	pLastPtr = pCurPtr;\
	val = Atoi(pLastPtr,&pCurPtr);\
	if (pLastPtr == pCurPtr)\
	{\
		ret = -EINVAL;\
		goto out;\
	}\
}while(0)

#define MUST_COPY_STRING(val,size)\
do\
{\
	pLastPtr = pCurPtr;\
	pCurPtr = CopyString(val,size,pLastPtr);\
	if (pCurPtr == pLastPtr)\
	{\
		ret = -EINVAL;\
		goto out;\
	}\
}while(0)


int ParseFileRead ( FILE* fp, int& emergency, LPALARMINFO pAlarmInfo )
{
    char* pLine = NULL;
    size_t linesize = 0;
    char* pCurPtr = NULL, *pLastPtr;
    int ret;

    ret = getline ( &pLine, &linesize, fp );
    if ( ret < 0 )
    {
        goto out;
    }
    pCurPtr = pLine;
    pLastPtr = pCurPtr;

    pCurPtr = SkipSpace ( pLastPtr );
    pLastPtr = pCurPtr;
    MUST_HAS_ATOI ( emergency );

    MUST_HAS_SKIPSPACE();
    MUST_HAS_ATOI ( pAlarmInfo->iLevel );

    MUST_HAS_SKIPSPACE();
    MUST_HAS_ATOI ( pAlarmInfo->iState );

    MUST_HAS_SKIPSPACE();
    MUST_COPY_STRING ( pAlarmInfo->szDevName, sizeof ( pAlarmInfo->szDevName ) );

    MUST_HAS_SKIPSPACE();
    MUST_HAS_ATOI ( pAlarmInfo->dwType );

    MUST_HAS_SKIPSPACE();
    MUST_COPY_STRING ( pAlarmInfo->szDescript, sizeof ( pAlarmInfo->szDescript ) );

    /*all is ok*/
    ret = 0;

out:
    if ( pLine )
    {
        free ( pLine );
    }
    pLine = NULL;
    return ret;
}

#undef MUST_COPY_STRING
#undef MUST_HAS_SKIPSPACE

static int st_RunLoop = 1;
void Sighandler ( int signo )
{
	fprintf ( stderr, "++++++++++++++++++call signo %d++++++++++++++++++\n", signo );
    if ( signo == SIGINT || signo == SIGTERM )
    {
        st_RunLoop = 0;
    }
}


int main ( int argc, char* argv[] )
{
    int ret;
    std::auto_ptr<ALARMINFO> pAlarmInfo2 ( new ALARMINFO );
    LPALARMINFO pAlarmInfo = pAlarmInfo2.get();
    FILE* fp = NULL;
    BOOL bret;
    int ermeg;
    int times = 10, i;
	sighandler_t sigret;

    signal ( SIGINT, Sighandler );
    signal ( SIGTERM, Sighandler );
    signal ( SIGSEGV, Sighandler );
	signal ( SIGPIPE,Sighandler );
    if ( argc < 3 )
    {
        ERROR_PRINT( "%s key sendfile [times]\n", argv[0] );
        exit ( 3 );
    }
    bret = InitUploadWarnMessage ();
    if ( !bret )
    {
        ERROR_PRINT( "can not init upload warn message\n" );
    }

    if ( argc >= 3 )
    {
        times = atoi ( argv[2] );
    }

    sigret = signal ( SIGINT, Sighandler );
	assert(sigret == Sighandler);
    sigret = signal ( SIGTERM, Sighandler );
	assert(sigret == Sighandler);
    sigret = signal ( SIGSEGV, Sighandler );
	assert(sigret == Sighandler);
    sigret = signal ( SIGPIPE, Sighandler );
	assert(sigret == Sighandler);

    for ( i = 0; i < times && st_RunLoop; i++ )
    {
        fprintf ( stderr, "On %d times st_RunLoop %d\n", i ,st_RunLoop);
        fp = fopen ( argv[1], "r" );
        if ( fp == NULL )
        {
            ret = -errno;
            ERROR_PRINT(  "can not open %s for read\n", argv[1] );
            goto out;
        }
        while ( st_RunLoop )
        {
            ret = ParseFileRead ( fp, ermeg, pAlarmInfo );
            if ( ret < 0 )
            {
                if ( !feof ( fp ) )
                {
                    /*it is not the end of the file*/
                    ret = -1;
                    goto out;
                }
                break;
            }

            bret = false;
            if ( ermeg  == 1 )
            {
				DEBUG_PRINT("name [%s]\n",pAlarmInfo->szDevName);
                bret = UploadWarnMessageEmergency (
                           pAlarmInfo->iLevel,
                           pAlarmInfo->iState,
                           pAlarmInfo->szDevName,
                           pAlarmInfo->dwType,
                           pAlarmInfo->szDescript );
            }
            else if ( ermeg == 2 )
            {
                bret = NotifyWarnMessageConfig();
            }
            else if (ermeg == 0)

            {
                bret = UploadWarnMessage (
                           pAlarmInfo->iLevel,
                           pAlarmInfo->iState,
                           pAlarmInfo->szDevName,
                           pAlarmInfo->dwType,
                           pAlarmInfo->szDescript );
            }
            if ( !bret )
            {
                ERROR_PRINT( "Give Emergency %d error\n", ermeg );
            }
        }
        fclose ( fp );
        fp  = NULL;
    }

    /*all is success */
    ret = 0;

out:
    if ( fp )
    {
        fclose ( fp );
    }
    fp = NULL;
    CloseUploadWarnMessage();
    return ret;
}
