
#include <mw_uploadddns.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include "timesock.cpp"
static int st_bRunLoop=1;

#ifndef  APP_ERROR
#define APP_ERROR(...) do{fprintf(stderr,"[%s:%d][%s][ERROR]:\t",__FILE__,__LINE__,__FUNCTION__);fprintf(stderr,__VA_ARGS__);}while(0)
#endif

#ifndef APP_INFO
#define APP_INFO(...) do{fprintf(stdout,"[%s:%d][%s][INFO]:\t",__FILE__,__LINE__,__FUNCTION__);fprintf(stdout,__VA_ARGS__);}while(0)
#endif

typedef struct tm tm_t;

typedef struct __time_value
{
    tm_t tmTime;
    time_t ttTime;

    void GetTime(int offset)
    {
        time_t timep;
        // get next minute time
        time(&timep); /*get UTC time */
        timep += (time_t)(offset);	// new time
        tm_t *pTime = localtime(&timep); /* get local time */
        ttTime = timep;

        //	EU_DEBUG("year=%4d,month=%2d,day=%2d,wday=%d\n",pTime->tm_year,pTime->tm_mon,pTime->tm_mday,pTime->tm_wday);
        //	EU_DEBUG("hour=%2d,min=%2d,sec=%2d\n",pTime->tm_hour,pTime->tm_min,pTime->tm_sec);
        memcpy(&tmTime, pTime, sizeof(tm_t));
    };

    unsigned int DiffTime(time_t inTime)
    {
        return (ttTime -inTime) ;
    };

    void TimeToTm(time_t inTime)
    {
        tm_t *pTime = localtime(&inTime); /* get local time */
        ttTime = inTime;
        memcpy(&tmTime, pTime, sizeof(tm_t));
    }
} TimeValue_t;


void SigHandler(int signo)
{
	if (signo == SIGINT || 
		signo == SIGTERM)
	{
		st_bRunLoop = 0;
	}
	else if (signo == SIGCHLD)
	{
		int status;
		int pid;

		do
		{
			pid = waitpid(-1,&status,WNOHANG);
		}while(pid > 0);
	}
}

static void DebugDDNS(DDNS_INFO_PACKAGE_t* pPkg)
{
	DDNS_INFO_t* pInfo=&(pPkg->DNSInfo);
	TimeValue_t tm;
	tm.GetTime(0);

	fprintf(stdout,"ReceiveTime=%d-%d-%d %d:%d:%d\n",tm.tmTime.tm_year+1900,tm.tmTime.tm_mon+1,tm.tmTime.tm_mday,tm.tmTime.tm_hour,tm.tmTime.tm_min,tm.tmTime.tm_sec);
	fprintf(stdout,"Header=%c%c%c%c\n",pPkg->szHeader[0],pPkg->szHeader[1],pPkg->szHeader[2],pPkg->szHeader[3]);
	fprintf(stdout,"Version=0x%08x\n",pPkg->uVersion);
	fprintf(stdout,"Length=%d\n",pPkg->uLength);
	fprintf(stdout,"Domain=%s\n",pInfo->szDomain);
	fprintf(stdout,"SN=%s\n",pInfo->szSN);
	fprintf(stdout,"IP=%s\n",pInfo->szIp);
	fprintf(stdout,"Name=%s\n",pInfo->szName);
	fprintf(stdout,"Port=%d\n",pInfo->iPort);
}

int HandleDDNSServer(CTimeSocket* pSocket)
{
	BOOL bret;
	std::auto_ptr<DDNS_INFO_PACKAGE_t> pPkg2(new DDNS_INFO_PACKAGE_t);
	DDNS_INFO_PACKAGE_t* pPkg=pPkg2.get();
	pSocket->SetTimeout(0);

	while(st_bRunLoop)
	{
		bret = pSocket->Read(pPkg,sizeof(*pPkg));
		if (! bret )
		{
			int err=pSocket->GetError();
			if (err== EPIPE)
			{
				break;
			}
			else if (err == EINTR)
			{
				continue;
			}
			APP_ERROR("could not read (%d)\n",err);
			break;
		}

		DebugDDNS(pPkg);
	}
	exit (0);
}


int ServerAccept(CTimeSocket* pAccSock)
{
	struct sockaddr_in sockaddr;
	int socklen;
	BOOL bret;
	int accsock=-1;
	CTimeSocket* pHandleSock=NULL;
	int cpid=-1;
	pAccSock->SetTimeout(0);

	while(st_bRunLoop)
	{
		socklen = sizeof(sockaddr);
		accsock = -1;
		bret = pAccSock->AcceptSocket(accsock,&sockaddr,&socklen,false);
		if (!bret)
		{
			continue;
		}

		pHandleSock = new CTimeSocket(0);
		pHandleSock->InitAcceptSocket(accsock);
		accsock = -1;

		cpid = fork();
		if (cpid < 0)
		{
			APP_ERROR("can not fork\n");
			delete pHandleSock;
			pHandleSock = NULL;
		}
		else if (cpid == 0)
		{
			delete pAccSock;
			pAccSock = NULL;
			HandleDDNSServer(pHandleSock);
		}

		delete pHandleSock;
		pHandleSock = NULL;
	}

	exit (0);
}

int main(int argc ,char* argv[])
{
	int port=3301;
	CTimeSocket* pServerSock=NULL;
	BOOL bret;
	if (argc < 2)
	{
		APP_ERROR("%s port ddns listen server\n",argv[0]);
		exit (3);
	}

	signal(SIGINT,SigHandler);
	signal(SIGTERM,SigHandler);	
	signal(SIGCHLD,SigHandler);
	port = atoi(argv[1]);

	pServerSock = new CTimeSocket(0);
	bret = pServerSock->Bind(port,1);
	if (!bret)
	{
		APP_ERROR("can not bind %d (%d)\n",port,pServerSock->GetError());
		delete pServerSock;
		pServerSock= NULL;
		return 3;
	}

	ServerAccept(pServerSock);

	delete pServerSock;
	pServerSock = NULL;
	return 0;
}

