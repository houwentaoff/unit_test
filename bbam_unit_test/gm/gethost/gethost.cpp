#include <stdlib.h>
#include <stdio.h>
#include <memory>
#include <netdb.h>
#include <sys/socket.h>
#include <errno.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#ifndef	DEBUG_PRINT
#define	DEBUG_PRINT(...) do{fprintf(stdout,"[%-30s][%5d]:\t",__FILE__,__LINE__);fprintf(stdout,__VA_ARGS__);}while(0)
#endif


int getipaddress ( char* ip )
{
    int hentsize = 512;
    std::auto_ptr<char> pBuf2 ( new char[hentsize] );
	char* pBuf=NULL;
    struct hostent *hent = NULL;
	int tryagain=0;
	int ret;
    struct hostent hentdummy;
	struct sockaddr_in saddr;
    int herror = 0;


try_alloc:
    pBuf2.reset ( new char[hentsize] );
    pBuf = pBuf2.get();
    herror = 0;
    ret = gethostbyname_r ( ip, &hentdummy, pBuf, hentsize, &hent, &herror );
    if ( ret < 0 )
    {
        if ( herror == TRY_AGAIN )
        {
            DEBUG_PRINT ( "hentsize %d errno = %d\n", hentsize, errno );
            tryagain ++;
            if ( tryagain > 3 )
            {
				ret = -herror;
                goto fail;
            }
            goto try_alloc;
        }
        /*can not get the */
		ret = -herror;
        goto fail;
    }
    if ( herror == -1 )
    {
        hentsize <<= 1;
        DEBUG_PRINT ( "hentsize %d errno = %d\n", hentsize, errno );
        goto try_alloc;
    }
    if ( herror )
    {
        if ( herror == TRY_AGAIN )
        {
            DEBUG_PRINT ( "hentsize %d errno = %d\n", hentsize, errno );
            tryagain ++;
            if ( tryagain > 3 )
            {
				ret = -herror;
                goto fail;
            }
            goto try_alloc;
        }
		ret = -herror;
        goto fail;
    }

	if (hent->h_length <= (int)sizeof(saddr.sin_addr))
	{
    	memcpy ( &saddr.sin_addr, hent->h_addr, hent->h_length );
	}
	else
	{
		memcpy ( &saddr.sin_addr, hent->h_addr, sizeof(saddr.sin_addr) );
	}

	fprintf(stderr,"%s address %s\n",ip,inet_ntoa(saddr.sin_addr));

	/*all is ok*/
	return 0;
	

fail:
	return ret < 0 ? ret : -1;
}


int main(int argc,char*argv[])
{

	if (argc < 2)
	{
		fprintf(stderr,"%s name\n",argv[0]);
		return -3;
	}

	return getipaddress(argv[1]);
}
