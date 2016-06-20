// -----------------------------------------------------------------------------
// Struct/Union Types and define
// -----------------------------------------------------------------------------

#include 	<basetypes.h>


#ifndef I2C_SLAVE
#define I2C_SLAVE			0x0703
#endif
#ifndef I2C_TENBIT
#define I2C_TENBIT			0x0704
#endif
#define	ROMDATA				const

//typedef unsigned char		BYTE;


enum Video
	{
	VDCNV_4CH_ON_PAL=0,
	VDCNV_4CH_ON_NTSC,
	VDCNV_4CH_ON_NTSC_960,
	};

int RN6264_SetVideoSource(u8 mode);
int RN6264_RegWrite(u8 reg,u8 val);
int RN6264_I2c_write(u8 reg,u8 val);
int RN6264_I2c_read(u8 reg,u8 *val);
