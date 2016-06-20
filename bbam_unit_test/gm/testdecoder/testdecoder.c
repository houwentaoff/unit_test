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

#if 1
void DebugAVOption ( void* pOption )
{
	return ;
}
#else
void DebugAVOption ( AVOption* pOption )
{
    PRINT_NAME_VALUE ( name, pOption );
    PRINT_NAME_VALUE ( help, pOption );
    PRINT_INT_VALUE ( offset, pOption );
    PRINT_UNSIGNED_INT_VALUE ( type, pOption );
    PRINT_FLOAT_VALUE ( default_val, pOption );
    PRINT_FLOAT_VALUE ( min, pOption );
    PRINT_FLOAT_VALUE ( max, pOption );
    PRINT_NAME_VALUE ( unit, pOption );
    return ;
}
#endif
void DebugAVRational ( AVRational* pRational )
{
    PRINT_INT_VALUE ( num, pRational );
    PRINT_INT_VALUE ( den, pRational );
    return ;
}


void DebugAVClass ( AVClass *pCls )
{
    PRINT_NAME_VALUE ( class_name, pCls );
    PRINT_FUNC_POINTER ( item_name, pCls );
    PRINT_FILL_STRUCT_VALUE ( pCls->option, AVOption, ".option (%p)\n", pCls->option );
    return ;
}

void DebugAVPacket ( AVPacket *pPacket )
{
    PRINT_LLINT_VALUE ( pts, pPacket );
    PRINT_LLINT_VALUE ( dts, pPacket );
    PRINT_DEBUG_CHARS ( pPacket->data, pPacket->size, "data" );
    PRINT_INT_VALUE ( stream_index, pPacket );
    PRINT_FLAGS_VALUE ( flags, pPacket );
    PRINT_INT_VALUE ( duration, pPacket );
    PRINT_FUNC_POINTER ( destruct, pPacket );
    PRINT_POINT_VALUE ( priv, pPacket );
    PRINT_LLINT_VALUE ( pos, pPacket );
    PRINT_LLINT_VALUE ( convergence_duration, pPacket );
    return ;
}


void DebugAVPacketList ( AVPacketList *pPacketList )
{
    PRINT_STRUCT_VALUE ( ( & ( pPacketList->pkt ) ), AVPacket, ".pkt (%p)\n", & ( pPacketList->pkt ) );
    PRINT_POINT_VALUE ( next, pPacketList );
}
void DebugAVChapter ( AVChapter *pChapter )
{
    PRINT_INT_VALUE ( id, pChapter );
    PRINT_STRUCT_VALUE ( ( & ( pChapter->time_base ) ), AVRational, ".time_base (%p)\n", & ( pChapter->time_base ) );
    PRINT_LLINT_VALUE ( start, pChapter );
    PRINT_LLINT_VALUE ( end, pChapter );
    PRINT_NAME_VALUE ( title, pChapter );
}

void DebugAVProgram ( AVProgram* pProgram )
{
    PRINT_INT_VALUE ( id, pProgram );
    PRINT_NAME_VALUE ( provider_name, pProgram );
    PRINT_NAME_VALUE ( name, pProgram );
    PRINT_FLAGS_VALUE ( flags, pProgram );
    PRINT_UNSIGNED_INT_VALUE ( discard, pProgram );
    PRINT_DEBUG_INTS ( pProgram->stream_index, ( ( int ) pProgram->nb_stream_indexes ), "stream_index" );
}

void DebugAVPanScan ( AVPanScan *pScan )
{
    int i, j;
    PRINT_INT_VALUE ( id, pScan );
    PRINT_INT_VALUE ( width, pScan );
    PRINT_INT_VALUE ( height, pScan );
    for ( i = 0; i < 3; i++ )
    {
        for ( j = 0; j < 2; j++ )
        {
            PRINT_DEBUG_VALUE ( "pScan->position[%d][%d] %d\n", i, j, pScan->position[i][j] );
        }
    }
    return;
}

void DebugAVCodec ( AVCodec *pCodec )
{
    PRINT_NAME_VALUE ( name, pCodec );
    PRINT_UNSIGNED_INT_VALUE ( type, pCodec );
    PRINT_UNSIGNED_INT_VALUE ( id, pCodec );
    PRINT_INT_VALUE ( priv_data_size, pCodec );
    PRINT_FUNC_POINTER ( init, pCodec );
    PRINT_FUNC_POINTER ( encode, pCodec );
    PRINT_FUNC_POINTER ( close, pCodec );
    PRINT_FUNC_POINTER ( decode, pCodec );
    PRINT_INT_VALUE ( capabilities, pCodec );
    PRINT_POINT_VALUE ( next, pCodec );
    PRINT_FUNC_POINTER ( flush, pCodec );
    PRINT_STRUCT_VALUE ( pCodec->supported_framerates, AVRational, ".supported_framerates (%p)\n", pCodec->supported_framerates );
    PRINT_POINT_VALUE ( pix_fmts, pCodec );
    PRINT_NAME_VALUE ( long_name, pCodec );
    PRINT_POINT_VALUE ( supported_samplerates, pCodec );
    PRINT_POINT_VALUE ( channel_layouts, pCodec );
    return ;
}

void DebugRcOverride ( RcOverride* pOver )
{
    PRINT_INT_VALUE ( start_frame, pOver );
    PRINT_INT_VALUE ( end_frame, pOver );
    PRINT_INT_VALUE ( qscale, pOver );
    PRINT_FLOAT_VALUE ( quality_factor, pOver );
}

void DebugAVFrame ( AVFrame* pFrame )
{
    int i;
    PRINT_DEBUG_CHARS ( pFrame->data, 4, "data" );
    PRINT_DEBUG_INTS ( pFrame->linesize, 4, "linesize" );
    for ( i = 0; i < ( int ) ( sizeof ( pFrame->base ) / sizeof ( pFrame->base[0] ) ); i++ )
    {
        PRINT_DEBUG_VALUE ( ".base[%d] %p\n", i, pFrame->base[i] );
    }
    PRINT_INT_VALUE ( key_frame, pFrame );
    PRINT_INT_VALUE ( pict_type, pFrame );
    PRINT_LLINT_VALUE ( pts, pFrame );
    PRINT_INT_VALUE ( coded_picture_number, pFrame );
    PRINT_INT_VALUE ( display_picture_number, pFrame );
    PRINT_INT_VALUE ( quality, pFrame );
    PRINT_INT_VALUE ( age, pFrame );
    PRINT_INT_VALUE ( reference, pFrame );
    PRINT_POINT_VALUE ( qscale_table, pFrame );
    PRINT_INT_VALUE ( qstride, pFrame );
    PRINT_POINT_VALUE ( mbskip_table, pFrame );
    for ( i = 0; i < 2; i++ )
    {
        PRINT_DEBUG_VALUE ( "pFrame->motion_val[%d] (%p)\n", i, pFrame->motion_val[i] );
    }

    PRINT_POINT_VALUE ( mb_type, pFrame );
    PRINT_INT_VALUE ( motion_subsample_log2, pFrame );
    PRINT_POINT_VALUE ( opaque, pFrame );
    PRINT_DEBUG_LLINTS ( pFrame->error, ( ( int ) ( sizeof ( pFrame->error ) / sizeof ( pFrame->error[0] ) ) ), "pFrame->error" );
    PRINT_INT_VALUE ( type, pFrame );
    PRINT_INT_VALUE ( repeat_pict, pFrame );
    PRINT_INT_VALUE ( qscale_type, pFrame );
    PRINT_INT_VALUE ( interlaced_frame, pFrame );
    PRINT_INT_VALUE ( top_field_first, pFrame );
    PRINT_POINT_VALUE ( pan_scan, pFrame );
    if ( ( int ) pFrame->pan_scan > 0x90000 )
    {
        st_Tabsset ++;
        DebugAVPanScan ( pFrame->pan_scan );
        st_Tabsset --;
    }
//    PRINT_STRUCT_VALUE ( pFrame->pan_scan, AVPanScan, "pFrame->pan_scan (%p)\n", pFrame->pan_scan );
    PRINT_INT_VALUE ( palette_has_changed, pFrame );
    PRINT_INT_VALUE ( buffer_hints, pFrame );
    PRINT_POINT_VALUE ( dct_coeff, pFrame );
    for ( i = 0; i < 2; i++ )
    {
        PRINT_DEBUG_VALUE ( ".ref_index[%d] (%p)\n", i, pFrame->ref_index[i] );
    }
    PRINT_LLINT_VALUE ( reordered_opaque, pFrame );
    return ;
}

void DebugAVPaletteControl ( void *pPalette )
{
    return ;
}

void DebugAVCodecContext ( AVCodecContext* pCodec )
{
    PRINT_STRUCT_VALUE ( pCodec->av_class, AVClass, ".av_class (%p)\n", pCodec->av_class );
    PRINT_INT_VALUE ( bit_rate, pCodec );
    PRINT_INT_VALUE ( bit_rate_tolerance, pCodec );
    PRINT_FLAGS_VALUE ( flags, pCodec );
    PRINT_INT_VALUE ( sub_id, pCodec );
    PRINT_INT_VALUE ( me_method, pCodec );
    PRINT_DEBUG_CHARS ( pCodec->extradata, pCodec->extradata_size, "extradata" );
    PRINT_STRUCT_VALUE ( ( & ( pCodec->time_base ) ), AVRational, ".time_base (%p)\n", ( & ( pCodec->time_base ) ) );
    PRINT_INT_VALUE ( width, pCodec );
    PRINT_INT_VALUE ( height, pCodec );
    PRINT_INT_VALUE ( gop_size, pCodec );
    PRINT_UNSIGNED_INT_VALUE ( pix_fmt, pCodec );
    PRINT_INT_VALUE ( rate_emu, pCodec );
    PRINT_POINT_VALUE ( draw_horiz_band, pCodec );
    PRINT_INT_VALUE ( sample_rate, pCodec );
    PRINT_INT_VALUE ( channels, pCodec );
    PRINT_UNSIGNED_INT_VALUE ( sample_fmt, pCodec );
    PRINT_INT_VALUE ( frame_size, pCodec );
    PRINT_INT_VALUE ( frame_number, pCodec );
    PRINT_INT_VALUE ( real_pict_num, pCodec );
    PRINT_INT_VALUE ( delay, pCodec );
    PRINT_FLOAT_VALUE ( qcompress, pCodec );
    PRINT_FLOAT_VALUE ( qblur, pCodec );
    PRINT_INT_VALUE ( qmin, pCodec );
    PRINT_INT_VALUE ( qmax, pCodec );
    PRINT_INT_VALUE ( max_qdiff, pCodec );
    PRINT_INT_VALUE ( max_b_frames, pCodec );
    PRINT_FLOAT_VALUE ( b_quant_factor, pCodec );
    PRINT_INT_VALUE ( rc_strategy, pCodec );
    PRINT_INT_VALUE ( b_frame_strategy, pCodec );
    PRINT_INT_VALUE ( hurry_up, pCodec );
    PRINT_FILL_STRUCT_VALUE ( pCodec->codec, AVCodec, ".codec (%p)\n", pCodec->codec );
    PRINT_POINT_VALUE ( priv_data, pCodec );
    PRINT_INT_VALUE ( rtp_payload_size, pCodec );
    PRINT_POINT_VALUE ( rtp_callback, pCodec );
    PRINT_INT_VALUE ( mv_bits, pCodec );
    PRINT_INT_VALUE ( header_bits, pCodec );
    PRINT_INT_VALUE ( i_tex_bits, pCodec );
    PRINT_INT_VALUE ( p_tex_bits, pCodec );
    PRINT_INT_VALUE ( i_count, pCodec );
    PRINT_INT_VALUE ( p_count, pCodec );
    PRINT_INT_VALUE ( skip_count, pCodec );
    PRINT_INT_VALUE ( misc_bits, pCodec );
    PRINT_INT_VALUE ( frame_bits, pCodec );
    PRINT_POINT_VALUE ( opaque, pCodec );
    PRINT_NAME_VALUE ( codec_name, pCodec );
    PRINT_UNSIGNED_INT_VALUE ( codec_type, pCodec );
    PRINT_UNSIGNED_INT_VALUE ( codec_id, pCodec );
    PRINT_UNSIGNED_INT_VALUE ( codec_tag, pCodec );
    PRINT_INT_VALUE ( workaround_bugs, pCodec );
    PRINT_INT_VALUE ( luma_elim_threshold, pCodec );
    PRINT_INT_VALUE ( chroma_elim_threshold, pCodec );
    PRINT_INT_VALUE ( strict_std_compliance, pCodec );
    PRINT_FLOAT_VALUE ( b_quant_offset, pCodec );
    PRINT_INT_VALUE ( error_recognition, pCodec );
    PRINT_FUNC_POINTER ( get_buffer, pCodec );
    PRINT_FUNC_POINTER ( release_buffer, pCodec );
    PRINT_INT_VALUE ( has_b_frames, pCodec );
    PRINT_INT_VALUE ( block_align, pCodec );
    PRINT_INT_VALUE ( parse_only, pCodec );
    PRINT_INT_VALUE ( mpeg_quant, pCodec );
    PRINT_NAME_VALUE ( stats_out, pCodec );
    PRINT_NAME_VALUE ( stats_in, pCodec );
    PRINT_FLOAT_VALUE ( rc_qsquish, pCodec );
    PRINT_FLOAT_VALUE ( rc_qmod_amp, pCodec );
    PRINT_INT_VALUE ( rc_qmod_freq, pCodec );
    PRINT_STRUCT_VALUE ( pCodec->rc_override, RcOverride, ".rc_override (%p)\n", pCodec->rc_override );
    PRINT_INT_VALUE ( rc_override_count, pCodec );
    PRINT_NAME_VALUE ( rc_eq, pCodec );
    PRINT_INT_VALUE ( rc_max_rate, pCodec );
    PRINT_INT_VALUE ( rc_min_rate, pCodec );
    PRINT_INT_VALUE ( rc_buffer_size, pCodec );
    PRINT_FLOAT_VALUE ( rc_buffer_aggressivity, pCodec );
    PRINT_FLOAT_VALUE ( i_quant_factor, pCodec );
    PRINT_FLOAT_VALUE ( i_quant_offset, pCodec );
    PRINT_FLOAT_VALUE ( rc_initial_cplx, pCodec );
    PRINT_INT_VALUE ( dct_algo, pCodec );
    PRINT_FLOAT_VALUE ( lumi_masking, pCodec );
    PRINT_FLOAT_VALUE ( temporal_cplx_masking, pCodec );
    PRINT_FLOAT_VALUE ( spatial_cplx_masking, pCodec );
    PRINT_FLOAT_VALUE ( p_masking, pCodec );
    PRINT_FLOAT_VALUE ( dark_masking, pCodec );
    PRINT_INT_VALUE ( idct_algo, pCodec );
    PRINT_DEBUG_INTS ( pCodec->slice_offset, pCodec->slice_count, "slice_offset" );
    PRINT_UNSIGNED_INT_VALUE ( dsp_mask, pCodec );
    PRINT_INT_VALUE ( bits_per_coded_sample, pCodec );
    PRINT_INT_VALUE ( prediction_method, pCodec );
    PRINT_STRUCT_VALUE ( ( & ( pCodec->sample_aspect_ratio ) ), AVRational, ".sample_aspect_ratio (%p)\n", ( & ( pCodec->sample_aspect_ratio ) ) );
    PRINT_STRUCT_VALUE ( ( & ( pCodec->coded_frame ) ), AVFrame, ".coded_frame (%p)\n", ( & ( pCodec->coded_frame ) ) );
    PRINT_INT_VALUE ( debug, pCodec );
    PRINT_INT_VALUE ( debug_mv, pCodec );
    PRINT_DEBUG_LLINTS ( pCodec->error, 4, "error" );
    PRINT_INT_VALUE ( mb_qmin, pCodec );
    PRINT_INT_VALUE ( mb_qmax, pCodec );
    PRINT_INT_VALUE ( me_cmp, pCodec );
    PRINT_INT_VALUE ( me_sub_cmp, pCodec );
    PRINT_INT_VALUE ( mb_cmp, pCodec );
    PRINT_INT_VALUE ( ildct_cmp, pCodec );
    PRINT_INT_VALUE ( dia_size, pCodec );
    PRINT_INT_VALUE ( last_predictor_count, pCodec );
    PRINT_INT_VALUE ( pre_me, pCodec );
    PRINT_INT_VALUE ( me_pre_cmp, pCodec );
    PRINT_INT_VALUE ( pre_dia_size, pCodec );
    PRINT_INT_VALUE ( me_subpel_quality, pCodec );
    PRINT_POINT_VALUE ( get_format, pCodec );
    PRINT_INT_VALUE ( dtg_active_format, pCodec );
    PRINT_INT_VALUE ( me_range, pCodec );
    PRINT_INT_VALUE ( intra_quant_bias, pCodec );
    PRINT_INT_VALUE ( inter_quant_bias, pCodec );
    PRINT_INT_VALUE ( color_table_id, pCodec );
    PRINT_INT_VALUE ( internal_buffer_count, pCodec );
    PRINT_POINT_VALUE ( internal_buffer, pCodec );
    PRINT_INT_VALUE ( global_quality, pCodec );
    PRINT_INT_VALUE ( coder_type, pCodec );
    PRINT_INT_VALUE ( context_model, pCodec );
    PRINT_INT_VALUE ( slice_flags, pCodec );
    PRINT_INT_VALUE ( xvmc_acceleration, pCodec );
    PRINT_INT_VALUE ( mb_decision, pCodec );
    PRINT_POINT_VALUE ( intra_matrix, pCodec );
    PRINT_POINT_VALUE ( inter_matrix, pCodec );
    PRINT_UNSIGNED_INT_VALUE ( stream_codec_tag, pCodec );
    PRINT_INT_VALUE ( scenechange_threshold, pCodec );
    PRINT_INT_VALUE ( lmin, pCodec );
    PRINT_INT_VALUE ( lmax, pCodec );
    PRINT_FILL_STRUCT_VALUE ( pCodec->palctrl, AVPaletteControl, "palctrl" );
    PRINT_INT_VALUE ( noise_reduction, pCodec );
    PRINT_POINT_VALUE ( reget_buffer, pCodec );
    PRINT_INT_VALUE ( rc_initial_buffer_occupancy, pCodec );
    PRINT_INT_VALUE ( inter_threshold, pCodec );
    PRINT_FLAGS_VALUE ( flags2, pCodec );
    PRINT_INT_VALUE ( error_rate, pCodec );
    PRINT_INT_VALUE ( antialias_algo, pCodec );
    PRINT_INT_VALUE ( quantizer_noise_shaping, pCodec );
    PRINT_INT_VALUE ( thread_count, pCodec );
    PRINT_FUNC_POINTER ( execute, pCodec );
    PRINT_POINT_VALUE ( thread_opaque, pCodec );
    PRINT_INT_VALUE ( me_threshold, pCodec );
    PRINT_INT_VALUE ( mb_threshold, pCodec );
    PRINT_INT_VALUE ( intra_dc_precision, pCodec );
    PRINT_INT_VALUE ( nsse_weight, pCodec );
    PRINT_INT_VALUE ( skip_top, pCodec );
    PRINT_INT_VALUE ( skip_bottom, pCodec );
    PRINT_INT_VALUE ( profile, pCodec );
    PRINT_INT_VALUE ( level, pCodec );
    PRINT_INT_VALUE ( lowres, pCodec );
    PRINT_INT_VALUE ( coded_width, pCodec );
    PRINT_INT_VALUE ( coded_height, pCodec );
    PRINT_INT_VALUE ( frame_skip_threshold, pCodec );
    PRINT_INT_VALUE ( frame_skip_factor, pCodec );
    PRINT_INT_VALUE ( frame_skip_exp, pCodec );
    PRINT_INT_VALUE ( frame_skip_cmp, pCodec );
    PRINT_FLOAT_VALUE ( border_masking, pCodec );
    PRINT_INT_VALUE ( mb_lmin, pCodec );
    PRINT_INT_VALUE ( mb_lmax, pCodec );
    PRINT_INT_VALUE ( me_penalty_compensation, pCodec );
    PRINT_UNSIGNED_INT_VALUE ( skip_loop_filter, pCodec );
    PRINT_UNSIGNED_INT_VALUE ( skip_idct, pCodec );
    PRINT_UNSIGNED_INT_VALUE ( skip_frame, pCodec );
    PRINT_INT_VALUE ( bidir_refine, pCodec );
    PRINT_INT_VALUE ( brd_scale, pCodec );
    PRINT_FLOAT_VALUE ( crf, pCodec );
    PRINT_INT_VALUE ( cqp, pCodec );
    PRINT_INT_VALUE ( keyint_min, pCodec );
    PRINT_INT_VALUE ( refs, pCodec );
    PRINT_INT_VALUE ( chromaoffset, pCodec );
    PRINT_INT_VALUE ( bframebias, pCodec );
    PRINT_INT_VALUE ( trellis, pCodec );
    PRINT_FLOAT_VALUE ( complexityblur, pCodec );
    PRINT_INT_VALUE ( deblockalpha, pCodec );
    PRINT_INT_VALUE ( deblockbeta, pCodec );
    PRINT_INT_VALUE ( partitions, pCodec );
    PRINT_INT_VALUE ( directpred, pCodec );
    PRINT_INT_VALUE ( cutoff, pCodec );
    PRINT_INT_VALUE ( scenechange_factor, pCodec );
    PRINT_INT_VALUE ( mv0_threshold, pCodec );
    PRINT_INT_VALUE ( b_sensitivity, pCodec );
    PRINT_INT_VALUE ( compression_level, pCodec );
    PRINT_INT_VALUE ( use_lpc, pCodec );
    PRINT_INT_VALUE ( lpc_coeff_precision, pCodec );
    PRINT_INT_VALUE ( min_prediction_order, pCodec );
    PRINT_INT_VALUE ( max_prediction_order, pCodec );
    PRINT_INT_VALUE ( prediction_order_method, pCodec );
    PRINT_INT_VALUE ( min_partition_order, pCodec );
    PRINT_INT_VALUE ( max_partition_order, pCodec );
    PRINT_LLINT_VALUE ( timecode_frame_start, pCodec );
    PRINT_FLOAT_VALUE ( drc_scale, pCodec );
    PRINT_LLINT_VALUE ( reordered_opaque, pCodec );
    PRINT_INT_VALUE ( bits_per_raw_sample, pCodec );
    PRINT_LLINT_VALUE ( channel_layout, pCodec );
    PRINT_LLINT_VALUE ( request_channel_layout, pCodec );
    PRINT_FLOAT_VALUE ( rc_max_available_vbv_use, pCodec );
    PRINT_FLOAT_VALUE ( rc_min_vbv_overflow_use, pCodec );
    return ;
}


void DebugAVFrac ( struct AVFrac* pFrac )
{
    PRINT_LLINT_VALUE ( val, pFrac );
    PRINT_LLINT_VALUE ( num, pFrac );
    PRINT_LLINT_VALUE ( den, pFrac );
}

void DebugAVCodecParser ( AVCodecParser *pParser )
{
    PRINT_DEBUG_INTS ( pParser->codec_ids, ( ( int ) ( sizeof ( pParser->codec_ids ) / sizeof ( pParser->codec_ids[0] ) ) ), "codec_ids" );
    PRINT_INT_VALUE ( priv_data_size, pParser );
    PRINT_FUNC_POINTER ( parser_init, pParser );
    PRINT_FUNC_POINTER ( parser_parse, pParser );
    PRINT_FUNC_POINTER ( parser_close, pParser );
    PRINT_FUNC_POINTER ( split, pParser );
    PRINT_POINT_VALUE ( next, pParser );
    return ;
}

void DebugAVCodecParserContext ( struct AVCodecParserContext *pCoCtx )
{

    PRINT_POINT_VALUE ( priv_data, pCoCtx );
    PRINT_FILL_STRUCT_VALUE ( pCoCtx->parser, AVCodecParser, ".parser (%p)\n", pCoCtx->parser );
    PRINT_LLINT_VALUE ( frame_offset, pCoCtx );
    PRINT_LLINT_VALUE ( cur_offset, pCoCtx );
    PRINT_LLINT_VALUE ( next_frame_offset, pCoCtx );
    PRINT_INT_VALUE ( pict_type, pCoCtx );
    PRINT_LLINT_VALUE ( pts, pCoCtx );
    PRINT_LLINT_VALUE ( dts, pCoCtx );
    PRINT_LLINT_VALUE ( last_pts, pCoCtx );
    PRINT_LLINT_VALUE ( last_dts, pCoCtx );
    PRINT_INT_VALUE ( fetch_timestamp, pCoCtx );
    PRINT_INT_VALUE ( cur_frame_start_index, pCoCtx );
    PRINT_DEBUG_LLINTS ( pCoCtx->cur_frame_offset, AV_PARSER_PTS_NB, "cur_frame_offset" );
    PRINT_DEBUG_LLINTS ( pCoCtx->cur_frame_pts, AV_PARSER_PTS_NB, "cur_frame_pts" );
    PRINT_DEBUG_LLINTS ( pCoCtx->cur_frame_dts, AV_PARSER_PTS_NB, "cur_frame_dts" );
    PRINT_FLAGS_VALUE ( flags, pCoCtx );
    PRINT_LLINT_VALUE ( offset, pCoCtx );
    PRINT_DEBUG_LLINTS ( pCoCtx->cur_frame_end, AV_PARSER_PTS_NB, "cur_frame_end" );

}

void DebugAVIndexEntry ( AVIndexEntry *pIdxEntry )
{
    PRINT_LLINT_VALUE ( pos, pIdxEntry );
    PRINT_LLINT_VALUE ( timestamp, pIdxEntry );
    PRINT_FLAGS_VALUE ( flags, pIdxEntry );
    PRINT_INT_VALUE ( size, pIdxEntry );
    PRINT_INT_VALUE ( min_distance, pIdxEntry );
    return;
}

void DebugAVProbeData ( AVProbeData *pProbe )
{
    PRINT_NAME_VALUE ( filename, pProbe );
    PRINT_DEBUG_CHARS ( pProbe->buf, pProbe->buf_size, "buf" );
    return ;
}


void DebugAVStream ( AVStream *pStream )
{
    PRINT_INT_VALUE ( index, pStream );
    PRINT_INT_VALUE ( id, pStream );
    PRINT_STRUCT_VALUE ( pStream->codec, AVCodecContext, ".codec (%p)\n", pStream->codec );
    PRINT_STRUCT_VALUE ( ( & ( pStream->r_frame_rate ) ), AVRational, ".r_frame_rate (%p)\n", & ( pStream->r_frame_rate ) );
    PRINT_POINT_VALUE ( priv_data, pStream );
    PRINT_LLINT_VALUE ( first_dts, pStream );
    PRINT_FILL_STRUCT_VALUE ( ( & ( pStream->pts ) ), AVFrac, ".pts (%p)\n", & ( pStream->pts ) );
    PRINT_STRUCT_VALUE ( ( & ( pStream->time_base ) ), AVRational, ".time_base (%p)\n", & ( pStream->time_base ) );
    PRINT_INT_VALUE ( pts_wrap_bits, pStream );
    PRINT_INT_VALUE ( stream_copy, pStream );
    PRINT_UNSIGNED_INT_VALUE ( discard, pStream );
    PRINT_FLOAT_VALUE ( quality, pStream );
    PRINT_LLINT_VALUE ( start_time, pStream );
    PRINT_LLINT_VALUE ( duration, pStream );
    PRINT_NAME_VALUE ( language, pStream );
    PRINT_UNSIGNED_INT_VALUE ( need_parsing, pStream );
    PRINT_FILL_STRUCT_VALUE ( pStream->parser, AVCodecParserContext, ".parser (%p)\n", pStream->parser );
    PRINT_LLINT_VALUE ( cur_dts, pStream );
    PRINT_INT_VALUE ( last_IP_duration, pStream );
    PRINT_LLINT_VALUE ( last_IP_pts, pStream );
    PRINT_STRUCT_VALUE ( pStream->index_entries, AVIndexEntry, ".index_entries (%p)\n", pStream->index_entries );
    PRINT_INT_VALUE ( nb_index_entries, pStream );
    PRINT_UNSIGNED_INT_VALUE ( index_entries_allocated_size, pStream );
    PRINT_LLINT_VALUE ( nb_frames, pStream );
    PRINT_NAME_VALUE ( filename, pStream );
    PRINT_INT_VALUE ( disposition, pStream );
    PRINT_STRUCT_VALUE ( ( & ( pStream->probe_data ) ), AVProbeData, ".probe_data (%p)\n", ( & ( pStream->probe_data ) ) );
    PRINT_DEBUG_LLINTS ( pStream->pts_buffer, MAX_REORDER_DELAY, "pts_buffer" );
    PRINT_STRUCT_VALUE ( ( & ( pStream->sample_aspect_ratio ) ), AVRational, ".sample_aspect_ratio (%p)\n", & ( pStream->sample_aspect_ratio ) );
}

void DebugAVCodecTag ( struct AVCodecTag* pTag )
{
    PRINT_UNSIGNED_INT_VALUE ( id, pTag );
    PRINT_UNSIGNED_INT_VALUE ( tag, pTag );
    return ;
}

void DebugAVInputFormat ( AVInputFormat* pInput )
{
    PRINT_NAME_VALUE ( name, pInput );
    PRINT_NAME_VALUE ( long_name, pInput );
    PRINT_INT_VALUE ( priv_data_size, pInput );
    PRINT_FUNC_POINTER ( read_probe, pInput );
    PRINT_FUNC_POINTER ( read_header, pInput );
    PRINT_FUNC_POINTER ( read_packet, pInput );
    PRINT_FUNC_POINTER ( read_close, pInput );
    PRINT_FUNC_POINTER ( read_seek, pInput );
    PRINT_FUNC_POINTER ( read_timestamp, pInput );
    PRINT_FLAGS_VALUE ( flags, pInput );
    PRINT_NAME_VALUE ( extensions, pInput );
    PRINT_INT_VALUE ( value, pInput );
    PRINT_FUNC_POINTER ( read_play, pInput );
    PRINT_FUNC_POINTER ( read_pause, pInput );
    PRINT_FILL_STRUCT_VALUE ( pInput->codec_tag, AVCodecTag, ".codec_tag (%p)\n", pInput->codec_tag );
    PRINT_POINT_VALUE ( next, pInput );
    return ;
}

void DebugAVOutputFormat ( AVOutputFormat *pOutput )
{
    PRINT_NAME_VALUE ( name, pOutput );
    PRINT_NAME_VALUE ( long_name, pOutput );
    PRINT_NAME_VALUE ( mime_type, pOutput );
    PRINT_NAME_VALUE ( extensions, pOutput );
    PRINT_INT_VALUE ( priv_data_size, pOutput );
    PRINT_UNSIGNED_INT_VALUE ( audio_codec, pOutput );
    PRINT_UNSIGNED_INT_VALUE ( video_codec, pOutput );
    PRINT_POINT_VALUE ( write_header, pOutput );
    PRINT_POINT_VALUE ( write_packet, pOutput );
    PRINT_POINT_VALUE ( write_trailer, pOutput );
    PRINT_FLAGS_VALUE ( flags, pOutput );
    PRINT_POINT_VALUE ( set_parameters, pOutput );
    PRINT_POINT_VALUE ( interleave_packet, pOutput );
    PRINT_FILL_STRUCT_VALUE ( pOutput->codec_tag, AVCodecTag, ".codec_tag (%p)\n", pOutput->codec_tag );
    PRINT_UNSIGNED_INT_VALUE ( subtitle_codec, pOutput );
    PRINT_POINT_VALUE ( next, pOutput );
    return ;
}

void DebugByteIOContext ( ByteIOContext *pByte )
{
    PRINT_DEBUG_CHARS ( pByte->buffer, pByte->buffer_size, "buffer" );
    PRINT_POINT_VALUE ( buf_ptr, pByte );
    PRINT_POINT_VALUE ( buf_end, pByte );
    PRINT_POINT_VALUE ( opaque, pByte );
    PRINT_FUNC_POINTER ( read_packet, pByte );
    PRINT_FUNC_POINTER ( write_packet, pByte );
    PRINT_FUNC_POINTER ( seek, pByte );
    PRINT_LLINT_VALUE ( pos, pByte );
    PRINT_INT_VALUE ( must_flush, pByte );
    PRINT_INT_VALUE ( eof_reached, pByte );
    PRINT_FLAGS_VALUE ( write_flag, pByte );
    PRINT_INT_VALUE ( is_streamed, pByte );
    PRINT_INT_VALUE ( max_packet_size, pByte );
    PRINT_LONG_VALUE ( checksum, pByte );
    PRINT_POINT_VALUE ( checksum_ptr, pByte );
    PRINT_FUNC_POINTER ( update_checksum, pByte );
    PRINT_INT_VALUE ( error, pByte );
    PRINT_FUNC_POINTER ( read_pause, pByte );
    PRINT_FUNC_POINTER ( read_seek, pByte );
    return ;
}


void DebugAvFormatContext ( AVFormatContext *pFormatCtx )
{
    int i;
    PRINT_STRUCT_VALUE ( pFormatCtx->av_class, AVClass, ".av_class (%p)\n", pFormatCtx->av_class );
    PRINT_STRUCT_VALUE ( pFormatCtx->iformat, AVInputFormat, ".iformat (%p)\n", pFormatCtx->iformat );
    PRINT_STRUCT_VALUE ( pFormatCtx->oformat, AVOutputFormat, ".oformat (%p)\n", pFormatCtx->oformat );
    PRINT_POINT_VALUE ( priv_data, pFormatCtx );
    PRINT_STRUCT_VALUE ( pFormatCtx->pb, ByteIOContext, ".pb (%p)\n", pFormatCtx->pb );
    PRINT_UNSIGNED_INT_VALUE ( nb_streams, pFormatCtx );
    for ( i = 0; i < ( int ) pFormatCtx->nb_streams; i++ )
    {
        AVStream* pStream = pFormatCtx->streams[i];
        PRINT_STRUCT_VALUE ( pStream, AVStream, ".streams[%d] %p\n", i, pStream );
    }
    PRINT_NAME_VALUE ( filename, pFormatCtx );
    PRINT_LLINT_VALUE ( timestamp, pFormatCtx );
    PRINT_NAME_VALUE ( title, pFormatCtx );
    PRINT_NAME_VALUE ( author, pFormatCtx );
    PRINT_NAME_VALUE ( copyright, pFormatCtx );
    PRINT_NAME_VALUE ( comment, pFormatCtx );
    PRINT_NAME_VALUE ( album, pFormatCtx );
    PRINT_INT_VALUE ( year, pFormatCtx );
    PRINT_INT_VALUE ( track, pFormatCtx );
    PRINT_DEBUG_VALUE ( ".track %d\n", pFormatCtx->track );
    PRINT_DEBUG_CHARS ( pFormatCtx->genre, 32, "genre" );
    PRINT_FLAGS_VALUE ( ctx_flags, pFormatCtx );
    PRINT_FILL_STRUCT_VALUE ( pFormatCtx->packet_buffer, AVPacketList, ".packet_buffer (%p)\n", pFormatCtx->packet_buffer );
    PRINT_LLINT_VALUE ( start_time, pFormatCtx );
    PRINT_LLINT_VALUE ( duration, pFormatCtx );
    PRINT_LLINT_VALUE ( file_size, pFormatCtx );
    PRINT_INT_VALUE ( bit_rate, pFormatCtx );
    PRINT_STRUCT_VALUE ( pFormatCtx->cur_st, AVStream, ".cur_st (%p)\n", pFormatCtx->cur_st );
    PRINT_DEBUG_CHARS ( pFormatCtx->cur_ptr, pFormatCtx->cur_len, "cur_ptr" );

    PRINT_STRUCT_VALUE ( ( & ( pFormatCtx->cur_pkt ) ), AVPacket, ".cur_pkt (%p)\n", & ( pFormatCtx->cur_pkt ) );
    PRINT_LLINT_VALUE ( data_offset, pFormatCtx );
    PRINT_INT_VALUE ( index_built, pFormatCtx );
    PRINT_INT_VALUE ( mux_rate, pFormatCtx );
    PRINT_INT_VALUE ( packet_size, pFormatCtx );
    PRINT_INT_VALUE ( preload, pFormatCtx );
    PRINT_INT_VALUE ( max_delay, pFormatCtx );
    PRINT_INT_VALUE ( loop_output, pFormatCtx );
    PRINT_FLAGS_VALUE ( flags, pFormatCtx );
    PRINT_INT_VALUE ( loop_input, pFormatCtx );
    PRINT_INT_VALUE ( probesize, pFormatCtx );
    PRINT_INT_VALUE ( max_analyze_duration, pFormatCtx );
    PRINT_DEBUG_CHARS ( pFormatCtx->key, pFormatCtx->keylen, "key" );
    PRINT_UNSIGNED_INT_VALUE ( nb_programs, pFormatCtx );
    for ( i = 0; i < ( int ) pFormatCtx->nb_programs; i++ )
    {
        AVProgram* pProgram = pFormatCtx->programs[i];
        PRINT_STRUCT_VALUE ( pProgram, AVProgram, ".programs[%d] (%p)\n", i, pProgram );
    }

    PRINT_UNSIGNED_INT_VALUE ( video_codec_id, pFormatCtx );
    PRINT_UNSIGNED_INT_VALUE ( audio_codec_id, pFormatCtx );
    PRINT_UNSIGNED_INT_VALUE ( subtitle_codec_id, pFormatCtx );
    PRINT_UNSIGNED_INT_VALUE ( max_index_size, pFormatCtx );
    PRINT_UNSIGNED_INT_VALUE ( max_picture_buffer, pFormatCtx );
    PRINT_UNSIGNED_INT_VALUE ( nb_chapters, pFormatCtx );
    for ( i = 0; i < ( int ) pFormatCtx->nb_chapters; i++ )
    {
        AVChapter *pChapter = pFormatCtx->chapters[i];
        PRINT_STRUCT_VALUE ( pChapter, AVChapter, ".chapters[%d] (%p)\n", i, pChapter );
    }
    PRINT_INT_VALUE ( debug, pFormatCtx );
    PRINT_FILL_STRUCT_VALUE ( pFormatCtx->raw_packet_buffer, AVPacketList, ".raw_packet_buffer (%p)\n", pFormatCtx->raw_packet_buffer );
    PRINT_FILL_STRUCT_VALUE ( pFormatCtx->raw_packet_buffer_end, AVPacketList, ".raw_packet_buffer_end (%p)\n", pFormatCtx->raw_packet_buffer_end );
    PRINT_FILL_STRUCT_VALUE ( pFormatCtx->packet_buffer_end, AVPacketList, ".packet_buffer_end (%p)\n", pFormatCtx->packet_buffer_end );
    return ;
}

/* new types */
//typedef enum bool{false=0,true};
//typedef enum bool bool;
static bool GetNextFrame ( AVFormatContext *pFormatCtx, AVCodecContext *pCodecCtx, int videoStream, AVFrame *pFrame )
{
    static AVPacket packet;
    static int      bytesRemaining = 0;
    static uint8_t  *rawData;
    static bool     fFirstTime = true;
    int             bytesDecoded;
    int             frameFinished;
    // First time we're called, set packet.data to NULL to indicate it
    // doesn't have to be freed
    if ( fFirstTime )
    {
        fFirstTime = false;
        packet.data = NULL;
    }
    DebugAvFormatContext ( pFormatCtx );
    // Decode packets until we have decoded a complete frame
    while ( true )
    {
        DEBUG_PRINT ( "byteRemaining %d\n", bytesRemaining );
        // Work on the current packet until we have decoded all of it
        while ( bytesRemaining > 0 )
        {
            // Decode the next chunk of data
            bytesDecoded = avcodec_decode_video ( pCodecCtx, pFrame,
                                                  &frameFinished, rawData, bytesRemaining );
            // Was there an error?
            if ( bytesDecoded < 0 )
            {
                fprintf ( stderr, "Error while decoding frame\n" );
                return false;
            }
            bytesRemaining -= bytesDecoded;
            rawData += bytesDecoded;
            // Did we finish the current frame? Then we can return
            if ( frameFinished )
            {
                DEBUG_PRINT ( "finished\n" );
                return true;
            }
        }
        DEBUG_PRINT ( "byteRemaining %d\n", bytesRemaining );
        // Read the next packet, skipping all packets that aren't for this
        // stream
        do
        {
            // Free old packet
            if ( packet.data != NULL )
                av_free_packet ( &packet );
            // Read new packet
            if ( av_read_packet ( pFormatCtx, &packet ) < 0 )
            {
                DEBUG_PRINT ( "\n" );
                goto loop_exit;
            }
        }
        while ( packet.stream_index != videoStream );
        bytesRemaining = packet.size;
        DEBUG_PRINT ( "byteRemaining %d\n", bytesRemaining );
        rawData = packet.data;
    }
loop_exit:
    // Decode the rest of the last frame
    bytesDecoded = avcodec_decode_video ( pCodecCtx, pFrame, &frameFinished,
                                          rawData, bytesRemaining );
    // Free last packet
    if ( packet.data != NULL )
        av_free_packet ( &packet );
    DEBUG_PRINT ( "finished %d\n", frameFinished );
    return frameFinished != 0;
}
int main ( int argc, char*argv[] )
{
    AVFormatContext *pFormatCtx;
    int             i, videoStream;
    AVCodecContext  *pCodecCtx;
    AVCodec         *pCodec;
    AVFrame         *pFrame;
    AVFrame         *pFrameYUV;
    clock_t         t;
    double          fps;
    int                y_size, i_frame = 0;
    int                numBytes;
    uint8_t         *buffer;
    char* infile = argc > 1 ? argv[1] : ( char* ) "test.264";
    char* outfile = argc > 2 ? argv[2] : ( char* ) "out.yuv";
    FILE* fp = fopen ( outfile, "wb" );
    if ( fp == NULL )
    {
        fprintf ( stderr, "\nCan't open file %s!", infile );
        return -1;
    }
    // Register all formats and codecs
    av_register_all();
    // Open video file
    if ( av_open_input_file ( &pFormatCtx, infile, NULL, 0, NULL ) != 0 )
    {
        DEBUG_PRINT ( "\n" );
        return -1; // Couldn't open file
    }
    // Retrieve stream information
    if ( av_find_stream_info ( pFormatCtx ) < 0 )
    {
        DEBUG_PRINT ( "\n" );
        return -1; // Couldn't find stream information
    }
    // Dump information about file onto standard error
    dump_format ( pFormatCtx, 0, infile, false );
    t = clock();
    // Find the first video stream
    videoStream = -1;
    for ( i = 0; i < ( int ) pFormatCtx->nb_streams; i++ )
        if ( pFormatCtx->streams[i]->codec->codec_type == CODEC_TYPE_VIDEO )
        {
            DEBUG_PRINT ( "[%d]code_type %d codec_id %d\n", i, pFormatCtx->streams[i]->codec->codec_type,
                          pFormatCtx->streams[i]->codec->codec_id );
            videoStream = i;
            break;
        }
    if ( videoStream == -1 )
    {
        DEBUG_PRINT ( "\n" );
        return -1; // Didn't find a video stream
    }
    // Get a pointer to the codec context for the video stream
    pCodecCtx = pFormatCtx->streams[videoStream]->codec;
    // Find the decoder for the video stream
    pCodec = avcodec_find_decoder ( pCodecCtx->codec_id );
    if ( pCodec == NULL )
    {
        DEBUG_PRINT ( "\n" );
        return -1; // Codec not found
    }
    DEBUG_PRINT ( "codec %s long name %s\n", pCodec->name, pCodec->long_name );
    // Inform the codec that we can handle truncated bitstreams -- i.e.,
    // bitstreams where frame boundaries can fall in the middle of packets
    if ( pCodec->capabilities & CODEC_CAP_TRUNCATED )
        pCodecCtx->flags |= CODEC_FLAG_TRUNCATED;
    // Open codec
    if ( avcodec_open ( pCodecCtx, pCodec ) < 0 )
    {
        DEBUG_PRINT ( "\n" );
        return -1; // Could not open codec
    }
    // Allocate video frame
    pFrame = avcodec_alloc_frame();
    // Allocate an AVFrame structure
    pFrameYUV = avcodec_alloc_frame();
    if ( pFrameYUV == NULL )
    {
        DEBUG_PRINT ( "\n" );
        return -1;
    }
    // Determine required buffer size and allocate buffer
    numBytes = avpicture_get_size ( PIX_FMT_YUV420P, pCodecCtx->width,
                                    pCodecCtx->height );
    buffer = ( uint8_t* ) malloc ( numBytes );
    // Assign appropriate parts of buffer to image planes in pFrameRGB
    avpicture_fill ( ( AVPicture * ) pFrameYUV, buffer, PIX_FMT_YUV420P,
                     pCodecCtx->width, pCodecCtx->height );
    // Read frames
    while ( GetNextFrame ( pFormatCtx, pCodecCtx, videoStream, pFrame ) )
    {
#if 0
        img_convert ( ( AVPicture * ) pFrameYUV, PIX_FMT_YUV420P, ( AVPicture* ) pFrame,
                      pCodecCtx->pix_fmt, pCodecCtx->width, pCodecCtx->height );
#else
        static struct SwsContext* img_convert_ctx = NULL;
        img_convert_ctx = sws_getContext ( pCodecCtx->width, pCodecCtx->height,
                                           pCodecCtx->pix_fmt, pCodecCtx->width, pCodecCtx->height,
                                           PIX_FMT_RGB24, SWS_BICUBIC, NULL, NULL, NULL );
        sws_scale ( img_convert_ctx, pFrame->data, pFrame->linesize,
                    0, pCodecCtx->height, pFrameYUV->data, pFrameYUV->linesize );
#endif
        i_frame++;
        y_size = pCodecCtx->width * pCodecCtx->height;
#if 1
        if ( i_frame == 1 ) //only output onr time
        {
            printf ( "\n:lolpFrame->linesize[0]=%d, pFrame->linesize[1]=%d, pFrame->linesize[2]=%d!\n",
                     pFrame->linesize[0], pFrame->linesize[1], pFrame->linesize[2] );
            printf ( "\n:lolpFrameYUV->linesize[0]=%d, pFrameYUV->linesize[1]=%d, pFrameYUV->linesize[2]=%d!",
                     pFrameYUV->linesize[0], pFrameYUV->linesize[1], pFrameYUV->linesize[2] );
        }
#endif
        fwrite ( pFrameYUV->data[0], 1, y_size, fp );
        fwrite ( pFrameYUV->data[1], 1, ( y_size / 4 ), fp );
        fwrite ( pFrameYUV->data[2], 1, ( y_size / 4 ), fp );
    }
    //calculate decode rate
    fclose ( fp );
    t = clock() - t;
    fps = ( double ) ( t ) / CLOCKS_PER_SEC;
    fps = i_frame / fps;
    printf ( "\n==>Decode rate %.4f fps!\n", fps );
    // Free the YUV image
    free ( buffer );
    av_free ( pFrameYUV );
    // Free the YUV frame
    av_free ( pFrame );
    // Close the codec
    avcodec_close ( pCodecCtx );
    // Close the video file
    av_close_input_file ( pFormatCtx );
    return 0;
}

