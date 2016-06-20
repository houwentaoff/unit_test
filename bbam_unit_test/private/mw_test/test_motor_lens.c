#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "unistd.h"
#include "mw_motor_lens.h"

#define DEMO_DEBUG(fmt, args ... )   printf("Debug! [%s][%d][%s:]" fmt, __FILE__, __LINE__, __FUNCTION__, ##args)
#define DEMO_ERROR(fmt, args ... )    printf("Error! [%s][%d][%s:]" fmt, __FILE__, __LINE__, __FUNCTION__, ##args)
#define DEMO_WARRING(fmt, args ... )   printf("Warning! [%s][%d][%s:]" fmt, __FILE__, __LINE__, __FUNCTION__, ##args)

int main(int argc, char **argv)
{
    int ret, MaxStep, CurrentPosition, i;
    char cmd;
    MOTOR_PARAM_CAPS motorCaps;

    if(argc < 2) {
        printf("usage : ./main f/z/i/m\n");
        return -1;
    }

    cmd = *argv[1];
    
    ret = MotorOpen();
    if(ret != 0)
    {
        DEMO_ERROR("%d\n", ret);
        return -1;
    }
/*
    Motor_Device_Reset(cmd);
    ret = MotorClose();
    if(ret != 0)
    {
        DEMO_ERROR("%d\n", ret);
        return -1;
    }
    return 0;

*/
    MotorGetCaps(&motorCaps);

    switch(cmd)
    {
        case 'f':
        {
            ret = FocusGetMaxStep(&MaxStep);
            if(ret != 0)
            {
                DEMO_ERROR("%d\n", ret);
                return -1;
            }
            DEMO_DEBUG("get focus max step : %d\n", MaxStep);
            sleep(1);

            ret = FocusGetPosition(&CurrentPosition);
            if(ret != 0)
            {
                DEMO_ERROR("%d\n", ret);
                return -1;
            }
            DEMO_DEBUG("current position : %d\n", CurrentPosition);
            sleep(1);

            i = 2;
            while(i --)
            {
                CurrentPosition = 0;
                DEMO_DEBUG("set position : %d...\n", CurrentPosition);
                ret = FocusSetPosition(CurrentPosition);
                if(ret != 0)
                {
                    DEMO_ERROR("%d\n", ret);
                    return -1;
                }
                sleep(1);

                CurrentPosition = MaxStep;
                DEMO_DEBUG("set position : %d...\n", CurrentPosition);
                ret = FocusSetPosition(CurrentPosition);
                if(ret != 0)
                {
                    DEMO_ERROR("%d\n", ret);
                    return -1;
                }
                sleep(1);
            }

            break;
        }
        
        case 'z':
        {
            ret = ZoomGetMaxStep(&MaxStep);
            if(ret != 0)
            {
                DEMO_ERROR("%d\n", ret);
                return -1;
            }
            DEMO_DEBUG("get focus max step : %d\n", MaxStep);
            sleep(1);

            ret = ZoomGetPosition(&CurrentPosition);
            if(ret != 0)
            {
                DEMO_ERROR("%d\n", ret);
                return -1;
            }
            DEMO_DEBUG("current position : %d\n", CurrentPosition);
            sleep(1);

            i = 2;
            while(i --)
            {
                CurrentPosition = 0;
                DEMO_DEBUG("set position : %d...\n", CurrentPosition);
                ret = ZoomSetPosition(CurrentPosition);
                if(ret != 0)
                {
                    DEMO_ERROR("%d\n", ret);
                    return -1;
                }
                sleep(1);

                CurrentPosition = MaxStep;
                DEMO_DEBUG("set position : %d...\n", CurrentPosition);
                ret = ZoomSetPosition(CurrentPosition);
                if(ret != 0)
                {
                    DEMO_ERROR("%d\n", ret);
                    return -1;
                }
                sleep(1);
            }
            
            break;
        }
        
        case 'i':
        {
            char ir_open;

            if(argc < 3) {
                printf("usage : ./main i 0/1\n");
                break;
            }
            
            ir_open = *argv[2];
            if(ir_open == '0') {
                IrcutClose();
            }
            else if(ir_open == '1') {
                IrcutOpen();
            }
            else {
                DEMO_ERROR("err ircut cmd : %c\n", ir_open);
            }

            break;
        }

        case 'm'://manual
        {
            char buf[16];
            int fPos, zPos;

            while(1) {
                printf("\nplease input cmd : f/z/mf/mz/fz/fn/zn/mfn/mzn/q\n");
                scanf("%s", buf);
                if(strcmp(buf, "f") == 0) {
                    printf("reset/step : \n");
                    scanf("%s", buf);
                    if(strcmp(buf, "reset") == 0) {
                        FocusMotorReset();
                    }
                    else {
                        FocusSetPosition(atoi(buf));
                    }
                }
                else if(strcmp(buf, "z") == 0) {
                    printf("reset/step : \n");
                    scanf("%s", buf);
                    if(strcmp(buf, "reset") == 0) {
                        ZoomMotorReset();
                    }
                    else {
                        ZoomSetPosition(atoi(buf));
                    }
                }
                else if(strcmp(buf, "mf") == 0) {
                    printf("step : \n");
                    scanf("%s", buf);
                    int step = atoi(buf);
                    FocusGetMaxStep(&MaxStep);
                    for(i = 0; i < MaxStep; i = i + step)
                        FocusSetPosition(i);
                }
                else if(strcmp(buf, "mz") == 0) {
                    printf("step : \n");
                    scanf("%s", buf);
                    int step = atoi(buf);
                    ZoomGetMaxStep(&MaxStep);
                    for(i = 0; i < MaxStep; i = i + step)
                        ZoomSetPosition(i);
                }
                else if(strcmp(buf, "fn") == 0) {
                    printf("reset/step : \n");
                    scanf("%s", buf);
                    if(strcmp(buf, "reset") == 0) {
                        FocusMotorReset();
                    }
                    else {
                        ZoomGetPosition(&zPos);
                        ZoomFocusSetPosition(zPos, atoi(buf));
                    }
                }
                else if(strcmp(buf, "zn") == 0) {
                    printf("reset/step : \n");
                    scanf("%s", buf);
                    if(strcmp(buf, "reset") == 0) {
                        ZoomMotorReset();
                    }
                    else {
                        FocusGetPosition(&fPos);
                        ZoomFocusSetPosition(atoi(buf), fPos);
                    }
                }
                else if(strcmp(buf, "mfn") == 0) {
                    char buf2[16];
                    printf("step : \n");
                    scanf("%s", buf);
                    printf("to : \n");
                    scanf("%s", buf2);
                    int step = atoi(buf);
                    int step_to = atoi(buf2);
                    //FocusGetMaxStep(&MaxStep);
                    //for(i = 0; i < MaxStep; i = i + step) {
                    for(i = 0; i < step_to; i = i + step) {
                        ZoomGetPosition(&zPos);
                        ZoomFocusSetPosition(zPos, i);
                    }
                }
                else if(strcmp(buf, "mzn") == 0) {
                    char buf2[16];
                    printf("step : \n");
                    scanf("%s", buf);
                    printf("to : \n");
                    scanf("%s", buf2);
                    int step = atoi(buf);
                    int step_to = atoi(buf2);
                    //ZoomGetMaxStep(&MaxStep);
                    //for(i = 0; i < MaxStep; i = i + step) {
                    for(i = 0; i < step_to; i = i + step) {
                        FocusGetPosition(&fPos);
                        ZoomFocusSetPosition(i, fPos);
                    }
                }
                else if(strcmp(buf, "fz") == 0) {
                    char buf2[16];
                    printf("f step : \n");
                    scanf("%s", buf);
                    printf("z step : \n");
                    scanf("%s", buf2);
                    int step_f = atoi(buf);
                    int step_z = atoi(buf2);
                    ZoomFocusSetPosition(step_z, step_f);
                }
                else if(strcmp(buf, "q") == 0) {
                    break;
                }
                else
                    printf("err cmd : %s\n", buf);

                FocusGetPosition(&fPos);
                ZoomGetPosition(&zPos);
                printf("Focus : %d, Zoom : %d\n\n\n", fPos, zPos);
            }
            
            break;
        }

        default:
            break;
    }

    ret = MotorClose();
    if(ret != 0)
    {
        DEMO_ERROR("%d\n", ret);
        return -1;
    }
    DEMO_DEBUG("closed\n");

    return 0;
}

