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


 /**********************************************************************
* $FileName: mcc.c$
* $Version : 3.8.2.0$
* $Date    : Oct-2-2012$
*
* Comments:
*
*   This file contains the source for one of the MCC program examples.
*
*   Dec-12-2012 Modified for Linux
*
**********************************************************************/
#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>

#include "pingpong.h"
#include <linux/mcc_config.h>
#include <linux/mcc_common.h>
#include <mcc_api.h>
#include <linux/mcc_linux.h>
 

MCC_ENDPOINT    mqx_endpoint_a5 = {0,0,1};
MCC_ENDPOINT    mqx_endpoint_m4 = {1,0,2};

static float elapsed_seconds(struct timeval t_start, struct timeval t_end)
{
	if(t_start.tv_usec > t_end.tv_usec) {
		t_end.tv_usec += 1000000;
		t_end.tv_sec--;
	}

	return (float)(t_end.tv_sec - t_start.tv_sec) + ((float)(t_end.tv_usec - t_start.tv_usec)) / 1000000.0;
}

static void display_queue_lengths() {

	int num_msgs = 99;
    int retval = mcc_msgs_available(&mqx_endpoint_a5, &num_msgs);
    printf("A5: retval: %d, endpoint (%d, %d, %d) length: %d\n", retval,
    		mqx_endpoint_a5.core, mqx_endpoint_a5.node, mqx_endpoint_a5.port, num_msgs);

	num_msgs = 99;
    retval = mcc_msgs_available(&mqx_endpoint_m4, &num_msgs);
    printf("M4: retval: %d, endpoint (%d, %d, %d) length: %d\n", retval,
    		mqx_endpoint_m4.core, mqx_endpoint_m4.node, mqx_endpoint_m4.port, num_msgs);

    sleep(2);
}

int main(int argc, char** argv)
{
    THE_MESSAGE     msg;
    MCC_MEM_SIZE    num_of_received_bytes;
    MCC_INFO_STRUCT info_data;
    struct timeval t_start, t_end;
    float t_diff;
    int i;
    int *data;
    int num_msgs;
    int retval;

    uint_32 node_num = mqx_endpoint_a5.node;

    msg.DATA = 1;

    mcc_initialize(node_num);

    mcc_get_info(node_num, &info_data);
    printf("version: %s\n", info_data.version_string);
    sleep(2);


    mcc_create_endpoint(&mqx_endpoint_a5, 1); //TODO create macro for port number, avoid use of MCC_RESERVED_PORT_NUMBER

    display_queue_lengths();

    /* wait until the remote endpoint is created by the other */
    while((retval = mcc_send(&mqx_endpoint_m4, &msg, sizeof(THE_MESSAGE), 0xffffffff)))
    {
    	if(retval == MCC_ERR_ENDPOINT)
    		sleep(1);
    	else
    	{
    		printf("mcc_send fails. retval=%d\n", retval);
    		return 1;
    	}
    }

    mcc_msgs_available(&mqx_endpoint_m4, &num_msgs);
    printf("endpoint (%d, %d, %d) length: %d\n", mqx_endpoint_m4.core, mqx_endpoint_m4.node, mqx_endpoint_m4.port, num_msgs);
    sleep(2);

    //while (1)
    for(i=0; i<100000; i++)
    {

    	gettimeofday(&t_start, null);
    	//if(mcc_recv_copy(&mqx_endpoint_a5, &msg, sizeof(THE_MESSAGE), &num_of_received_bytes, 5000000))
//int mcc_recv_nocopy(MCC_ENDPOINT *endpoint, void **buffer_p, MCC_MEM_SIZE *recv_size, unsigned int timeout_us)
    	if(mcc_recv_nocopy(&mqx_endpoint_a5, &data, &num_of_received_bytes, 5000000))
        {
        	gettimeofday(&t_end, null);
        	t_diff = elapsed_seconds(t_start, t_end);
       		printf("mcc_recv_copy() failed after %f seconds.\n", t_diff);
       		return 1;
        }

    	gettimeofday(&t_end, null);
    	t_diff = elapsed_seconds(t_start, t_end);
//        printf("Message: Size=%x, DATA = %x after %f seconds.\n", num_of_received_bytes, msg.DATA, t_diff);
    	printf("Message: Size=%x, DATA = %x after %f seconds.\n", num_of_received_bytes, *data, t_diff);

//        msg.DATA++;
    	msg.DATA = *data+1;

    	gettimeofday(&t_start, null);
        if(mcc_send(&mqx_endpoint_m4, &msg, sizeof(THE_MESSAGE), 5000000))
        {
        	gettimeofday(&t_end, null);
        	t_diff = elapsed_seconds(t_start, t_end);
       		printf("mcc_send() 1 failed at %f seconds.\n", t_diff);
       		return 1;
        }

        display_queue_lengths();
    	gettimeofday(&t_start, null);
        if(mcc_send(&mqx_endpoint_m4, &msg, sizeof(THE_MESSAGE), 5000000))
        {
        	gettimeofday(&t_end, null);
        	t_diff = elapsed_seconds(t_start, t_end);
       		printf("mcc_send() 2 failed at %f seconds.\n", t_diff);
       		return 1;
        }


        mcc_free_buffer(null, data);
    }

    mcc_destroy(node_num);
    printf("exiting in 2 secs\n");
    sleep(2);
}


