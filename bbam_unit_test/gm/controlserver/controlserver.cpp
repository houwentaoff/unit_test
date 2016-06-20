#include <stdlib.h>
#include <stdio.h>
#include <memory>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#define	DEBUG_PRINT(...) do{fprintf(stdout,"[%-30s][%5d][INFO]:\t",__FILE__,__LINE__);fprintf(stdout,__VA_ARGS__);}while(0)
#define	ERROR_PRINT(...) do{fprintf(stderr,"[%-30s][%5d][ERROR]:\t",__FILE__,__LINE__);fprintf(stderr,__VA_ARGS__);}while(0)


struct __ip_info
{
    int dhcp;
    char ip[32];
    char netmask[32];
    char gateway[32];
    char macaddr[32];
    char dns1[64];
    char dns2[64];
    char hostname[64];
};

typedef struct __ip_info IPINFO_t;


/*return 0 for none get count if success ,negative for error*/
int GetCmdPipe(char* pCmd, char* GetValue, int size)
{
    FILE* fp = NULL;
    int ret;
    size_t linesize = 128;
    std::auto_ptr<char> pLine2(new char[linesize]);
    char* pLine = pLine2.get();



    //DEBUG_PRINT("Cmd %s\n",pCmd);
    memset(GetValue, 0, size);
    fp = popen(pCmd, "r");
    if(fp == NULL)
    {
        return -1;
    }

    errno = 0;
    ret = fread(pLine, 1, linesize, fp);
    if(ret < 0)
    {
        ret = -1;
        goto out;
    }

    if(ret > 0)
    {
        strncpy(GetValue, pLine, ret > size ? size - 1 : ret);
    }
    ret = strlen(GetValue);

out:
    if(fp)
    {
        fclose(fp);
    }
    fp = NULL;
    return ret;
}

#define ETC_INTERFACE  "/etc/network/interfaces"
#define NETWORK_ETH    "eth"
#define RESOLVE_CONF   "/etc/resolv.conf"

#define SAFE_SNPRINTF(mark,...) \
do\
{\
	ret = snprintf(pChar,maxsize,__VA_ARGS__);\
	if (ret >= maxsize)\
	{\
		maxsize <<=1 ;\
		goto mark;\
	}\
}while(0)




static int GetIPInfo(struct __ip_info *pIpInfo)
{
    int ret;
    int maxsize = 128;
    int retsize = 128;
    std::auto_ptr<char> pChar2(new char[maxsize]);
    std::auto_ptr<char> pRetChar2(new char[retsize]);
    char* pChar = NULL;
    char* pRetChar = pRetChar2.get();


try_again:
    memset(pIpInfo, 0, sizeof(*pIpInfo));
    pChar2.reset(new char[maxsize]);
    pChar = pChar2.get();


    SAFE_SNPRINTF(try_again, "cat %s | grep -v '^#'| grep -v auto |awk 'BEGIN{p=0}/%s/{p=1} {if (p == 1)print $0}' | grep '%s'",
                  ETC_INTERFACE, NETWORK_ETH, "dhcp");

    ret = GetCmdPipe(pChar, pRetChar, retsize);
    if(ret < 0)
    {
        goto out;
    }

    /*now to copy the dhcp*/
    if(ret == 0)
    {
        pIpInfo->dhcp = 0;
    }
    else
    {
        pIpInfo->dhcp = 1;
    }
    //DEBUG_PRINT("dhcp %d\n",pIpInfo->dhcp);

    if(pIpInfo->dhcp == 0)
    {
        SAFE_SNPRINTF(try_again, "cat %s | grep -v '^#'| grep -v auto | awk 'BEGIN{p=0}/%s/{p++} {if (p == 1)print $0}' | grep '%s' | awk '{printf(\"%%s\",$2)}'",
                      ETC_INTERFACE, NETWORK_ETH, "address");
        ret = GetCmdPipe(pChar, pRetChar, retsize);
        if(ret < 0)
        {
            goto out;
        }

        if(ret == 0)
        {
            ret = -EINVAL;
            goto out;
        }

        strncpy(pIpInfo->ip, pRetChar, sizeof(pIpInfo->ip) - 1);

        SAFE_SNPRINTF(try_again, "cat %s  | grep -v '^#' | grep -v auto | awk 'BEGIN{p=0}/%s/{p++} {if (p == 1)print $0}' | grep '%s' | awk '{printf(\"%%s\",$2)}'",
                      ETC_INTERFACE, NETWORK_ETH, "netmask");
        ret = GetCmdPipe(pChar, pRetChar, retsize);
        if(ret < 0)
        {
            goto out;
        }

        if(ret == 0)
        {
            ret = -EINVAL;
            goto out;
        }

        strncpy(pIpInfo->netmask, pRetChar, sizeof(pIpInfo->netmask) - 1);

        SAFE_SNPRINTF(try_again, "cat %s | grep -v '^#'| grep -v auto | awk 'BEGIN{p=0}/%s/{p++} {if (p == 1)print $0}' | grep '%s' | awk '{printf(\"%%s\",$2)}'",
                      ETC_INTERFACE, NETWORK_ETH, "gateway");
        ret = GetCmdPipe(pChar, pRetChar, retsize);
        if(ret < 0)
        {
            goto out;
        }

        if(ret == 0)
        {
            ret = -EINVAL;
            goto out;
        }
        strncpy(pIpInfo->gateway, pRetChar, sizeof(pIpInfo->gateway) - 1);


        SAFE_SNPRINTF(try_again, "ifconfig %s | grep HWaddr | awk '{printf(\"%%s\",$5)}'", "eth0");
        ret = GetCmdPipe(pChar, pRetChar, retsize);
        if(ret < 0)
        {
            goto out;
        }
        if(ret == 0)
        {
            ret = -EINVAL;
            goto out;
        }
        strncpy(pIpInfo->macaddr, pRetChar, sizeof(pIpInfo->macaddr) - 1);


        SAFE_SNPRINTF(try_again, "cat %s | grep -v '^#' | grep %s | awk 'BEGIN{v=0} /%s/{v++} {if (v == %d) printf(\"%%s\",$2)}'",
                      RESOLVE_CONF, "nameserver", "nameserver", 1);
        ret = GetCmdPipe(pChar, pRetChar, retsize);
        if(ret < 0)
        {
            goto out;
        }

        if(ret == 0)
        {
            ret = -EINVAL;
            goto out;
        }
        strncpy(pIpInfo->dns1, pRetChar, sizeof(pIpInfo->dns1) - 1);



        SAFE_SNPRINTF(try_again, "cat %s | grep -v '^#' | grep %s | awk 'BEGIN{v=0} /%s/{v++} {if (v == %d) printf(\"%%s\",$2)}'",
                      RESOLVE_CONF, "nameserver", "nameserver", 2);
        ret = GetCmdPipe(pChar, pRetChar, retsize);
        if(ret < 0)
        {
            goto out;
        }

        if(ret == 0)
        {
            memset(pIpInfo->dns2, 0, sizeof(pIpInfo->dns2));
        }
        else
        {
            strncpy(pIpInfo->dns2, pRetChar, sizeof(pIpInfo->dns2) - 1);
        }

        SAFE_SNPRINTF(try_again,"hostname | awk '{printf(\"%%s\",$1)}'");
        ret = GetCmdPipe(pChar,pRetChar,retsize);
        if(ret < 0)
        {
            goto out;
        }
        if(ret == 0)
        {
            ret = -EINVAL;
            goto out;
        }

        strncpy(pIpInfo->hostname,pRetChar,sizeof(pIpInfo->hostname)-1);
    }

    ret = 0;
out:
    return ret;

}


enum NetCmdType  /* Network Related */
{
    NOACTION = 0,
    DHCP     = 1,
    STATIC   = 2,
    QUERY    = 3,
    RTSP     = 4,
    REBOOT   = 5,
    /* Video Related */
    GET_ENCODE_SETTING = 6,
    GET_IMAGE_SETTING  = 7,
    GET_PRIVACY_MASK_SETTING = 8,
    GET_VIN_VOUT_SETTING     = 9,
    SET_ENCODE_SETTING       = 10,
    SET_IMAGE_SETTING        = 11,
    SET_PRIVACY_MASK_SETTING = 12,
    SET_VIN_VOUT_SETTING     = 13,
    NETBOOT   = 14,
    HOSTNAME = 100,
    MAC      = 101,
};


struct NetConfigData
{
    NetCmdType command;
    char mac     [16];
    char ip      [40];
    char netmask [40];
    char gateway [40];
    char dns1    [40];
    char dns2    [40];
    char dns3    [40];
    char newmac  [16];
    char hostname[40];
};

#define COPY_STR_CONFIG(dst,src)\
do\
{\
	strncpy(dst,src,sizeof(dst)-1);\
}while(0)

int CopyMacAddr(char* dst,int dstlen,char* src)
{
	char* pCurSrc=src;
	char* pCurDst=dst;
	int leftlen=dstlen;
	while(*pCurSrc)
	{
		if (*pCurSrc != ':')
		{
			if (leftlen <1)
			{
				return -1;
			}
			*pCurDst = *pCurSrc;
			pCurDst ++;
			leftlen --;
		}
		pCurSrc ++;
	}

	return 0;
}

int TransFormNetConfigData(NetConfigData* pConfigData,IPINFO_t *pIpInfo)
{
	int ret;
    memset(pConfigData,0,sizeof(*pConfigData));

	if (pIpInfo->dhcp == 0)
	{
		pConfigData->command = STATIC;
		/*now to copy the mac address*/
		ret = CopyMacAddr(pConfigData->mac,sizeof(pConfigData->mac),pIpInfo->macaddr);		
		if (ret < 0)
		{
			return ret;
		}
		COPY_STR_CONFIG(pConfigData->ip,pIpInfo->ip);
		COPY_STR_CONFIG(pConfigData->netmask,pIpInfo->netmask);
		COPY_STR_CONFIG(pConfigData->gateway,pIpInfo->gateway);
		COPY_STR_CONFIG(pConfigData->dns1,pIpInfo->dns1);
		COPY_STR_CONFIG(pConfigData->dns2,pIpInfo->dns2);
		COPY_STR_CONFIG(pConfigData->hostname,pIpInfo->hostname);
	}
	else
	{
		pConfigData->command = DHCP;
	}

	return 0;
}



int ConnectUDP(char* pIp,int port)
{
	int ret;
	int sock=-1;
	struct sockaddr_in saddr;

	errno = 0;
	sock = socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP);
	if (sock < 0)
	{
		ret = -errno;
		goto fail;
	}

	memset(&saddr,0,sizeof(saddr));
	saddr.sin_family= AF_INET;
	saddr.sin_addr.s_addr = inet_addr(pIp);
	saddr.sin_port = htons(port);

	errno = 0;
	ret = connect(sock,(struct sockaddr*)&saddr,sizeof(saddr));
	if (ret < 0)
	{
		ret = -errno;
		goto fail;
	}
	

	return sock;
fail:
	if (sock>=0)
	{
		close(sock);
	}
	sock = -1;
	return ret < 0 ? ret : -1;
}


int main(int argc,char* argv[])
{
	std::auto_ptr<IPINFO_t> pIpInfo2(new IPINFO_t);
	std::auto_ptr<NetConfigData> pConfigData2(new NetConfigData);
	IPINFO_t *pIpInfo=pIpInfo2.get();
	NetConfigData* pConfigData=pConfigData2.get();
	char* pIp=NULL;
	int port =0;
	int sock=-1;
	int ret;

	if (argc < 3)
	{
		ERROR_PRINT("%s ip port\n",argv[0]);
		exit (3);
	}

	pIp = argv[1];
	port = atoi(argv[2]);

	errno = 0;
	sock = ConnectUDP(pIp,port);
	if (sock < 0)
	{
		
		ret =errno ?  -errno: -1;
		ERROR_PRINT("could not connect %s:%d(%d)%m\n",pIp,port,ret);
		goto out;		
	}

	ret = GetIPInfo(pIpInfo);
	if (ret < 0)
	{
		ERROR_PRINT("could not get ipinfo(%d)\n",ret);
		goto out;
	}

	ret=  TransFormNetConfigData(pConfigData,pIpInfo);
	if (ret < 0)
	{
		ERROR_PRINT("could not transform config data(%d)\n",ret);
		goto out;
	}

	// to give the host name for it will give avahi reboot
	
	pConfigData->command = HOSTNAME;
	errno = 0;
	ret = write(sock,pConfigData,sizeof(*pConfigData));
	if (ret < 0)
	{
		ret = errno ? -errno : -1;
		ERROR_PRINT("could not send config data (%d)%m\n",ret);
		goto out;
	}

	/*all is ok*/
	DEBUG_PRINT("Control server Succ\n");
	ret = 0;

out:
	if (sock>=0)
	{
		close(sock);
	}
	sock = -1;
	return ret;
}

