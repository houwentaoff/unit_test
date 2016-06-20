/*
	filename : startsnmp.c
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

#include "mw_struct.h"
#include "mw_api.h"
#if 0
typedef enum {
	MW_ERROR_LEVEL = 2,
	MW_MSG_LEVEL,
	MW_INFO_LEVEL,
	MW_DEBUG_LEVEL,
	MW_LOG_LEVEL_NUM,
} mw_log_level_t;
#endif
static int G_mw_lof_level_ex = 1;
#define MPREFIX_NONE   "\033[0m"
#define MPREFIX_RED    "\033[0;31m"
#define MPREFIX_GREEN  "\033[0;32m"
#define MPREFIX_YELLOW "\033[1;33m"

#define MW_PRINT(mylog, LOG_LEVEL, format, args...)		do {if (mylog >= LOG_LEVEL) {printf(format, ##args);fflush(stdout);}} while (0)

#define MW_ERROR(format, args...)		MW_PRINT(G_mw_lof_level_ex, MW_ERROR_LEVEL, MPREFIX_RED"Error!  [%-20s][%05d][%-20s] " format MPREFIX_NONE"\n", __FILE__, __LINE__, __FUNCTION__, ##args)
#define MW_MSG(format, args...)			MW_PRINT(G_mw_lof_level_ex, MW_MSG_LEVEL, MPREFIX_YELLOW"Warning![%-20s][%05d][%-20s] " format MPREFIX_NONE"\n", __FILE__, __LINE__, __FUNCTION__, ##args)
#define MW_INFO(format, args...)			MW_PRINT(G_mw_lof_level_ex, MW_INFO_LEVEL, MPREFIX_YELLOW"Info!   [%-20s][%05d][%-20s] " format MPREFIX_NONE"\n", __FILE__, __LINE__, __FUNCTION__, ##args)
#define MW_DEBUG(format, args...)		MW_PRINT(G_mw_lof_level_ex, MW_DEBUG_LEVEL, MPREFIX_GREEN"Debug!  [%-20s][%05d][%-20s]" format MPREFIX_NONE"\n", __FILE__, __LINE__, __FUNCTION__, ##args)

#define PROTO_CONFIG_FILE       ("/etc/ambaipcam/mediaserver/proto.cfg")
#define DEFAULT_TCP_PORT        (30002)

/*
	return	value
	-1:		error
	other:	OK
 */
static int get_port_from_cfg(const char* cfg, const char *port_name)
{
	FILE *fp = NULL;
	int port = 0;
	char line[256] = {0};
	char line_f[256] = {0};
	int find_flag = 0;

	if (!cfg || !port_name) {
		MW_ERROR("invalid NULL param");
		goto errExit;
	}

	fp = fopen(cfg, "rb");
	if (NULL == fp) {
		MW_ERROR("open %s failed\n", cfg);
		goto errExit;
	}

	sprintf(line_f, "%s = %%d", port_name);
	while (NULL != fgets(line, sizeof(line) - 1, fp)) {
		if (1 == sscanf(line, line_f, &port)) {
			find_flag = 1;
			break;
		}
		memset(line, 0, sizeof(line));
	}//endof while (NULL != fgets(line, sizeof(line) - 1, fp))
	MW_MSG("get %s:%s is %d", cfg, port_name, port);

errExit:
	if (NULL != fp) {
		fclose(fp);
		fp = NULL;
	}

	if (find_flag)
		return port;	
	else 
		return -1;
}

/*
	when try_tims is 0, try to connect until success
	return: 0:ready	-1:not ready or error
*/
static int check_tcp_port_ready(const char *ipstr, unsigned int port, unsigned int try_times)
{
	struct sockaddr_in addr;
	int sock_fd = 0;

	if (!ipstr) {
		MW_ERROR("invalid argument");
		return -1;
	}

	sock_fd = socket(AF_INET,  SOCK_STREAM, 0);
	if (sock_fd < 0) {
		MW_ERROR("get socket failed");
		return -1;	
	}

	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	if (inet_pton(AF_INET, (char *)ipstr, &(addr.sin_addr.s_addr)) != 1) {
		close(sock_fd);
		MW_ERROR("inet_pton failed");
		return -1;	
	}

	while (1) {
		int ret = 0;
		ret = connect(sock_fd, (struct sockaddr *)&addr, sizeof(addr));
		if (ret == 0)
			break;
		sleep(1);
	}
	MW_MSG("connect to %s:%d success", ipstr, port);

	close(sock_fd);
	return 0;
}

/* return value: 0:OK, else:wrong */
static int check_server_port_ready(const char *ipstr, const char *port_name)
{
	int port = 0;
	int ret = 0;
	
	if (!ipstr || !port_name) {
		MW_ERROR("inbalid NULL param");
		return -1;
	}

	port = get_port_from_cfg(PROTO_CONFIG_FILE, port_name);
	if (port < 0) {
		MW_ERROR("get %s %s(port) failed", PROTO_CONFIG_FILE, port_name);
		return -1;
	}

	ret = check_tcp_port_ready(ipstr, (unsigned int)port, 0);
	if (ret != 0) {
		MW_ERROR("%s %d is not ready or some thing is wrong", port_name, port);
		return -1;
	}

	return 0;
}

#define LOCAL_FOST_IP		"127.0.0.1"
#define CMD_RESTART_SNMP	"/etc/init.d/s50snmp restart"
#define GET_IP_ADDR_CMD     "ifconfig eth0 | grep 'inet addr' | tr -s ':' ' ' | awk '{ print $3 }' | tr -d '\n'"
#define GET_SNMP_PID_CMD	"pgrep snmpd | tr -d '\n'"
#define	GET_APPWEB_PID_CMD	"pgrep appweb | tr -d '\n'"

static int popen_read(const char *cmd, char *buf, size_t length)
{
	memset(buf, 0, length);

	FILE *fp = popen(cmd, "r");
	if (NULL == fp)
	{
		printf("popen [%s] failed!", cmd);
		return -1;
	}

	int retval = fread(buf, 1, length, fp);
	pclose(fp);
	if (retval < 0)
	{
		printf("read pipe failed!");
		return -1;
	}
	buf[retval] = '\0';

	return 0;
}

static int find_snmp(void)
{
	char pid[20] = {0};
	popen_read(GET_SNMP_PID_CMD, &pid[0], sizeof(pid));

	if (atoi(pid) > 10) { 
		MW_MSG("start snmp success, pid %s", pid);
		//mw_log_insert_alarm("HeartBeat", "start snmp OK", pid);
		return 1;
	}
	else {
		MW_ERROR("start snmp failed");
		//mw_log_insert_alarm("HeartBeat", "start snmp failed", "cant't find snmp's pid");
		return 0;
	}
}

static int wait_for_pid(const char *pname, int wait_sec)
{
	if (!pname) {
		MW_ERROR("invalid NULL param");
		return -1;
	}
	char pid[20] = {0};
	int wait_cnt = wait_sec;
	
	while (!wait_sec || (wait_cnt>0)) {
		popen_read(GET_APPWEB_PID_CMD, &pid[0], sizeof(pid));
		if (atoi(pid) > 0) {
			MW_MSG("appweb's pid is %s", pid);
			return 0;
		}

		if (wait_cnt > 0)
			wait_cnt--;
		sleep(1);
	}
	
	return 0;
}


#define CAMERA_SERVER_PORT		"camera_server_port"
#define TCP_STREAM_PORT			"tcp_port"
#define RTSP_AUDIO_PORT			"rtsp_audio_port"
#define AUDIO_SERVER_PORT		"audio_server_port"
#define APPWEB_SERVER_NAME		"appweb"

int main(int argc, char *argv[])
{
    char ip_addr[32] = {0};
	
	wait_for_pid(APPWEB_SERVER_NAME, 0);

	popen_read(GET_IP_ADDR_CMD, ip_addr, sizeof(ip_addr));
	if (ip_addr[0] == '\0')
		strcpy(ip_addr, LOCAL_FOST_IP);

	//mw_log_insert_alarm("HeartBeat", "start snmp", (char *)"begin");
	if (check_server_port_ready(ip_addr, CAMERA_SERVER_PORT) == 0 &&		//wait for login port
			check_server_port_ready(ip_addr, TCP_STREAM_PORT) == 0 ) {		//wait for tcp stream port
	//		check_server_port_ready(ip_addr, AUDIO_SERVER_PORT) == 0 &&		//wait for audio_server's port
	//		check_server_port_ready(ip_addr, RTSP_AUDIO_PORT) ==0) {		//wait for testG711's port	
		system(CMD_RESTART_SNMP);
		find_snmp();
	}

	return 0;
}

