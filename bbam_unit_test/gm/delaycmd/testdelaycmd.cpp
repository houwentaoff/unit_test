#include <memory>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <assert.h>
#include <errno.h>


#include "timesock.cpp"

#define APP_INFO(...) do{{fprintf(stdout,"[%-30s][%5d][INFO]:\t",__FILE__,__LINE__);fprintf(stdout,__VA_ARGS__);}}while(0)
#define APP_WARNING(...)  do{{fprintf(stdout,"[%-30s][%5d][WARNING]:\t",__FILE__,__LINE__);fprintf(stdout,__VA_ARGS__);}}while(0)

#define APP_ASSERT assert

#define SET_ARGS(...) \
do\
{\
	ret = snprintf(pCurPtr,leftsize,__VA_ARGS__);\
	if (ret >= leftsize)\
	{\
		maxsize <<=1;\
		goto try_again;\
	}\
	pArgs[argcount]=pCurPtr;\
	pCurPtr += (ret+1);\
	leftsize -= (ret+1);\
	argcount ++;\
}while(0)

static int execute_command_delay ( const char *cmd, unsigned int delay )
{
    int maxsize = 512;
    std::auto_ptr<char> pChar2 ( new char[maxsize] );
    char* pChar;
	struct rlimit Rlimit;
    int argcount = 0;
    char* pArgs[30];
    char* pCurPtr;
    int leftsize = 0;
    int ret;
	int pid=-1;
	int status;
	int i;
    APP_INFO ( "%s\n", cmd );

try_again:
    pChar2.reset ( new char[maxsize] );
    pChar = pChar2.get();
    pCurPtr = pChar;
    leftsize = maxsize;
    argcount = 0;
    SET_ARGS ( "sh" );
    SET_ARGS ( "-c" );
    SET_ARGS ( "sleep %d ; %s",delay , cmd );

    APP_ASSERT ( argcount < (int)( sizeof ( pArgs ) / sizeof ( pArgs[0] ) ) );
    pArgs[argcount] = NULL;

    /*now to fork */
	errno = 0;
	pid = fork();
	if (pid < 0)
	{
		ret = -errno;
		APP_WARNING("fork %s cmd error %d %m\n",cmd,errno);
		return ret;
	}
	else if (pid== 0)
	{
		/*child*/
		errno = 0;
		ret = getrlimit(RLIMIT_NOFILE,&Rlimit);
		if (ret < 0)
		{
			APP_WARNING("could not get RLIMIT_NOFILE %d %m\n",errno);
			exit (3);
		}

		for (i=3;i<(int)Rlimit.rlim_cur;i++)
		{
			close(i);
		}	
		setsid();
		errno = 0;
		pid = fork();
		if (pid < 0)
		{
			APP_WARNING("could not fork in child %d %m\n",errno);
			exit (3);
		}
		else if (pid > 0)
		{
			/*in father exit quickly*/
			exit(0);
		}
		
		execv("/bin/sh",pArgs);
		APP_WARNING("could not do this cmd %s",cmd);
		exit (3);
	}

	/*father */
	do
	{
		ret = waitpid(-1,&status,WNOHANG);
		if (ret == pid)
		{
            if ( WIFEXITED ( status ) )
            {
                APP_INFO ( "process [%d] exit value is %d\n", pid, WEXITSTATUS ( status ) );
                return ( 0 == WEXITSTATUS ( status ) ? 0 : -1 );
            }

            if ( WIFSIGNALED ( status ) )
            {
                APP_INFO ( "process [%d] is stopped by signal [%d]", pid, WTERMSIG ( status ) );
                return -1;
            }			
		}
		sleep(1);
	}while(1);

	return -1;
}


int main(int argc,char* argv[])
{
	int ret;
	unsigned int delay;
	int port=17923;
	std::auto_ptr<CTimeSocket> pSocket2(new CTimeSocket(5));
	CTimeSocket* pSocket= pSocket2.get();
	BOOL bret;

	if (argc < 3)
	{
		fprintf(stderr,"%s cmd delaytime\n",argv[0]);
		exit(3);
	}

	if (argc > 3)
	{
		port = atoi(argv[3]);
	}

	bret = pSocket->Bind(port,1);
	if (!bret)
	{
		APP_WARNING("could not bind %d port %d\n",port,pSocket->GetError());
		exit(3);
	}

	delay = atoi(argv[2]);
	ret = execute_command_delay(argv[1],delay);
	return ret;
}
