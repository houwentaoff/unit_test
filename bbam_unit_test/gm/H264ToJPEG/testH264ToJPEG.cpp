
#include <basetypes.h>
#include <bsreader.h>
#include <gmi_bs_shm.h>
#include <memory>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>

//encode pic_type
#define IDR_FRAME       1
#define I_FRAME         2
#define P_FRAME         3
#define B_FRAME         4
#define JPEG_STREAM     5
#define JPEG_THUMBNAIL  6

#define	APP_ERROR(...) do{fprintf(stderr,"[%30s][%5d]:\t",__FILE__,__LINE__);fprintf(stderr,__VA_ARGS__);}while(0)


/*1 for get h264 2 for get jpeg format*/
int GetH264Buffer(void* handle,void **ppBuffer,u32 *pBufSize)
{
	void *pGetBuffer=*ppBuffer;
	u32 bufsize = *pBufSize;
	u32 charsize=100*1024;
	std::auto_ptr<char> pChar2(new char[charsize]);
	int ret;
	bs_info_t header;
	char  *packaddr,*lastpackaddr=NULL;
	unsigned int packsize,packread=0,lastpacksize=0;
	int hasread=0;
	int tries = 0;

	do
	{
		memset(&header,0,sizeof(header));
		ret = gmi_bs_shm_read_one_packet(handle,(char*)&header,&packaddr,&packsize,&packread);
		APP_ERROR("ret = %d\n",ret);
		if (ret >= 0)
		{
			APP_ERROR("type = %d\n",header.pic_type);
			if (header.pic_type == JPEG_STREAM)
			{
				hasread = 2;
				lastpackaddr = packaddr;
				lastpacksize = packsize;
				break;
			}
			else if (header.pic_type == I_FRAME || header.pic_type == IDR_FRAME)
			{
				if (hasread < 2)
				{
					hasread = 1;
					lastpackaddr = packaddr;
					lastpacksize = packsize;
					break;
				}
			}

		}
		else
		{
			usleep(100000);
		}
		tries ++;
	}while(tries <= 100);

	if (hasread == 0)
	{
		/*nothing to read */
		ret = -ENODEV;
		APP_ERROR("\n");
		goto fail;
	}

	if (bufsize < lastpacksize || pGetBuffer == NULL)
	{
		if (bufsize < lastpacksize)
		{
			bufsize = lastpacksize;
		}
		pGetBuffer = malloc(bufsize);
	}

	if (pGetBuffer == NULL)
	{
		ret = -ENOMEM;
		APP_ERROR("\n");
		goto fail;
	}

	memcpy(pGetBuffer,lastpackaddr,lastpacksize);
	*ppBuffer = pGetBuffer;
	*pBufSize = lastpacksize;

	return hasread;

fail:
	if (pGetBuffer && pGetBuffer != *ppBuffer)
	{
		free(pGetBuffer);
	}
	pGetBuffer = NULL;
	return ret;
}


int WriteToFile(char*file,void *pBuffer,u32 size)
{
	char *pChar= (char*)pBuffer;
	u32 leftsize = size;
	int fd=-1;
	int ret;

	fd = open(file,O_WRONLY|O_CREAT);
	if (fd < 0)
	{
		ret = -errno;
		goto out;
	}

	leftsize = size;
	while(leftsize)
	{
		ret = write(fd,pChar,leftsize);
		if (ret < 0)
		{
			if (errno != EINTR)
			{
				ret = -errno;
				goto out;
			}
			ret = 0;
		}

		pChar += ret;
		leftsize -= ret;
	}

	/*all is ok*/
	ret = 0;

out:
	if (fd >=0)
	{
		close(fd);
	}
	fd = -1;
	return ret;
}

int main(int argc,char*argv[])
{
	int ret;
	void* handle=NULL;
	int streamid=0;
	char *file=NULL;
	void *pBuffer=NULL;
	u32 bufsize=0;

	if (argc < 3)
	{
		fprintf(stderr,"%s streamid dumpfilename\n",argv[0]);
		exit(3);
	}

	streamid = atoi(argv[1]);
	file = argv[2];

	handle = gmi_bs_shm_open(streamid);
	if (handle == NULL)
	{
		APP_ERROR("\n");
		ret = -3;
		goto out;
	}

	ret = GetH264Buffer(handle,&pBuffer,&bufsize);
	if (ret < 0)
	{
		APP_ERROR("\n");
		goto out;
	}

	if (ret == 1)
	{
		fprintf(stderr,"get iframe\n");
	}
	else if (ret == 2)
	{
		fprintf(stderr,"get jpeg\n");
	}

	ret = WriteToFile(file,pBuffer,bufsize);
	if (ret < 0)
	{
		APP_ERROR("\n");
		goto out;
	}

	ret = 0;
	
out:
	if (handle)
	{
		gmi_bs_shm_close(handle);
	}
	handle = NULL;

	if (pBuffer)
	{
		free(pBuffer);
	}
	pBuffer = NULL;
	bufsize = 0;
	return ret;	
}


