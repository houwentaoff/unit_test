extern "C" {

#include "avcodec.h"
#include "avformat.h"
#include "libavutil/log.h"

#include <stdlib.h>
#include <time.h>
#include <stdio.h>

// when used for filters they must have an odd number of elements
// coeffs cannot be shared between vectors
    typedef struct
    {
        double *coeff;              ///< pointer to the list of coefficients
        int length;                 ///< number of coefficients in the vector
    } SwsVector;

// vectors can be shared
    typedef struct
    {
        SwsVector *lumH;
        SwsVector *lumV;
        SwsVector *chrH;
        SwsVector *chrV;
    } SwsFilter;
    typedef struct AVCodecTag
    {
        enum CodecID id;
        unsigned int tag;
    } AVCodecTag;


    struct SwsContext *sws_getContext ( int srcW, int srcH, enum PixelFormat srcFormat,
                                        int dstW, int dstH, enum PixelFormat dstFormat,
                                        int flags, SwsFilter *srcFilter,
                                        SwsFilter *dstFilter, const double *param );
    int sws_scale ( struct SwsContext *c, const uint8_t *const srcSlice[],
                    const int srcStride[], int srcSliceY, int srcSliceH,
                    uint8_t *const dst[], const int dstStride[] );
} /*extern "C"*/


#define SWS_BICUBIC           4

#define	DEBUG_PRINT(...) do{fprintf(stderr,"[%-30s][%5d]:\t",__FILE__,__LINE__);fprintf(stderr,__VA_ARGS__);}while(0)

static int st_Tabsset = 0;
static FILE* st_DebugFp = stdout;

#define	PRINT_TABS_HEADER() \
do\
{\
	int _itabs;\
	for (_itabs=0;_itabs < st_Tabsset;_itabs++)\
	{\
		fprintf(st_DebugFp,"\t");\
	}\
}while(0)

#define	PRINT_DEBUG_VALUE(...) \
do\
{\
	PRINT_TABS_HEADER(); \
	fprintf(st_DebugFp,__VA_ARGS__);\
}while(0)

#define	PRINT_DEBUG_CHARS(ch,n,name) \
do\
{\
	unsigned char* _pChar = (unsigned char*)ch;\
	int _i;\
	PRINT_TABS_HEADER();\
	fprintf(st_DebugFp,".%s[%d]:",#ch,n);\
	for (_i=0;_i < (n);_i++)\
	{\
		if ((_i %16)==0)\
		{\
			fprintf(st_DebugFp,"\n");\
			PRINT_TABS_HEADER();\
			fprintf(st_DebugFp,"0x%08x:\t",_i);\
		}\
		fprintf(st_DebugFp,"0x%02x ",_pChar[_i]);\
	}\
	fprintf(st_DebugFp,"\n");\
}while(0)


#define	PRINT_DEBUG_INTS(li,n,name) \
do\
{\
	unsigned int* _pInt = (unsigned int*)li;\
	int _i;\
	PRINT_TABS_HEADER();\
	fprintf(st_DebugFp,".%s[%d]:",#li,n);\
	for (_i=0;_i < (n);_i++)\
	{\
		if ((_i %16)==0)\
		{\
			fprintf(st_DebugFp,"\n");\
			PRINT_TABS_HEADER();\
			fprintf(st_DebugFp,"0x%08x:\t",_i);\
		}\
		fprintf(st_DebugFp,"0x%08x ",_pInt[_i]);\
	}\
	fprintf(st_DebugFp,"\n");\
}while(0)



#define	PRINT_DEBUG_LLINTS(lli,n,name) \
do\
{\
	uint64_t* _pLLI = (uint64_t*)lli;\
	int _i;\
	PRINT_TABS_HEADER();\
	fprintf(st_DebugFp,".%s[%d]:",#lli,n);\
	for (_i=0;_i < (n);_i++)\
	{\
		if ((_i %16)==0)\
		{\
			fprintf(st_DebugFp,"\n");\
			PRINT_TABS_HEADER();\
			fprintf(st_DebugFp,"0x%08x:\t",_i);\
		}\
		fprintf(st_DebugFp,"0x%llx ",_pLLI[_i]);\
	}\
	fprintf(st_DebugFp,"\n");\
}while(0)




#define	PRINT_STRUCT_VALUE(s,type,...)\
do\
{\
	PRINT_DEBUG_VALUE(__VA_ARGS__);\
if (s)\
{\
	st_Tabsset ++;\
	Debug##type((type*)s);\
	st_Tabsset --;\
}\
}while(0)

#define	PRINT_FILL_STRUCT_VALUE(s,type,...)\
do\
		{\
PRINT_DEBUG_VALUE(__VA_ARGS__);\
if (s)\
{\
	st_Tabsset ++;\
	Debug##type((struct type*)s);\
	st_Tabsset --;\
}\
}while(0)

#define PRINT_INT_VALUE(v,p) \
do\
{\
	PRINT_DEBUG_VALUE(".%s %d (0x%x)\n",#p"->"#v,p->v,p->v);\
}while(0)

#define PRINT_LONG_VALUE(v,p) \
do\
{\
	PRINT_DEBUG_VALUE(".%s %lu (0x%lx)\n",#p"->"#v,p->v,p->v);\
}while(0)


#define	PRINT_LLINT_VALUE(v,p) \
do\
{\
	PRINT_DEBUG_VALUE(".%s %lld (0x%llx)\n",#p"->"#v,p->v,p->v);\
}while(0)

#define	PRINT_UNSIGNED_INT_VALUE(v,p)\
do\
{\
	PRINT_DEBUG_VALUE(".%s %u (0x%x)\n",#p"->"#v,p->v,p->v);\
}while(0)


#define	PRINT_FLAGS_VALUE(v,p)\
do\
{\
	PRINT_DEBUG_VALUE(".%s 0x%08x\n",#p"->"#v,p->v);\
}while(0)

#define	PRINT_FLOAT_VALUE(v,p)\
do\
{\
	PRINT_DEBUG_VALUE(".%s %f\n",#p"->"#v,p->v);\
}while(0)

#define	PRINT_POINT_VALUE(v,p)\
do\
{\
	PRINT_DEBUG_VALUE(".%s (%p)\n",#p"->"#v,p->v);\
}while(0)

#define PRINT_FUNC_POINTER(v,p)\
do\
{\
	PRINT_DEBUG_VALUE(".%s function (%p)\n",#p"->"#v,p->v);\
}while(0)


#define	PRINT_NAME_VALUE(v,p) \
do\
{\
	PRINT_DEBUG_VALUE(".%s %s\n",#p"->"#v,p->v);\
}while(0)


/*
 * Video encoding example
 */
static void video_encode_example ( const char *filename, enum CodecID codec_id )
{
    AVCodec *codec;
    AVCodecContext *c = NULL;
    int i, out_size, x, y, outbuf_size;
    FILE *f;
    AVFrame *picture;
    uint8_t *outbuf;
    int had_output = 0;

    printf ( "Encode video file %s\n", filename );

    /* find the mpeg1 video encoder */
    codec = avcodec_find_encoder ( codec_id );
    if ( !codec )
    {
        fprintf ( stderr, "codec not found\n" );
        exit ( 1 );
    }

    c = avcodec_alloc_context ();
    picture = avcodec_alloc_frame();
    if ( picture == NULL )
    {
        fprintf ( stderr, "can not load frame\n" );
        exit ( 3 );
    }

    /* put sample parameters */
    c->bit_rate = 400000;
    /* resolution must be a multiple of two */
    c->width = 352;
    c->height = 288;
    /* frames per second */
    c->time_base = ( AVRational )
    {
        1, 25
    };
    c->gop_size = 10; /* emit one intra frame every ten frames */
    c->max_b_frames = 1;
    c->pix_fmt = PIX_FMT_YUV420P;

	PRINT_INT_VALUE(strict_std_compliance,c);
    /* open it */
    if ( avcodec_open ( c, codec ) < 0 )
    {
        fprintf ( stderr, "could not open codec\n" );
        exit ( 1 );
    }

    f = fopen ( filename, "wb" );
    if ( !f )
    {
        fprintf ( stderr, "could not open %s\n", filename );
        exit ( 1 );
    }

    /* alloc image and output buffer */
    outbuf_size = 100000 + 12 * c->width * c->height;
    outbuf =(uint8_t*) malloc ( outbuf_size );

    /* the image can be allocated by any means and av_image_alloc() is
     * just the most convenient way if av_malloc() is to be used */
//    av_image_alloc ( picture->data, picture->linesize,
//                     c->width, c->height, c->pix_fmt, 1 );

    /* encode 1 second of video */
    for ( i = 0; i < 25; i++ )
    {
        fflush ( stdout );
        /* prepare a dummy image */
        /* Y */
        for ( y = 0; y < c->height; y++ )
        {
            for ( x = 0; x < c->width; x++ )
            {
                picture->data[0][y * picture->linesize[0] + x] = x + y + i * 3;
            }
        }

        /* Cb and Cr */
        for ( y = 0; y < c->height / 2; y++ )
        {
            for ( x = 0; x < c->width / 2; x++ )
            {
                picture->data[1][y * picture->linesize[1] + x] = 128 + y + i * 2;
                picture->data[2][y * picture->linesize[2] + x] = 64 + x + i * 5;
            }
        }

        /* encode the image */
        out_size = avcodec_encode_video ( c, outbuf, outbuf_size, picture );
        had_output |= out_size;
        printf ( "encoding frame %3d (size=%5d)\n", i, out_size );
        fwrite ( outbuf, 1, out_size, f );
    }

    /* get the delayed frames */
    for ( ; out_size || !had_output; i++ )
    {
        fflush ( stdout );

        out_size = avcodec_encode_video ( c, outbuf, outbuf_size, NULL );
        had_output |= out_size;
        printf ( "write frame %3d (size=%5d)\n", i, out_size );
        fwrite ( outbuf, 1, out_size, f );
    }

    /* add sequence end code to have a real mpeg file */
    outbuf[0] = 0x00;
    outbuf[1] = 0x00;
    outbuf[2] = 0x01;
    outbuf[3] = 0xb7;
    fwrite ( outbuf, 1, 4, f );
    fclose ( f );
    free ( outbuf );

    avcodec_close ( c );
    av_free ( c );
    av_free ( picture->data[0] );
    av_free ( picture );
    printf ( "\n" );
}



int main ( int argc, char*argv[] )
{
	if (argc< 2)
	{
		fprintf(stderr,"%s outfile\n",argv[0]);
		exit(3);
	}
	avcodec_register_all();

	video_encode_example(argv[1],CODEC_ID_MPEG1VIDEO);
	return 0;

}
