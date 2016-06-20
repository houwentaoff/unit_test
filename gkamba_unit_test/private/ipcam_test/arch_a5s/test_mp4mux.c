#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <assert.h>
#include <basetypes.h>
#include "ambas_common.h"
#include "iav_drv.h"


typedef struct format_s {
	int     encode_type;
	int     width;
	int     height;
} format_t;

typedef struct h264_info_s {
	int	version;
	u32	frame_cnt;
	u32	size;
	format_t format;
	iav_h264_config_ex_t h264_config;
} h264_info_t;

typedef struct video_frame_s {
	u32     size;
	u32     pts;
	u32     pic_type;
	u32     reserved;
} video_frame_t;

typedef struct mux_config_s
{
	//====video encoder attributes
	//_ar_x:_ar_y is aspect ratio
	u16 _ar_x;                   //16
	u16 _ar_y;                   //9
	u8 _mode;                    //1:frame encoding, 2:field encoding
	u8 _M;                       //=4, every M frames has one I frame or P frame
	u8 _N;                       //=32, every N frames has one I frame, 1 GOP is made of N frames
	u8 _advanced;                //=0, xxxyyyyy, xxx:level, yyyyy:GOP level, see DSP pg18
	u32 _idr_interval;          //=4, one idr frame per idr_interval GOPs
	//here scale and rate meaning is same as DV, just opposite to iav_drv in ipcam
	u32 _scale;                 //fps=rate/scale, fps got from (video_enc_info.vout_frame_rate&0x7F)
	u32 _rate;                  //=180000
	u32 _brate;
	u32 _brate_min;
	u32 _Width;                 //=1280
	u32 _Height;                //=720
	u32 _new_gop;
	//====audio encoder attributes
	u32 _SampleFreq;            //=48000, 44.1K 48K
	u16 _Channels;               //=2
	u16 _SampleSize;             //=16
	//====general attributes
	u32 _clk;                   //=90000, system clock
	u8 _FileName[256];           //="/mnt/test.mp4"

} mux_config_t;


#define _IDX_SIZE 256 //keep it equal 2^N, 8.5 seconds of video@30fps, or 5.3 seconds of audio@48Khz
#define _SPS_SIZE 64
#define _PPS_SIZE 64

#define _SINK_BUF_SIZE    32000 //==0x7d00
#define PTS_UNIT 90000

int IdxWriter();
void put_u8(u32 data);
void put_be16(u32 data);
void put_be24(u32 data);
void put_be32(u32 data);
void put_be64(u64 data);
void put_buffer(u8 *pdata, u32 size);
u32 get_byte();
u32 get_be16();
u32 get_be32();
void get_buffer(u8 *pdata, u32 size);
u32 le_to_be32(u32 data);
u64 le_to_be64(u64 data);
void put_mp4File(u32 FreeSpaceSize);
void put_FileTypeBox();
void put_MovieBox();
void put_videoTrackBox(u32 TrackId, u32 Duration);
void put_VideoMediaBox(u32 Duration);
void put_VideoMediaInformationBox();
void put_AudioTrackBox(u32 TrackId, u32 Duration);
void put_AudioMediaBox();
void put_AudioMediaInformationBox();
void put_FreeSpaceBox(u32 FreeSpaceSize);

u32 GetSpsPps(u8* pBuffer, u32 offset);
u32 GetSeiLen(u8* pBuffer, u32 offset);
u32 GetAuDelimiter(u8 *pBuffer, u32 offset);
void InitSdtp();
u32 GetAacInfo(u32 samplefreq, u32 channel);


FILE *_pSink;        //mp4 file writer
FILE *_pSinkIdx;     //(tmp) index data writer

u64 _curPos;                 //next file written position for mp4 file writer

//mp4 file information
mux_config_t _Config;
u32 _CreationTime, _ModificationTime;

u16 _PixelAspectRatioWidth;  //0
u16 _PixelAspectRatioHeight; //0
u8 _sps[_SPS_SIZE];
u32 _spsSize, _spsPad;      //_spsSize include _spsPad padding bytes to align to 4 bytes, but exclude the 0x00000001 start code
u8 _pps[_PPS_SIZE];
u32 _ppsSize, _ppsPad;      //_ppsSize include _ppsPad padding bytes to align to 4 bytes, but exclude the 0x00000001 start code
u32 _FreeSpaceSize;         //free space size between ftyp atom and mdat atom (include free atom header 8 bytes)
    //xxx_SIZE macros need these 3 member
u32 _VPacketCnt;            //current video frame count
u32 _APacketCnt;            //current audio frame count
u32 _IdrPacketCnt;          //current IDR frame count
u32 _SpsPps_pos;
//for sdtp atom
u8 _lv_idc[32];              //[0]=last level, [1.._M+1], map frame to level idc


//xxx_MAXSIZE macros need these 3 member
u32 _MaxVPacketCnt;         //max video frames due to index space limit
u32 _MaxAPacketCnt;         //max audio frames due to index space limit
u32 _MaxIdrPacketCnt;       //max idr video frames
u32 _v_delta;               //time count of one video frame
u32 _a_delta;               //time count fo one audio frame
u64 _vdts;                   //video decoding time stamp, ==_v_delta*_VPacketCnt
u64 _adts;                   //audio decoding time stamp, ==_a_delta*_APacketCnt

u32 _frame_field_set;       //0:init state, 1:frame mode, 2:field mode

//index management
u32  _v_idx, _a_idx;
//the following tmp buffers are located just before the mdat atom in the final mp4 file
u32 _vctts[_IDX_SIZE*2+128*2];     //ctts.sample_count+sample_delta for video
u32 _vctts_fpos, _vctts_cur_fpos;            //file pos of the tmp file buffer for vctts
u32 _vstsz[_IDX_SIZE*1+128*1];     //stsz.entry_size for video
u32 _vstsz_fpos, _vstsz_cur_fpos;            //file pos of the tmp file buffer for vctts
u32 _vstco[_IDX_SIZE*1+128*1];     //stco.chunk_offset for video
u32 _vstco_fpos, _vstco_cur_fpos;            //file pos of the tmp file buffer for vctts
u32 _astsz[_IDX_SIZE*1+256*1];     //stsz.entry_size for audio
u32 _astsz_fpos, _astsz_cur_fpos;            //file pos of the tmp file buffer for vctts
u32 _astco[_IDX_SIZE*1+256*1];     //stco.chunk_offset for audio
u32 _astco_fpos, _astco_cur_fpos;            //file pos of the tmp file buffer for vctts

u64 _average_ctts;

int Init(iav_h264_config_ex_t* h264_config, char* mp4_filename, h264_info_t* h264_info);
int Stop();
int putRawData(u8* pBuffer, video_frame_t video_frame);

#define FOURCC(a,b,c,d)     ((a<<24)|(b<<16)|(c<<8)|(d))
#define CLOCK               90000

//--------------------------------------------------
u8  GetByte(u8 *pBuffer,u32 pos)
{
	return pBuffer[pos];
}


void put_byte(u32 data)
{
	u8 w[1];
	w[0] = data;      //(data&0xFF);
	fwrite((u8 *)w, sizeof(u8),1, _pSink);
	_curPos++;
}

void put_be16(u32 data)
{
	u8 w[2];
	w[1] = data;      //(data&0x00FF);
	w[0] = data>>8;   //(data&0xFF00)>>8;
	fwrite((u8 *)w, sizeof(u8),2, _pSink);
	_curPos += 2;
}

void put_be24(u32 data)
{
	u8 w[3];
	w[2] = data;     //(data&0x0000FF);
	w[1] = data>>8;  //(data&0x00FF00)>>8;
	w[0] = data>>16; //(data&0xFF0000)>>16;
	fwrite((u8 *)w, sizeof(u8),3, _pSink);
	_curPos += 3;
}

void put_be32(u32 data)
{
	u8 w[4];
	w[3] = data;     //(data&0x000000FF);
	w[2] = data>>8;  //(data&0x0000FF00)>>8;
	w[1] = data>>16; //(data&0x00FF0000)>>16;
	w[0] = data>>24; //(data&0xFF000000)>>24;
	fwrite((u8 *)w, sizeof(u8),4, _pSink);
	_curPos += 4;
}

void put_be64(u64 data)
{
	u8 w[8];
	w[7] = data;     //(data&000000000x000000FFULL);
	w[6] = data>>8;  //(data&0x000000000000FF00ULL)>>8;
	w[5] = data>>16; //(data&0x0000000000FF0000ULL)>>16;
	w[4] = data>>24; //(data&0x00000000FF000000ULL)>>24;
	w[3] = data>>32; //(data&0x000000FF00000000ULL)>>32;
	w[2] = data>>40; //(data&0x0000FF0000000000ULL)>>40;
	w[1] = data>>48; //(data&0x00FF000000000000ULL)>>48;
	w[0] = data>>56; //(data&0xFF00000000000000ULL)>>56;
	fwrite((u8 *)w, sizeof(u8),8, _pSink);
	_curPos += 8;
}

void put_buffer(u8* pdata, u32 size)
{
	fwrite(pdata, sizeof(u8), size, _pSink);
	_curPos += size;
}

u32 get_byte()
{
    u8 data[1];
    fread((void *)data,sizeof(u8),1,_pSink);
    return data[0];
}

u32 get_be16()
{
	u8 data[2];
	fread((void *)data,sizeof(u8),2,_pSink);
	return (data[1] | (data[0]<<8));
}

u32 get_be32()
{
	u8 data[4];
	fread((void *)data,sizeof(u8),4,_pSink);
	return (data[3] | (data[2]<<8) | (data[1]<<16) | (data[0]<<24));
}

void get_buffer(u8 *pdata, u32 size)
{
	fread((void *)pdata,sizeof(u8),size,_pSink);
}

u32 le_to_be32(u32 data)
{
	u32 rval;
	rval  = (data&0x000000FF)<<24;
	rval += (data&0x0000FF00)<<8;
	rval += (data&0x00FF0000)>>8;
	rval += (data&0xFF000000)>>24;
	return rval;
}

u64 le_to_be64(u64 data)
{
	u64 rval = 0ULL;
	u8 tmp;
	tmp = data;     rval += (u64)tmp<<56;
	tmp = data>>8;  rval += (u64)tmp<<48;
	tmp = data>>16; rval += (u64)tmp<<40;
	tmp = data>>24; rval += (u64)tmp<<32;
	tmp = data>>32; rval += tmp<<24;
	tmp = data>>40; rval += tmp<<16;
	tmp = data>>48; rval += tmp<<8;
	tmp = data>>56; rval += tmp;
	return rval;
}


//--------------------------------------------------

#define VideoMediaHeaderBox_SIZE            20
#define DataReferenceBox_SIZE               28
#define DataInformationBox_SIZE             (8+DataReferenceBox_SIZE) // 36
#define PixelAspectRatioBox_SIZE            0//16
#define CleanApertureBox_SIZE               0//40
#define AvcConfigurationBox_SIZE            (19+_spsSize+_ppsSize)
#define BitrateBox_SIZE                     20
#define VisualSampleEntry_SIZE              (86+PixelAspectRatioBox_SIZE+CleanApertureBox_SIZE+AvcConfigurationBox_SIZE+BitrateBox_SIZE) // 181+_spsSize+_ppsSize
#define VideoSampleDescriptionBox_SIZE      (16+VisualSampleEntry_SIZE) // 197+_spsSize+_ppsSize
#define DecodingTimeToSampleBox_SIZE        24
#define CompositionTimeToSampleBox_SIZE     (16+(_VPacketCnt<<3))
#define CompositionTimeToSampleBox_MAXSIZE  (16+(_MaxVPacketCnt<<3))
#define SampleToChunkBox_SIZE               28
#define VideoSampleSizeBox_SIZE             (20+(_VPacketCnt<<2))
#define VideoSampleSizeBox_MAXSIZE          (20+(_MaxVPacketCnt<<2))
#define VideoChunkOffsetBox_SIZE64          (16+(_VPacketCnt<<3))
#define VideoChunkOffsetBox_MAXSIZE64       (16+(_MaxVPacketCnt<<3))
#define VideoChunkOffsetBox_SIZE            (16+(_VPacketCnt<<2))
#define VideoChunkOffsetBox_MAXSIZE         (16+(_MaxVPacketCnt<<2))
#define SyncSampleBox_SIZE                  (16+(_IdrPacketCnt<<2))
#define SyncSampleBox_MAXSIZE               (16+(_MaxIdrPacketCnt<<2))
#define SampleDependencyTypeBox_SIZE        (12+_VPacketCnt)
#define SampleDependencyTypeBox_MAXSIZE     (12+_MaxVPacketCnt)
#define VideoSampleTableBox_SIZE64          (8+VideoSampleDescriptionBox_SIZE+\
	                                         DecodingTimeToSampleBox_SIZE+\
	                                         CompositionTimeToSampleBox_SIZE+\
	                                         SampleToChunkBox_SIZE+\
	                                         VideoSampleSizeBox_SIZE+\
	                                         VideoChunkOffsetBox_SIZE64+\
	                                         SyncSampleBox_SIZE+\
	                                         SampleDependencyTypeBox_SIZE) // 337+_spsSize+_ppsSize+V*21+IDR*4
#define VideoSampleTableBox_MAXSIZE64       (8+VideoSampleDescriptionBox_SIZE+\
	                                         DecodingTimeToSampleBox_SIZE+\
	                                         CompositionTimeToSampleBox_MAXSIZE+\
	                                         SampleToChunkBox_SIZE+\
	                                         VideoSampleSizeBox_MAXSIZE+\
	                                         VideoChunkOffsetBox_MAXSIZE64+\
	                                         SyncSampleBox_MAXSIZE+\
	                                         SampleDependencyTypeBox_MAXSIZE) // 337+_spsSize+_ppsSize+V*17+IDR*4
#define VideoSampleTableBox_SIZE            (8+VideoSampleDescriptionBox_SIZE+\
	                                         DecodingTimeToSampleBox_SIZE+\
	                                         CompositionTimeToSampleBox_SIZE+\
	                                         SampleToChunkBox_SIZE+\
	                                         VideoSampleSizeBox_SIZE+\
	                                         VideoChunkOffsetBox_SIZE+\
	                                         SyncSampleBox_SIZE+\
	                                         SampleDependencyTypeBox_SIZE) // 337+_spsSize+_ppsSize+V*17+IDR*4
#define VideoSampleTableBox_MAXSIZE         (8+VideoSampleDescriptionBox_SIZE+\
	                                         DecodingTimeToSampleBox_SIZE+\
	                                         CompositionTimeToSampleBox_MAXSIZE+\
	                                         SampleToChunkBox_SIZE+\
	                                         VideoSampleSizeBox_MAXSIZE+\
	                                         VideoChunkOffsetBox_MAXSIZE+\
	                                         SyncSampleBox_MAXSIZE+\
	                                         SampleDependencyTypeBox_MAXSIZE) // 337+_spsSize+_ppsSize+V*17+IDR*4
#define VideoMediaInformationBox_SIZE64     (8+VideoMediaHeaderBox_SIZE+\
                                             DataInformationBox_SIZE+\
                                             VideoSampleTableBox_SIZE64) // 401+_spsSize+_ppsSize+V*21+IDR*4
#define VideoMediaInformationBox_MAXSIZE64  (8+VideoMediaHeaderBox_SIZE+\
                                             DataInformationBox_SIZE+\
                                             VideoSampleTableBox_MAXSIZE64) // 401+_spsSize+_ppsSize+V*21+IDR*4
#define VideoMediaInformationBox_SIZE       (8+VideoMediaHeaderBox_SIZE+\
                                             DataInformationBox_SIZE+\
                                             VideoSampleTableBox_SIZE) // 401+_spsSize+_ppsSize+V*17+IDR*4
#define VideoMediaInformationBox_MAXSIZE    (8+VideoMediaHeaderBox_SIZE+\
                                             DataInformationBox_SIZE+\
                                             VideoSampleTableBox_MAXSIZE) // 401+_spsSize+_ppsSize+V*17+IDR*4
void put_VideoMediaInformationBox()
{
	u32 i, j, k, idr_frm_no;
	//MediaInformationBox
	put_be32(VideoMediaInformationBox_SIZE);     //uint32 size
	put_be32(FOURCC('m', 'i', 'n', 'f'));        //'minf'
		//VideoMediaHeaderBox
		put_be32(VideoMediaHeaderBox_SIZE);          //uint32 size
		put_be32(FOURCC('v', 'm', 'h', 'd'));        //'vmhd'
		put_byte(0);                                 //uint8 version
		//This is a compatibility flag that allows QuickTime to distinguish between movies created with QuickTime
		//1.0 and newer movies. You should always set this flag to 1, unless you are creating a movie intended
		//for playback using version 1.0 of QuickTime
		put_be24(1);                                 //bits24 flags
		put_be16(0);                                 //uint16 graphicsmode  //0=copy over the existing image
		put_be16(0);                                 //uint16 opcolor[3]      //(red, green, blue)
		put_be16(0);
		put_be16(0);

		//DataInformationBox
		put_be32(DataInformationBox_SIZE);           //uint32 size
		put_be32(FOURCC('d', 'i', 'n', 'f'));        //'dinf'
			//DataReferenceBox
			put_be32(DataReferenceBox_SIZE);             //uint32 size
			put_be32(FOURCC('d', 'r', 'e', 'f'));        //'dref'
			put_byte(0);                                 //uint8 version
			put_be24(0);                                 //bits24 flags
			put_be32(1);                                 //uint32 entry_count
			put_be32(12);                                //uint32 size
			put_be32(FOURCC('u', 'r', 'l', ' '));        //'url '
			put_byte(0);                                 //uint8 version
			put_be24(1);                                 //bits24 flags    //1=media data is in the same file as the MediaBox

		//SampleTableBox
		put_be32(VideoSampleTableBox_SIZE);          //uint32 size
		put_be32(FOURCC('s', 't', 'b', 'l'));        //'stbl'

			//SampleDescriptionBox
			put_be32(VideoSampleDescriptionBox_SIZE);    //uint32 size
			put_be32(FOURCC('s', 't', 's', 'd'));        //uint32 type
			put_byte(0);                                 //uint8 version
			put_be24(0);                                 //bits24 flags
			put_be32(1);                                 //uint32 entry_count
				//VisualSampleEntry
				u8 EncoderName[31]="AVC Coding";
				put_be32(VisualSampleEntry_SIZE);            //uint32 size
				put_be32(FOURCC('a', 'v', 'c', '1'));        //'avc1'
				put_byte(0);                                 //uint8 reserved[6]
				put_byte(0);
				put_byte(0);
				put_byte(0);
				put_byte(0);
				put_byte(0);
				put_be16(1);                                 //uint16 data_reference_index
				put_be16(0);                                 //uint16 pre_defined
				put_be16(0);                                 //uint16 reserved
				put_be32(0);                                 //uint32 pre_defined[3]
				put_be32(0);
				 put_be32(0);
				put_be16(_Config._Width);                    //uint16 width
				put_be16(_Config._Height);                   //uint16 height
				put_be32(0x00480000);                        //uint32 horizresolution  72dpi
				put_be32(0x00480000);                        //uint32 vertresolution   72dpi
				put_be32(0);                                 //uint32 reserved
				put_be16(1);                                 //uint16 frame_count
				u32 len = (u32)strlen((const char *)EncoderName);
				put_byte(len);                               //char compressor_name[32]   //[0] is actual length
				put_buffer(EncoderName, 31);
				put_be16(0x0018);                            //uint16 depth   //0x0018=images are in colour with no alpha
				put_be16(-1);                                //int16 pre_defined
					//AvcConfigurationBox
					put_be32(AvcConfigurationBox_SIZE);          //uint32 size
					put_be32(FOURCC('a', 'v', 'c', 'C'));        //'avcC'
					put_byte(1);                                 //uint8 configurationVersion
					put_byte(_sps[1]);                           //uint8 AVCProfileIndication
					put_byte(_sps[2]);                          //uint8 profile_compatibility
					put_byte(_sps[3]);                           //uint8 level
					put_byte(0xFF);                              //uint8 nal_length  //(nal_length&0x03)+1 [reserved:6, lengthSizeMinusOne:2]
					put_byte(0xE1);                              //uint8 sps_count  //sps_count&0x1f [reserved:3, numOfSequenceParameterSets:5]
					put_be16(_spsSize);                          //uint16 sps_size      //sequenceParameterSetLength
					put_buffer(_sps, _spsSize);                  //uint8 sps[sps_size] //sequenceParameterSetNALUnit
					put_byte(1);                                 //uint8 pps_count      //umOfPictureParameterSets
					put_be16(_ppsSize);                          //uint16 pps_size      //pictureParameterSetLength
					put_buffer(_pps, _ppsSize);                  //uint8 pps[pps_size] //pictureParameterSetNALUnit
					//BitrateBox
					put_be32(BitrateBox_SIZE);                   //uint32 size
					put_be32(FOURCC('b', 't', 'r', 't'));        //'btrt'
					put_be32(0);                                 //uint32 buffer_size
					put_be32(0);                                 //uint32 max_bitrate
					put_be32(0);                                 //uint32 avg_bitrate

			//DecodingTimeToSampleBox
			put_be32(DecodingTimeToSampleBox_SIZE);      //uint32 size
			put_be32(FOURCC('s', 't', 't', 's'));        //'stts'
			put_byte(0);                                 //uint8 version
			put_be24(0);                                 //bits24 flags
			put_be32(1);                                 //uint32 entry_count
			put_be32(_VPacketCnt);                       //uint32 sample_count
			u32 delta = (u32)(1.0*_frame_field_set*_Config._scale*_Config._clk/_Config._rate);
			if(_VPacketCnt != 0)
			{
				delta= (u32)(_average_ctts / _VPacketCnt);
			}
			put_be32(delta);                             //uint32 sample_delta

			//CompositionTimeToSampleBox
			put_be32(CompositionTimeToSampleBox_SIZE);   //uint32 size
			put_be32(FOURCC('c', 't', 't', 's'));        //'ctts'
			put_byte(0);                                 //uint8 version
			put_be24(0);                                 //bits24 flags
			put_be32(_VPacketCnt);                       //uint32 entry_count
			if (_VPacketCnt > 0) fseek(_pSinkIdx, _vctts_fpos, SEEK_SET);
			for (i=0; i<_VPacketCnt/_IDX_SIZE; i++)      //uint32 sample_count + uint32 sample_delta
			{
				fread((u8 *)_vctts,sizeof(u32), _IDX_SIZE*2, _pSinkIdx);
				put_buffer((u8 *)_vctts, _IDX_SIZE*2*sizeof(u32));
			}
			if ((j=_VPacketCnt%_IDX_SIZE) > 0)
			{
				fread((u8 *)_vctts, sizeof(u32),j*2, _pSinkIdx);
				put_buffer((u8 *)_vctts, j*2*sizeof(u32));
			}

			//SampleToChunkBox
			put_be32(SampleToChunkBox_SIZE);             //uint32 size
			put_be32(FOURCC('s', 't', 's', 'c'));        //'stsc'
			put_byte(0);                                 //uint8 version
			put_be24(0);                                 //bits24 flags
			put_be32(1);                                 //uint32 entry_count
			put_be32(1);                                 //uint32 first_chunk
			put_be32(1);                                 //uint32 samples_per_chunk
			put_be32(1);                                 //uint32 sample_description_index

			//SampleSizeBox
			put_be32(VideoSampleSizeBox_SIZE);           //uint32 size
			put_be32(FOURCC('s', 't', 's', 'z'));        //'stsz'
			put_byte(0);                                 //uint8 version
			put_be24(0);                                 //bits24 flags
			put_be32(0);                                 //uint32 sample_size
			put_be32(_VPacketCnt);                       //uint32 sample_count
			if (_VPacketCnt > 0) fseek(_pSinkIdx, _vstsz_fpos, SEEK_SET);
			for (i=0; i<_VPacketCnt/_IDX_SIZE; i++)      //uint32 entry_size[sample_count] (if sample_size==0)
			{
				fread((u8 *)_vstsz, sizeof(u32),_IDX_SIZE,_pSinkIdx);
				put_buffer((u8 *)_vstsz, _IDX_SIZE*sizeof(u32));
			}
			if ((j=_VPacketCnt%_IDX_SIZE) > 0)
			{
				fread((u8 *)_vstsz, sizeof(u32),j,_pSinkIdx);
				put_buffer((u8 *)_vstsz, j*sizeof(32));
			}

			//ChunkOffsetBox
			put_be32(VideoChunkOffsetBox_SIZE);          //uint32 size
			put_be32(FOURCC('s', 't', 'c', 'o'));        //'stco'
			put_byte(0);                                 //uint8 version
			put_be24(0);                                 //bits24 flags
			put_be32(_VPacketCnt);                       //uint32 entry_count
			if (_VPacketCnt > 0) fseek(_pSinkIdx,_vstco_fpos, SEEK_SET);
			for (i=0; i<_VPacketCnt/_IDX_SIZE; i++)      //uint32 chunk_offset[entry_count]
			{
				fread((u8 *)_vstco, sizeof(u32),_IDX_SIZE,_pSinkIdx);
				put_buffer((u8 *)_vstco, _IDX_SIZE*sizeof(u32));
			}
			if ((j=_VPacketCnt%_IDX_SIZE) > 0)
			{
				fread((u8 *)_vstco, sizeof(u32),j,_pSinkIdx);
				put_buffer((u8 *)_vstco, j*sizeof(u32));
			}

			//SyncSampleBox
			put_be32(SyncSampleBox_SIZE);                //uint32 size
			put_be32(FOURCC('s', 't', 's', 's'));        //'stss'
			put_byte(0);                                 //uint8 version
			put_be24(0);                                 //bits24 flags
			put_be32(_IdrPacketCnt);                     //uint32 entry_count
			if (_IdrPacketCnt > 0)                       //uint32 sample_number[entry_count]
			{
				idr_frm_no = 1;
				put_be32(idr_frm_no);                    //1st idr
			}
			if (_Config._new_gop == 1)
			{
				idr_frm_no += _Config._idr_interval*_Config._N-(_Config._M-1);
				if (_IdrPacketCnt > 1)
				{
					put_be32(idr_frm_no);                //2nd idr
				}
				k = 2;
			}
			else
			{
				k = 1;
			}
			for (i=k; i<_IdrPacketCnt; i++)
			{
				idr_frm_no += _Config._idr_interval*_Config._N;
				put_be32(idr_frm_no);
			}

			//SampleDependencyTypeBox
			put_be32(SampleDependencyTypeBox_SIZE);      //uint32 size
			put_be32(FOURCC('s', 'd', 't', 'p'));        //'sdtp'
			put_byte(0);                                 //uint8 version
			put_be24(0);                                //bits24 flags
			//SampleSizeBox.sample_count entries
			if (_VPacketCnt > 0) put_byte(0x00);         //first entry
			for (i=1; i<_VPacketCnt; i++)
			{
				if (_Config._new_gop == 1)
				{
					k = i;
				}
				else
				{
					k = i+1;
				}
				k = k%_Config._M; if (k==0) k=_Config._M;
				if (_lv_idc[k] == _lv_idc[0])
				{
					put_byte(0x08);//no other sample depends on this one (disposable)
				}
				else
				{
					put_byte(0x00);
				}
			 }
}

#define ElementaryStreamDescriptorBox_SIZE  50
#define AudioSampleEntry_SIZE               (36+ElementaryStreamDescriptorBox_SIZE) // 86
#define AudioSampleDescriptionBox_SIZE      (16+AudioSampleEntry_SIZE) // 102
#define AudioSampleSizeBox_SIZE             (20+(_APacketCnt<<2))
#define AudioSampleSizeBox_MAXSIZE          (20+(_MaxAPacketCnt<<2))
#define AudioChunkOffsetBox_SIZE64          (16+(_APacketCnt<<3))
#define AudioChunkOffsetBox_MAXSIZE64       (16+(_MaxAPacketCnt<<3))
#define AudioChunkOffsetBox_SIZE            (16+(_APacketCnt<<2))
#define AudioChunkOffsetBox_MAXSIZE         (16+(_MaxAPacketCnt<<2))
#define AudioSampleTableBox_SIZE64          (8+AudioSampleDescriptionBox_SIZE+\
                                             DecodingTimeToSampleBox_SIZE+\
                                             SampleToChunkBox_SIZE+\
                                             AudioSampleSizeBox_SIZE+\
                                             AudioChunkOffsetBox_SIZE64) // 198+A*12
#define AudioSampleTableBox_MAXSIZE64       (8+AudioSampleDescriptionBox_SIZE+\
                                             DecodingTimeToSampleBox_SIZE+\
                                             SampleToChunkBox_SIZE+\
                                             AudioSampleSizeBox_MAXSIZE+\
                                             AudioChunkOffsetBox_MAXSIZE64) // 198+A*12
#define AudioSampleTableBox_SIZE            (8+AudioSampleDescriptionBox_SIZE+\
                                             DecodingTimeToSampleBox_SIZE+\
                                             SampleToChunkBox_SIZE+\
                                             AudioSampleSizeBox_SIZE+\
                                             AudioChunkOffsetBox_SIZE) // 198+A*8
#define AudioSampleTableBox_MAXSIZE         (8+AudioSampleDescriptionBox_SIZE+\
                                             DecodingTimeToSampleBox_SIZE+\
                                             SampleToChunkBox_SIZE+\
                                             AudioSampleSizeBox_MAXSIZE+\
                                             AudioChunkOffsetBox_MAXSIZE) // 198+A*8
#define SoundMediaHeaderBox_SIZE            16
#define AudioMediaInformationBox_SIZE64     (8+SoundMediaHeaderBox_SIZE+\
                                             DataInformationBox_SIZE+\
                                             AudioSampleTableBox_SIZE64) // 258+A*12
#define AudioMediaInformationBox_MAXSIZE64  (8+SoundMediaHeaderBox_SIZE+\
                                             DataInformationBox_SIZE+\
                                             AudioSampleTableBox_MAXSIZE64) // 258+A*12
#define AudioMediaInformationBox_SIZE       (8+SoundMediaHeaderBox_SIZE+\
                                             DataInformationBox_SIZE+\
                                             AudioSampleTableBox_SIZE) // 258+A*8
#define AudioMediaInformationBox_MAXSIZE    (8+SoundMediaHeaderBox_SIZE+\
                                             DataInformationBox_SIZE+\
                                             AudioSampleTableBox_MAXSIZE) // 258+A*8
/*void put_AudioMediaInformationBox()
{
    AM_UINT i, j;
    //MediaInformationBox
    if (_largeFile == true)
    {
    put_be32(AudioMediaInformationBox_SIZE64);   //uint32 size
    }
    else
    {
    put_be32(AudioMediaInformationBox_SIZE);     //uint32 size
    }
    put_be32(FOURCC('m', 'i', 'n', 'f'));        //'minf'

        //SoundMediaHeaderBox
        put_be32(SoundMediaHeaderBox_SIZE);          //uint32 size
        put_be32(FOURCC('s', 'm', 'h', 'd'));        //'smhd'
        put_byte(0);                                 //uint8 version
        put_be24(0);                                 //bits24 flags
        put_be16(0);                                 //int16 balance
        put_be16(0);                                 //uint16 reserved

        //DataInformationBox
        put_be32(DataInformationBox_SIZE);           //uint32 size
        put_be32(FOURCC('d', 'i', 'n', 'f'));        //'dinf'
            //DataReferenceBox
            put_be32(DataReferenceBox_SIZE);             //uint32 size
            put_be32(FOURCC('d', 'r', 'e', 'f'));        //'dref'
            put_byte(0);                                 //uint8 version
            put_be24(0);                                 //bits24 flags
            put_be32(1);                                 //uint32 entry_count
            put_be32(12);                                //uint32 size
            put_be32(FOURCC('u', 'r', 'l', ' '));        //'url '
            put_byte(0);                                 //uint8 version
            put_be24(1);                                 //bits24 flags    //1=media data is in the same file as the MediaBox

        //SampleTableBox
        if (_largeFile == true)
        {
        put_be32(AudioSampleTableBox_SIZE64);        //uint32 size
        }
        else
        {
        put_be32(AudioSampleTableBox_SIZE);          //uint32 size
        }
        put_be32(FOURCC('s', 't', 'b', 'l'));        //'stbl'

            //SampleDescriptionBox
            put_be32(AudioSampleDescriptionBox_SIZE);    //uint32 size
            put_be32(FOURCC('s', 't', 's', 'd'));        //uint32 type
            put_byte(0);                                 //uint8 version
            put_be24(0);                                 //bits24 flags
            put_be32(1);                                 //uint32 entry_count
                //AudioSampleEntry
                put_be32(AudioSampleEntry_SIZE);            //uint32 size
                put_be32(FOURCC('m', 'p', '4', 'a'));        //'mp4a'
                put_byte(0);                                 //uint8 reserved[6]
                put_byte(0);
                put_byte(0);
                put_byte(0);
                put_byte(0);
                put_byte(0);
                put_be16(1);                                 //uint16 data_reference_index
                put_be32(0);                                 //uint32 reserved[2]
                put_be32(0);
                put_be16(_Config._Channels);                 //uint16 channelcount
                put_be16(_Config._SampleSize);               //uint16 samplesize
                put_be16(0xfffe);                            //uint16 pre_defined   //for QT sound
                put_be16(0);                                 //uint16 reserved
                put_be32(_Config._SampleFreq<<16);           //uint32 samplerate   //= (timescale of media << 16)
                    //ElementaryStreamDescriptorBox
                    put_be32(ElementaryStreamDescriptorBox_SIZE);//uint32 size
                    put_be32(FOURCC('e', 's', 'd', 's'));        //'esds'
                    put_byte(0);                                 //uint8 version
                    put_be24(0);                                 //bits24 flags
                    //ES descriptor takes 38 bytes
                    put_byte(3);                                 //ES descriptor type tag
                    put_be16(0x8080);
                    put_byte(34);                                //descriptor type length
                    put_be16(0);                                 //ES ID
                    put_byte(0);                                 //stream priority
                    //Decoder config descriptor takes 26 bytes (include decoder specific info)
                    put_byte(4);                                 //decoder config descriptor type tag
                    put_be16(0x8080);
                    put_byte(22);                                //descriptor type length
                    put_byte(0x40);                              //object type ID MPEG-4 audio=64 AAC
                    put_byte(0x15);                              //stream type:6, upstream flag:1, reserved flag:1 (audio=5)    Audio stream
                    put_be24(8192);                              // buffer size
                    put_be32(128000);                            // max bitrate
                    put_be32(128000);                            // avg bitrate
                    //Decoder specific info descriptor takes 9 bytes
                    put_byte(5);                                 //decoder specific descriptor type tag
                    put_be16(0x8080);
                    put_byte(5);                                 //descriptor type length
                    put_be16(GetAacInfo(_Config._SampleFreq, _Config._Channels));
                    put_be16(0x0000);
                    put_byte(0x00);
                    //SL descriptor takes 5 bytes
                    put_byte(6);                                 //SL config descriptor type tag
                    put_be16(0x8080);
                    put_byte(1);                                 //descriptor type length
                    put_byte(2);                                 //SL value

            //DecodingTimeToSampleBox
            put_be32(DecodingTimeToSampleBox_SIZE);      //uint32 size
            put_be32(FOURCC('s', 't', 't', 's'));        //'stts'
            put_byte(0);                                 //uint8 version
            put_be24(0);                                 //bits24 flags
            put_be32(1);                                 //uint32 entry_count
            put_be32(_APacketCnt);                       //uint32 sample_count
            put_be32(1024);                              //uint32 sample_delta

            //SampleToChunkBox
            put_be32(SampleToChunkBox_SIZE);             //uint32 size
            put_be32(FOURCC('s', 't', 's', 'c'));        //'stsc'
            put_byte(0);                                 //uint8 version
            put_be24(0);                                 //bits24 flags
            put_be32(1);                                 //uint32 entry_count
            put_be32(1);                                 //uint32 first_chunk
            put_be32(1);                                 //uint32 samples_per_chunk
            put_be32(1);                                 //uint32 sample_description_index

            //SampleSizeBox
            put_be32(AudioSampleSizeBox_SIZE);           //uint32 size
            put_be32(FOURCC('s', 't', 's', 'z'));        //'stsz'
            put_byte(0);                                 //uint8 version
            put_be24(0);                                 //bits24 flags
            put_be32(0);                                 //uint32 sample_size
            put_be32(_APacketCnt);                       //uint32 sample_count
            if (_APacketCnt > 0) _pSinkIdx->Seek(_astsz_fpos, SEEK_SET);
            for (i=0; i<_APacketCnt/_IDX_SIZE; i++)      //uint32 entry_size[sample_count] (if sample_size==0)
            {
                _pSinkIdx->Read((AM_U8 *)_astsz, _IDX_SIZE*sizeof(AM_UINT));
                put_buffer((AM_U8 *)_astsz, _IDX_SIZE*sizeof(AM_UINT));
            }
            if ((j=_APacketCnt%_IDX_SIZE) > 0)
            {
                _pSinkIdx->Read((AM_U8 *)_astsz, j*sizeof(AM_UINT));
                put_buffer((AM_U8 *)_astsz, j*sizeof(AM_UINT));
            }

            //ChunkOffsetBox
            if (_largeFile == true)
            {
            put_be32(AudioChunkOffsetBox_SIZE64);        //uint32 size
            put_be32(FOURCC('c', 'o', '6', '4'));        //'co64'
            put_byte(0);                                 //uint8 version
            put_be24(0);                                 //bits24 flags
            put_be32(_APacketCnt);                       //uint32 entry_count
            if (_APacketCnt > 0) _pSinkIdx->Seek(_astco_fpos, SEEK_SET);
            for (i=0; i<_APacketCnt/_IDX_SIZE; i++)      //uint64 chunk_offset[entry_count]
            {
                _pSinkIdx->Read((AM_U8 *)_astco, _IDX_SIZE*sizeof(AM_U64));
                put_buffer((AM_U8 *)_astco, _IDX_SIZE*sizeof(AM_U64));
            }
            if ((j=_APacketCnt%_IDX_SIZE) > 0)
            {
                _pSinkIdx->Read((AM_U8 *)_astco, j*sizeof(AM_U64));
                put_buffer((AM_U8 *)_astco, j*sizeof(AM_U64));
            }
            }
            else
            {
            put_be32(AudioChunkOffsetBox_SIZE);          //uint32 size
            put_be32(FOURCC('s', 't', 'c', 'o'));        //'stco'
            put_byte(0);                                 //uint8 version
            put_be24(0);                                 //bits24 flags
            put_be32(_APacketCnt);                       //uint32 entry_count
            if (_APacketCnt > 0) _pSinkIdx->Seek(_astco_fpos, SEEK_SET);
            for (i=0; i<_APacketCnt/_IDX_SIZE; i++)      //uint32 chunk_offset[entry_count]
            {
                _pSinkIdx->Read((AM_U8 *)_astco, _IDX_SIZE*sizeof(AM_UINT));
                put_buffer((AM_U8 *)_astco, _IDX_SIZE*sizeof(AM_UINT));
            }
            if ((j=_APacketCnt%_IDX_SIZE) > 0)
            {
                _pSinkIdx->Read((AM_U8 *)_astco, j*sizeof(AM_UINT));
                put_buffer((AM_U8 *)_astco, j*sizeof(AM_UINT));
            }
            }
}
*/

//--------------------------------------------------

#define VIDEO_HANDLER_NAME             ((u8 *)"Ambarella AVC")
#define VIDEO_HANDLER_NAME_LEN         14 //strlen(VIDEO_HANDLER_NAME)+1
#define MediaHeaderBox_SIZE            32
#define VideoHandlerReferenceBox_SIZE  (32+VIDEO_HANDLER_NAME_LEN) // 46
#define VideoMediaBox_SIZE64           (8+MediaHeaderBox_SIZE+\
                                        VideoHandlerReferenceBox_SIZE+\
                                        VideoMediaInformationBox_SIZE64) // 487+_spsSize+_ppsSize+V*21+IDR*4
#define VideoMediaBox_MAXSIZE64        (8+MediaHeaderBox_SIZE+\
                                        VideoHandlerReferenceBox_SIZE+\
                                        VideoMediaInformationBox_MAXSIZE64) // 487+_spsSize+_ppsSize+V*21+IDR*4
#define VideoMediaBox_SIZE             (8+MediaHeaderBox_SIZE+\
                                        VideoHandlerReferenceBox_SIZE+\
                                        VideoMediaInformationBox_SIZE) // 487+_spsSize+_ppsSize+V*17+IDR*4
#define VideoMediaBox_MAXSIZE          (8+MediaHeaderBox_SIZE+\
                                        VideoHandlerReferenceBox_SIZE+\
                                        VideoMediaInformationBox_MAXSIZE) // 487+_spsSize+_ppsSize+V*17+IDR*4
void put_VideoMediaBox(u32 Duration)
{
    //MediaBox
    put_be32(VideoMediaBox_SIZE);               //uint32 size
    put_be32(FOURCC('m', 'd', 'i', 'a'));       //'mdia'

        //MediaHeaderBox
        put_be32(MediaHeaderBox_SIZE);              //uint32 size
        put_be32(FOURCC('m', 'd', 'h', 'd'));       //'mdhd'
        put_byte(0);                                //uint8 version
        put_be24(0);                                //bits24 flags
        put_be32(_CreationTime);                    //uint32 creation_time [version==0] uint64 creation_time [version==1]
        put_be32(_ModificationTime);                //uint32 modification_time [version==0] uint64 modification_time [version==1]
        put_be32(_Config._clk);                     //uint32 timescale
        put_be32(Duration);                         //uint32 duration [version==0] uint64 duration [version==1]
        put_be16(0);                                //bits5 language[3]  //ISO-639-2/T language code
        put_be16(0);                                //uint16 pre_defined

        //HandlerReferenceBox
        put_be32(VideoHandlerReferenceBox_SIZE);    //uint32 size
        put_be32(FOURCC('h', 'd', 'l', 'r'));       //'hdlr'
        put_byte(0);                                //uint8 version
        put_be24(0);                                //bits24 flags
        put_be32(0);                                //uint32 pre_defined
        put_be32(FOURCC('v', 'i', 'd', 'e'));       //'vide':video track
        put_be32(0);                                //uint32 reserved[3]
        put_be32(0);
        put_be32(0);
        put_byte(VIDEO_HANDLER_NAME_LEN);           //char name[], name[0] is actual length
        put_buffer(VIDEO_HANDLER_NAME, VIDEO_HANDLER_NAME_LEN-1);

        put_VideoMediaInformationBox();
}

#define AUDIO_HANDLER_NAME             ((u8 *)"Ambarella AAC")
#define AUDIO_HANDLER_NAME_LEN         14 //strlen(AUDIO_HANDLER_NAME)+1
#define AudioHandlerReferenceBox_SIZE  (32+AUDIO_HANDLER_NAME_LEN) // 46
#define AudioMediaBox_SIZE64           (8+MediaHeaderBox_SIZE+\
                                        AudioHandlerReferenceBox_SIZE+\
                                        AudioMediaInformationBox_SIZE64) // 344+A*12
#define AudioMediaBox_MAXSIZE64        (8+MediaHeaderBox_SIZE+\
                                        AudioHandlerReferenceBox_SIZE+\
                                        AudioMediaInformationBox_MAXSIZE64) // 344+A*12
#define AudioMediaBox_SIZE             (8+MediaHeaderBox_SIZE+\
                                        AudioHandlerReferenceBox_SIZE+\
                                        AudioMediaInformationBox_SIZE) // 344+A*8
#define AudioMediaBox_MAXSIZE          (8+MediaHeaderBox_SIZE+\
                                        AudioHandlerReferenceBox_SIZE+\
                                        AudioMediaInformationBox_MAXSIZE) // 344+A*8
/*
void CamIsoMuxer::put_AudioMediaBox()
{
    //MediaBox
    if (_largeFile == true)
    {
    put_be32(AudioMediaBox_SIZE64);             //uint32 size
    }
    else
    {
    put_be32(AudioMediaBox_SIZE);               //uint32 size
    }
    put_be32(FOURCC('m', 'd', 'i', 'a'));       //'mdia'

        //MediaHeaderBox
        put_be32(MediaHeaderBox_SIZE);              //uint32 size
        put_be32(FOURCC('m', 'd', 'h', 'd'));       //'mdhd'
        put_byte(0);                                //uint8 version
        put_be24(0);                                //bits24 flags
        put_be32(_CreationTime);                    //uint32 creation_time [version==0] uint64 creation_time [version==1]
        put_be32(_ModificationTime);                //uint32 modification_time [version==0] uint64 modification_time [version==1]
        put_be32(_Config._SampleFreq);              //uint32 timescale
        put_be32(1024*_APacketCnt);                 //uint32 duration [version==0] uint64 duration [version==1]
        put_be16(0);                                //bits5 language[3]  //ISO-639-2/T language code
        put_be16(0);                                //uint16 pre_defined

        //HandlerReferenceBox
        put_be32(AudioHandlerReferenceBox_SIZE);    //uint32 size
        put_be32(FOURCC('h', 'd', 'l', 'r'));       //'hdlr'
        put_byte(0);                                //uint8 version
        put_be24(0);                                //bits24 flags
        put_be32(0);                                //uint32 pre_defined
        put_be32(FOURCC('s', 'o', 'u', 'n'));       //'soun':audio track
        put_be32(0);                                //uint32 reserved[3]
        put_be32(0);
        put_be32(0);
        put_byte(AUDIO_HANDLER_NAME_LEN);           //char name[], name[0] is actual length
        put_buffer(AUDIO_HANDLER_NAME, AUDIO_HANDLER_NAME_LEN-1);

        put_AudioMediaInformationBox();
}
*/


//--------------------------------------------------

#define TrackHeaderBox_SIZE     92
#define clefBox_SIZE            20
#define profBox_SIZE            20
#define enofBox_SIZE            20
#define taptBox_SIZE            (8+clefBox_SIZE+profBox_SIZE+enofBox_SIZE) // 68
#define VideoTrackBox_SIZE64    (8+TrackHeaderBox_SIZE+taptBox_SIZE+VideoMediaBox_SIZE64) // 655+_spsSize+_ppsSize+V*21+IDR*4
#define VideoTrackBox_MAXSIZE64 (8+TrackHeaderBox_SIZE+taptBox_SIZE+VideoMediaBox_MAXSIZE64) // 655+_spsSize+_ppsSize+V*21+IDR*4
#define VideoTrackBox_SIZE      (8+TrackHeaderBox_SIZE+taptBox_SIZE+VideoMediaBox_SIZE) // 655+_spsSize+_ppsSize+V*17+IDR*4
#define VideoTrackBox_MAXSIZE   (8+TrackHeaderBox_SIZE+taptBox_SIZE+VideoMediaBox_MAXSIZE) // 655+_spsSize+_ppsSize+V*17+IDR*4
void put_videoTrackBox(u32 TrackId, u32 Duration)
{
    //TrackBox
    put_be32(VideoTrackBox_SIZE);          //uint32 size
    put_be32(FOURCC('t', 'r', 'a', 'k'));  //'trak'

        //TrackHeaderBox
        put_be32(TrackHeaderBox_SIZE);         //uint32 size
        put_be32(FOURCC('t', 'k', 'h', 'd'));  //'tkhd'
        put_byte(0);                           //uint8 version
        //0x01:track_enabled, 0x02:track_in_movie, 0x04:track_in_preview
        put_be24(0x07);                        //bits24 flags
        put_be32(_CreationTime);               //uint32 creation_time [version==0] uint64 creation_time [version==1]
        put_be32(_ModificationTime);           //uint32 modification_time [version==0] uint64 modification_time [version==1]
        put_be32(TrackId);                     //uint32 track_ID
        put_be32(0);                           //uint32 reserved
        put_be32(Duration);                    //uint32 duration [version==0] uint64 duration [version==1]
        put_be32(0);                           //uint32 reserved[2]
        put_be32(0);
        put_be16(0);                           //int16 layer
        put_be16(0);                           //int16 alternate_group
        put_be16(0x0000);                      //int16 volume
        put_be16(0);                           //uint16 reserved
        put_be32(0x00010000);                  //int32 matrix[9]
        put_be32(0);
        put_be32(0);
        put_be32(0);
        put_be32(0x00010000);
        put_be32(0);
        put_be32(0);
        put_be32(0);
        put_be32(0x40000000);
        put_be32(_Config._Width<<16);          //uint32 width  //16.16 fixed-point
        put_be32(_Config._Height<<16);         //uint32 height //16.16 fixed-point

        //taptBox
        put_be32(taptBox_SIZE);
        put_be32(FOURCC('t', 'a', 'p', 't'));
            //clefBox
            put_be32(clefBox_SIZE);
            put_be32(FOURCC('c', 'l', 'e', 'f'));
            put_be32(0);
            put_be16(_Config._Width);
            put_be16(0);
            put_be16(_Config._Height);
            put_be16(0);
            //profBox
            put_be32(profBox_SIZE);
            put_be32(FOURCC('p', 'r', 'o', 'f'));
            put_be32(0);
            put_be16(_Config._Width);
            put_be16(0);
            put_be16(_Config._Height);
            put_be16(0);
            //enofBox
            put_be32(enofBox_SIZE);
            put_be32(FOURCC('e', 'n', 'o', 'f'));
            put_be32(0);
            put_be16(_Config._Width);
            put_be16(0);
            put_be16(_Config._Height);
            put_be16(0);

        put_VideoMediaBox(Duration);
}

#define AudioTrackBox_SIZE64     (8+TrackHeaderBox_SIZE+AudioMediaBox_SIZE64) // 444+A*12
#define AudioTrackBox_MAXSIZE64  (8+TrackHeaderBox_SIZE+AudioMediaBox_MAXSIZE64) // 444+A*12
#define AudioTrackBox_SIZE       (8+TrackHeaderBox_SIZE+AudioMediaBox_SIZE) // 444+A*8
#define AudioTrackBox_MAXSIZE    (8+TrackHeaderBox_SIZE+AudioMediaBox_MAXSIZE) // 444+A*8
/*void put_AudioTrackBox(AM_UINT TrackId, AM_UINT Duration)
{
    //TrackBox
    if (_largeFile == true)
    {
    put_be32(AudioTrackBox_SIZE64);        //uint32 size
    }
    else
    {
    put_be32(AudioTrackBox_SIZE);          //uint32 size
    }
    put_be32(FOURCC('t', 'r', 'a', 'k'));  //'trak'

        //TrackHeaderBox
        put_be32(TrackHeaderBox_SIZE);         //uint32 size
        put_be32(FOURCC('t', 'k', 'h', 'd'));  //'tkhd'
        put_byte(0);                           //uint8 version
        //0x01:track_enabled, 0x02:track_in_movie, 0x04:track_in_preview
        put_be24(0x07);                        //bits24 flags
        put_be32(_CreationTime);               //uint32 creation_time [version==0] uint64 creation_time [version==1]
        put_be32(_ModificationTime);           //uint32 modification_time [version==0] uint64 modification_time [version==1]
        put_be32(TrackId);                     //uint32 track_ID
        put_be32(0);                           //uint32 reserved
        put_be32(Duration);                    //uint32 duration [version==0] uint64 duration [version==1]
        put_be32(0);                           //uint32 reserved[2]
        put_be32(0);
        put_be16(0);                           //int16 layer
        put_be16(0);                           //int16 alternate_group
        put_be16(0x0100);                      //int16 volume
        put_be16(0);                           //uint16 reserved
        put_be32(0x00010000);                  //int32 matrix[9]
        put_be32(0);
        put_be32(0);
        put_be32(0);
        put_be32(0x00010000);
        put_be32(0);
        put_be32(0);
        put_be32(0);
        put_be32(0x40000000);
        put_be32(0);                           //uint32 width  //16.16 fixed-point
        put_be32(0);                           //uint32 height //16.16 fixed-point

        put_AudioMediaBox();
}
*/

//--------------------------------------------------

#define FileTypeBox_SIZE  24 //32
void put_FileTypeBox()
{
	put_be32(FileTypeBox_SIZE);            //uint32 size
	put_be32(FOURCC('f', 't', 'y', 'p'));  //'ftyp'
	put_be32(FOURCC('m', 'p', '4', '2'));  //uint32 major_brand
	put_be32(0);                           //uint32 minor_version
	put_be32(FOURCC('m', 'p', '4', '2'));  //uint32 compatible_brands[]
	put_be32(FOURCC('i', 's', 'o', 'm'));
}

#define MovieHeaderBox_SIZE  108
#define ObjDescrpBox_SIZE	24
#define AMBABox_SIZE         40
#define UserDataBox_SIZE     (8+AMBABox_SIZE) // 48
#define MovieBox_SIZE        (8+MovieHeaderBox_SIZE+\
				  ObjDescrpBox_SIZE +\
                              UserDataBox_SIZE+\
                              VideoTrackBox_SIZE)
#define MovieBox_MAXSIZE     (8+MovieHeaderBox_SIZE+\
				  ObjDescrpBox_SIZE +\
				  UserDataBox_SIZE+\
                              VideoTrackBox_MAXSIZE)
void put_MovieBox()
{
    u32 i, vDuration, aDuration, Duration;

    //MovieBox
    put_be32(MovieBox_SIZE);               //uint32 size
    put_be32(FOURCC('m', 'o', 'o', 'v'));  //'moov'

        //MovieHeaderBox
        //vDuration = (u32)(1.0*_frame_field_set*_Config._clk*_VPacketCnt*_Config._scale/_Config._rate);
        vDuration = (u32)_average_ctts;
	aDuration = (u32)(1.0*_Config._clk*_APacketCnt*1024/_Config._SampleFreq);
        Duration = (vDuration>aDuration)?vDuration:aDuration;
        put_be32(MovieHeaderBox_SIZE);         //uint32 size
        put_be32(FOURCC('m', 'v', 'h', 'd'));  //'mvhd'
        put_byte(0);                           //uint8 version
        put_be24(0);                           //bits24 flags
        put_be32(_CreationTime);               //uint32 creation_time [version==0] uint64 creation_time [version==1]
        put_be32(_ModificationTime);           //uint32 modification_time [version==0] uint64 modification_time [version==1]
        put_be32(_Config._clk);                //uint32 timescale
        put_be32(Duration);                    //uint32 duration [version==0] uint64 duration [version==1]
        put_be32(0x00010000);                  //int32 rate
        put_be16(0x0100);                      //int16 volume
        put_be16(0);                           //bits16 reserved
        put_be32(0);                           //uint32 reserved[2]
        put_be32(0);
        put_be32(0x00010000);                  //int32 matrix[9]
        put_be32(0);
        put_be32(0);
        put_be32(0);
        put_be32(0x00010000);
        put_be32(0);
        put_be32(0);
        put_be32(0);
        put_be32(0x40000000);
        put_be32(0);                           //bits32 pre_defined[6]
        put_be32(0);
        put_be32(0);
        put_be32(0);
        put_be32(0);
        put_be32(0);
        put_be32(1+1);     //uint32 next_track_ID

 		put_be32(ObjDescrpBox_SIZE); 	 //uint32 size
		put_be32(FOURCC('i', 'o', 'd', 's'));  //'iods'
		put_byte(0);                           //uint8 version
    		put_be24(0);                           //bits24 flags
		put_be32(0x10808080);
		put_be32(0x07004FFF );
		put_be32(0xFF0F7FFF);



        //UserDataBox
        put_be32(UserDataBox_SIZE);            //uint32 size
        put_be32(FOURCC('u', 'd', 't', 'a'));  //'udta'
            //AMBABox
            put_be32(AMBABox_SIZE);                          //uint32 size
            put_be32(FOURCC('A', 'M', 'B', 'A'));  //'AMBA'
            put_be16(_Config._ar_x);               //uint16 ar_x
            put_be16(_Config._ar_y);               //uint16 ar_y
            put_byte(_Config._mode);               //uint8 mode
            put_byte(_Config._M);                  //uint8 M
            put_byte(_Config._N);                  //uint8 N
            put_byte(_Config._advanced);           //uint8 advanced
            put_be32(_Config._idr_interval);       //uint32 idr_interval
            put_be32(_Config._scale);              //uint32 scale
            put_be32(_Config._rate);               //uint32 rate
            put_be32(_Config._brate);              //uint32 brate
            put_be32(_Config._brate_min);          //uint32 brate_min
            put_be32(0);

        i=1; put_videoTrackBox(i, vDuration);
}


//--------------------------------------------------

#define EmptyFreeSpaceBox_SIZE (16+_SPS_SIZE+_PPS_SIZE) // 144  16=8 bytes header + 8 bytes data
void put_FreeSpaceBox(u32 FreeSpaceSize)
{
    u8 zero[64];
    u32 i;
    assert(FreeSpaceSize >= 8);
    memset((void *)zero, 0, sizeof(zero));
    put_be32(FreeSpaceSize);
    put_be32(FOURCC('f', 'r', 'e', 'e'));
    FreeSpaceSize -= 8;
    for (i=0; i<FreeSpaceSize/sizeof(zero); i++)
    {
        put_buffer(zero, sizeof(zero));
    }
    FreeSpaceSize %= sizeof(zero);
    put_buffer(zero, FreeSpaceSize);
}

#define EmptyMediaDataBox_SIZE   8
#define FileHeader_SIZE          (FileTypeBox_SIZE+MovieBox_SIZE+EmptyFreeSpaceBox_SIZE) // 1439+_spsSize+_ppsSize+V*17+IDR*4+A*8
#define FileHeader_MAXSIZE       (FileTypeBox_SIZE+MovieBox_MAXSIZE+EmptyFreeSpaceBox_SIZE) // 1439+_spsSize+_ppsSize+V*17+IDR*4+A*8
void put_Mp4File(u32 FreeSpaceSize)
{
	fseek(_pSink,0, SEEK_SET);
	_curPos = 0;
    //FileTypeBox
    	put_FileTypeBox();
    //MovieBox
    	put_MovieBox();
    //FreeSpaceBox
	put_FreeSpaceBox(FreeSpaceSize);
    //MediaDataBox is generated during RUNNING
	if ( 0 != fseek(_pSink, 0, SEEK_END))
	{
		printf("seek error\n");
		return;
	}
	u64 size = ftell(_pSink);
    	size -= _curPos;
    	fseek(_pSink, _curPos, SEEK_SET);
    	put_be32((u32)size);                          //uint32 size
    	put_be32(FOURCC('m', 'd', 'a', 't'));      //'mdat'
}


//copy from DV
void InitSdtp()
{
    u32 lv0=(u32)_Config._M, lv1, lv2, lv3, lv4, lv5, lv6=1;
    u32 EntryNo;
    u32 ctr_L1, ctr_L2, ctr_L3, ctr_L4, ctr_L5, ctr_L6;
    u8 last_lv;

    if (0)//(_Config._advanced)
    {
        lv1 = (lv0/2==0)?1:lv0/2;
        lv2 = (lv0/4==0)?1:lv0/4;
        lv3 = (lv0/8==0)?1:lv0/8;
        lv4 = (lv0/16==0)?1:lv0/16;
        lv5 = (lv0/32==0)?1:lv0/32;
        if (lv0/2 == 1)
        {
            last_lv = 1;
        }
        else if (lv0/4 == 1)
        {
            last_lv = 2;
        }
        else if (lv0/8 == 1)
        {
            last_lv = 3;
        }
        else if (lv0/16 == 1)
        {
            last_lv = 4;
        }
        else
        {
            last_lv = 5;
        }
        EntryNo = 0;
        _lv_idc[EntryNo] = last_lv;
        EntryNo++;
        _lv_idc[EntryNo] = 0;
        EntryNo++;
        for (ctr_L1=lv1; ctr_L1<=lv0; ctr_L1+=lv1)
        {
            if (ctr_L1 != lv0)
            {
                _lv_idc[EntryNo] = 1;
                EntryNo++;
            }
            for (ctr_L2=lv2; ctr_L2<=lv1; ctr_L2+=lv2)
	        {
                if (ctr_L2 != lv1)
                {
                    _lv_idc[EntryNo] = 2;
                    EntryNo++;
                }
                for (ctr_L3=lv3; ctr_L3<=lv2; ctr_L3+=lv3)
                {
                    if (ctr_L3 != lv2)
                    {
                        _lv_idc[EntryNo] = 3;
                        EntryNo++;
                    }
                    for (ctr_L4=lv4; ctr_L4<=lv3; ctr_L4+=lv4)
                    {
                        if (ctr_L4 != lv3)
                        {
                            _lv_idc[EntryNo] = 4;
                            EntryNo++;
                        }
                        for (ctr_L5=lv5; ctr_L5<=lv4; ctr_L5+=lv5)
                        {
                            if (ctr_L5 != lv4)
                            {
                                _lv_idc[EntryNo] = 5;
                                EntryNo++;
                            }
                            for (ctr_L6=lv6; ctr_L6<=lv5; ctr_L6+=lv6)
                            {
                                if (ctr_L6 != lv5)
                                {
                                    _lv_idc[EntryNo] = 6;
                                    EntryNo++;
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    else
    {
        EntryNo = 0;
        _lv_idc[EntryNo] = 1;
        EntryNo++;
        _lv_idc[EntryNo] = 0;
        EntryNo++;
        for (ctr_L1=1; ctr_L1<lv0; ctr_L1++)
        {
            _lv_idc[EntryNo] = 1;
            EntryNo++;
        }
    }
}

//Generate AAC decoder information
/*AM_UINT GetAacInfo(u32 samplefreq, u32 channel)
{
    //bits5: Audio Object Type  AAC Main=1, AAC LC (low complexity)=2, AAC SSR=3, AAC LTP=4
    //bits4: Sample Frequency Index
    //     bits24: Sampling Frequency [if Sample Frequency Index == 0xf]
    //bits4: Channel Configuration
    //bits1: FrameLength 0:1024, 1:960
    //bits1: DependsOnCoreCoder
    //     bits14: CoreCoderDelay [if DependsOnCoreCoder==1]
    //bits1: ExtensionFlag
    AM_UINT info = 0x1000;//AAC LC
    AM_UINT sfi = 0x0f;
    //ISO/IEC 14496-3 Table 1.16 Sampling Frequency Index
    switch (samplefreq)
    {
    case 96000:
        sfi = 0;
        break;
    case 88200:
        sfi = 1;
        break;
    case 64000:
        sfi = 2;
        break;
    case 48000:
        sfi = 3;
        break;
    case 44100:
        sfi = 4;
        break;
    case 32000:
        sfi = 5;
        break;
    case 24000:
        sfi = 6;
        break;
    case 22050:
        sfi = 7;
        break;
    case 16000:
        sfi = 8;
        break;
    case 12000:
        sfi = 9;
        break;
    case 11025:
        sfi = 10;
        break;
    case 8000:
        sfi = 11;
        break;
    }
    info |= (sfi << 7);
    info |= (channel << 3);
    return info;
}*/


int Init(iav_h264_config_ex_t* h264_config, char* mp4_filename, h264_info_t* h264_info)
{
	enum {
        	CBR = 1,
        	VBR = 3,
    	};
	_Config._M = h264_config->M;
	_Config._N = h264_config->N;
	_Config._advanced = h264_config->gop_model;
	_Config._idr_interval = h264_config->idr_interval;
	_Config._ar_x = h264_config->pic_info.ar_x;
	_Config._ar_y = h264_config->pic_info.ar_y;
	_Config._mode = h264_config->pic_info.frame_mode;
	_Config._scale = h264_config->pic_info.rate;
	_Config._rate = h264_config->pic_info.scale;

	 if (h264_config->bitrate_control == CBR)
	{
		_Config._brate = _Config._brate_min = h264_config->average_bitrate;
	}
//	else
//	{
//		_Config._brate = (u32)( 1.0 * h264_config->average_bitrate * h264_config->max_vbr_rate_factor / 100);
//		if (h264_config->min_vbr_rate_factor < 40) h264_config->min_vbr_rate_factor = 40;
//			_Config._brate_min = (u32)(1.0 * h264_config->average_bitrate * h264_config->min_vbr_rate_factor / 100);
//		assert(_Config._brate_min != 0);
//	}
	_Config._Width = h264_info->format.width;
	_Config._Height = h264_info->format.height;
	_Config._new_gop = 1;

	memcpy(_Config._FileName, mp4_filename, strlen(mp4_filename)+1);
	_Config._SampleFreq = 48000;
	_Config._Channels = 2;
	_Config._SampleSize = 16;
	_Config._clk = 90000;
	_average_ctts = 0;

	InitSdtp();

	time_t t = time(NULL);//seconds since 1970-01-01 00:00:00 local
	struct tm *utc = gmtime(&t);//UTC time yyyy-mm-dd HH:MM:SS
	t = mktime(utc);//seconds since 1970-01-01 00:00:00 UTC

	//1904-01-01 00:00:00 UTC -> 1970-01-01 00:00:00 UTC
	//66 years plus 17 days of the 17 leap years [1904, 1908, ..., 1968]
	_CreationTime = _ModificationTime = t+66*365*24*60*60+17*24*60*60;

    	//_Config should be ready

	memset(_sps, 0, sizeof(_sps));
	_spsSize = _spsPad = 0;
	memset(_pps, 0, sizeof(_pps));
	_ppsSize = _ppsPad = 0;

	_VPacketCnt = _APacketCnt = _IdrPacketCnt = 0;
	_v_delta = (u32)(1.0*_Config._scale*CLOCK/_Config._rate);

	//one audio frame have 1024 samples, CLOCK/samples==count of time units per sample
    	_a_delta = (u32)(1.0*1024*CLOCK/_Config._SampleFreq);
    	_vdts = _adts = 0;

	_MaxVPacketCnt = h264_info->frame_cnt;
	_MaxAPacketCnt = 0;
	_MaxIdrPacketCnt = 1 + _MaxVPacketCnt/(_Config._idr_interval*_Config._N);

	u32 tmpsize;
	tmpsize = (FileHeader_MAXSIZE+1023)/1024;

        //add some tolerance
        tmpsize += 2; tmpsize *= 1024;
        _FreeSpaceSize = tmpsize-FileTypeBox_SIZE;

	_pSink = fopen((const char *)_Config._FileName, "w+");
    	if (_pSink == NULL )
	{
        	printf(">>>can't open file %s\n",_Config._FileName);
		return -1;
    	}

	_curPos = 0;

	//index settings
	_v_idx = _a_idx = 0;
    	u32 fpos;
	//make sure that each start file pos of the tmp index data is aligned to 8 bytes

	put_Mp4File(_FreeSpaceSize-MovieBox_SIZE);
        fpos = (u32)(_curPos - EmptyMediaDataBox_SIZE);       //just before mdat box
        //between each kind of index, there is 16 bytes gap

	fpos -= _MaxAPacketCnt*2*sizeof(u32) + _MaxVPacketCnt*4*sizeof(u32) + 80;  //start of tmp index buffer
	fpos &= 0xfffffff8;
	_vctts_cur_fpos = _vctts_fpos = fpos; fpos+=_MaxVPacketCnt*2*sizeof(u32)+16;
	_vstsz_cur_fpos = _vstsz_fpos = fpos; fpos+=_MaxVPacketCnt*1*sizeof(u32)+16;
	_vstco_cur_fpos = _vstco_fpos = fpos; fpos+=_MaxVPacketCnt*1*sizeof(u32)+16;
	_astsz_cur_fpos = _astsz_fpos = fpos; fpos+=_MaxAPacketCnt*1*sizeof(u32)+16;
	_astco_cur_fpos = _astco_fpos = fpos; fpos+=_MaxAPacketCnt*1*sizeof(u32)+16;

	_pSinkIdx = fopen((const char *)_Config._FileName, "r+");
	if(_pSinkIdx == NULL)
	{
	    return -1;
	}

	_SpsPps_pos = FileTypeBox_SIZE+MovieBox_SIZE+8;//8 for the free atom header
	_frame_field_set = 0;
	return 0;

}

int IdxWriter()
{
	typedef enum BOOL {
		false = 0,
		true = !false
	}bool;
	bool write_v = false;
	bool write_a = false;

	if (_v_idx >= _IDX_SIZE)
	{
		write_v = true;
	}
	if (_a_idx >= _IDX_SIZE)
	{
		write_a = true;
	}

	if (write_v == true)
	{
        	fseek(_pSinkIdx, _vctts_cur_fpos, SEEK_SET);
        	fwrite((u8*)_vctts, sizeof(u32), _IDX_SIZE*2,_pSinkIdx);
		_vctts_cur_fpos += _IDX_SIZE*2*sizeof(u32);
        	fseek(_pSinkIdx,_vstsz_cur_fpos, SEEK_SET);
        	fwrite((u8*)_vstsz,sizeof(u32), _IDX_SIZE*1, _pSinkIdx);
		_vstsz_cur_fpos += _IDX_SIZE*1*sizeof(u32);
		fseek(_pSinkIdx, _vstco_cur_fpos, SEEK_SET);
		fwrite((u8 *)_vstco,sizeof(32),_IDX_SIZE*1, _pSinkIdx);
		//DbgLog((LOG_TRACE, 1, TEXT("sinkIdx write _IDX_SIZE")));
        	_vstco_cur_fpos += _IDX_SIZE*1*sizeof(32);
        }

        _v_idx -= _IDX_SIZE;
        if (_v_idx > 0)
        {
            memcpy(&_vctts[0], &_vctts[_IDX_SIZE*2], _v_idx*2*sizeof(u32));
            memcpy(&_vstsz[0], &_vstsz[_IDX_SIZE*1], _v_idx*1*sizeof(u32));
            memcpy(&_vstco[0], &_vstco[_IDX_SIZE*1], _v_idx*1*sizeof(u32));
        }

    if (write_a == true)
    {
     /*   printf("write audio index: 0x%08x 0x%08x\n", _astsz_cur_fpos, _astco_cur_fpos);
        _pSinkIdx->Seek(_astsz_cur_fpos, SEEK_SET);
        _pSinkIdx->Write((AM_U8 *)_astsz, _IDX_SIZE*1*sizeof(AM_UINT));
        _astsz_cur_fpos += _IDX_SIZE*1*sizeof(AM_UINT);
        _pSinkIdx->Seek(_astco_cur_fpos, SEEK_SET);
        if (_largeFile == true)
        {
        _pSinkIdx->Write((AM_U8 *)_astco64, _IDX_SIZE*1*sizeof(AM_U64));
        _astco_cur_fpos += _IDX_SIZE*1*sizeof(AM_U64);
        }
        else
        {
        _pSinkIdx->Write((AM_U8 *)_astco, _IDX_SIZE*1*sizeof(AM_UINT));
        _astco_cur_fpos += _IDX_SIZE*1*sizeof(AM_UINT);
        }
		_a_idx -= _IDX_SIZE;
        if (_a_idx > 0)
        {
            ::memcpy(&_astsz[0], &_astsz[_IDX_SIZE*1], _a_idx*1*sizeof(AM_UINT));
            if (_largeFile == true)
            {
            ::memcpy(&_astco64[0], &_astco64[_IDX_SIZE*1], _a_idx*1*sizeof(AM_U64));
            }
            else
            {
            ::memcpy(&_astco[0], &_astco[_IDX_SIZE*1], _a_idx*1*sizeof(AM_UINT));
            }
        }*/
    }

    return 0;
}

int Stop()
{
    //write tmp index in memory to file
	if (_v_idx != 0)
    	{
		fseek(_pSinkIdx, _vctts_cur_fpos, SEEK_SET);
		fwrite((u8 *)_vctts, sizeof(u32),_v_idx*2, _pSinkIdx);
		_vctts_cur_fpos += _v_idx*2*sizeof(u32);
		fseek(_pSinkIdx,_vstsz_cur_fpos, SEEK_SET);
        	fwrite((u8 *)_vstsz, sizeof(u32),_v_idx*1, _pSinkIdx);
		_vstsz_cur_fpos += _v_idx*1*sizeof(u32);
		fseek(_pSinkIdx,_vstco_cur_fpos, SEEK_SET);
        	fwrite((u8 *)_vstco, sizeof(u32),_v_idx*1, _pSinkIdx);
		_vstco_cur_fpos += _v_idx*1*sizeof(u32);
    	}
    	if (_a_idx != 0)
    	{
        /*_pSinkIdx->Seek(_astsz_cur_fpos, SEEK_SET);
        _pSinkIdx->Write((AM_U8 *)_astsz, _a_idx*1*sizeof(AM_UINT));
        _astsz_cur_fpos += _a_idx*1*sizeof(AM_UINT);
        _pSinkIdx->Seek(_astco_cur_fpos, SEEK_SET);
        if (_largeFile == true)
        {
        _pSinkIdx->Write((AM_U8 *)_astco64, _a_idx*1*sizeof(AM_U64));
        _astco_cur_fpos += _a_idx*1*sizeof(AM_U64);
        }
        else
        {
        _pSinkIdx->Write((AM_U8 *)_astco, _a_idx*1*sizeof(AM_UINT));
        _astco_cur_fpos += _a_idx*1*sizeof(AM_UINT);
        }*/
    	}
	put_Mp4File(_FreeSpaceSize-MovieBox_SIZE);

	fclose(_pSink);
	fclose(_pSinkIdx);
	return 0;
}

//return 0 means pBuffer processed, return 1 means pBuffer should not return to pool
int ProcessVideoData(video_frame_t *info, u8 * pBuffer)
{
	u32 ctts;
	//static u32 positive_ctts_delta;
	if (_frame_field_set == 0)
	{
		/*if(info->pic_struct == 1)
			_frame_field_set = 2;
        	else
			_frame_field_set = 1;
		if (info->pic_struct == 1)
			_v_delta = (AM_UINT)(2.0*_Config._scale*CLOCK/_Config._rate);*/

        }
            // Simple GOP
	//positive_ctts_delta = _v_delta;
	if( _VPacketCnt == 0 && info->pic_type != 0x01)
	{
		return 0;
	}

	u32 pts = info->pts;
	static u32 pts_old = 0;

	u32  size1, len1, sei_len1, sei_len;
	size1 = info->size;
	len1 = GetAuDelimiter(pBuffer, 0);
        len1 += (info->pic_type==0x01 || info->pic_type==0x02 ||
		(_Config._M == 255 && _Config._N == 255 && _VPacketCnt%10 == 0))?GetSpsPps(pBuffer, len1):0;

	 size1 -= len1;//we do not write sps/pps packet

        // AU delimiter
	static u8 aubuf[8]={0x00,0x00,0x00,0x02,0x09,0x00,0x00,0x00};
        size1 += 6;

		//ctts = (AM_UINT)((pts-_pts_off+positive_ctts_delta-_vdts)*_Config._clk/CLOCK);
		//ctts = _v_delta;
	if(_VPacketCnt ==0)
	{
		ctts = _v_delta;
	}
	else
	{
		ctts = pts -pts_old;
	}
	_average_ctts += ctts;
		//ctts = _v_delta *2;
	pts_old = pts;
		//DbgLog((LOG_TRACE, 1, TEXT("ctts[%d]=%d,pts=%lld"),_VPacketCnt,ctts,pts));
	u64 curPos = _curPos;

        //write AU delimiter
	if (info->pic_type == 1 || info->pic_type == 2 )
        {
            aubuf[5] = 0x10;
        }
        else if (info->pic_type == 4)
        {
            aubuf[5] = 0x50;
        }
        else
        {
            aubuf[5] = 0x30;
        }
        put_buffer(aubuf, 6);
        //write SEI packet
	for (sei_len1=0;;)
        {

            sei_len = GetSeiLen(pBuffer, len1+sei_len1);
            if (sei_len > 0)
            {
		put_be32(sei_len-4);
                u32 offset = len1+sei_len1+4;
                u32 remain = sei_len-4;
		if (offset < info->size)
                {
			u32 left = info->size-offset;
			if (remain <= left)
			{
				put_buffer(pBuffer+offset, remain);
			}
			else
			{
				put_buffer(pBuffer+offset, left);
				remain -= left; offset = 0;
			}
		}
		else
		{
			offset -= info->size;
		}
		sei_len1 += sei_len;
            }
            else
            {
                break;
            }
        }


        //write I/P/B frame
        if (size1-6-sei_len1 > 0)
        {
            put_be32(size1-6-sei_len1-4);
            u32 offset = len1+sei_len1+4;
            u32 remain = size1-6-sei_len1-4;

            if (offset < info->size)
            {
                u32 left = info->size - offset;
                if (remain <= left)
                {
                    put_buffer(pBuffer+offset, remain);
                }
                else
                {
                    put_buffer(pBuffer+offset, left);
                    remain -= left; offset = 0;
                }
            }
            else
            {
                offset -= info->size;
            }

        }

	//make sure current frame not wrapped by encoder


        //update index

        _vctts[_v_idx*2+0] = le_to_be32(1);     //sample_count
        _vctts[_v_idx*2+1] = le_to_be32(ctts);  //sample_delta
        _vstsz[_v_idx] = le_to_be32(size1);
	_vstco[_v_idx] = le_to_be32((u32)curPos);
        _v_idx++;

        if (_v_idx >= _IDX_SIZE)
	{
		IdxWriter();
	}


        _VPacketCnt++;

        if (info->pic_type == 0x01)
        {
            _IdrPacketCnt++;
        }
        //_vdts = (AM_U64)(1.0*_VPacketCnt*_scale*CLOCK/_rate);
        _vdts += (u64)_v_delta;

    return 0;
}


//Sequence Parameters Set, Picture Parameters Set
u32 GetSpsPps(u8 *pBuffer,u32 offset)
{
	u32 i, pos, len;
	u32 code, tmp;
	pos = 4;
	tmp = GetByte(pBuffer, offset+pos);
	if ((tmp&0x1F) != 0x07)
   	{
        /* no SPS */
        	return 0;
    	}
	if (_spsSize != 0)
    	{
        /* we do get SPS & PPS, skip the repeats */
        	return _spsSize+_ppsSize+8-_spsPad-_ppsPad ;
	}
    /* find SPS */
    	for (i=0,code=0xffffffff; ; i++,pos++)
    	{
        	tmp = GetByte(pBuffer, offset+pos);
		if ((code=(code<<8)|tmp) == 0x00000001)
        	{
            		break;//found next start code
        	}
		//printf("%d\n",i);
		_sps[i] = tmp;
    	}
    //full_sps_len = pos-4+1;//including 00000001 start code
    //len: sps size excluding 00000001 start code
	len = (pos-4+1) - 4;
	_spsPad = len & 3;
	if (_spsPad > 0) _spsPad = 4 - _spsPad;
	_spsSize = len + _spsPad;

    /* find PPS */
	pos++;//now pos point to one byte after 0x00000001
	tmp = GetByte(pBuffer, offset+pos);
	if ((tmp&0x1F) != 0x08)
	{
        	/* no PPS */
        	return 0;
    	}
    	for (i=0,code=0xffffffff; ; i++,pos++)
    	{
        	tmp = GetByte(pBuffer, offset+pos);
        	if ((code=(code<<8)|tmp) == 0x00000001)
        	{
            		break;
        	}
        	_pps[i] = tmp;

	}
    	//000000 of the next start code 00000001 also written into _pps[]
    	//len: pps size excluding 00000001 start code
    	//full_pps_len = (pos-4+1) - full_sps_len; //including 00000001 start code
    	len = (pos-4+1) - (len+4) - 4;
    	_ppsPad = len & 3;
    	if (_ppsPad > 0) _ppsPad = 4 - _ppsPad;
    	_ppsSize = len + _ppsPad;
    	if (_SpsPps_pos != 0)
    	{
	//	_pSinkIdx->Seek(_SpsPps_pos, SEEK_SET);
		fseek(_pSinkIdx, _SpsPps_pos, SEEK_SET);
        	//_pSinkIdx->Write((AM_U8 *)&_spsSize, 4); _pSinkIdx->Write(_sps, _SPS_SIZE);
        	fwrite((u8 *)&_spsSize,sizeof(u8),4, _pSinkIdx);
		fwrite((u8 *)_sps,sizeof(u8),_SPS_SIZE, _pSinkIdx);
		//_pSinkIdx->Write((AM_U8 *)&_ppsSize, 4); _pSinkIdx->Write(_pps, _PPS_SIZE);
        	fwrite((u8 *)&_ppsSize,sizeof(u8),4, _pSinkIdx);
		fwrite((u8 *)_pps,sizeof(u8),_PPS_SIZE, _pSinkIdx);
		//_pSinkIdx->Write((AM_U8 *)&_frame_field_set, 4);
		fwrite((u8 *)&_frame_field_set,sizeof(u8),4, _pSinkIdx);
        	_SpsPps_pos = 0;
	}
	return _spsSize+_ppsSize+8-_spsPad-_ppsPad;
}

//Sequence Enhancement Information
u32 GetSeiLen(u8 *pBuffer, u32 offset)
{
    u32 code, tmp, pos;
    pos = 4;
    tmp = GetByte(pBuffer, offset+pos);
    if ((tmp&0x1F) != 0x06)
    {
        /* no SEI */
        return 0;
    }
    /* find SEI */
    pos++;
    for (code=0xffffffff; ; pos++)
    {
        tmp = GetByte(pBuffer, offset+pos);

	if ((code=(code<<8)|tmp) == 0x00000001)
        {
            break;//found next start code
        }
    }
    //pos point to 01 of start code 0x00000001
    //pos-4 is the last position of SEI
    return pos-4+1;
}

//access unit delimiter 0000000109x0
u32 GetAuDelimiter(u8 *pBuffer, u32 offset)
{
    u32 tmp, pos;
    pos = 4;
    tmp = GetByte(pBuffer, offset+pos);
    if ((tmp&0x1F) == 0x09)
    {
        return 6;
    }
    return 0;
}


int main(int argc, char **argv)
{
	if ( (argc != 2 ) || strcmp(argv[1],"-h") == 0 )
	{
		printf("usage: ./test_mp4mux h264_file\nNotice:\'h264_file\' and \'h264_file.info\' should exist, which are the output of \'./test_stream --frame-info\'\n");
		return 0;
	}
	char file_name[256] = {'\0'};

	FILE *fp, *fp_info;
	memcpy(file_name,argv[1], strlen(argv[1]));
	fp = fopen(file_name,"r");
	if (fp == NULL)
	{
		printf("Open file[%s] failed\n", file_name);
		return -1;
	}
	memcpy(file_name + strlen(argv[1]),".info", strlen(".info")+1);
	fp_info = fopen(file_name,"r");
	if (fp_info == NULL)
	{
		fclose(fp);
		printf("Open file[%s] failed\n", file_name);
		return -1;
	}
	memcpy(file_name + strlen(argv[1]),".mp4", strlen(".mp4")+1);
	h264_info_t h264_info;
	video_frame_t video_frame;
	if ( 1 !=fread((void *)&h264_info,sizeof(h264_info_t),1,fp_info) )
	{
		printf("info error\n");
		return -1;
	}
	int frame_cnt = 0;
	if (0 != Init(&h264_info.h264_config, file_name, &h264_info) )
	{
		printf("init error\n");
                return -1;
	}
	else
	{
		 printf("init success[width:%d][height:%d]\n",h264_info.format.width, h264_info.format.height);
	}
	u8 pBuffer[1024*1024];
	while ( 1 == fread((void *)&video_frame, sizeof(video_frame_t),1,fp_info) )
	{

		if ( video_frame.size  == fread((void *)pBuffer,sizeof(u8),video_frame.size, fp))
		{
			if( 0 == ProcessVideoData(&video_frame,  pBuffer))
			{
				frame_cnt++;
			}

		}
		else
		{
			break;

		}
	}
	printf("process success, frame count = %d,the output file is %s\n",frame_cnt,file_name);
	Stop();
	fclose(fp);
	fclose(fp_info);
	return 0;
}


