
#include <stdio.h>

int main(int argc,char* argv[])
{
	unsigned char buf[512];
	int ret,i;

	while(1)
	{
		ret = fread(buf,1,sizeof(buf),stdin);
		if (ret <= 0)
		{
			break;
		}

		for (i=0;i<ret;i++)
		{
			fprintf(stdout,"%%%02x",buf[i]);
		}
	}

	fflush(stdout);

	return 0;
}
