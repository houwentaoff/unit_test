#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <getopt.h>
#include <ctype.h>
#include <fcntl.h>
#include <signal.h>

#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <basetypes.h>

#include "ambas_common.h"
#include "iav_drv.h"

#define MAX_ENCODE_STREAM_NUM		4

#ifndef ROUND_UP
#define ROUND_UP(size, align) (((size) + ((align) - 1)) & ~((align) - 1))
#endif
#ifndef ROUND_DOWN
#define ROUND_DOWN(size, align) ((size) & ~((align) - 1))
#endif

#define VERIFY_STREAMID(x)   do {		\
		if (((x) < 0) || ((x) >= MAX_ENCODE_STREAM_NUM)) {	\
			printf ("stream id wrong %d \n", (x));			\
			return -1; 	\
		}	\
	} while (0)


typedef struct qp_roi_s {
	int quality_level;		// 0 means the ROI is in the default QP RC.
	int start_x;
	int start_y;
	int width;
	int height;
} qp_roi_t;

typedef struct stream_roi_s {
	int enable;
	int encode_width;
	int encode_height;
	s8 qp_delta[3];
} stream_roi_t;

static int exit_flag = 0;

static int fd_iav;
static u8 *qp_matrix_addr = NULL;
static int stream_qp_matrix_size = 0;

static stream_roi_t stream_roi[MAX_ENCODE_STREAM_NUM];
static qp_roi_t qp_roi[MAX_ENCODE_STREAM_NUM];

static void usage(void)
{
	printf("test_qp_roi usage:\n");
	printf("\t test_qp_roi\n"
		"\t Select a stream to set ROI and quality.\n"
		"\t Up to 3 quality levels are supported.\n"
		"\t Shall be used in ENCODE state.");
	printf("\n");
}

static void show_main_menu(void)
{
	printf("\n|-------------------------------|\n");
	printf("| Main Menu              \t|\n");
	printf("| a - Config Stream A ROI\t|\n");
	printf("| b - Config Stream B ROI\t|\n");
	printf("| c - Config Stream C ROI\t|\n");
	printf("| d - Config Stream D ROI\t|\n");
	printf("| q - Quit               \t|\n");
	printf("|-------------------------------|\n");
}

static int show_stream_menu(int stream_id)
{
	VERIFY_STREAMID(stream_id);
	printf("\n|-------------------------------|\n");
	printf("| Stream %c                \t|\n", 'A' + stream_id);
	printf("| 1 - Set quality level \t|\n");
	printf("| 2 - Add an ROI         \t|\n");
	printf("| 3 - Remove an ROI      \t|\n");
	printf("| 4 - Clear all ROIs     \t|\n");
	printf("| 5 - View ROI           \t|\n");
	printf("| q - Back to Main menu  \t|\n");
	printf("|-------------------------------|\n");
	return 0;
}

static int map_qp_matrix(void)
{
	iav_mmap_info_t qp_info;
	if (ioctl(fd_iav, IAV_IOC_MAP_QP_ROI_MATRIX_EX, &qp_info) < 0) {
		perror("IAV_IOC_MAP_QP_ROI_MATRIX_EX");
		return -1;
	}
	qp_matrix_addr = qp_info.addr;
	stream_qp_matrix_size = qp_info.length / MAX_ENCODE_STREAM_NUM;
	return 0;
}

static int check_for_qp_roi(int stream_id)
{
	iav_encode_stream_info_ex_t stream_info;
	iav_encode_format_ex_t encode_format;
	u32 *addr = (u32 *)(qp_matrix_addr + stream_qp_matrix_size * stream_id);
	VERIFY_STREAMID(stream_id);

	memset(&stream_info, 0, sizeof(stream_info));
	stream_info.id = (1 << stream_id);
	if (ioctl(fd_iav, IAV_IOC_GET_ENCODE_STREAM_INFO_EX, &stream_info) < 0) {
		perror("IAV_IOC_GET_ENCODE_STREAM_INFO_EX");
		return -1;
	}
	if (stream_info.state != IAV_STREAM_STATE_ENCODING) {
		printf("Stream %c shall be in ENCODE state.\n", 'A' + stream_id);
		return -1;
	}

	memset(&encode_format, 0, sizeof(encode_format));
	encode_format.id = (1 << stream_id);
	if (ioctl(fd_iav, IAV_IOC_GET_ENCODE_FORMAT_EX, &encode_format) < 0) {
		perror("IAV_IOC_GET_ENCODE_FORMAT_EX");
		return -1;
	}
	if (encode_format.encode_type != IAV_ENCODE_H264) {
		printf("Stream %c encode format shall be H.264.\n", 'A' + stream_id);
		return -1;
	}
	// Clear roi buffer when changing resolution.
	if (stream_roi[stream_id].encode_width != encode_format.encode_width ||
		stream_roi[stream_id].encode_height != encode_format.encode_height)
		memset(addr, 0, stream_qp_matrix_size);
	stream_roi[stream_id].encode_width = encode_format.encode_width;
	stream_roi[stream_id].encode_height = encode_format.encode_height;
	stream_roi[stream_id].enable = 1;
	return 0;
}

static int get_qp_roi(int stream_id, iav_qp_roi_matrix_ex_t *matrix)
{
	VERIFY_STREAMID(stream_id);
	matrix->id = (1 << stream_id);
	if (ioctl(fd_iav, IAV_IOC_GET_QP_ROI_MATRIX_EX, matrix) < 0) {
		perror("IAV_IOC_GET_QP_ROI_MATRIX_EX");
		return -1;
	}
	return 0;
}

static int set_qp_roi(int stream_id)
{
	iav_qp_roi_matrix_ex_t qp_matrix;
	int i, j;
	u32 *addr = (u32 *)(qp_matrix_addr + stream_qp_matrix_size * stream_id);
	u32 buf_width, buf_pitch, buf_height, start_x, start_y, end_x, end_y;
	VERIFY_STREAMID(stream_id);
	if (get_qp_roi(stream_id, &qp_matrix) < 0)
		return -1;
	qp_matrix.enable = 1;

	// QP matrix is MB level. One MB is 16x16 pixels.
	buf_width = ROUND_UP(stream_roi[stream_id].encode_width, 16) / 16;
	buf_pitch = ROUND_UP(buf_width, 8);
	buf_height = ROUND_UP(stream_roi[stream_id].encode_height, 16) / 16;

	start_x = ROUND_DOWN(qp_roi[stream_id].start_x, 16) / 16;
	start_y = ROUND_DOWN(qp_roi[stream_id].start_y, 16) / 16;
	end_x = ROUND_UP(qp_roi[stream_id].width, 16) / 16 + start_x;
	end_y = ROUND_UP(qp_roi[stream_id].height, 16) / 16 + start_y;

	for (i = start_y; i < end_y && i < buf_height; i++) {
		for (j = start_x; j < end_x && j < buf_width; j++)
			addr[i * buf_pitch + j] = qp_roi[stream_id].quality_level;
	}

	if (ioctl(fd_iav, IAV_IOC_SET_QP_ROI_MATRIX_EX, &qp_matrix) < 0) {
		perror("IAV_IOC_SET_QP_ROI_MATRIX_EX");
		return -1;
	}
	return 0;
}

static int clear_qp_roi(int stream_id)
{
	iav_qp_roi_matrix_ex_t qp_matrix;
	u32 *addr = (u32 *)(qp_matrix_addr + stream_qp_matrix_size * stream_id);
	VERIFY_STREAMID(stream_id);
	if (get_qp_roi(stream_id, &qp_matrix) < 0)
		return -1;
	qp_matrix.enable = 0;
	if (ioctl(fd_iav, IAV_IOC_SET_QP_ROI_MATRIX_EX, &qp_matrix) < 0) {
		perror("IAV_IOC_SET_QP_ROI_MATRIX_EX");
		return -1;
	}
	stream_roi[stream_id].enable = 0;
	memset(addr, 0, stream_qp_matrix_size);
	printf("\nClear all qp matrix for stream %c.\n", 'A' + stream_id);
	return 0;
}

static int display_qp_roi(int stream_id)
{
	VERIFY_STREAMID(stream_id);
	int i, j;
	iav_qp_roi_matrix_ex_t qp_matrix;
	u32 *addr = (u32 *)(qp_matrix_addr + stream_qp_matrix_size * stream_id);
	u32 buf_width, buf_pitch, buf_height;
	buf_width = ROUND_UP(stream_roi[stream_id].encode_width, 16) / 16;
	buf_pitch = ROUND_UP(buf_width, 8);
	buf_height = ROUND_UP(stream_roi[stream_id].encode_height, 16) / 16;
	if (get_qp_roi(stream_id, &qp_matrix) < 0)
		return -1;
	printf("\n\n===================================================\n");
	printf("Quality level: 0-[%d], 1-[%d], 2-[%d], 3-[%d]\n",
	       qp_matrix.delta[0], qp_matrix.delta[1], qp_matrix.delta[2],qp_matrix.delta[3]);
	printf("===================================================\n");
	for (i = 0; i < buf_height; i++) {
		printf("\n");
		for (j = 0; j < buf_width; j++)
			printf("%-2d", addr[i * buf_pitch + j]);
	}
	printf("\n");
	return 0;
}

static int get_quality_level(int stream_id)
{
	iav_qp_roi_matrix_ex_t qp_matrix;
	VERIFY_STREAMID(stream_id);
	if (get_qp_roi(stream_id, &qp_matrix) < 0)
		return -1;
	stream_roi[stream_id].qp_delta[0] = qp_matrix.delta[1];
	stream_roi[stream_id].qp_delta[1] = qp_matrix.delta[2];
	stream_roi[stream_id].qp_delta[2] = qp_matrix.delta[3];
	return 0;
}

static int set_quality_level(int stream_id)
{
	iav_qp_roi_matrix_ex_t qp_matrix;
	VERIFY_STREAMID(stream_id);
	if (get_qp_roi(stream_id, &qp_matrix) < 0)
		return -1;
	qp_matrix.delta[0] = 0;
	qp_matrix.delta[1] = stream_roi[stream_id].qp_delta[0];
	qp_matrix.delta[2] = stream_roi[stream_id].qp_delta[1];
	qp_matrix.delta[3] = stream_roi[stream_id].qp_delta[2];
	if (ioctl(fd_iav, IAV_IOC_SET_QP_ROI_MATRIX_EX, &qp_matrix) < 0) {
		perror("IAV_IOC_SET_QP_ROI_MATRIX_EX");
		return -1;
	}
	return 0;
}

static int input_value(int min, int max)
{
	int retry = 1, input;
	char tmp[16];
	do {
		scanf("%s", tmp);
		input = atoi(tmp);
		if (input > max || input < min) {
			printf("\nInvalid. Enter a number (%d~%d): ", min, max);
			continue;
		}
		retry = 0;
	} while (retry);
	return input;
}

static int config_stream_roi(int stream_id)
{
	int back2main = 0, i;
	char opt, input[16];
	VERIFY_STREAMID(stream_id);
	while (back2main == 0) {
		show_stream_menu(stream_id);
		printf("Your choice: ");
		fflush(stdin);
		scanf("%s", input);
		opt = input[0];
		tolower(opt);
		switch(opt) {
		case '1':
			if (check_for_qp_roi(stream_id) < 0)
				break;
			if (get_quality_level(stream_id) < 0)
				break;
			printf("\nCurrent quality level is 1:[%d], 2:[%d], 3:[%d].\n",
					stream_roi[stream_id].qp_delta[0],
					stream_roi[stream_id].qp_delta[1],
					stream_roi[stream_id].qp_delta[2]);
			i = 0;
			do {
				printf("Input QP delta (-51~51) for level %d: ", i+1);
				stream_roi[stream_id].qp_delta[i] = input_value(-51, 51);
			} while (++i < 3);
			set_quality_level(stream_id);
			break;
		case '2':
		case '3':
			if (check_for_qp_roi(stream_id) < 0)
				break;
			printf("\nInput ROI offset x (0~%d): ", stream_roi[stream_id].encode_width - 1);
			qp_roi[stream_id].start_x = input_value(0, stream_roi[stream_id].encode_width - 1);
			printf("Input ROI offset y (0~%d): ", stream_roi[stream_id].encode_height - 1);
			qp_roi[stream_id].start_y = input_value(0, stream_roi[stream_id].encode_height -1);
			printf("Input ROI width (1~%d): ", stream_roi[stream_id].encode_width
		        - qp_roi[stream_id].start_x);
			qp_roi[stream_id].width = input_value(1, stream_roi[stream_id].encode_width
				- qp_roi[stream_id].start_x);
			printf("Input ROI height (1~%d): ", stream_roi[stream_id].encode_height
			    - qp_roi[stream_id].start_y);
			qp_roi[stream_id].height = input_value(1, stream_roi[stream_id].encode_height
				- qp_roi[stream_id].start_y);
			if (opt == '2') {
				printf("Input ROI quality level (1~3): ");
				qp_roi[stream_id].quality_level = input_value(1, 3);
			} else
				qp_roi[stream_id].quality_level = 0;
			set_qp_roi(stream_id);
			break;
		case '4':
			if (check_for_qp_roi(stream_id) < 0)
				break;
			clear_qp_roi(stream_id);
			break;
		case '5':
			if (check_for_qp_roi(stream_id) < 0)
				break;
			display_qp_roi(stream_id);
			break;
		case 'q':
			back2main = 1;
			break;
		default:
			printf("Unknown option %d.", opt);
			break;
		}
	}
	return 0;
}

static void quit_qp_roi()
{
	int i;
	exit_flag = 1;
	for (i = 0; i < MAX_ENCODE_STREAM_NUM; i++)
		if (stream_roi[i].enable)
			clear_qp_roi(i);
	exit(0);
}

int main(int argc, char **argv)
{
	char opt;
	char input[16];
	if ((fd_iav = open("/dev/iav", O_RDWR, 0)) < 0) {
		perror("/dev/iav");
		return -1;
	}
	if (argc > 1) {
		usage();
		return 0;
	}

	if (map_qp_matrix() < 0)
		return -1;

	signal(SIGINT, quit_qp_roi);
	signal(SIGTERM, quit_qp_roi);
	signal(SIGQUIT, quit_qp_roi);

	while (exit_flag == 0) {
		show_main_menu();
		printf("Your choice: ");
		fflush(stdin);
		scanf("%s", input);
		opt = input[0];
		tolower(opt);
		if ( opt >= 'a' && opt <= 'd')
			config_stream_roi(opt - 'a');
		else if (opt == 'q')
			exit_flag = 1;
		else
			printf("Unknown option %d.", opt);
	}
	return 0;
}
