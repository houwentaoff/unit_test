#include <stdio.h>
#include <unistd.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include<sys/types.h>
#include<netinet/in.h>
#include <string.h>
#include <linux/if.h>
#include <linux/ethtool.h>
#include <linux/sockios.h>
#include <errno.h> 
#include "types.h"

#include <sys/ioctl.h>
#include "iav_drv.h"
#include "iav_drv_ex.h"
#include "ambas_imgproc_arch.h"
#include "ambas_vin.h"
#include "img_struct.h"
#include "img_dsp_interface.h"
#include "img_api.h"

#include "adj_params/mt9t002_adj_param.c"
#include "adj_params/mt9t002_aeb_param.c"

#define	IMGPROC_PARAM_PATH	"/etc/idsp"
#define ABS(a)	   (((int)(a) < 0) ? -(a) : (a))

struct sockaddr_in server, client;
char rev_buf[1024];
char snd_buf[1024];

static image_sensor_param_t app_param_image_sensor;
u16* raw_buff;
vignette_cal_t vig_detect_setup;
static iav_raw_info_t raw_info;
static u16 vignette_table[VIGNETTE_MAX_WIDTH * VIGNETTE_MAX_HEIGHT * 4] = {0};
blc_level_t blc;
int fd_lenshading = -1;
#define ROUND_UP(size, align) (((size) + ((align) - 1)) & ~((align) - 1))
static u8 hot_bpc_done, dark_bpc_done;
u8* dark_fpn_map_addr = NULL;
u8* hot_fpn_map_addr = NULL;
wb_gain_t cal_gain_l, cal_gain_h, ref_gain_h, ref_gain_l;
static u8 wb_h_done, wb_l_done;

enum
{
	VIG_CALIB = 1,
	DARK_CALIB = 2,
	HOT_CALIB = 3,
	AWB_CALIB_H = 4,
	AWB_CALIB_L = 5,
	WIFI_CONFIG = 6,
};

int creat_socket_server(char* host, int port)
{
	int socket_fd, on;
	struct timeval timeout = {10,0};

	if(host == NULL) {
		printf("host addr not specified\n");
		return -1;
	}
	server.sin_family = AF_INET;
	server.sin_port = htons(port);
	server.sin_addr.s_addr = inet_addr(host);
	
	if((socket_fd = socket(AF_INET, SOCK_STREAM, 0))<0) {
		perror("listen socket uninit\n");
		return -1;
	}

	setsockopt(socket_fd, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout));
	setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(int) );
	printf("on %x\n", on);
	if((bind(socket_fd, (struct sockaddr *)&server, sizeof(server)))<0) {
		perror("cannot bind srv socket\n");
		return -1;
	}

	if(listen(socket_fd, SOMAXCONN)<0) {
		perror("cannot listen");
		close(socket_fd);
		return -1;
	}

	return socket_fd;
}

int load_dsp_cc_table(void)
{
	u8 reg[18752], matrix[17536];
	char filename[128];
	int file, count;

	sprintf(filename, "%s/reg.bin", IMGPROC_PARAM_PATH);
	if ((file = open(filename, O_RDONLY, 0)) < 0) {
		if ((file = open("reg.bin", O_RDONLY, 0)) < 0) {
			printf("reg.bin cannot be opened\n");
			return -1;
		}
	}
	if((count = read(file, reg, 18752)) != 18752) {
		printf("read reg.bin error\n");
		return -1;
	}
	close(file);

	sprintf(filename, "%s/3D.bin", IMGPROC_PARAM_PATH);
	if((file = open(filename, O_RDONLY, 0)) < 0) {
		if((file = open("3D.bin", O_RDONLY, 0)) < 0) {
			printf("reg.bin cannot be opened\n");
			return -1;
		}
	}
	if((count = read(file, matrix, 17536)) != 17536) {
		printf("read 3D.bin error\n");
		return -1;
	}
	close(file);
	img_dsp_load_color_correction_table((u32)reg, (u32)matrix);

	return 0;
}

int load_adj_cc_table(char * sensor_name)
{
	int file, count;
	char filename[128];
	u8 matrix[17536];
	u8 i, adj_mode = 4;

	for (i = 0; i < adj_mode; i++) {
		sprintf(filename,"%s/sensors/%s_0%d_3D.bin",
			IMGPROC_PARAM_PATH, sensor_name, (i+1));
		if ((file = open(filename, O_RDONLY, 0)) < 0) {
			printf("can not open 3D.bin\n");
			return -1;
		}
		if((count = read(file, matrix, 17536)) != 17536) {
			printf("read imx036_01_3D.bin error\n");
			return -1;
		}
		close(file);
		img_adj_load_cc_table((u32)matrix, i);
	}

	return 0;
}

u8 *bsb_mem;
u32 bsb_size;
int map_bsb(int fd_iav)
{
	static int mem_mapped = 0;
	iav_mmap_info_t info;

	if (mem_mapped)
		return 0;

	ioctl(fd_iav, IAV_IOC_MAP_BSB, &info);
	bsb_mem = info.addr;
	bsb_size = info.length;

	ioctl(fd_iav, IAV_IOC_MAP_DSP, &info);
	printf("bsb_mem = 0x%x, size = 0x%x\n", (u32)bsb_mem, bsb_size);
	mem_mapped = 1;
	return 0;
}

static int check_state(int fd_iav, int fd)
{
	int state;

	ioctl(fd_iav, IAV_IOC_GET_STATE, &state);
	if (state != IAV_STATE_PREVIEW) {
		sprintf(snd_buf, "IAV is not in preview state, CANNOT take still capture!\n");
		send(fd, snd_buf, 128, MSG_WAITALL);
		return -1;
	}

	return 0;
}

static int get_raw_file(int fd_iav, iav_raw_info_t * raw_info, int fd)
{
	still_cap_info_t cap_info;

	printf("still 0\n");
	img_init_still_capture(fd_iav, 95);

	memset(&cap_info, 0, sizeof(cap_info));
	cap_info.capture_num = 1;
	cap_info.jpeg_w = 0;
	cap_info.jpeg_h = 0;
	cap_info.thumb_w = 0;
	cap_info.thumb_h = 0;
	cap_info.need_raw = 1;
	cap_info.keep_AR_flag = 0;
	img_start_still_capture(fd_iav, &cap_info);

	ioctl(fd_iav, IAV_IOC_READ_RAW_INFO, raw_info);
	sprintf(snd_buf, "RAW w %d h %d bayer %d id %d", raw_info->width, raw_info->height, raw_info->bayer_pattern, raw_info->sensor_id);
	send(fd, snd_buf, 128, MSG_WAITALL);

	printf("still 1\n");
	return 0;
}

static int return_2_preview(int fd_iav)
{

	img_stop_still_capture(fd_iav);
	ioctl(fd_iav, IAV_IOC_LEAVE_STILL_CAPTURE, 0);
	img_dsp_config_statistics_info(fd_iav, app_param_image_sensor.p_tile_config);

	return 0;
}

static int save_calib_file(char* file_name, void * buf, int buf_size, void * append_info, int append_size_of_int, int fd)
{
	int file_fd;

	if ((file_fd = open(file_name, O_CREAT | O_TRUNC | O_WRONLY, 0777)) < 0) {
		sprintf(snd_buf, "vignette table file open error!\n");
		send(fd, snd_buf, 128, MSG_WAITALL);
		return -1;
	}

	if(append_info!=NULL) {
		if (write(file_fd, append_info, append_size_of_int*4)< 0) {
			sprintf(snd_buf, "vignette table file write error!\n");
			send(fd, snd_buf, 128, MSG_WAITALL);
			return -1;
		}
	}

	if(buf != NULL) {
		if(write(file_fd, buf, buf_size)<0) {
			sprintf(snd_buf, "vignette table file write error!\n");
			send(fd, snd_buf, 128, MSG_WAITALL);
			return -1;
		}
	}

	close(file_fd);

	return 0;

}

int awb_cali_get_current_gain(wb_gain_t* wb_gain, int fd)
{
	if (img_awb_set_method(AWB_CUSTOM) < 0) {
		sprintf(snd_buf, "img_awb_set_method error!\n");
		send(fd, snd_buf, 128, MSG_WAITALL);
		return -1;
	}
	sprintf(snd_buf, "Wait to get current White Balance Gain...\n");
	send(fd, snd_buf, 128, MSG_WAITALL);
	sleep(2);

	img_awb_get_wb_cal(wb_gain);
	sprintf(snd_buf, "Current Red Gain %d, Green Gain %d, Blue Gain %d.\n",
		wb_gain->r_gain, wb_gain->g_gain, wb_gain->b_gain);
	send(fd, snd_buf, 128, MSG_WAITALL);

	if (img_awb_set_method(AWB_NORMAL) < 0) {
		sprintf(snd_buf, "img_awb_set_method error!\n");
		send(fd, snd_buf, 128, MSG_WAITALL);
		return -1;
	}

	return 0;
}

/*
static int save_raw_picture(int fd_iav, char *pic_file_name)
{
	char filename[FILENAME_LENGTH] = {0};
	iav_raw_info_t raw_info;
	int fd_raw = -1;

	memset(&raw_info, 0, sizeof(raw_info));
	AM_IOCTL(fd_iav, IAV_IOC_READ_RAW_INFO, &raw_info);

	printf("raw_addr = %p.\n", raw_info.raw_addr);
	printf("bit_resolution = %d.\n", raw_info.bit_resolution);
	printf("resolution = %dx%d.\n", raw_info.width, raw_info.height);
	printf("bayer_pattern = %d.\n", raw_info.bayer_pattern);
	printf("sensor_id = %d.\n", raw_info.sensor_id);

	sprintf(filename, "%s.raw", pic_file_name);

	fd_raw = amba_transfer_open(filename, transfer_method, transfer_port);
	if (fd_raw < 0) {
		printf("Error opening file [%s]!\n", filename);
		return -1;
	}
	if (amba_transfer_write(fd_raw, raw_info.raw_addr,
		raw_info.width * raw_info.height * 2, transfer_method) < 0) {
		perror("write(5)");
		return -1;
	}

	printf("Save RAW picture [%s].\n", filename);
	amba_transfer_close(fd_raw, transfer_method);

	return 0;
}

static int save_jpeg_picture(int fd_iav, char *pic_file_name, int pic_num)
{
	int i = 0, fd_jpeg = -1;
	char filename[FILENAME_LENGTH] = {0};
	u32 start_addr, pic_size, bsb_start, bsb_end;
	bs_fifo_info_t bs_info;
	int pic_counter = 0;
	static struct timeval pre_time = {0,0};
	static struct timeval curr_time = {0,0};
	int time_interval_us = 0;

	bsb_start = (u32)bsb_mem;
	bsb_end = bsb_start + bsb_size;
	memset(&bs_info, 0, sizeof(bs_info));

	gettimeofday(&pre_time, NULL);

	//
	 //DSP always generates thumbnail with JPEG. It's decided in ARM side
	 // to read out thumbnail or not. All even pic numbers are reserved for
	 // JPEGs, and odd number are for thumbnails.
	 //
	for (pic_counter = 0; pic_counter < 2 * pic_num; ++pic_counter) {
		AM_IOCTL(fd_iav, IAV_IOC_READ_BITSTREAM, &bs_info);

		while (bs_info.count == 0) {
			printf("JPEG is not available, wait for a while!\n");
			AM_IOCTL(fd_iav, IAV_IOC_READ_BITSTREAM, &bs_info);
		}

		if ((capture_thumb == 0) && (pic_counter % 2))
			continue;

		for (i = 0; i < bs_info.count; ++i) {
			sprintf(filename, "%s_%d%03d%s.jpeg", pic_file_name, i,
				(pic_counter / 2), (pic_counter % 2) ? "_thumb" : "");
			fd_jpeg = amba_transfer_open(filename,
				transfer_method, transfer_port);
			if (fd_jpeg < 0) {
				printf("Error opening file [%s]!\n", filename);
				return -1;
			}
			pic_size = (bs_info.desc[i].pic_size + 31) & ~31;
			start_addr = bs_info.desc[i].start_addr;

			if (start_addr + pic_size <= bsb_end) {
				if (amba_transfer_write(fd_jpeg, (void *)start_addr,
					pic_size, transfer_method) < 0) {
					perror("Save jpeg file!\n");
					return -1;
				}
			} else {
				u32 size, remain_size;
				size = bsb_end - start_addr;
				remain_size = pic_size - size;
				if (amba_transfer_write(fd_jpeg, (void *)start_addr,
					size, transfer_method) < 0) {
					perror("Save jpeg file first part!\n");
					return -1;
				}
				if (amba_transfer_write(fd_jpeg, (void *)bsb_start,
					remain_size, transfer_method) < 0) {
					perror("Save jpeg file second part!\n");
					return -1;
				}
			}
			amba_transfer_close(fd_jpeg, transfer_method);
		}
		printf("Save JPEG %s #: %d.\n",
			(capture_thumb && (pic_counter % 2)) ? "Thumbnail " : "",
			pic_counter / 2);
	}

	gettimeofday(&curr_time, NULL);
	time_interval_us = (curr_time.tv_sec - pre_time.tv_sec) * 1000000 +
		curr_time.tv_usec - pre_time.tv_usec;
        printf("total count : %d, total cost time : %d ms, per jpeg cost time: %d ms\n",
		pic_num, time_interval_us/1000, (time_interval_us / pic_num)/1000);

	return 0;
}
*/

static u32 eth_link = 0;
void check_eth_link(void* arg)
{
	int socket = (int)arg;
	struct ifreq ifr;
	struct ethtool_value edata;


	strncpy(ifr.ifr_name, "eth0", sizeof(ifr.ifr_name));
	ifr.ifr_data = &edata;
	edata.cmd =  ETHTOOL_GLINK;

	while(1) {
		edata.data = 0;
		if(ioctl(socket, SIOCETHTOOL, &ifr)<0) {
			perror("SIOCETHTOOL");
		}
		//printf("edata.data %d\n", edata.data);
		//if(edata.data == 0) {
		//}
		eth_link<<=1;
		eth_link |= (edata.data&0x1);
		sleep(2);
	}
	return;
	
}

int main(int argc, char **argv)
{
	int socket_fd, client_fd;
	int fd_iav, rval;
	char * ip = "10.0.0.2";
	int port = 3000;
	socklen_t client_size;
	struct amba_vin_source_info vin_info;
	sensor_label sensor_lb;
	char sensor_name[32];
	char filename[128];
	still_cap_info_t still_cap_info;
	int eth_id;

	socket_fd = creat_socket_server(ip, port);
	if(socket_fd<0)	return -1;

	pthread_create(&eth_id, NULL, (void*)check_eth_link, (void*)socket_fd);
	client_size = sizeof(client);
	do {
		client_fd = accept(socket_fd, (struct sockaddr*)&client, &client_size);
		if(client_fd<0) {
			perror("accpet");
			printf("errno %d\n", errno);
		}
	}while(client_fd<0);

	printf("connected to %s:%u\n", inet_ntoa(client.sin_addr), ntohs(client.sin_port));
	printf("Numeric: %u\n", ntohl(client.sin_addr.s_addr));
	sprintf(snd_buf, "connected to A5s, starting 3A loop");
	send(client_fd, snd_buf, 128, MSG_WAITALL);
	//start 3A
	{
		if(img_lib_init(0,0)<0) {
			//printf("img_lib_init error\n");
			sprintf(snd_buf, "img_lib_init error");
			send(client_fd, snd_buf, 128, MSG_WAITALL);
			return -1;
		}

		if ((fd_iav = open("/dev/iav", O_RDWR, 0)) < 0) {
			//perror("open /dev/iav");
			sprintf(snd_buf, "open /dev/iav error");
			send(client_fd, snd_buf, 128, MSG_WAITALL);
			return -1;
		}
		map_bsb(fd_iav);
		if (ioctl(fd_iav, IAV_IOC_VIN_SRC_GET_INFO, &vin_info) < 0) {
			//perror("IAV_IOC_VIN_SRC_GET_INFO error\n");
			sprintf(snd_buf, "IAV_IOC_VIN_SRC_GET_INFO errorr");
			send(client_fd, snd_buf, 128, MSG_WAITALL);
			return -1;
		}

		/*if (ioctl(fd_iav, IAV_IOC_VIN_SRC_GET_VIDEO_MODE, vin_mode)) {
			perror("IAV_IOC_VIN_SRC_GET_VIDEO_MODE");
			return -1;
		}

		if ((rval = ioctl(fd_iav, IAV_IOC_VIN_SRC_GET_FRAME_RATE, pFrame_time)) < 0) {
			perror("IAV_IOC_VIN_SRC_GET_FRAME_RATE");
			return rval;
		}*/

		switch (vin_info.sensor_id) {
		case SENSOR_MT9T002:
			sensor_lb = MT_9T002;
			app_param_image_sensor.p_adj_param = &mt9t002_adj_param;
			app_param_image_sensor.p_rgb2yuv = mt9t002_rgb2yuv;
			app_param_image_sensor.p_chroma_scale = &mt9t002_chroma_scale;
			app_param_image_sensor.p_awb_param = &mt9t002_awb_param;
			app_param_image_sensor.p_50hz_lines = mt9t002_50hz_lines;
			app_param_image_sensor.p_60hz_lines = mt9t002_60hz_lines;
			app_param_image_sensor.p_tile_config = &mt9t002_tile_config;
			app_param_image_sensor.p_ae_agc_dgain = mt9t002_ae_agc_dgain;
			/*if(frame_rate ==AMBA_VIDEO_FPS_15&&vin_mode ==AMBA_VIDEO_MODE_QXGA)
			{
				printf("3M15\n");
				app_param_image_sensor.p_ae_sht_dgain =mt9t002_ae_sht_dgain_3m15;
			}
			else if(frame_rate == AMBA_VIDEO_FPS_29_97&&vin_mode ==AMBA_VIDEO_MODE_1080P30)
			{
				printf("1080p30\n");
				app_param_image_sensor.p_ae_sht_dgain = mt9t002_ae_sht_dgain_1080p30;
			}
			else */
			app_param_image_sensor.p_ae_sht_dgain = mt9t002_ae_sht_dgain;
			app_param_image_sensor.p_dlight_range = mt9t002_dlight;
			app_param_image_sensor.p_manual_LE = mt9t002_manual_LE;
			sprintf(sensor_name, "mt9t002");
			break;
		default:
			sprintf(snd_buf, "error sensor id %d", vin_info.sensor_id);
			send(client_fd, snd_buf, 128, MSG_WAITALL);
			return -1;

		}
		img_config_sensor_info(sensor_lb);
		load_dsp_cc_table();

		img_load_image_sensor_param(app_param_image_sensor);
		load_adj_cc_table(sensor_name);
		if(img_start_aaa(fd_iav))
			return -1;

	}

	while(1) {
		int bytes, i;
		do {
			bytes = recv(client_fd, (void*)rev_buf, 128, MSG_WAITALL);
			if(bytes<=0) {
				printf("recv errno %d\n", errno);
				switch(errno){
				case EAGAIN:
					printf("restart recv\n");
					if((eth_link&0x1) == 0) {
						do {
							client_fd = accept(socket_fd, (struct sockaddr*)&client, &client_size);
							if(client_fd<0) {
								printf("accept errno %d\n", errno);
							}
						}while(client_fd<0);
						printf("connected to %s:%u\n", inet_ntoa(client.sin_addr), ntohs(client.sin_port));
						printf("Numeric: %u\n", ntohl(client.sin_addr.s_addr));
						sprintf(snd_buf, "re-connected to A5s from link down, 3A loop running");
						send(client_fd, snd_buf, 128, MSG_WAITALL);
					}
					break;
				case ECONNRESET:
					close(client_fd);
					printf("restart accept\n");
					do {
						client_fd = accept(socket_fd, (struct sockaddr*)&client, &client_size);
						if(client_fd<0) {
							printf("accept errno %d\n", errno);
						}
					}while(client_fd<0);
					printf("connected to %s:%u\n", inet_ntoa(client.sin_addr), ntohs(client.sin_port));
					printf("Numeric: %u\n", ntohl(client.sin_addr.s_addr));
					sprintf(snd_buf, "re-connected to A5s from socket reset, 3A loop running");
					send(client_fd, snd_buf, 128, MSG_WAITALL);

					break;
				default:
					break;
//				case 
				}
			}
		}while(bytes<=0);

/*		if(bytes<=0) {
			printf("errno %d\n", errno);
			if(errno == EAGAIN) {
				
			}
			perror("recv");
			printf("errno %d", errno);
			close(client_fd);
			printf("connection closed\n");
			client_fd = accept(socket_fd, (struct sockaddr*)&client, &client_size);
			printf("connected to %s:%u\n", inet_ntoa(client.sin_addr), ntohs(client.sin_port));
			printf("Numeric: %u\n", ntohl(client.sin_addr.s_addr));
			sprintf(snd_buf, "connected to A5s, 3A loop enabled");
			send(client_fd, snd_buf, 128, MSG_WAITALL);
		}
		else*/
		{
			img_enable_adj(0);
			img_enable_af(0);
			switch(rev_buf[0]) {
				case VIG_CALIB:
					{
					vignette_cal_t vig_detect_setup;
					int shutter_idx;
					int anti_flicker_mode = rev_buf[3]<<8|rev_buf[2];
					u16 target_ratio =  rev_buf[5]<<8|rev_buf[4];
					u16 lookup_shift = rev_buf[7]<<8|rev_buf[6];
					u16 comp_ratio = rev_buf[9]<<8|rev_buf[8];
					u16 ng_thres = rev_buf[11]<<8|rev_buf[10];

					sprintf(snd_buf, "VIG Calib mode %d ratio %d shift %d comp %d ng %d\n", anti_flicker_mode, target_ratio, lookup_shift, comp_ratio, ng_thres);
					send(client_fd, snd_buf, 128, MSG_WAITALL);

					img_ae_set_antiflicker(anti_flicker_mode);
					img_ae_set_target_ratio(target_ratio);

					sleep(2);
					shutter_idx = img_get_sensor_shutter_index();
					if(anti_flicker_mode == 50) {
						/*do {
							if(shutter_idx< 1106) {
								sprintf(snd_buf, "light too weak");
								send(client_fd, snd_buf, 128, MSG_WAITALL);
							}
							else if(shutter_idx > 1234) {
								sprintf(snd_buf, "light too strong");
								send(client_fd, snd_buf, 128, MSG_WAITALL);
							}
							
						}while(shutter_idx!=1106 && shutter_idx != 1234);*/
						if(shutter_idx< 1106) {
							sprintf(snd_buf, "sht %d light too weak, please adjust light box and re-do", shutter_idx);
							send(client_fd, snd_buf, 128, MSG_WAITALL);
							continue;
						}
						else if(shutter_idx > 1234) {
							sprintf(snd_buf, "sht %d light too strong, please adjust light box and re-do",shutter_idx);
							send(client_fd, snd_buf, 128, MSG_WAITALL);
							continue;
						}
						
					}
					else {
						/*do {
							sleep(2);
							shutter_idx = img_get_sensor_shutter_index();
							if(shutter_idx< 1012) {
								sprintf(snd_buf, "light too weak");
								send(client_fd, snd_buf, 128, MSG_WAITALL);
							}
							else if(shutter_idx > 1268) {
								sprintf(snd_buf, "light too strong");
								send(client_fd, snd_buf, 128, MSG_WAITALL);
							}
							printf("shutter idx %d\n",shutter_idx );
							
						}while(shutter_idx!=1012 && shutter_idx != 1140 && shutter_idx!=1268);*/

						if(shutter_idx< 1012) {
							sprintf(snd_buf, "light too weak, please adjust light box and re-do");
							send(client_fd, snd_buf, 128, MSG_WAITALL);
							continue;
						}
						else if(shutter_idx > 1268) {
							sprintf(snd_buf, "light too strong, please adjust light box and re-do");
							send(client_fd, snd_buf, 128, MSG_WAITALL);
							continue;
						}
					}

					sprintf(snd_buf, "current shutter %d", shutter_idx);
					send(client_fd, snd_buf, 128, MSG_WAITALL);

					img_enable_ae(0);
					img_enable_awb(0);
					sleep(1);
					//Capture raw here

					get_raw_file(fd_iav, &raw_info, client_fd);
					raw_buff = (u16*)malloc(raw_info.width*raw_info.height*sizeof(u16));
					memcpy(raw_buff,raw_info.raw_addr,(raw_info.width * raw_info.height * 2));
					return_2_preview(fd_iav);
					img_enable_ae(1);
					/*input*/
					vig_detect_setup.raw_addr = raw_buff;
					vig_detect_setup.raw_w = raw_info.width;
					vig_detect_setup.raw_h = raw_info.height;
					vig_detect_setup.bp = raw_info.bayer_pattern;
					vig_detect_setup.threshold = ng_thres;
					vig_detect_setup.compensate_ratio = comp_ratio;
					vig_detect_setup.lookup_shift = lookup_shift;
					/*output*/
					vig_detect_setup.r_tab = vignette_table + 0*VIGNETTE_MAX_SIZE;
					vig_detect_setup.ge_tab = vignette_table + 1*VIGNETTE_MAX_SIZE;
					vig_detect_setup.go_tab = vignette_table + 2*VIGNETTE_MAX_SIZE;
					vig_detect_setup.b_tab = vignette_table + 3*VIGNETTE_MAX_SIZE;
					img_dsp_get_global_blc(&blc);
					vig_detect_setup.blc.r_offset = blc.r_offset;
					vig_detect_setup.blc.gr_offset = blc.gr_offset;
					vig_detect_setup.blc.gb_offset = blc.gb_offset;
					vig_detect_setup.blc.b_offset = blc.b_offset;
					
					if ((rval = img_cal_vignette(&vig_detect_setup))<0) {
						sprintf(snd_buf, "img_cal_vignette error");
						send(client_fd, snd_buf, 128, MSG_WAITALL);
						continue;
					}

					{
						int appendix[3];
						appendix[0] = vig_detect_setup.lookup_shift;
						appendix[1] = raw_info.width;
						appendix[2] = raw_info.height;
						sprintf(filename, "vignette.bin");
						lookup_shift = vig_detect_setup.lookup_shift;
						
						save_calib_file(filename, vignette_table, (4*VIGNETTE_MAX_SIZE*sizeof(u16)), appendix, 3, client_fd);
					}
					
					free(raw_buff);
					//revert preview state
					}
					break;
				case DARK_CALIB:
				case HOT_CALIB:
					{
						u32 width, height, raw_pitch, bpc_num, fpn_size;

						cali_badpix_setup_t badpixel_detect_algo;
						u16 bpc_type = rev_buf[3]<<8|rev_buf[2];
						u16 agc_idx = rev_buf[5]<<8|rev_buf[4];
						u16 sht_idx = rev_buf[7]<<8|rev_buf[6];
						u16 blk_size = rev_buf[9]<<8|rev_buf[8];
						u16 upper_thres = rev_buf[11]<<8|rev_buf[10];
						u16 lower_thres = rev_buf[13]<<8|rev_buf[12];
						u16 detect_times = rev_buf[15]<<8|rev_buf[14];
						u16 cali_mode = rev_buf[17]<<8|rev_buf[16];
						sprintf(snd_buf, "bpc type %d agc %d sht %d blk %d up %d low %d times %d cali %d", bpc_type,
								agc_idx, sht_idx, blk_size, upper_thres, lower_thres, detect_times, cali_mode);
						send(client_fd, snd_buf, 128, MSG_WAITALL);
						img_enable_ae(0);

						width = 1920;
						height = 1080;
						raw_pitch = ROUND_UP(width/8, 32);
						fpn_size = raw_pitch*height;
						if(bpc_type == 0) {//hot
							if(hot_fpn_map_addr==NULL) {
								hot_fpn_map_addr = malloc(fpn_size);
								if(hot_fpn_map_addr<0) {
									sprintf(snd_buf, "can not malloc memory for hot bpc map\n");
									send(client_fd, snd_buf, 128, MSG_WAITALL);
								}
							}
							memset(hot_fpn_map_addr,0,raw_pitch*height);
						}
						else if(bpc_type == 1) {
							if(dark_fpn_map_addr==NULL) {
								dark_fpn_map_addr = malloc(fpn_size);
								if(dark_fpn_map_addr<0) {
									sprintf(snd_buf, "can not malloc memory for dark bpc map\n");
									send(client_fd, snd_buf, 128, MSG_WAITALL);
								}
							}
							memset(dark_fpn_map_addr,0,raw_pitch*height);
						}
						else {
							sprintf(snd_buf, "incorrect bpc type\n");
							send(client_fd, snd_buf, 128, MSG_WAITALL);
							continue;
						}

						badpixel_detect_algo.cap_width = 1920;
						badpixel_detect_algo.cap_height= 1080;
						badpixel_detect_algo.block_w= blk_size;
						badpixel_detect_algo.block_h = blk_size;
						badpixel_detect_algo.badpix_type = bpc_type;
						badpixel_detect_algo.upper_thres= upper_thres;
						badpixel_detect_algo.lower_thres= lower_thres;
						badpixel_detect_algo.detect_times = detect_times;
						badpixel_detect_algo.agc_idx = agc_idx;
						badpixel_detect_algo.shutter_idx= sht_idx;
						badpixel_detect_algo.badpixmap_buf = bpc_type? dark_fpn_map_addr:hot_fpn_map_addr;
						badpixel_detect_algo.cali_mode = 0; // 0 for video, 1 for still

						bpc_num = img_cali_bad_pixel(fd_iav, &badpixel_detect_algo);
						sprintf(snd_buf, "Total number is %d\n", bpc_num);
						send(client_fd, snd_buf, 128, MSG_WAITALL);

						return_2_preview(fd_iav);
						if(rev_buf[0] == DARK_CALIB) {
							dark_bpc_done = 1;
						}
						else if(rev_buf[0] == HOT_CALIB) {
							hot_bpc_done = 1;
						}
						//img_enable_ae(1);
						if(hot_bpc_done && dark_bpc_done) {
							int i;
							int appendix[4];
							for(i = 0; i< fpn_size;i++)
								hot_fpn_map_addr[i] |= dark_fpn_map_addr[i];
							sprintf(filename, "bpc.bin");
							appendix[0] = 3; //fpn.enable
							appendix[1] = raw_pitch;
							appendix[2] = height;
							appendix[3] = width;
							save_calib_file( filename, hot_fpn_map_addr, fpn_size, appendix, 4, client_fd);
							free(hot_fpn_map_addr);
							free(dark_fpn_map_addr);
							dark_fpn_map_addr = hot_fpn_map_addr = NULL;
							img_enable_ae(1);
						}
						sleep(2);
						if(hot_bpc_done && dark_bpc_done) {
							fpn_correction_t fpn;
							u8* fpn_table;
							int appendix[4];
							int file = open(filename, O_RDONLY, 0777);
							read(file, appendix, 4*4);
							printf("file %d %d %d %d\n", appendix[0], appendix[1],appendix[2],appendix[3]);
							fpn_table = malloc(appendix[1]*appendix[2]);
							read(file, fpn_table, appendix[1]*appendix[2]);
							fpn.enable = appendix[0];
							fpn.fpn_pitch = appendix[1];
							fpn.pixel_map_height = appendix[2];
							fpn.pixel_map_width = appendix[3];
							fpn.pixel_map_size = appendix[1]*appendix[2];
							fpn.pixel_map_addr = (u32)fpn_table;
							img_dsp_set_static_bad_pixel_correction(fd_iav, &fpn);
							free(fpn_table);
							
						}

					}
					break;
				case AWB_CALIB_H:
				case AWB_CALIB_L:
					{
						wb_gain_t cal_gain, ref_gain;
						u16 r_diff, b_diff;

						ref_gain.r_gain = rev_buf[3]<<8|rev_buf[2];
						ref_gain.b_gain = rev_buf[5]<<8|rev_buf[4];
						ref_gain.g_gain = 1024;

						r_diff = rev_buf[7]<<8|rev_buf[6];
						b_diff = rev_buf[9]<<8|rev_buf[8];

						printf("wb %d r %d b %d rd %d bd %d\n", rev_buf[0], ref_gain.r_gain, ref_gain.b_gain, r_diff, b_diff);
						awb_cali_get_current_gain(&cal_gain, client_fd);

						printf("NG: threshold %d %d diff %d %d\n", r_diff, b_diff, ABS(cal_gain.r_gain-ref_gain.r_gain), ABS(cal_gain.b_gain-ref_gain.b_gain));
						if(ABS(cal_gain.r_gain-ref_gain.r_gain) > r_diff ||
							ABS(cal_gain.b_gain-ref_gain.b_gain) > b_diff) {
							
							sprintf(snd_buf, "NG: threshold %d %d diff %d %d\n", r_diff, b_diff, ABS(cal_gain.r_gain-ref_gain.r_gain), ABS(cal_gain.b_gain-ref_gain.b_gain));
							send(client_fd, snd_buf, 128, MSG_WAITALL);
							continue;
						}
						if(rev_buf[0] == AWB_CALIB_H) {
							cal_gain_h = cal_gain;
							ref_gain_h = ref_gain;
							wb_h_done = 1;
						}
						else {
							cal_gain_l = cal_gain;
							ref_gain_l = ref_gain;
							wb_l_done = 1;
						}

						if(wb_h_done&&wb_l_done) {
							int appendix[12];
							appendix[0] = ref_gain_l.r_gain;
							appendix[1] = ref_gain_l.g_gain;
							appendix[2] = ref_gain_l.b_gain;
							appendix[3] = ref_gain_h.r_gain;
							appendix[4] = ref_gain_h.g_gain;
							appendix[5] = ref_gain_h.b_gain;

							appendix[6] = cal_gain_l.r_gain;
							appendix[7] = cal_gain_l.g_gain;
							appendix[8] = cal_gain_l.b_gain;
							appendix[9] = cal_gain_h.r_gain;
							appendix[10] = cal_gain_h.g_gain;
							appendix[11] = cal_gain_h.b_gain;
							
							sprintf(filename, "wb.bin");
							save_calib_file( filename, NULL, 0, appendix, 12, client_fd);
						}
					}
					break;
				case WIFI_CONFIG:
					break;
				default:
					
					break;
			}
		}
		printf("loop\n");
	}

	close(socket_fd);
	return 0;
}
