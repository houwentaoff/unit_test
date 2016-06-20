#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include "gmi_acc_interface.h"

#define DEMO_DEBUG(fmt, args ... )     printf("Debug! [%s][%d][%s:]" fmt, __FILE__, __LINE__, __FUNCTION__, ##args)
#define DEMO_ERROR(fmt, args ... )     printf("Error! [%s][%d][%s:]" fmt, __FILE__, __LINE__, __FUNCTION__, ##args)
#define DEMO_WARRING(fmt, args ... )   printf("Warning! [%s][%d][%s:]" fmt, __FILE__, __LINE__, __FUNCTION__, ##args)

typedef struct _SYSTEM_MENU {
    char    *cmd_simple;
    char    *cmd_verbose_english;
    char    *cmd_verbose_chinese;
    int	    (*func)(void *param);
} SYSTEM_MENU;

//#define I2C_CARD
#ifdef I2C_CARD//yll m
extern void ll_OpenI2c(void);
extern void ll_CloseI2c(void);
#else
extern int gmi_acc_gpio_open(void);
extern int gmi_acc_gpio_close(void);
extern unsigned int gmi_acc_gpio_read(unsigned int gpioID);
#endif

static unsigned char buf_passwd[8] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};

//#include "ll_port.h"
#include "lib_Crypto.h"

#define DEFAULT_ADDRESS 0xb ///< Default address for CryptoMemory devices

static int testMTZ(void)
{
    uchar ucData[2];
    int ret;

    ucData[0] = 0x5a;
    ucData[1] = 0xa5;
    ret = cm_WriteConfigZone(DEFAULT_ADDRESS, 0x0A, ucData, 2, FALSE);
    if(0!=ret)
    {
        printf("write MTZ fail....ret=0x%x\n",ret);
        return -1;
    }
    ucData[0] = 0x00;
    ucData[1] = 0x00;
    ret = cm_ReadConfigZone(DEFAULT_ADDRESS, 0x0A, ucData, 2);
    if(0!=ret)
    {
        printf("read MTZ fail....ret=0x%x\n",ret);
        return -1;
    }
    if(ucData[0] != 0x5a || ucData[1] != 0xa5)
    {
        printf("testMTZ Error!! : %d : %d\n", ucData[0], ucData[1]);
        return -1;
    }
    return 0;
}

static int at88_config_unlock(void)
{
    unsigned char tempData;
    unsigned char ucData[3];
    RETURN_CODE ucReturn;

    tempData =0x00;
    int ret = cm_ReadConfigZone(DEFAULT_ADDRESS, 0xE8, &tempData, 1);
    if(ret != 0)
    {
        printf("cm_ReadConfigZone PAC7 fail!\n");
        return -1;	
    }

    if(tempData==0x00)
    {
        printf("\nPAC7 =0x%x\n",tempData);
        printf("\n at88 has been permantly locked, please change chip..\n");
        return -1;
    }

    if(tempData!=0xff)
    {
        printf("\nPAC7 =0x%x\n",tempData);
    }

    ucData[0] = 0x22;
    ucData[1] = 0xE8;
    ucData[2] = 0x3F;
    ucReturn = cm_VerifyPassword(DEFAULT_ADDRESS, ucData,7, 0);
    if (ucReturn != SUCCESS) 
    {
            printf("\ncm_VerifyPassword fail ...ret=0x%x\n",ucReturn);
            return -1;
    }

    return 0;
}

static int at88_rwead_aac(void)
{
    int ret;
    unsigned char tempData;

    //dcr , device addr 0x0b
    tempData=0x00;
    ret = cm_ReadConfigZone(DEFAULT_ADDRESS, 0x18, &tempData, 1);
    if(ret != 0)
    {		
        printf("cm_ReadConfigZone reg 0x18 fail\n");
        return -1;	
    }
    //printf("DCR = 0x%2x\n", tempData);
    
    if(tempData!=0xeb)
    {
        tempData=0xeb;//clear etr
        ret = cm_WriteConfigZone(DEFAULT_ADDRESS, 0x18, &tempData, 1, FALSE);
        if (ret != SUCCESS) 
        {
            printf("cm_WriteConfigZone reg 0x18 fail!\n");
            return -1;
        }
    }
    
    tempData=0x00;
    ret = cm_ReadConfigZone(DEFAULT_ADDRESS, 0x60, &tempData, 1);
    if(ret != 0)
    {		
        printf("cm_ReadConfigZone run wrong!!\n");
        return -1;	
    }
    //printf("AAC1 = 0x%2x\n", tempData);
    if(tempData!=0xff)
    {
        tempData = 0xff;
        ret = cm_WriteConfigZone(DEFAULT_ADDRESS, 0x60, &tempData, 1, FALSE);
        if (ret != SUCCESS) 
        {
            printf("cm_WriteConfigZone run wrong!\n");
            return -1;
        }
    }
    
    unsigned char init_cryptogram_data[7];
    init_cryptogram_data[0]=0x22;
    init_cryptogram_data[1]=0x22;
    init_cryptogram_data[2]=0x22;
    init_cryptogram_data[3]=0x22;
    init_cryptogram_data[4]=0x22;
    init_cryptogram_data[5]=0x22;
    init_cryptogram_data[6]=0x22;
    ret = cm_WriteConfigZone(DEFAULT_ADDRESS, 0x61, init_cryptogram_data, 7, FALSE);
    if (ret != SUCCESS) 
    {
        printf("cm_WriteConfigZone init_cryptogram_data fail !\n");
        return -1;
    }
    
    return 0;
}

static int cryptoReset(void)
{
    int ret;
    
    ret = cm_ResetCrypto(DEFAULT_ADDRESS);
    if(0!=ret)
        printf("\n cm_ResetCrypto fail....ret=0x%x\n",ret);
    
    return 0;
}

int cryptoInit(void)
{
    /*
    char cmd[128];

    strcpy(cmd, "a5s_debug -w 0x70009018 -d 0x000dc11c");
    system(cmd);
    usleep(0);
    */

#ifdef I2C_CARD 
	ll_OpenI2c();
#else
    gmi_acc_gpio_open();
    printf("init gpio 11 : %d, gpio 12 : %d\n", gmi_acc_gpio_read(11), gmi_acc_gpio_read(12));
    printf("init gpio 11 : %d, gpio 12 : %d\n", gmi_acc_gpio_read(11), gmi_acc_gpio_read(12));
#endif

    ll_PowerOn();
    if( cm_Init() != SUCCESS) // Initialize the CM
	{
        printf("CM Init Failed !!!\n");
		return -1;
	}

    testMTZ();

    return 0;
}

int cryptoDestroy(void)
{
#ifdef I2C_CARD
	ll_CloseI2c();
#else
    printf("destroy gpio 11 : %d, gpio 12 : %d\n", gmi_acc_gpio_read(11), gmi_acc_gpio_read(12));
    printf("destroy gpio 11 : %d, gpio 12 : %d\n", gmi_acc_gpio_read(11), gmi_acc_gpio_read(12));
    gmi_acc_gpio_close();
#endif

    /*
    char cmd[128];
    usleep(0);
    strcpy(cmd, "a5s_debug -w 0x70009018 -d 0x000dc11f");
    system(cmd);
    */

    return 0;
}

/*
 * 校检
 */
int crypetoVerify(unsigned char *seed)
{
    int i;
    unsigned char ucG[8];
    RETURN_CODE ucReturn;

    cryptoReset();

    for(i = 0 ; i < 8 ; i ++)
        ucG[i] = seed[i];
    ucReturn = cm_VerifyCrypto(DEFAULT_ADDRESS, 1, ucG, NULL, FALSE);
    if (ucReturn != SUCCESS)
    {
        printf("cm_VerifyCrypto fail\n");
        return -1;
    }

    cryptoReset();

    return 0;
}

/*
 * 写密码
 */
int cryptoWriteSeed(unsigned char *seed)
{
    unsigned char ucG[8];
    int i;
    RETURN_CODE ucReturn;

    at88_config_unlock();
    at88_rwead_aac();
    
    for(i = 0 ; i < 8 ; i ++)
        ucG[i] = seed[i];
    ucReturn = cm_WriteConfigZone(DEFAULT_ADDRESS, 0x98, ucG, 8, FALSE);
    if (ucReturn != SUCCESS)
    {
        printf("WRITESEED fail...ret=0x%x\n",ucReturn);
        return -1;
    }
    
    cryptoReset();

    return 0;
}

static int do_crypto_test(void *param) {
    int ret;

    if(param == NULL) {
        printf("please input param:\n");
        printf("    test_accvisio -v      verify passwd\n");
        printf("    test_accvisio -w      write passwd\n");

        return 0;
    }

    if(strcmp((char *)param, "-v") == 0) {
        cryptoInit();

        ret = crypetoVerify(buf_passwd);
        if(ret == 0)
            printf("password verify success...\n");
        else
            printf("password wrong...\n");

        cryptoDestroy();
    }
    else if(strcmp((char *)param, "-w") == 0) {
        cryptoInit();

        ret = cryptoWriteSeed(buf_passwd);
        if(ret == 0)
            printf("write passwd success...\n");
        else
            printf("write password failed...\n");

        cryptoDestroy();
    }

    return 0;
}

static int do_devSN_test(void *param) {
    unsigned char devSN[64];

    if(param == NULL) {
        printf("please input writing test val\n");
        param = "GMI88888888";
    }
    GMI_ReadDeviceSN(devSN);
    DEMO_DEBUG("read sn first : %s\n", devSN);

    strcpy((char *)devSN, (char *)param);
    DEMO_DEBUG("write sn : %s\n", devSN);
    GMI_WriteDeviceSN(devSN);

    GMI_ReadDeviceSN(devSN);
    DEMO_DEBUG("read sn second : %s\n", devSN);

    return 0;
}

static int do_IDR_force_test(void *param) {
    GMI_ForceIDR();
    return 0;
}

static int do_jpeg_test(void *param) {
    char *buf = NULL;
    int size;
    FILE *fp;

    fp = fopen("test.jpg", "wb");
    if(fp == NULL) {
        DEMO_ERROR("open file failed\n");
        return -1;
    }

    GMI_CaptureJpegData((unsigned char **)&buf, (unsigned int *)&size);
    fwrite(buf, 1, size, fp);
    GMI_DestroyJpegData((unsigned char *)buf);

    fclose(fp);

    return 0;
}

static int do_rtc_test(void *param) {
#if 0
    SYSTEM_TIME sys_time;

    printf("read rtc first : \n");
    rtcGetCurrentTime(&sys_time);
    printf("sys time :%d-%d-%d, today is weekday %d , time:%d:%d:%d:%d\n",
            sys_time.year, sys_time.month, sys_time.day, sys_time.wday,
            sys_time.hour, sys_time.minute, sys_time.second, sys_time.msecond);

    sys_time.year = 2012;
    sys_time.month = 12;
    sys_time.day = 21;
    sys_time.wday = 5;
    sys_time.hour = 0;
    sys_time.minute = 0;
    sys_time.second = 0;
    sys_time.msecond = 0;
    printf("write rtc : %d-%d-%d-%d-%d-%d-%d\n",
            sys_time.year, sys_time.month, sys_time.day, sys_time.hour,
            sys_time.minute, sys_time.second, sys_time.msecond);
    rtcSetCurrentTime(&sys_time);

    printf("read rtc second : \n");
    rtcGetCurrentTime(&sys_time);
    printf("sys time :%d-%d-%d, today is weekday %d , time:%d:%d:%d:%d\n",
            sys_time.year, sys_time.month, sys_time.day, sys_time.wday,
            sys_time.hour, sys_time.minute, sys_time.second, sys_time.msecond);
#else
    struct tm_ex sys_time;

    printf("read system time\n");
    GMI_GetSystemTime(&sys_time);

    printf("sys time :%d-%d-%d, time:%d:%d:%d:%d\n",
            sys_time.tm_year, sys_time.tm_mon, sys_time.tm_mday, 
            sys_time.tm_hour, sys_time.tm_min, sys_time.tm_sec, sys_time.tm_msec);

    printf("\nread rtc time\n");
    GMI_GetRTCTime(&sys_time);

    printf("rtc time :%d-%d-%d, time:%d:%d:%d:%d\n",
            sys_time.tm_year, sys_time.tm_mon, sys_time.tm_mday, 
            sys_time.tm_hour, sys_time.tm_min, sys_time.tm_sec, sys_time.tm_msec);
#endif

    return 0;
}

static int do_mac_test(void *param) {
    char addr[32];

    if(param == NULL) {
        printf("please input writing test val\n");
        param = "22:33:44:55:66:77";
    }

    GMI_ReadMAC((unsigned char *)addr);
    printf("read mac addr first : %s\n", addr);

    strcpy(addr, (char *)param);
    printf("write new mac addr : %s\n", addr);
    GMI_WriteMAC((unsigned char *)addr);

    GMI_ReadMAC((unsigned char *)addr);
    printf("read mac addr second : %s\n", addr);

    return 0;
}

static int do_flash_test(void *param) {
    char buf[512];
    int i, size, offset, valWrite;

    size = 10;
    offset = 50;
    if(param == NULL) {
        printf("please input writing test val\n");
        param = "55";
    }
    valWrite = atoi((char *) param);

    //read
    memset(buf, 0, 512);
    GMI_ReadFlashData(offset, size, (unsigned char *)buf);
    printf("read first :\n");
    for(i = 0; i < size; i ++)
        printf("%d ", buf[i]);
    printf("\n");

    //write
    for(i = 0; i < size; i ++)
        buf[i] = valWrite;
    printf("write : %d\n", valWrite);
    GMI_WriteFlashData(offset, size, (unsigned char *)buf);
    
    //read
    memset(buf, 0, 512);
    GMI_ReadFlashData(offset, size, (unsigned char *)buf);
    printf("read second :\n");
    for(i = 0; i < size; i ++)
        printf("%d ", buf[i]);
    printf("\n");

    return 0;
}

static SYSTEM_MENU menu_array[] = {
    {"a",    "flash test",          "flash读写测试",          do_flash_test},
    {"b",    "MAC address test",    "MAC地址读写测试",        do_mac_test},
    {"c",    "rtc test",            "实时时钟读写测试",       do_rtc_test},
    {"d",    "jpeg test",           "JPEG图片抓拍测试",       do_jpeg_test},
    {"e",    "IDR force test",      "强制I帧测试",            do_IDR_force_test},
    {"f",    "sn R/W test",         "序列号读写测试",         do_devSN_test},
    {"g",    "crypto test",         "加密芯片测试",           do_crypto_test},
};

int main(int argc, char **argv) {
    int i;
    char input_cmd[32] = "";

    while(1) {
        for(i = 0; i < (int)(sizeof(menu_array) / sizeof(SYSTEM_MENU)); i++) {
            printf("%s: %s(%s)\n", menu_array[i].cmd_simple, menu_array[i].cmd_verbose_english, 
                menu_array[i].cmd_verbose_chinese);
        }
        printf("q: quit(结束运行)\n"); 

        printf("\ninput: ");
        scanf("%s", input_cmd);
        for(i = 0; i < (int)(sizeof(menu_array) / sizeof(SYSTEM_MENU)); i++) {
            if(0 == strncmp(menu_array[i].cmd_simple, input_cmd, 31)) {
                printf("\n####################################\n");
                if(argc >= 2)
                    menu_array[i].func(argv[1]);
                else
                    menu_array[i].func(NULL);
                printf("####################################\n\n");
                break;
            }
        }

        if(strcmp(input_cmd, "q") == 0)
            break;
    }
    
    return 0;
}
