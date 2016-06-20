/*
 * test_pca9554.c
 *
 */

#include <string.h>
#include <linux/i2c-dev.h>
#include <linux/i2c.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <time.h>
#include <assert.h>
#include <basetypes.h>

#include "ambas_common.h"
#include "ambas_vin.h"
#include "iav_drv.h"

#include "mw_struct.h"
#include "mw_api.h"

#define PCA9554_CFG_NAME "/etc/ambaipcam/mediaserver/pca9554.cfg"
#define PCA9554_DEFAULT_CFG_NAME "/etc/ambaipcam/mediaserver/default/pca9554.cfg"
#define PCA9554_CFG_MAX_SIZE      512


#define PCA9554_INPUT_R     0
#define PCA9554_OUTPUT_R    1
#define PCA9554_POLAR_R     2
#define PCA9554_DIR_R       3


static int i2c_fd = -1;
static unsigned int addr_array[3] = {0x70>>1, 0x40>>1, 0x70>>1 };
static unsigned int i2c_addr = 0x70>>1;
static char * i2c_path = "/dev/i2c-0";

static int is_pca9554 = -1;// -1: unknow 0: not existed 1: existed 

static int mw_pca9554_send_recv_test(unsigned char *data, unsigned int dlen, unsigned char *buf, unsigned int blen)
{
//	if (i2c_fd < 0) 
//		mw_pca9554_open(NULL, 0);
	if (i2c_fd < 0) 
		return -1;
			
	if (!data || !dlen || !buf || !blen) {
		printf("invalid arg: data %d, blen %d, buf %d, blen %d", (int)data, dlen, (int)buf, blen);
		return -1;
	}

	struct i2c_msg msgs[2];
	msgs[0].addr = i2c_addr;
	msgs[0].flags = 0;
	msgs[0].len = dlen;
	msgs[0].buf = data;

	msgs[1].addr = i2c_addr;
	msgs[1].flags = I2C_M_RD;
	msgs[1].len = blen;
	msgs[1].buf = buf;

	struct i2c_rdwr_ioctl_data ioctl_data;
	ioctl_data.msgs = msgs;
	ioctl_data.nmsgs = 2;
	
	if (2 != ioctl(i2c_fd, I2C_RDWR, &ioctl_data)) {
		printf("send&recv to %s,addr 0x%x failed", i2c_path, i2c_addr);
		return -1;
	} else 
		return 0;
}	


/* reg: 0-3*/
static int mw_pca9554_read_register_test(unsigned char  reg, unsigned char *buf)
{	
	int ret = 0;
	unsigned char regaddr = reg;
	
	if (reg < PCA9554_INPUT_R || reg > PCA9554_DIR_R || !buf) {
		printf("invalid arg: reg %d, buf 0x%x", reg, (int)buf);	
		return -1;
	}

	ret = mw_pca9554_send_recv_test(&regaddr, 1, buf, 1);
	if (ret != 0) {
		printf("read register failed");
		return -1;
	} else
		return 0;
}


static int mw_pca9554_get_register_bit_test(unsigned char reg, unsigned int pnum)
{
	int ret = 0;
	unsigned char data = 0; 

	if (pnum < 0  || pnum > 7) {
		printf("argument ivalid: pnum %d", pnum);
		return -1;
	}

	ret = mw_pca9554_read_register_test(reg, &data);
	if (ret != 0) {
		printf("pca9554 read register %d bit %d  faield", reg, pnum);
		return -1;
	}

	return (data&(0x1<<pnum)) ? 1: 0;
}

/* input level */
int mw_pca9554_get_input_level_test(unsigned int pnum)
{
	if (i2c_fd < 0) {
		return -1;
	}
	return mw_pca9554_get_register_bit_test(PCA9554_INPUT_R, pnum);
}

static int mw_pca9554_close_test(void)
{
	if (i2c_fd > 0) { 
		printf("close %s, int addr 0x%x", i2c_path, i2c_addr);
		close(i2c_fd);
		i2c_fd = -1;
	}

	return 0;
}

/* dev_path may be NULL, addr may be 0 */
static int mw_pca9554_open_test(char *dev_path,  unsigned int addr)
{
	int i = 0;

	if (i2c_fd > 0) {
		printf("%s in 0x%x :pca9554 is already opened", i2c_path, i2c_addr);
		return 0;
	}

	if (dev_path) 
		i2c_path = dev_path;
	if (addr) 
		i2c_addr = addr;

	i2c_fd = open(i2c_path, O_RDWR);
	if (i2c_fd < 0) {
		printf("open %s failed", i2c_path);
		return -1;
	}
	
	for (i = 0; i< 3; i++) {
		i2c_addr = addr_array[i];
		if (mw_pca9554_get_input_level_test(0) < 0) {
			continue;
		}
		break;
	}

	if (3 == i) {
		printf("test read pca9554 on  %s failed, Maybe pca9554 is not exist.", i2c_path);
		mw_pca9554_close_test();
		return -1;
	}
	
	printf("Test read pca9554 on  %s 0x%x OK", i2c_path, i2c_addr);

	return 0;
}

int main(int argc, char **argv)
{
	char cmdbuffer[128]={0};
	is_pca9554 = 0;

	if( !mw_pca9554_open_test( i2c_path, 0 ) )
	{
		is_pca9554 = 1;
	}
	mw_pca9554_close_test();
	sprintf( cmdbuffer, "echo pca9554_is_exist = %1d > %s", is_pca9554, PCA9554_CFG_NAME );
	system( cmdbuffer );
	sprintf( cmdbuffer, "echo pca9554_is_exist = %1d > %s", is_pca9554, PCA9554_DEFAULT_CFG_NAME );
	system( cmdbuffer );
	if( is_pca9554 )
	{
		sprintf( cmdbuffer, "echo pca9554_i2c_addr = 0x%x >> %s", i2c_addr, PCA9554_CFG_NAME );
		system( cmdbuffer );
		sprintf( cmdbuffer, "echo pca9554_i2c_addr = 0x%x >> %s", i2c_addr, PCA9554_DEFAULT_CFG_NAME );
		system( cmdbuffer );
	}
	return 0;
}


