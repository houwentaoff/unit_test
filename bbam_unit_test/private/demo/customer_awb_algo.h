#ifndef      CUSTOMER_AWB_ALGO_H
#define       CUSTOMER_AWB_ALGO_H

#include    "img_struct.h"

extern    void    customer_awb_control_init();
extern    void    customer_awb_control( awb_data_t* pTileInfo,  awb_gain_t *pawbGain);

#endif
