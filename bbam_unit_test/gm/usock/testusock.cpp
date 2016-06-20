#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <errno.h>
#include <stdlib.h>
#include <stdio.h>

#include <sys/socket.h>
#include <sys/un.h>

#include <unistd.h>

int WriteUnixSock(char* sockname,char* pairname,char* str,char* timestr,char* bufsizestr)
{
	int sock=-1;
	struct sockaddr_un sunaddr;
	int ret;
	int bufsize;
	int buflen = atoi(bufsizestr);
	int len;
	int i,time;
	int persize;

	time = atoi(timestr);
	sock = socket(AF_UNIX,SOCK_DGRAM,0);
	if (sock < 0)
	{
		ret = -errno;
		fprintf(stderr,"can not make socket %d %m\n",errno);
		goto out;
	}

	unlink(pairname);
	memset(&sunaddr,0,sizeof(sunaddr));
	sunaddr.sun_family = AF_UNIX;
	strncpy(sunaddr.sun_path,pairname,sizeof(sunaddr.sun_path)-1);

	ret = bind(sock,(struct sockaddr*)&sunaddr,sizeof(sunaddr));
	if (ret < 0)
	{
		ret = -errno;
		fprintf(stderr,"can not bind %s %d %m\n",pairname,errno);
		goto out;
	}

	len = sizeof(bufsize);
	ret = getsockopt(sock,SOL_SOCKET,SO_SNDBUF,&bufsize,(socklen_t*)&len);
	if (ret < 0)
	{
		ret = -errno;
		fprintf(stderr,"can not get socket sndbuf %d %m\n",errno);
		goto out;
	}

	fprintf(stdout,"sendbuf %d\n",bufsize);
	bufsize = buflen;
	len = sizeof(bufsize);
	ret = setsockopt(sock,SOL_SOCKET,SO_SNDBUF,&bufsize,len);
	if (ret < 0 )
	{
		ret = -errno;
		fprintf(stderr,"can not set %d %d %m\n",bufsize,errno);
		goto out;
	}

	len = sizeof(bufsize);
	ret = getsockopt(sock,SOL_SOCKET,SO_SNDBUF,&bufsize,(socklen_t*)&len);
	if (ret < 0)
	{
		ret = -errno;
		fprintf(stderr,"can not get socket sndbuf %d %m\n",errno);
		goto out;
	}
	fprintf(stdout,"after set ,sndbufsize %d\n",bufsize);

	persize = bufsize;
	persize >>= 1;

	bufsize = strlen(str);
	memset(&sunaddr,0,sizeof(sunaddr));
	sunaddr.sun_family = AF_UNIX;
	strncpy(sunaddr.sun_path,sockname,sizeof(sunaddr.sun_path)-1);
	len = sizeof(sunaddr);
	
	for (i=0;i<time || time == 0;i++)
	{
		int leftsize,writesize;
		char *pCurptr=(char*)str;

		leftsize = bufsize;
		writesize = 0;
		

		while(leftsize > 0)
		{
			writesize = bufsize > persize ? persize : bufsize;

			fprintf(stdout,"Write %d left %d\n",writesize,leftsize);
			ret = sendto(sock,pCurptr,writesize,0,(struct sockaddr*)&sunaddr,len);
			if (ret < 0)
			{
				ret = -errno;
				fprintf(stderr,"Send[%d] %d error %d %m\n",i,writesize,errno);
				goto out;
			}

			leftsize -= ret;
			pCurptr += ret;
		}
	}
  out:
	if(sock>=0)
	{
		close(sock);
	}
	sock = -1;
	return ret;
}

int ReadUnixSock(char* sockname,char* buflenstr)
{
	int sock=-1;
	unsigned char *buf=NULL;
	int buflen = atoi(buflenstr);
	struct sockaddr_un sunaddr;
	int ret;
	int bufsize,len;
	int i;

	memset(&sunaddr,0,sizeof(sunaddr));
	sock = socket(PF_UNIX,SOCK_DGRAM,0);
	if (sock < 0)
	{
		ret = -errno;
		fprintf(stderr,"can not create unixsocket %d %m\n",errno);
		goto out;
	}

	unlink(sockname);

	buf = (unsigned char*)malloc(buflen);
	if (buf == NULL)
	{
		ret = -errno;
		fprintf(stderr,"can not malloc %d\n",buflen);
		goto out;
	}

	sunaddr.sun_family = AF_UNIX;
	strncpy(sunaddr.sun_path,sockname,sizeof(sunaddr.sun_path)-1);

	ret = bind(sock,(struct sockaddr*)&sunaddr,sizeof(sunaddr));
	if (ret < 0)
	{
		ret = -errno;
		fprintf(stderr,"can not bind %s %d %m\n",sockname,errno);
		goto out;
	}

	len = sizeof(bufsize);
	ret  = getsockopt(sock,SOL_SOCKET,SO_RCVBUF,&bufsize,(socklen_t*)&len);
	if (ret < 0)
	{
		ret = -errno;
		fprintf(stderr,"can not get sock bufsize %d %m\n",errno);
		goto out;
	}
	fprintf(stdout,"sockbuf size %d\n",bufsize);
	bufsize = buflen;
	len = sizeof(bufsize);
	ret = setsockopt(sock,SOL_SOCKET,SO_RCVBUF,&bufsize,len);
	if (ret < 0)
	{
		ret = -errno;
		fprintf(stderr,"can not set sockbufsize %d %d %m\n",bufsize,errno);
		goto out;
	}
	
	len = sizeof(bufsize);
	ret  = getsockopt(sock,SOL_SOCKET,SO_RCVBUF,&bufsize,(socklen_t*)&len);
	if (ret < 0)
	{
		ret = -errno;
		fprintf(stderr,"can not get sock bufsize %d %m\n",errno);
		goto out;
	}
	fprintf(stdout,"after set sockbuf size %d\n",bufsize);
	

	i = 0;
	while(1)
	{
		i ++;
		len = sizeof(sunaddr);
		memset(buf,0,buflen);
		ret = recvfrom(sock,buf,buflen,0,(struct sockaddr*)&sunaddr,(socklen_t*)&len);
		if (ret < 0)
		{
			fprintf(stderr,"can not recv from %s [%d] %d %m\n",sockname,i,errno);
		}
		else
		{
			fprintf(stdout,"receive[%d:%d] from %s %s\n",i,ret,sunaddr.sun_path,buf);
		}
	}

  out:

	if (buf)
	{
		free(buf);
	}
	buf = NULL;
	
	if (sock>=0)
	{
		close(sock);
	}
	sock = -1;
	return ret;
}

int main(int argc,char* argv[])
{
	int ret;
	if (argc < 3)
	{
		fprintf(stderr,"%s sockname size [for read]\n",argv[0]);
		fprintf(stderr,"%s sockname pairname str time bufsize [for write]\n",argv[0]);
		fprintf(stderr,"%s sockname pairname time bufsize [for write ,and the str is from stind]\n",argv[0]);
		exit(3);
	}

	if (argc < 4)
	{
		ret = ReadUnixSock(argv[1],argv[2]);
	}
	else if (argc < 6)
	{
		char *pContent=NULL;
		int contentsize=1024*1024;
		char *pCurPtr=NULL;
		int readsize;

		pContent= (char*)malloc(contentsize);
		if (pContent == NULL)
		{
			return -ENOMEM;
		}

		readsize = 0;
		pCurPtr = pContent;
		while(readsize < contentsize)
		{
			ret = fread(pCurPtr,1,(contentsize - readsize),stdin);
			if (ret <= 0)
			{
				ret = -errno;
				if(feof(stdin))
				{
					break;
				}

				fprintf(stderr,"can not read stdin %d %d %m\n",readsize,errno);
				free(pContent);
				return ret;
			}

			pCurPtr += ret;
			readsize += ret;
		}

		ret = WriteUnixSock(argv[1],argv[2],pContent,argv[3],argv[4]);

		free(pContent);
		
	}
	else
	{
		ret = WriteUnixSock(argv[1],argv[2],argv[3],argv[4],argv[5]);
	}

	return ret;
}


