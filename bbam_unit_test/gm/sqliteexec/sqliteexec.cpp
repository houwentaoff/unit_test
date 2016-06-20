
/*to include the sqlite running*/
#include <inc/sqlite3.h>
#include <getopt.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <assert.h>

#define ERROR_PRINT(...) do{{fprintf(stderr,"[%-30s][%5d][ERROR]:\t",__FILE__,__LINE__);fprintf(stderr,__VA_ARGS__);}}while(0)
#define DEBUG_PRINT(...) do{{fprintf(stdout,"[%-30s][%5d][INFO]:\t",__FILE__,__LINE__);fprintf(stdout,__VA_ARGS__);}}while(0)

#define APP_ASSERT assert


static sqlite3* st_pDB = NULL;
static const char* st_DBName = NULL;
static const char* st_InsertSQL = NULL;
static const char* st_SelectSQL = NULL;
static const char* st_StepSQL = NULL;
static const char* st_GetTableSQL = NULL;
static int st_Times = 1;

static struct option st_LongOptions[] =
{
    {"help",   0, NULL, 'h'},
    {"db",     1, NULL, 'd'},
    {"insert", 1, NULL, 'i'},
    {"select", 1, NULL, 's'},
    {"times",  1, NULL, 't'},
    {"step",   1, NULL, 'S'},
    {"gte",    1, NULL, 'g'},
    {NULL,     0, NULL, 0},
};

static const char* st_ShortOptions = "hd:s:i:t:S:g:";

void Usage ( int exitcode )
{
    FILE* fp = stderr;
    if ( exitcode == 0 )
    {
        fp = stdout;
    }

    fprintf ( fp, "sqlitexec [OPTIONS]\n" );
    fprintf ( fp, "-h|--help                : to display this help\n" );
    fprintf ( fp, "-d|--db dbname           : db name to connect\n" );
    fprintf ( fp, "-s|--select statement    : to exec select statement\n" );
    fprintf ( fp, "-i|--insert statement    : to exec insert statement\n" );
    fprintf ( fp, "-t|--times  time         : to specify the times default is 1\n" );
    fprintf ( fp, "-S|--step sqlstate       : to step sql state\n" );
    fprintf ( fp, "-g|--get sqlstate        : to use get_table in sqlite\n" );
    exit ( exitcode );
}

int SelectCallBack ( void* data, int ncols, char** values, char** headers )
{
    int i;
    for ( i = 0; i < ncols; i++ )
    {
        DEBUG_PRINT ( "[%d] %s = %s\n", i, headers[i], values[i] );
    }

    return 0;
}

int ParseOptions ( int argc, char* argv[] )
{
    int ret;
    int lidx = 0;

    while ( 1 )
    {
        ret = getopt_long ( argc, argv, st_ShortOptions, st_LongOptions, &lidx );
        if ( ret == -1 )
        {
            break;
        }
        switch ( ret )
        {
        case 'h':
            Usage ( 0 );
            break;
        case 'd':
            st_DBName = optarg;
            break;
        case 's':
            st_SelectSQL = optarg;
            break;
        case 'i':
            st_InsertSQL = optarg;
            break;
        case 't':
            st_Times = atoi ( optarg );
            break;
        case 'S':
            st_StepSQL = optarg;
            break;
        case 'g':
            st_GetTableSQL = optarg;
            break;

        default:
            Usage ( 3 );
            break;
        }
    }

    if ( st_DBName == NULL || ( st_InsertSQL == NULL && st_SelectSQL == NULL && st_StepSQL == NULL && st_GetTableSQL == NULL ) )
    {
        Usage ( 3 );
    }

    return 0;
}

int main ( int argc, char*argv[] )
{
    int ret, res, i,j;
    char* errmsg = NULL;

    ret = ParseOptions ( argc, argv );
    if ( ret < 0 )
    {
        return ret;
    }

    ret = sqlite3_open_v2 ( st_DBName, &st_pDB, SQLITE_OPEN_READWRITE, NULL );
    if ( ret != SQLITE_OK )
    {
        ERROR_PRINT ( "can not open sqlite db %s %d\n", st_DBName, ret );
        return -3;
    }

    DEBUG_PRINT ( "open %s succ\n", st_DBName );

    if ( st_InsertSQL )
    {
        for ( i = 0; i < st_Times || st_Times == 0; i++ )
        {
            ret = sqlite3_exec ( st_pDB, st_InsertSQL, NULL, NULL, &errmsg );
            if ( ret != SQLITE_OK )
            {
                ERROR_PRINT ( "exec \"%s\" sql error %d (%s)\n",
                              st_InsertSQL, ret, errmsg );
                ret = -ret;
                goto out;
            }
            DEBUG_PRINT ( "Insert [%d] succ\n", i );
        }

    }
    else if ( st_StepSQL )
    {
        /*for step*/
        sqlite3_stmt* pStmt = NULL;
        const char* tail = NULL;
        int ncols;
        const char* pSql = NULL;

        pSql = st_StepSQL;
        while ( sqlite3_complete ( pSql ) )
        {
            ret = sqlite3_prepare_v2 ( st_pDB, pSql, -1, &pStmt, &tail );
            if ( ret != SQLITE_OK )
            {
                ERROR_PRINT ( "exec \"%s\" sql step error %d (%s)\n", pSql, ret, sqlite3_errmsg ( st_pDB ) );
                ret = -ret;
                goto out;
            }

            ret = sqlite3_step ( pStmt );
            ncols = sqlite3_column_count ( pStmt );
            j = 0;
            while ( ret == SQLITE_ROW )
            {
                for ( i = 0; i < ncols; i++ )
                {
                    DEBUG_PRINT ( "[%d][%d] %s\n", j, i, sqlite3_column_text ( pStmt, i ));
                }

                ret = sqlite3_step ( pStmt );
                j ++;
            }

			if (ret != SQLITE_DONE)
			{
				ERROR_PRINT("Not Done \"%s\" ret %d %s\n",sqlite3_sql(pStmt),ret,sqlite3_errmsg(st_pDB));
				ret = -ret;
				goto out;
			}

            DEBUG_PRINT ( "step return %d\n", ret );
            sqlite3_finalize ( pStmt );

			/*set the tail one*/
			pSql = tail;
        }
    }
    else if ( st_GetTableSQL )
    {
		char** pResulte=NULL;
		int ncols=0,nrows=0;
		char *errmsg=NULL;
		ret = sqlite3_get_table(st_pDB,st_GetTableSQL,&pResulte,&nrows,&ncols,&errmsg);
		if (ret != SQLITE_OK)
		{
			ERROR_PRINT("can not get table \"%s\" %d (%s)\n",st_GetTableSQL,ret,errmsg);
			ret = -ret;
			goto out;
		}

		for(i=0;i<nrows;i++)
		{
			for (j=0;j<ncols;j++)
			{
				DEBUG_PRINT("[%d][%d] = %s\n",i,j,pResulte[(i+1)*ncols+j]);
			}
		}

		sqlite3_free_table(pResulte);
    }
    else
    {
        APP_ASSERT ( st_SelectSQL );
        ret = sqlite3_exec ( st_pDB, st_SelectSQL, SelectCallBack, NULL, &errmsg );
        if ( ret != SQLITE_OK )
        {
            ERROR_PRINT ( "exec \"%s\" sql error %d (%s)\n",
                          st_InsertSQL, ret, errmsg );
            ret = -ret;
            goto out;
        }
    }

    ret = 0;
out:
    if ( st_pDB )
    {
        res = sqlite3_close ( st_pDB );
        DEBUG_PRINT ( "close %s %d\n", st_DBName, res );
    }
    st_pDB = NULL;
    return ret;
}

