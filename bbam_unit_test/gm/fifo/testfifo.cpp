#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <sys/time.h>
#include <signal.h>

int IsFifo ( char* file )
{
    struct stat sbuf;
    int ret;

    ret = stat ( file, &sbuf );
    if ( ret < 0 )
    {
        return 0;
    }

    if ( S_ISFIFO ( sbuf.st_mode ) )
    {
        return 1;
    }

    return 0;
}


int OpenFifo ( char* file, int flag )
{
    int fd = -1;
    int ret;

    ret = IsFifo ( file );
    if ( ret == 0 )
    {
        unlink ( file );
        mkfifo ( file, 0660 );
    }
    errno = 0;
    fd = open ( file, flag );
    if ( fd == -1 )
    {
        fd = -errno;
    }
    return fd;
}

int WriteFifo ( char* file, const char* content, char* timestr )
{
    int time;
    int i;
    int fd = -1;
    int ret;
    int len;

    time = atoi ( timestr );

    /*to open file */
    fd = OpenFifo ( file, O_WRONLY );
    if ( fd < 0 )
    {
        ret = fd;
        fprintf ( stderr, "can not open %s %d\n", file, ret );
        goto out;
    }

    len = strlen ( content ) + 1;

    for ( i = 0; ( i < time || time == 0 ); i++ )
    {
		errno = 0;
        ret = write ( fd, content, len );
        if ( ret < 0 )
        {
			if (errno == EPIPE)
			{
				close(fd);
				fd = -1;
				fd = OpenFifo ( file, O_WRONLY );
				if ( fd < 0 )
				{
					ret = fd;
					fprintf ( stderr, "can not open %s %d\n", file, ret );
					goto out;
				}
				continue;				
			}

            fprintf ( stderr, "write[%d] error %d\n", i, errno );
            goto out;
        }
        fprintf ( stdout, "Write[%d] %s succ\n", i, content );
    }

    ret = 0;
out:
    if ( fd >= 0 )
    {
        close ( fd );
    }
    fd = -1;
    return ret;
}

int ReadFifo ( char* file, char* timeoutstr )
{
    int ret;
    int fd = -1;
    char buf[512];
    int i;
    int timeout;

    timeout = atoi ( timeoutstr );
    i = 0;
    while ( 1 )
    {
        fd_set rset;
        struct timeval tmout;

		if ( fd < 0 )
		{
			fd = OpenFifo ( file, O_RDONLY );
			if ( fd < 0 )
			{
				ret =  fd;
				fprintf ( stderr, "Open %s error %d\n", file, ret );
				goto out;
			}
		}

        tmout.tv_sec = timeout;
        tmout.tv_usec = 0;
        memset ( buf, 0, sizeof ( buf ) );
        FD_ZERO ( &rset );
        FD_SET ( fd, &rset );

        errno = 0;
        ret = select ( fd + 1, &rset, NULL, NULL, timeout ? &tmout : NULL );
        if ( ret <= 0 )
        {
            fprintf ( stderr, "select[%d] ret %d error %d\n", fd, ret, errno );
            continue;
        }
        if ( !FD_ISSET ( fd, &rset ) )
        {
            continue;
        }
        ret = read ( fd, buf, sizeof ( buf ) );
        if ( ret < 0 )
        {
            ret = -errno;
            fprintf ( stderr, "Read[%d] error %d\n", i, errno );
            goto out;
        }

        fprintf ( stdout, "Read[%d][%d] %s\n", i, ret, buf );
        i ++;
		close(fd);
		fd = -1;
    }
out:
    if ( fd >= 0 )
    {
        close ( fd );
    }
    fd = -1;
    return ret;
}



int main ( int argc, char*argv[] )
{
    int ret;
    if ( argc < 3 )
    {
        fprintf ( stderr, "%s fifo [content] [time] for write\n", argv[0] );
        fprintf ( stderr, "%s fifo [timeout] for read\n", argv[0] );
        exit ( 3 );
    }

    signal ( SIGPIPE, SIG_IGN );

    if ( argc < 4 )
    {
        ret = ReadFifo ( argv[1], argv[2] );
    }
    else
    {
        ret = WriteFifo ( argv[1], argv[2], argv[3] );
    }

    return ret;
}
