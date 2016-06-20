/** \file
 *  \brief Library Support
 */
/*
******************************************************************************
*                 |
*  File Name      | lib_Support.c
*-----------------|-----------------------------------------------------------
*  Project        | CryptoMemory
*-----------------|-----------------------------------------------------------
*  Created        | September 10, 2007
*-----------------|-----------------------------------------------------------
*  Description    | This file contains all the support functions for library.
*-----------------|-----------------------------------------------------------
*                 | Copyright (c) 2008 Atmel Corp.
*                 | All Rights Reserved.
*                 |
******************************************************************************
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lib_Crypto.h"

/******************************************************************************
*
* Below are definition of all APIs call by library at run-time.
*
********************************************************************************/

/** \brief Number of CM device in system.
 */

#define CM_NUM_DEVICES 1

typedef struct app_sDevice_Type_Info {
    uchar ucDeviceType;         ///< Device type
    uchar ucAnswer2Reset[8];    ///< Answer to Reset for this device type
} DEVICE_TYPE_INFO;

const DEVICE_TYPE_INFO sDeviceTypeInfoTBL[9 + 1] PROGMEM =
{
{AT88SC0104C,  {0x3b, 0xb2, 0x11, 0, 0x10, 0x80, 0x00, 0x01}},
{AT88SC0204C,  {0x3b, 0xb2, 0x11, 0, 0x10, 0x80, 0x00, 0x02}},
{AT88SC0404C,  {0x3b, 0xb2, 0x11, 0, 0x10, 0x80, 0x00, 0x04}},
{AT88SC0808C,  {0x3b, 0xb2, 0x11, 0, 0x10, 0x80, 0x00, 0x08}},
{AT88SC1616C,  {0x3b, 0xb2, 0x11, 0, 0x10, 0x80, 0x00, 0x16}},
{AT88SC3216C,  {0x3b, 0xb3, 0x11, 0, 0x00, 0x00, 0x00, 0x32}},
{AT88SC6416C,  {0x3b, 0xb3, 0x11, 0, 0x00, 0x00, 0x00, 0x64}},
{AT88SC12816C, {0x3b, 0xb3, 0x11, 0, 0x00, 0x00, 0x01, 0x28}},
{AT88SC25616C, {0x3b, 0xb3, 0x11, 0, 0x00, 0x00, 0x02, 0x56}},
};

/** \brief CM Device Address Table.  One entry per CM device on the bus
 *
 *  Enter the device address used for each CM device on the bus.  End the table
 *  with a NULL entry to the library knows how many devices there are.
 *  Entries can be in any order.
 */
const uchar cm_device_addresses[CM_NUM_DEVICES + 1] PROGMEM =
    {0xb, 0};

// Adjust the array size according to number of device.
// Each device uses 21 Bytes.
uchar dummy_malloc[21];  

#ifdef CMC

/** \if CMC_DOXY
 * \brief Number of CMC device in system.
 *  \endif
 */
#define CMC_NUM_DEVICES 1

/** \if CMC_DOXY
 *  \brief CMC Device Address Table.  One entry per CMC device on the bus
 *
 *  Enter the device address used for each CMC device on the bus.  End the table
 *  with a NULL entry to the library knows how many devices there are.
 *  Entries can be in any order.
 *  \endif
 */
const uchar cmc_device_addresses[CMC_NUM_DEVICES+1] PROGMEM =
    {0xc0, 0};

#endif


/*
******************************************************************************
*  Function Name  | cm_FindDeviceIndex
*-----------------|-----------------------------------------------------------
*  Project        | CryptoMemory
*-----------------|-----------------------------------------------------------
*  Created        | October 11, 2007
*-----------------|-----------------------------------------------------------
*  Description    | This routine returns an index based on CM device address.
*-----------------|-----------------------------------------------------------
*  Inputs         | ucCmDevAddr       : CM device address
*-----------------|-----------------------------------------------------------
*  Outputs        | CM device index
******************************************************************************
*/
uchar cm_FindDeviceIndex ( uchar ucCmDevAddr )
{
    uchar i;

    for (i = 0; i < CM_MAX_DEV_ADDR; i++)
    {
        if (ROM_READ_BYTE ( &(cm_device_addresses[i]) ) == ucCmDevAddr)
        {
            return i;
        }
    }
    return CM_MAX_DEV_ADDR;
}

/*
******************************************************************************
*  Function Name  | getCMDevAddr
*-----------------|-----------------------------------------------------------
*  Project        | CryptoMemory
*-----------------|-----------------------------------------------------------
*  Created        | October 11, 2007
*-----------------|-----------------------------------------------------------
*  Description    | This routine returns an device address based on CM device
*                 | index.
*-----------------|-----------------------------------------------------------
*  Inputs         | ucIndex       : CM device index
*-----------------|-----------------------------------------------------------
*  Outputs        | CM device address
******************************************************************************
*/
uchar getCMDevAddr ( uchar ucIndex )
{
    return (ROM_READ_BYTE ( &(cm_device_addresses[ucIndex]) ));
}

/*
******************************************************************************
*  Function Name  | getCMDevType
*-----------------|-----------------------------------------------------------
*  Project        | CryptoMemory
*-----------------|-----------------------------------------------------------
*  Created        | October 11, 2007
*-----------------|-----------------------------------------------------------
*  Description    | This routine returns a CM device type based on device index.
*-----------------|-----------------------------------------------------------
*  Inputs         | ucIndex       : device index
*-----------------|-----------------------------------------------------------
*  Outputs        | CM device type
******************************************************************************
*/
CM_DEVICE_TYPE getCMDevType ( uchar ucIndex )
{
    uchar i, j;
    RETURN_CODE ucReturn;
    uchar ucCmAddr;
    uchar ucData[8];
    DEVICE_TYPE_INFO* pcm_sDeviceTypeInfo;

    ucCmAddr = getCMDevAddr ( ucIndex );
    ucReturn = cm_ReadConfigZone ( ucCmAddr, 0x00, ucData, 8 );
    if (ucReturn != SUCCESS)
    {
        return (UNKNOWN_DEVICE_TYPE);
    }

    pcm_sDeviceTypeInfo = (DEVICE_TYPE_INFO*) &sDeviceTypeInfoTBL[0];
    for (i = 0; i < 9; i++, pcm_sDeviceTypeInfo++)
    {
        for (j = 0; j <= 7; j++)
        {
            if (ucData[j] !=
               (uchar) ROM_READ_BYTE ( &(pcm_sDeviceTypeInfo->ucAnswer2Reset[j]) ) )
            {
                break;
            }
        }
        if (j == 8)
        {
            break;
        }
    }

    if( i == 9)
    {
        return (UNKNOWN_DEVICE_TYPE);
    }
    else
    {
        return (ROM_READ_BYTE ( &(pcm_sDeviceTypeInfo->ucDeviceType) ) );
    }
}

/*
******************************************************************************
*  Function Name  | lib_memcpy
*-----------------|-----------------------------------------------------------
*  Project        | CryptoMemory
*-----------------|-----------------------------------------------------------
*  Created        | October 11, 2007
*-----------------|-----------------------------------------------------------
*  Description    | This routine copies number of bytes from source to destination.
*-----------------|-----------------------------------------------------------
*  Inputs         | pucDestMem       : destination buffer
*                 | pucDestMem       : source buffer
*                 | uiCnt            : number of bytes to copy
*-----------------|-----------------------------------------------------------
*  Outputs        | none
******************************************************************************
*/
void lib_memcpy ( puchar pucDestMem, puchar pucSrcMem, uint uiCnt )
{
    memcpy ( pucDestMem, pucSrcMem, uiCnt );
}

/*
******************************************************************************
*  Function Name  | lib_memcmp
*-----------------|-----------------------------------------------------------
*  Project        | CryptoMemory
*-----------------|-----------------------------------------------------------
*  Created        | October 11, 2007
*-----------------|-----------------------------------------------------------
*  Description    | This routine compares two strings.
*-----------------|-----------------------------------------------------------
*  Inputs         | pucMem1         : string 1
*                 | pucMem2         : string 2
*                 | unCnt           : number of bytes to compare
*-----------------|-----------------------------------------------------------
*  Outputs        | 0:              : SUCCESS
*                 | non-0           : FAIL
******************************************************************************
*/
uchar lib_memcmp ( puchar pucMem1, puchar pucMem2, uint ucCnt )
{
    return (memcmp ( pucMem1, pucMem2, ucCnt ) );
}

/*
******************************************************************************
*  Function Name  | lib_malloc
*-----------------|-----------------------------------------------------------
*  Project        | CryptoMemory
*-----------------|-----------------------------------------------------------
*  Created        | October 11, 2007
*-----------------|-----------------------------------------------------------
*  Description    | This routine allocates number of bytes from memory pool.
*-----------------|-----------------------------------------------------------
*  Inputs         | ucBytes         : number of bytes to allocate
*-----------------|-----------------------------------------------------------
*  Outputs        | puchar          : pointer to allocated buffer
******************************************************************************
*/
puchar lib_malloc ( uchar ucBytes )
{
    return dummy_malloc;
//  return ((uchar*) malloc ( ucBytes ) );
}

/*
******************************************************************************
*  Function Name  | lib_free
*-----------------|-----------------------------------------------------------
*  Project        | CryptoMemory
*-----------------|-----------------------------------------------------------
*  Created        | October 11, 2007
*-----------------|-----------------------------------------------------------
*  Description    | This routine returns buffer to memory pool.
*-----------------|-----------------------------------------------------------
*  Inputs         | puchar          : pointer to buffer
*-----------------|-----------------------------------------------------------
*  Outputs        | void
******************************************************************************
*/
void lib_free ( puchar pucBuff )
{
    free ( pucBuff );
}

/*
******************************************************************************
*  Function Name  | lib_rand
*-----------------|-----------------------------------------------------------
*  Project        | CryptoMemory
*-----------------|-----------------------------------------------------------
*  Created        | October 11, 2007
*-----------------|-----------------------------------------------------------
*  Description    | This routine generates a random number.
*-----------------|-----------------------------------------------------------
*  Inputs         | None
*-----------------|-----------------------------------------------------------
*  Outputs        | a random byte
******************************************************************************
*/
uchar lib_rand ( void )
{
    return ( rand () );
}


#ifdef CMC
/*
******************************************************************************
*  Function Name  | WaitForData
*-----------------|-----------------------------------------------------------
*  Project        | CryptoMemory
*-----------------|-----------------------------------------------------------
*  Created        | October 11, 2007
*-----------------|-----------------------------------------------------------
*  Description    | This routine waits for Data Available bit to SET.
*-----------------|-----------------------------------------------------------
*  Inputs         | uchar ucCmcDevAddr
*-----------------|-----------------------------------------------------------
*  Outputs        | None
******************************************************************************
*/
uchar WaitForData ( uchar ucCmcDevAddr )
{
    uchar ucStatus = 0;
    uchar ucError = 0;
    uchar ucDataAvail = 0;

    WDT_Start ();
    while (!WDTFlag)
    {
        ucStatus = cmc_Status ( ucCmcDevAddr );
        ucError = (ucStatus & 0xE0) >> 5;
        ucDataAvail = ucStatus & 0x01;

        if (ucDataAvail || ((ucError > 0) && (ucError < 5)))
        {
            break;
        }
    }
    WDT_Off ();

    if (!ucDataAvail || ((ucError > 0) && (ucError < 5)))
    {
        return CMC_DATA_NOT_AVAIL;
    }

    if (WDTFlag)
    {
        return FAILED;
    }
    else
    {
        return SUCCESS;
    }
}

/*
******************************************************************************
*  Function Name  | WaitForNotBusy
*-----------------|-----------------------------------------------------------
*  Project        | CryptoMemory
*-----------------|-----------------------------------------------------------
*  Created        | October 11, 2007
*-----------------|-----------------------------------------------------------
*  Description    | This routine waits for BUSY bit to CLEAR.
*-----------------|-----------------------------------------------------------
*  Inputs         | uchar ucCmcDevAddr
*-----------------|-----------------------------------------------------------
*  Outputs        | None
******************************************************************************
*/
uchar WaitForNotBusy ( uchar ucCmcDevAddr )
{
    uchar ucStatus = 0;
    uchar ucError = 0;
    uchar ucBusy = 0;

    WDT_Start ();
    while (!WDTFlag)
    {
        ucStatus = cmc_Status ( ucCmcDevAddr );
        ucError = (ucStatus & 0xE0) >> 5;
        ucBusy = ucStatus & 0x02;

        if (!ucBusy || ((ucError > 0) && (ucError < 5)))
        {
            break;
        }
    }
    WDT_Off ();

    if (ucBusy || ((ucError > 0) && (ucError < 5)))
    {
        return CMC_BUSY;
    }

    if (WDTFlag)
    {
        return FAILED;
    }
    else
    {
        return SUCCESS;
    }
}

/*
******************************************************************************
*  Function Name  | WaitForStartupDone
*-----------------|-----------------------------------------------------------
*  Project        | CryptoMemory
*-----------------|-----------------------------------------------------------
*  Created        | October 11, 2007
*-----------------|-----------------------------------------------------------
*  Description    | This routine waits for StatUpDone bit to SET
*-----------------|-----------------------------------------------------------
*  Inputs         | uchar ucCmcDevAddr
*-----------------|-----------------------------------------------------------
*  Outputs        | None
******************************************************************************
*/

uchar WaitForStartupDone ( uchar ucCmcDevAddr )
{
    uchar ucStatus = 0;
    uchar ucError = 0;
    uchar ucStartupDone = 0;

    WDT_Start ();
    while (!WDTFlag)
    {
        ucStatus = cmc_Status ( ucCmcDevAddr );
        ucError = (ucStatus & 0xE0) >> 5;
        ucStartupDone = ucStatus & 0x04;

        if (ucStartupDone || ((ucError > 0) && (ucError < 5)))
        {
            break;
        }
    }
    WDT_Off ();

    if (!ucStartupDone || ((ucError > 0) && (ucError < 5)))
    {
        return CMC_STARTUP_NOT_DONE;
    }

    if (WDTFlag)
    {
        return FAILED;
    }
    else
    {
        return SUCCESS;
    }
}

/*
******************************************************************************
*  Function Name  | cmc_FindDeviceIndex
*-----------------|-----------------------------------------------------------
*  Project        | CryptoMemory
*-----------------|-----------------------------------------------------------
*  Created        | October 11, 2007
*-----------------|-----------------------------------------------------------
*  Description    | This routine returns an index based on CMC device address.
*-----------------|-----------------------------------------------------------
*  Inputs         | ucCmcDevAddr       : CMC device address
*-----------------|-----------------------------------------------------------
*  Outputs        | CMC device index
******************************************************************************
*/
uchar cmc_FindDeviceIndex ( uchar ucCmcDevAddr )
{
    uchar i;

    for (i = 0; i < CMC_MAX_DEV_ADDR; i++)
    {
        if (ROM_READ_BYTE ( &(cmc_device_addresses[i]) ) == ucCmcDevAddr)
        {
            return i;
        }
    }
    return CMC_MAX_DEV_ADDR;
}

/*
******************************************************************************
*  Function Name  | getCMCDevAddr
*-----------------|-----------------------------------------------------------
*  Project        | CryptoMemory
*-----------------|-----------------------------------------------------------
*  Created        | October 11, 2007
*-----------------|-----------------------------------------------------------
*  Description    | This routine returns a device address based on CMC device
*                 | index.
*-----------------|-----------------------------------------------------------
*  Inputs         | ucIndex       : CMC device index
*-----------------|-----------------------------------------------------------
*  Outputs        | CMC device address
******************************************************************************
*/
uchar getCMCDevAddr ( uchar ucIndex )
{
    return (ROM_READ_BYTE ( &(cmc_device_addresses[ucIndex]) ) );
}

#endif  /* #ifdef CMC */

/*
******************************************************************************
*  Function Name  | getNumCmDev
*-----------------|-----------------------------------------------------------
*  Project        | CryptoMemory
*-----------------|-----------------------------------------------------------
*  Created        | October 11, 2007
*-----------------|-----------------------------------------------------------
*  Description    | This routine returns number of CM device
*                 | index
*-----------------|-----------------------------------------------------------
*  Inputs         | none
*-----------------|-----------------------------------------------------------
*  Outputs        | uchar   : number of CM device.
******************************************************************************
*/
uchar getNumCmDev ( void )
{
    return CM_NUM_DEVICES;
}
