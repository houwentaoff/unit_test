
#include <LocalProfile.h>
#include <memory>
#include <stdlib.h>

#define	PROFILE_ERROR(...) do{fprintf(stderr,"%s:%d:\t",__FILE__,__LINE__);fprintf(stderr,__VA_ARGS__);}while(0)


int GetProfile(char *file)
{
	std::auto_ptr<CLocalProfileEx> pLocalFile2(new CLocalProfileEx(file));
	CLocalProfileEx *pLocalFile=pLocalFile2.get();
	int ret=-1;
	bool bret;

	bret = pLocalFile->Open();
	if (!bret)
	{
		PROFILE_ERROR("can not open %s\n",file);
		return -1;
	}

	bret = pLocalFile->ReloadFile();
	if (!bret)
	{
		PROFILE_ERROR("can not load %s\n",file);
		goto out;	
	}
	fprintf(stdout,"Open %s succ\n",file);
out:
	pLocalFile->Close();

	return ret;
	
}

int main(int argc,char*argv[])
{
	int ret;

	if (argc<2)
	{
		fprintf(stderr,"%s profile\n",argv[0]);
		exit(3);
	}

	ret = GetProfile(argv[1]);
	return ret;
}

