/*
 * Copyright 2013 Freescale Semiconductor
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

/*
 * mqxboot.c
 *
 *  Created on: Dec 17, 2012
 *      Author: Ed Nash
 *
 *      program to load and boot an mqx image on Vybrid M4 core
 *
 *      usage: mqxboot <image-file> <load address> <start address>
 *
 *      <addr> is the physical address in decimal or leading 0x for hex
 *      <image-file> is the .bin executable ** NOT the axf file **
 *      			(use fromelf utility included with ds5 to convert)
 */
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <unistd.h>

#include <linux/mcc_config.h>
#include <linux/mcc_common.h>
#include <linux/mcc_linux.h>

static struct mqx_boot_info_struct mqx_boot_info;
static FILE* img_file;

static int parse_args(int argc, char** argv)
{
	char *usage = "usage: mqxboot <image-file> <load address> <start address>\n";
	char *endptr;

	if(argc != 4)
	{
		printf("%s",usage);
		return 1;
	}

	// load address
	errno = 0;
	mqx_boot_info.phys_load_addr = strtoul(argv[2], &endptr, 0);
	if(errno) {
		perror(argv[2]);
		printf("invalid load address. %s",usage);
		return 1;
	}

	// start address
	mqx_boot_info.phys_start_addr = strtoul(argv[3], &endptr, 0);
	if(errno) {
		perror(argv[3]);
		printf("invalid start address. %s",usage);
		return 1;
	}

	img_file = fopen(argv[1], "r");
	if(!img_file)
	{
		perror(argv[1]);
		printf("%s",usage);
		return 1;
	}

	return 0;
}
int main(int argc, char** argv)
{
	unsigned char buf[1024];
	int len;
	int mcc_dev;
	int total_bytes;

	if(parse_args(argc, argv))
		return 1;

	mcc_dev = open("/dev/mcc", O_RDWR);
	if (mcc_dev == -1)
	{
		perror("open(\"/dev/mcc\", O_RDWR)");
		return 1;
	}

	// set the mode for loading
	if (ioctl(mcc_dev, MCC_SET_MODE_LOAD_MQX_IMAGE, &mqx_boot_info))
	{
		perror("ioctl(fd, MCC_SET_MODE_LOAD_MQX_IMAGE, &mqx_boot_info");
		close(mcc_dev);
		fclose(img_file);
		return 1;
	}

	printf("Loading %s to 0x%08x ...\n", argv[1], mqx_boot_info.phys_load_addr);
	total_bytes = 0;

	while(!feof(img_file))
	{
		len = fread(buf, 1, sizeof(buf), img_file);
		if(ferror(img_file))
		{
			perror("fread()");
			return 1;
		}

		printf("len=%d\n", len);
		if(len)
		{
			if(write(mcc_dev, buf, len) != len)
			{
				perror("write()");
				return 1;
			}
		}

		total_bytes += len;
	}
	fclose(img_file);

	printf("Loaded %d bytes. Booting at 0x%08x...\n", total_bytes, mqx_boot_info.phys_start_addr);
	if (ioctl(mcc_dev, MCC_BOOT_MQX_IMAGE, (void *)0))
	{
		perror("ioctl(fd, MCC_BOOT_MQX_IMAGE, (void *)0");
		close(mcc_dev);
		return 1;
	}

	close(mcc_dev);
	printf("done\n");

	return 0;
}


