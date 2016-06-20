#include <stdio.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>

#define WM8974_I2C_SLAVE_ADDR     0X34>>1

int main(int argc, char *argv[])
{
    int fd = 0;
    unsigned char addr[2] = {0x1, 0x0};
    int ret = 0;


    fd = open("/dev/i2c-0", O_RDWR, 0);
    if (fd < 0)
    {
        printf("Open i2c device [i2c-0] fail!!\n");
        return -1;
    }

    ret = ioctl(fd, I2C_TENBIT, 0);

    ret = ioctl(fd, I2C_SLAVE_FORCE, WM8974_I2C_SLAVE_ADDR);

    if (ret < 0)
    {
        printf("set i2c slave addr fail,ret is %d....\n", ret);
        return -1;
    }

    ret = write(fd, addr, 1);
    printf("write ret is %d....\n", ret);
    close(fd);
    if (ret != 1)
    {
        return 0;
    }
    else
    {
        return 1;
    }
    
}


