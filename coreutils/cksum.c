/* vi: set sw=4 ts=4: */
/*
 * cksum - calculate the CRC32 checksum of a file
 *
 * Copyright (C) 2006 by Rob Sullivan, with ideas from code by Walter Harms
 *
 * Licensed under GPLv2 or later, see file LICENSE in this tarball for details. */

#include "busybox.h"

int cksum_main(int argc, char **argv)
{

	uint32_t *crc32_table = crc32_filltable(1);

	FILE *fp;
	uint32_t crc, filecrc;
	long length, filesize, testsize;
	int bytes_read, rest;
	char *cp;

	int test_result;
	int inp_stdin;
	int do_test;
	int do_seal;

	test_result = EXIT_SUCCESS;
	inp_stdin = (argc == optind) ? 1 : 0;
	do_test = ( (              (!inp_stdin) && (argc>2) && (strcmp(argv[1], "-test") == 0)) ? (argv++,1) : 0 );
	do_seal = ( ((!do_test) && (!inp_stdin) && (argc>2) && (strcmp(argv[1], "-seal") == 0)) ? (argv++,1) : 0 );

	//printf("\tinp_stdin=%d  do_test=%d do_seal=%d  argc=%d\n", inp_stdin, do_test, do_seal, argc);
	
	do {
		argv++;
		fp = fopen_or_warn_stdin((inp_stdin) ? bb_msg_standard_input : *argv);
		if (do_test) {
			fseek(fp, SEEK_SET, SEEK_END);
			testsize = ftell(fp) - 4;
			fseek(fp, SEEK_SET, 0);
		}
		else {
			testsize = -1;
		}
		
		crc = 0;
		length = 0;

		//printf("\tBUFSIZ=%d testsize=%li\n", BUFSIZ, testsize);
		while ((bytes_read = fread(bb_common_bufsiz1, 1, BUFSIZ, fp)) > 0) {
			//printf("\tfread returned bytes_read=%d\n", bytes_read);
			cp = bb_common_bufsiz1;
			length += bytes_read;
			while (testsize && bytes_read) {
				crc = (crc << 8) ^ crc32_table[((crc >> 24) ^ (*cp++)) & 0xffL];
				testsize--;
				bytes_read--;
			}
			
			if (testsize==0) {  /* this should happen only in do_test mode */
				//printf("\ttestsize==0: bytes_read=%d\n", bytes_read);
				if (bytes_read == 4) {
					memcpy(&filecrc, cp, sizeof(uint32_t));
					rest = 0;
				}
				else {
					if (bytes_read)
						memcpy((char*)&filecrc, cp, bytes_read);
					//printf("\t&filecrc=0x%p, start=0x%p\n", &filecrc, ((char*)&filecrc)+bytes_read);
					rest = fread( ((char*)&filecrc)+bytes_read, 1, 4-bytes_read, fp);
					//printf("\tfread returned rest=%d\n", rest);
				}
				filecrc = ntohl(filecrc);
				length -= (4-rest);
			}
		}

		filesize = length;

		for (; length; length >>= 8)
			crc = (crc << 8) ^ crc32_table[((crc >> 24) ^ length) & 0xffL];
		crc ^= 0xffffffffL;

		
		if (do_test)
			test_result |= (crc==filecrc ? 0 : 1);
		
		fclose(fp);

		
		if (inp_stdin) {
			printf("%" PRIu32 " %li\n", crc, filesize);
			break;  /* 1 file only */
		}
		else if (do_seal) {
			if  ((fp=fopen(*argv, "ab")) != NULL) {
				printf("%" PRIu32 " %li %s - add 4 bytes for crc.\n", crc, filesize, *argv);
				fseek(fp, SEEK_SET, SEEK_END);
				filecrc = ntohl(crc);
				fwrite(&filecrc, 1, sizeof(uint32_t), fp);
				fclose(fp);
			}
			else {
				printf("%" PRIu32 " %li %s #### cannot add 4 bytes for crc.\n", crc, filesize, *argv);
				fflush_stdout_and_exit(2);
			}		
		}
		else if (do_test) {
			printf("%" PRIu32 " %li %s - filecrc %" PRIu32 " result %s.\n", crc, filesize, *argv, filecrc, test_result==0?"ok":"failure");
		}
		else {
			printf("%" PRIu32 " %li %s\n", crc, filesize, *argv);
		}
		
		
	} while (*(argv + 1));

	fflush_stdout_and_exit(test_result);
}
