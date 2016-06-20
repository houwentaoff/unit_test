/*
 * test_fb.c
 *
 * History:
 *	2011/05/04 - [Jian Tang] created file
 *
 * Copyright (C) 2007-2011, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <getopt.h>

#include <sys/stat.h>
#include <sys/ioctl.h>

#include <basetypes.h>
#include <amba_crypto.h>


#define NO_ARG				(0)
#define HAS_ARG				(1)

#define PAGE_SIZE			(4 * 1024)
#define FILENAME_LENGTH	(256)
#define KEY_LENGTH			(32)
#define TEXT_LENGTH			(32)
#define BLOCK_SIZE			(16)

enum {
	CRYPTO_NONE = -1,
	CRYPTO_ENCRYPTE = 0,
	CRYPTO_DECRYPTE,
	CRYPTO_ENC_DEC,
	CRYPTO_SHA1,
	CRYPTO_MD5,
} CRYPTO_METHOD;

static int fd_crypto = -1;

static int crypto_method = CRYPTO_NONE;

static char filename[FILENAME_LENGTH] = "media/plain.txt";

static char algoname[256] = "aes";
static char default_algo[256] = "ecb(aes)";

static u8 default_key[KEY_LENGTH] = {
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
};

static int autotest_flag = 0;


static struct option long_options[] = {
	{"algo", HAS_ARG, 0, 'a'},
	{"crypto", HAS_ARG, 0, 'c'},
	{"file", HAS_ARG, 0, 'f'},
	{"key", HAS_ARG, 0, 'k'},
	{"autotest", NO_ARG, 0, 'r'},

	{0, 0, 0, 0}
};

static const char *short_options = "a:b:c:f:k:r";

struct hint_s {
	const char *arg;
	const char *str;
};

static const struct hint_s hint[] = {
	{"algoname", "\tset algorithm name ('aes', 'des'), default is 'ecb(aes)'"},
	{"enc | dec | sha1 | md5", "\tenc: encryption; dec: decryption; sha1: SHA1; md5: MD5"},
	{"filename", "\tread text from file"},
	{"key-string", "\tdefault key length is 32bytes, if key length is less than 32 bytes, filled all 0xff in the end"},
	{"", "\trun auto-test function"},
};

static int get_crypto_method(const char *str)
{
	if (strcmp(str, "enc") == 0)
		return CRYPTO_ENCRYPTE;
	if (strcmp(str, "dec") == 0)
		return CRYPTO_DECRYPTE;
	if (strcmp(str,"sha1") == 0)
		return CRYPTO_SHA1;
	if (strcmp(str,"md5") == 0)
		return CRYPTO_MD5;
	return CRYPTO_NONE;
}

static void usage(void)
{
	int i;

	printf("test_crypto usage:\n");
	for (i = 0; i < sizeof(long_options) / sizeof(long_options[0]) - 1; i++) {
		if (isalpha(long_options[i].val))
			printf("-%c ", long_options[i].val);
		else
			printf("   ");
		printf("--%s", long_options[i].name);
		if (hint[i].arg[0] != 0)
			printf(" [%s]", hint[i].arg);
		printf("\t%s\n", hint[i].str);
	}
	printf("\nExamples: \n");
	printf("\n  Auto test:\n    test_crypto -r\n");
	printf("\n  Encrypte from file:\n    test_crypto -c enc -f /mnt/media/plain.txt"
		"  # save cipher text to /mnt/media/plain.txt.cipher.txt\n");
	printf("\n  Decrypte from file:\n    test_crypto -c dec -f /mnt/media/cipher.txt"
		"  # save cipher text to /mnt/media/cipher.txt.plain.txt\n");
	printf("\n  SHA1 from file:\n    test_crypto -c sha1 -f /mnt/media/plain.txt"
		"  # print SHA1 on terminal");
	printf("\n  MD5 from file:\n    test_crypto -c md5 -f /mnt/media/plain.txt"
		"  # print MD5 on terminal");
	printf("\n");
}

static int open_crypto_dev(void)
{
	if ((fd_crypto = open("/dev/ambac", O_RDWR, 0)) < 0) {
		perror("/dev/ambac");
		return -1;
	}
	return 0;
}

static int close_crypto_dev(void)
{
	close(fd_crypto);
	return 0;
}

static int set_plain_text(u8 * arr, int n)
{
	int i;
	for (i = 0; i < n; ++i) {
		arr[i] = 0x30 + i;
	}
	return 0;
}

static int print_u8_array(const char * prefix, u8 * arr, int n)
{
	int i;
	printf("\n%s\n[", prefix);
	for (i = 0; i < n; ++i) {
		printf("0x%02X ", arr[i]);
	}
	printf("]\n");
	return 0;
}

static int prepare_cipher_algo(const char *algo_name)
{
	amba_crypto_algo_t algo;
	strcpy(algo.cipher_algo, algo_name);
	strcpy(algo.default_algo, default_algo);
	if (ioctl(fd_crypto, AMBA_IOC_CRYPTO_SET_ALGO, &algo) < 0) {
		perror("AMBA_IOC_CRYPTO_SET_ALGO");
		return -1;
	}
	return 0;
}

static int prepare_cipher_key(u8 * cipher_key, u32 key_len)
{
	amba_crypto_key_t key;
	key.key = cipher_key;
	key.key_len = key_len;
	if (ioctl(fd_crypto, AMBA_IOC_CRYPTO_SET_KEY, &key) < 0) {
		perror("AMBA_IOC_CRYPTO_SET_KEY");
		return -1;
	}
	print_u8_array("Cipher Key", cipher_key, key_len);
	return 0;
}

static int cipher_encrypt(u8 * text, u32 total_bytes)
{
	amba_crypto_info_t info;
	info.text = text;
	info.total_bytes = total_bytes;
	if (ioctl(fd_crypto, AMBA_IOC_CRYPTO_ENCRYPTE, &info) < 0) {
		perror("AMBA_IOC_CRYPTO_ENCRYPTE");
		return -1;
	}
	return 0;
}

static int cipher_decrypt(u8 * text, u32 total_bytes)
{
	amba_crypto_info_t info;
	info.text = text;
	info.total_bytes = total_bytes;
	if (ioctl(fd_crypto, AMBA_IOC_CRYPTO_DECRYPTE, &info) < 0) {
		perror("AMBA_IOC_CRYPTO_DECRYPTE");
		return -1;
	}
	return 0;
}

int auto_sha(u8 *in,u8 *out)
{
	u8 i;
	amba_crypto_sha_md5_t info;

	info.in=in;
	info.total_bytes =(u32)strlen((const char*)info.in);
	info.out = out;

	if (ioctl(fd_crypto, AMBA_IOC_CRYPTO_SHA, &info) < 0) {
		perror("AMBA_IOC_CRYPTO_SHA");
		return -1;
	}

	printf("SHA1=");
	for(i=0;i<20;i++){
		printf("%02X",out[i]);
	}
	printf("\r\n");

	return 0;
}

int auto_md5(u8 *in,u8 *out)
{
	u8 i;
	amba_crypto_sha_md5_t info;

	info.in=in;
	info.total_bytes =(u32)strlen((const char*)info.in);
	info.out = out;

	if (ioctl(fd_crypto, AMBA_IOC_CRYPTO_MD5, &info) < 0) {
		perror("AMBA_IOC_CRYPTO_MD5");
		return -1;
	}

	printf("MD5=");
	for(i=0;i<16;i++){
		printf("%02X",out[i]);
	}
	printf("\r\n");

	return 0;
}


static int auto_test(char * algo_name)
{
	u8 plain[TEXT_LENGTH] = {0};
	u8 cipher[TEXT_LENGTH] = {0};
	u8 temp[TEXT_LENGTH] = {0};
	u32 key_len = sizeof(default_key);
	u32 text_bytes = sizeof(plain);
	u8 in[]="The quick brown fox jumps over the lazy dog";
	u8 correctsha[]={0x2f,0xd4,0xe1,0xc6,0x7a,0x2d,0x28,0xfc,0xed,0x84,0x9e,0xe1,0xbb,0x76,0xe7,0x39,0x1b,0x93,0xeb,0x12,'\0'};
	u8 correctmd5[]={0x9e,0x10,0x7d,0x9d,0x37,0x2b,0xb6,0x82,0x6b,0xd8,0x1d,0x35,0x42,0xa4,0x19,0xd6,'\0'};
	u8 out[21];

	if (prepare_cipher_algo(algo_name) < 0) {
		printf("prepare_cipher_algo failed !\n");
		return -1;
	}

	if (prepare_cipher_key(default_key, key_len) < 0) {
		printf("prepare_cipher_key failed !\n");
		return -1;
	}

	set_plain_text(plain, sizeof(plain) - 4);
	memcpy(cipher, plain, sizeof(plain));
	printf("== Before encryption ==");
	print_u8_array("Plain Text", plain, sizeof(plain));
	print_u8_array("Cipher Text", cipher, sizeof(cipher));
	if (cipher_encrypt(plain, text_bytes) < 0) {
		printf("cipher_encrypt failed !\n");
		return -1;
	}
	printf("\n== After encrytion ==");
	memcpy(temp, cipher, sizeof(temp));
	memcpy(cipher, plain, sizeof(cipher));
	memcpy(plain, temp, sizeof(plain));
	print_u8_array("Plain Text", plain, sizeof(plain));
	print_u8_array("Cipher Text", cipher, sizeof(cipher));
	printf("\n== Before decryption ==");
	print_u8_array("Plain Text", plain, sizeof(plain));
	print_u8_array("Cipher Text", cipher, sizeof(cipher));
	if (cipher_decrypt(cipher, text_bytes) < 0) {
		printf("cipher_decrypt failed !\n");
		return -1;
	}
	printf("\n== After decrytion ==");
	print_u8_array("Plain Text", plain, sizeof(plain));
	print_u8_array("Cipher Text", cipher, sizeof(cipher));

	printf("\n== TEST SHA1&MD5 ==\n");
	printf("in= %s\n",in);
	if(auto_sha(in,out) != 0){
		printf("SHA1 driver failed!\n");
	}
	out[20]='\0';
	if(strcmp((const char*)out,(const char*)correctsha) == 0){
		printf("SHA1 CORRECT!\n");
	}else{
		printf("SHA1 ERROR; The correct should be 2fd4e1c67a2d28fced849ee1bb76e7391b93eb12\n");
	}

	if(auto_md5(in,out) != 0){
		printf("MD5 driver failed!\n");
	}
	out[16]='\0';
	if(strcmp((const char*)out,(const char*)correctmd5) == 0){
		printf("MD5 CORRECT!\n");
	}else{
		printf("MD5 ERROR; The correct should be 9e107d9d372bb6826bd81d3542a419d6\n");
	}
	return 0;
}

static int get_file_size(int fd)
{
	struct stat stat;
	if (fstat(fd, &stat) < 0) {
		perror("fstat");
		return -1;
	}
	return stat.st_size;
}

static int open_plain_cipher_files(int *fd_plain, int *fd_cipher)
{
	int plain, cipher;
	char plain_file[FILENAME_LENGTH], cipher_file[FILENAME_LENGTH];

	if (crypto_method == CRYPTO_ENCRYPTE) {
		strcpy(plain_file, filename);
		sprintf(cipher_file, "%s.cipher.txt", plain_file);
		if ((plain = open(plain_file, O_RDONLY, 0)) < 0) {
			printf("Cannot open plain file [%s].\n", plain_file);
			return -1;

		}
		if ((cipher = open(cipher_file, O_CREAT | O_TRUNC | O_WRONLY, 0644)) < 0) {
			printf("Cannot create cipher file [%s].\n", cipher_file);
			return -1;
		}
	} else {
		strcpy(cipher_file, filename);
		sprintf(plain_file, "%s.plain.txt", cipher_file);
		if ((cipher = open(cipher_file, O_RDONLY, 0)) < 0) {
			printf("Cannot open cipher file [%s].\n", cipher_file);
			return -1;
		}
		if ((plain = open(plain_file, O_CREAT | O_TRUNC | O_WRONLY, 0644)) < 0) {
			printf("Cannot create plain file [%s].\n", plain_file);
			return -1;
		}
	}

	*fd_plain = plain;
	*fd_cipher = cipher;

	return 0;
}

static int close_plain_cipher_files(int plain, int cipher)
{
	close(plain);
	close(cipher);
	return 0;
}

static int do_encryption(const char *algo_name)
{
	int fd_plain, fd_cipher;
	u8 data[PAGE_SIZE];
	u32 file_size;
	int retv;

	if (open_plain_cipher_files(&fd_plain, &fd_cipher) < 0) {
		printf("Cannot open files!\n");
		return -1;
	}
	file_size = get_file_size(fd_plain);

	if (prepare_cipher_algo(algo_name) < 0) {
		printf("prepare_cipher_algo failed !\n");
		return -1;
	}

	if (prepare_cipher_key(default_key, KEY_LENGTH) < 0) {
		printf("prepare_cipher_key failed !\n");
		return -1;
	}

	while (file_size > PAGE_SIZE) {
		if ((retv = read(fd_plain, data, PAGE_SIZE)) != PAGE_SIZE) {
			printf("Read data failed [%d].\n", retv);
			return -1;
		}
		print_u8_array("Plain Text", data, PAGE_SIZE);
		if (cipher_encrypt(data, PAGE_SIZE) < 0) {
			printf("cipher_encrypt failed !\n");
			return -1;
		}
		print_u8_array("Cipher Text", data, PAGE_SIZE);
		if ((retv = write(fd_cipher, data, PAGE_SIZE)) != PAGE_SIZE) {
			printf("Write data failed [%d].\n", retv);
			return -1;
		}
		file_size -= PAGE_SIZE;
	}
	if ((retv = read(fd_plain, data, file_size)) != file_size) {
		printf("Read data failed [%d].\n", retv);
		return -1;
	}
	print_u8_array("Plain Text", data, file_size);
	if (cipher_encrypt(data, file_size) < 0) {
		printf("cipher_encrypt failed !\n");
		return -1;
	}
	print_u8_array("Cipher Text", data, file_size);
	if ((retv = write(fd_cipher, data, file_size)) != file_size) {
		printf("Write data failed [%d].\n", retv);
		return -1;
	}

	close_plain_cipher_files(fd_plain, fd_cipher);

	return 0;
}

static int do_decryption(const char *algo_name)
{
	int fd_plain, fd_cipher;
	u8 data[PAGE_SIZE];
	u32 file_size;
	int retv;

	if (open_plain_cipher_files(&fd_plain, &fd_cipher) < 0) {
		printf("Cannot open files!\n");
		return -1;
	}
	file_size = get_file_size(fd_cipher);

	if (prepare_cipher_algo(algo_name) < 0) {
		printf("prepare_cipher_algo failed !\n");
		return -1;
	}

	if (prepare_cipher_key(default_key, KEY_LENGTH) < 0) {
		printf("prepare_cipher_key failed !\n");
		return -1;
	}

	while (file_size > PAGE_SIZE) {
		if ((retv = read(fd_cipher, data, PAGE_SIZE)) != PAGE_SIZE) {
			printf("Read data failed [%d].\n", retv);
			return -1;
		}
		print_u8_array("Cipher Text", data, PAGE_SIZE);
		if (cipher_decrypt(data, PAGE_SIZE) < 0) {
			printf("cipher_decrypt failed !\n");
			return -1;
		}
		print_u8_array("Plain Text", data, PAGE_SIZE);
		if ((retv = write(fd_plain, data, PAGE_SIZE)) != PAGE_SIZE) {
			printf("Write data failed [%d].\n", retv);
			return -1;
		}
		file_size -= PAGE_SIZE;
	}
	if ((retv = read(fd_cipher, data, file_size)) != file_size) {
		printf("Read data failed [%d].\n", retv);
		return -1;
	}
	print_u8_array("Cipher Text", data, file_size);
	if (cipher_decrypt(data, file_size) < 0) {
		printf("cipher_decrypt failed !\n");
		return -1;
	}
	print_u8_array("Plain Text", data, file_size);
	if ((retv = write(fd_plain, data, file_size)) != file_size) {
		printf("Write data failed [%d].\n", retv);
		return -1;
	}

	close_plain_cipher_files(fd_plain, fd_cipher);

	return 0;
}

static int init_param(int argc, char **argv)
{
	int ch, value;
	int option_index = 0;
	opterr = 0;

	while ((ch = getopt_long(argc, argv, short_options, long_options, &option_index)) != -1) {
		switch (ch) {
		case 'a':
			strcpy(algoname, optarg);
			break;
		case 'c':
			crypto_method = get_crypto_method(optarg);
			break;
		case 'f':
			strcpy(filename, optarg);
			break;
		case 'k':
			value = (strlen(optarg) > KEY_LENGTH) ? KEY_LENGTH : strlen(optarg);
			strncpy((char *)default_key, optarg, value);
			break;
		case 'r':
			autotest_flag = 1;
			crypto_method = CRYPTO_ENC_DEC;
			break;
		}
	}

	if (crypto_method == CRYPTO_NONE) {
		printf("DID NOT set crypto method [enc | dec | sha1 | md5] for '-c' option!\n");
		return -1;
	}

	return 0;
}

int sha()
{
	int plain;
	u32 file_size;
	u8 in[PAGE_SIZE];
	u8 out[20];
	u8 i;
	amba_crypto_sha_md5_t info;
	if ((plain = open(filename,O_RDONLY,0)) < 0){
		printf("open file failed\r\n");
	}

	file_size = get_file_size(plain);
	read(plain,in,PAGE_SIZE);

	info.in=in;
	info.total_bytes = file_size;
	info.out = out;

	if (ioctl(fd_crypto, AMBA_IOC_CRYPTO_SHA, &info) < 0) {
		perror("AMBA_IOC_CRYPTO_SHA");
		return -1;
	}
	close(plain);

	printf("file size is %d\r\n",file_size);
	printf("SHA1=");
	for(i=0;i<20;i++){
		printf("%02X",out[i]);
	}
	printf("\r\n");

	return 0;
}

int md5()
{
	int plain;
	u32 file_size;
	u8 in[PAGE_SIZE];
	u8 out[16];
	u8 i;
	amba_crypto_sha_md5_t info;
	if ((plain = open(filename,O_RDONLY,0)) < 0){
		printf("open file failed\r\n");
	}

	file_size = get_file_size(plain);
	read(plain,in,PAGE_SIZE);

	info.in=in;
	info.total_bytes = file_size;
	info.out = out;

	if (ioctl(fd_crypto, AMBA_IOC_CRYPTO_MD5, &info) < 0) {
		perror("AMBA_IOC_CRYPTO_MD5");
		return -1;
	}
	close(plain);

	printf("file size is %d\r\n",file_size);
	printf("MD5=");
	for(i=0;i<16;i++){
		printf("%02X",out[i]);
	}
	printf("\r\n");

	return 0;
}


int main(int argc, char ** argv)
{
	int retv = 0;

	if (argc < 2) {
		usage();
		return 0;
	}

	if (init_param(argc, argv) < 0)
		return -1;

	if (open_crypto_dev() < 0)
		return -1;

	if (crypto_method == CRYPTO_SHA1){
		retv = sha();
		return retv;
	}

	if (crypto_method == CRYPTO_MD5){
		retv = md5();
		return retv;
	}

	if (autotest_flag) {
		retv = auto_test(algoname);
		return retv;
	}

	if (crypto_method == CRYPTO_ENCRYPTE) {
		if (do_encryption(algoname) < 0)
			return -1;
	} else {
		if (do_decryption(algoname) < 0)
			return -1;
	}

	close_crypto_dev();

	return 0;
}


