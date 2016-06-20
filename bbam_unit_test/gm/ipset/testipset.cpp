#include <memory>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>


#define	DEBUG_PRINT(...) do{fprintf(stderr,"[%-30s][%5d]:\t",__FILE__,__LINE__);fprintf(stderr,__VA_ARGS__);}while(0)


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


int main(int argc,char* argv[])
{
    int ret;
    std::auto_ptr<IPINFO_t> pIpInfo2(new IPINFO_t);
    IPINFO_t *pIpInfo=pIpInfo2.get();

    ret = GetIPInfo(pIpInfo);
    if(ret >= 0)
    {
        DEBUG_PRINT("mode %d\n",pIpInfo->dhcp);
        if(pIpInfo->dhcp == 0)
        {
            DEBUG_PRINT("ip \"%s\"\n",pIpInfo->ip);
            DEBUG_PRINT("netmask \"%s\"\n",pIpInfo->netmask);
            DEBUG_PRINT("gateway \"%s\"\n",pIpInfo->gateway);
            DEBUG_PRINT("macaddr \"%s\"\n",pIpInfo->macaddr);
            DEBUG_PRINT("dns1 \"%s\"\n",pIpInfo->dns1);
            DEBUG_PRINT("dns2 \"%s\"\n",pIpInfo->dns2);
            DEBUG_PRINT("hostname \"%s\"\n",pIpInfo->hostname);
        }
    }

    return ret;
}



