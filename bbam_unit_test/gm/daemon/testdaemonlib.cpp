#include <libdaemon/dfork.h>
#include <libdaemon/dsignal.h>
#include <libdaemon/dlog.h>
#include <libdaemon/dpid.h>
#include <libdaemon/dexec.h>


int main(int argc,char*argv[])
{

	daemon_log(LOG_ERR,"call argc %d\n",argc);
	return 0;
}
