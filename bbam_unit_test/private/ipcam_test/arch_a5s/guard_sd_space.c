#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>
#include <time.h>
#include <ctype.h>
#include <syslog.h>
#include <pthread.h>
#include <sys/resource.h>
#include <errno.h>

#include <sys/stat.h>
#include <sys/vfs.h>

//#define NEED_DAEMONIZE
#define SD_DEFAULT_RESERVED_SPACE	(128)	// keep SD free space 128MB
#define LOCKFILE "/var/run/guard.pid"
#define LOCKMODE (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)
#define GUARDCONFIG "/etc/guard.conf"
#define MAX_STRING_LENGTH (512)

struct dirent **g_namelist = 0;
int g_namelist_num = 0;
static int need_reread = 1;
static pthread_mutex_t guard_locker;
static sigset_t g_mask;
static int running_flag = 1;

#ifdef NEED_DAEMONIZE
static void guard_daemonize(void)
{
	int i, fd0, fd1, fd2;
	pid_t pid;
	struct rlimit rl;
	struct sigaction sa;

	umask(0);
	if (getrlimit(RLIMIT_NOFILE, &rl) < 0){
		//err_quit(" can't get file limit");
	}

	if ((pid = fork()) < 0){
		//err_quit("can't fork");
	} else if (pid !=0){
		exit(0);
	}

	setsid();

	sa.sa_handler = SIG_IGN;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;

	if (sigaction(SIGHUP, &sa, NULL) < 0){
		//err_quit("can't ignore SIGHUP");
	}

	if ((pid = fork()) < 0){
		//err_quit("cann't fork");
	} else if (pid != 0){
		exit(0);
	}

	if (chdir("/") < 0){
		//err_quit("chdir is error");
	}

	if (rl.rlim_max == RLIM_INFINITY){
		rl.rlim_max = 1024;
	}

	for (i = 0; i < rl.rlim_max; i++){
		close(i);
	}

	fd0 = open("/dev/null", O_RDWR);
	fd1 = dup(0);
	fd2 = dup(0);

	if (fd0 != 0 || fd1 != 1 || fd2 != 2) {
		syslog(LOG_ERR, "unexpected file descriptors %d, %d, %d", fd0, fd1, fd2);
		exit(1);
	}

}

static int guard_lockfile(int fd)
{
	struct flock fl;

	fl.l_type = F_WRLCK;
	fl.l_start = 0;
	fl.l_whence = SEEK_SET;
	fl.l_len = 0;
	return (fcntl(fd, F_SETLK, &fl));
}

static int guard_already_running(void)
{
	int fd;
	char buf[16];

	fd = open(LOCKFILE, O_RDWR | O_CREAT, LOCKMODE);
	if (fd < 0){
		syslog(LOG_ERR, "can't open %s : %s", LOCKFILE, strerror(errno));
		exit(1);
	}

	if (guard_lockfile(fd) < 0){
		if (errno == EACCES ||errno == EAGAIN){
			close(fd);
			return(1);
		}
		syslog(LOG_ERR, "can't lock %s : %s", LOCKFILE, strerror(errno));
		exit(1);
	}

	ftruncate(fd, 0);
	sprintf(buf, "%ld", (long)getpid());
	write(fd, buf, strlen(buf) + 1);
	return 0;
}

#endif
static int guard_read_config(char *outbuf)
{
	FILE *fp = NULL;
	struct stat statbuf;

	if(access(GUARDCONFIG, R_OK) == 0){
		fp = fopen(GUARDCONFIG, "rb");
		if (!fp){
			printf("fopen %s file fail\n", GUARDCONFIG);
			return -1;
		}
		while (fgets(outbuf, MAX_STRING_LENGTH, fp) != NULL){
			if (stat(outbuf, &statbuf) == 0){
				fclose(fp);
				fp = NULL;
				return 0;
			}

		}

		printf("The path %s is not exist!", outbuf);
		memset(outbuf, 0, MAX_STRING_LENGTH);
		fclose(fp);
		fp = NULL;
		return -1;
	} else {
		fp = fopen(GUARDCONFIG, "wb");
		if (!fp){
			printf("fopen %s file fail\n", GUARDCONFIG);
			return -1;
		}

		// try to set default dir for guard config
		if(stat("/tmp/mmcblk0p1/", &statbuf) == 0){
			strcpy(outbuf, "/tmp/mmcblk0p1/");
			fputs(outbuf, fp);
			fclose(fp);
			fp = NULL;
		}
	}

	return 0;
}
void * sig_process(void *arg)
{
	int signo;
	while (1){
		if (sigwait(&g_mask, &signo) != 0){
			printf("sigwait failed\n");
			exit(1);
		}

		printf("got SIG  %d\n", signo);
		switch (signo){
			case SIGHUP:
				printf("reread guard config file\n");
				pthread_mutex_lock(&guard_locker);
				need_reread = 1;
				pthread_mutex_unlock(&guard_locker);
				break;
			case SIGTERM:
				printf("got SIGTERM, exiting\n");
				running_flag = 0;
				exit(0);
			default :
				printf("unexpected signal %d\n", signo);
		}
	}

	return 0;
}

static int scandir_filter(const struct dirent *p_dir)
{
	struct stat st;

	stat(p_dir->d_name, &st);

	// skip all not regular file
	if (!S_ISREG(st.st_mode))
		return 0;

	return 1;
}

static int scandir_cmp_time(const struct dirent **p1, const struct dirent **p2)
{
	struct stat st1, st2;
	stat((*p1)->d_name, &st1);
	stat((*p2)->d_name, &st2);
	return (st2.st_mtime - st1.st_mtime);
}

static int count_files(const char *dir_path)
{
	char current_dir[256];
	int i = 0;

	if(!dir_path || !getcwd(current_dir, 256))
		return -1;

	if(chdir(dir_path)){
		printf("%s is not exist\n", dir_path);
		return -1;
	}

	if(g_namelist_num){
		while(g_namelist_num--){
			free(g_namelist[g_namelist_num]);
			g_namelist[g_namelist_num] = NULL;
		}
		free(g_namelist);
		g_namelist = NULL;
	}

	if ((g_namelist_num = scandir(".", &g_namelist, scandir_filter, scandir_cmp_time)) > 0){
		for (i = 0; i < g_namelist_num; i++){
			printf("%s\n", g_namelist[i]->d_name);
		}
		printf("\n\n");
	}

	// resore the old dir
	chdir(current_dir);
	return g_namelist_num;
}

static int free_space(const char *dir_path)
{
	struct statfs s;
	int free_bsz;

	if (!dir_path)
		return -1;

	statfs(dir_path, &s);
	free_bsz = (s.f_bsize>>8) * s.f_bavail;
	return (free_bsz>>12);	// MiB
}

int main(int argc, char *argv[])
{
	struct sigaction sa;
	pthread_t tid;
	int err = 0;
	char scanned_dir[MAX_STRING_LENGTH];
	char old_scanned_dir[MAX_STRING_LENGTH];
	char	remove_file_path[MAX_STRING_LENGTH];
	pthread_mutex_init(&guard_locker, NULL);

#ifdef NEED_DAEMONIZE
	guard_daemonize();

	if (guard_already_running() < 0){
		syslog(LOG_ERR, "guard daemon already running!");
		exit(1);
	}
#endif

	//restore SIGHUP default and block all signals
	sa.sa_handler = SIG_DFL;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;

	if (sigaction(SIGHUP, &sa, NULL) < 0){
		printf("call sigaction failed!\n"); // error
	}
	sigfillset(&g_mask);
	if (pthread_sigmask(SIG_BLOCK, &g_mask, NULL) != 0){
		printf("call pthread_sigmask failed!\n"); //error
	}

	// create a thread to handle SIGHUP and SIGTERM
	if (pthread_create(&tid, NULL, sig_process, 0) != 0){
		printf("call pthread_create failed!\n"); //error
	}

	memset(scanned_dir, 0, sizeof(scanned_dir) / sizeof(scanned_dir[0]));
	memset(old_scanned_dir, 0, sizeof(old_scanned_dir) / sizeof(old_scanned_dir[0]));

	while (running_flag){
		pthread_mutex_lock(&guard_locker);
		if (need_reread){
			pthread_mutex_unlock(&guard_locker);
			if (guard_read_config(scanned_dir) < 0){
				// error,  need correct guard config
				printf("read guard config error, please reconfig the guard.config file!\n");
				usleep(3000000);
				continue;
			}

			printf("scanned_dir: %s\n", scanned_dir);
			if (strlen(old_scanned_dir) == 0 || strcmp(scanned_dir, old_scanned_dir) != 0){
				if (count_files(scanned_dir) < 0){
					printf("count file failed !\n");
					continue;
				}
				memcpy(old_scanned_dir, scanned_dir, sizeof(old_scanned_dir) / sizeof(old_scanned_dir[0]));
			}

			pthread_mutex_lock(&guard_locker);
			need_reread = 0;
			pthread_mutex_unlock(&guard_locker);

		} else{
			pthread_mutex_unlock(&guard_locker);
		}

		while (free_space(scanned_dir) < SD_DEFAULT_RESERVED_SPACE){
			if (g_namelist_num){
				--g_namelist_num;
				sprintf(remove_file_path,"%s%s", scanned_dir, g_namelist[g_namelist_num]->d_name);
				printf("remove the file : %s.\n", remove_file_path);
				remove(remove_file_path);
				free(g_namelist[g_namelist_num]);
				g_namelist[g_namelist_num] = NULL;
			} else{
				free(g_namelist);
				g_namelist = NULL;
				err = count_files(scanned_dir);
				if (err < 0){
					printf("count file failed !\n");
				} else if (err == 0){
					printf("no regular file to delete");
					// exit
					kill(tid, SIGTERM);
				}
			}
		}

		// sleep 3s
		usleep(3000000);
	}

	pthread_join(tid, NULL);
	pthread_mutex_destroy(&guard_locker);
	return 0;
}

