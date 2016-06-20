
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <time.h>
#include <unistd.h>

#ifndef ERROR_PRINT
#define ERROR_PRINT(...) do{{fprintf(stderr,"[%-30s][%5d][ERROR]:\t",__FILE__,__LINE__);fprintf(stderr,__VA_ARGS__);}}while(0)
#endif

#ifndef DEBUG_PRINT
#define DEBUG_PRINT(...) do{{fprintf(stdout,"[%-30s][%5d][INFO]:\t",__FILE__,__LINE__);fprintf(stdout,__VA_ARGS__);}}while(0)
#endif


#ifndef APP_ERROR
#define APP_ERROR ERROR_PRINT
#endif

#define APP_INFO DEBUG_PRINT

class CSemaphoreMutex
{
public:
    CSemaphoreMutex();
    ~CSemaphoreMutex();

    bool Create(const char *filename, int id);
    void Destroy();

    void Lock() const;
    void Unlock() const;
    bool TryLock() const;
    bool IsLocked() const;

private:
    bool m_created;
    int m_id;
};


CSemaphoreMutex::CSemaphoreMutex()
    : m_created(false), m_id(-1)
{
}

CSemaphoreMutex::~CSemaphoreMutex()
{
}

bool CSemaphoreMutex::Create(const char *filename, int id)
{
	int ret;
    if ( m_created )
    {
        return true;
    }

    key_t key = ftok(filename, id);
    if (key == (key_t)-1)
    {
        APP_ERROR("ftok file [%s] failed", filename);
        return false;
    }

    m_id = semget(key, 1, 0666);
    if (m_id == -1)
    {
        m_id = semget(key, 1, IPC_CREAT | 0666);
        if (m_id == -1)
        {
            APP_ERROR("Create semaphore failed");
            return false;
        }
		ret = semctl(m_id, 0, SETVAL, 1);
		if (ret < 0)
		{
			/*we should remove the id when failed to set*/
			semctl(m_id, 0, IPC_RMID);
			m_id = -1;
			return false;
		}
    }    

    m_created = true;
    APP_INFO("CSemaphoreMutex is Created");
    return true;
}

void CSemaphoreMutex::Destroy()
{
    if ( m_created )
    {
		/*we need not to make the sem id destroy ,because other process will use it*/
    	// semctl(m_id, 0, IPC_RMID);
    }

    m_created = false;
}

void CSemaphoreMutex::Lock() const
{
    if ( m_created )
    {
        struct sembuf op;
        op.sem_num = 0;
        op.sem_op = -1;
        op.sem_flg = SEM_UNDO;

        semop(m_id, &op, 1);
    }
}

void CSemaphoreMutex::Unlock() const
{
    if ( m_created )
    {
        struct sembuf op;
        op.sem_num = 0;
        op.sem_op = 1;
        op.sem_flg = SEM_UNDO;

        semop(m_id, &op, 1);
    }
}

bool CSemaphoreMutex::TryLock() const
{
    if ( m_created )
    {
        struct sembuf op;
        op.sem_num = 0;
        op.sem_op = -1;
        op.sem_flg = SEM_UNDO | IPC_NOWAIT;

        return (semop(m_id, &op, 1) == 0);
    }

    return false;
}

bool CSemaphoreMutex::IsLocked() const
{
    if ( m_created )
    {
        return semctl(m_id, 0, GETVAL) <= 0;
    }

    return false;
}



static int st_RunLoop=1;
void Sighandler ( int signo )
{
    if ( signo == SIGTERM || signo == SIGINT )
    {
		st_RunLoop = 0;
    }
}

void CliSem ( CSemaphoreMutex& sem, int sec )
{
    bool bret;
	int i=0;
	time_t nowtime;
	struct tm tmval,*pResult;

    while ( st_RunLoop)
    {
        bret = sem.TryLock();
		nowtime = time(NULL);
		pResult = localtime_r(&nowtime,&tmval);
        DEBUG_PRINT ( "locked[%d]%02d:%02d:%02d %s\n",i,pResult->tm_hour,pResult->tm_min,pResult->tm_sec,bret ? "succ" : "failed" );
        sleep ( sec );
        if ( !bret )
        {
            continue;
        }
        sem.Unlock();
		i ++;
    }

    return ;
}

int main ( int argc, const char*argv[] )
{
    bool bret;
    CSemaphoreMutex sem;
    int sec = 3;
    int id;

	signal(SIGINT,Sighandler);
	signal(SIGTERM,Sighandler);
    if ( argc < 3 )
    {
        ERROR_PRINT ( "%s filename key [sec]\n", argv[0] );
        exit ( 3 );
    }

    if ( argc >= 4 )
    {
        sec = atoi ( argv[3] );
    }
    id = atoi ( argv[2] );

    bret = sem.Create ( argv[1], id );
    if ( !bret )
    {
        ERROR_PRINT ( "can not create %s:%d\n", argv[1], id );
		return -2;
			
    }
	DEBUG_PRINT("create %s:%d sem\n",argv[1],id);

    CliSem ( sem, sec );
	DEBUG_PRINT("\n");
	sem.Destroy();
    return 0;
}

