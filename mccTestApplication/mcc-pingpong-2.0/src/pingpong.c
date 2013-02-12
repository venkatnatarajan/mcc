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
#include <mcc_config.h>
#include <mcc_common.h>
#include <mcc_api.h>
#include <mcc_linux.h>

#define MAX_NUM_NODES 				512
#define MAX_ENDPOINTS 				255

#define TEST_SETUP_FAILED 			1
#define TEST_TEARDOWN_FAILED 		1
#define TEST_FAILED 				1

#define TEST_SETUP_SUCCESSFUL 		0
#define TEST_TEARDOWN_SUCCESSFUL 	0
#define TEST_SUCCESSFUL 			0

MCC_ENDPOINT    mqx_endpoint_a5 = {0,0,1};
MCC_ENDPOINT    mqx_endpoint_m4 = {1,0,2};
MCC_ENDPOINT 	mqx_endpoint_a5_a2 = {0,0,3};
MCC_ENDPOINT 	mqx_endpoint_m4_a2 = {1,0,4};

#if 0
static float elapsed_seconds(struct timeval t_start, struct timeval t_end)
{
	if(t_start.tv_usec > t_end.tv_usec) {
		t_end.tv_usec += 1000000;
		t_end.tv_sec--;
	}

	return (float)(t_end.tv_sec - t_start.tv_sec) + ((float)(t_end.tv_usec - t_start.tv_usec)) / 1000000.0;
}
#endif

static void display_queue_lengths() {

	unsigned int num_msgs = 99;
    int retval = mcc_msgs_available(&mqx_endpoint_a5, &num_msgs);
    printf("A5: retval: %d, endpoint (%d, %d, %d) length: %d\n", retval,
    		mqx_endpoint_a5.core, mqx_endpoint_a5.node, mqx_endpoint_a5.port, num_msgs);

	num_msgs = 99;
    retval = mcc_msgs_available(&mqx_endpoint_m4, &num_msgs);
    printf("M4: retval: %d, endpoint (%d, %d, %d) length: %d\n", retval,
    		mqx_endpoint_m4.core, mqx_endpoint_m4.node, mqx_endpoint_m4.port, num_msgs);

    retval = mcc_msgs_available(&mqx_endpoint_a5_a2, &num_msgs);
    printf("A5: retval: %d, endpoint (%d, %d, %d) length: %d\n", retval,
    		mqx_endpoint_a5_a2.core, mqx_endpoint_a5_a2.node, mqx_endpoint_a5_a2.port, num_msgs);

	num_msgs = 99;
    retval = mcc_msgs_available(&mqx_endpoint_m4_a2, &num_msgs);
    printf("M4: retval: %d, endpoint (%d, %d, %d) length: %d\n", retval,
    		mqx_endpoint_m4_a2.core, mqx_endpoint_m4_a2.node, mqx_endpoint_m4_a2.port, num_msgs);

    sleep(2);
}

/*
 * mcc_signal_queue_order_test_setup
 * Returns 0 - Success, 1 - Failure
 *
 *
 */
int mcc_signal_queue_order_test_setup(uint_32 node_num)
{
	int retval;
    MCC_INFO_STRUCT info_data;

	retval = mcc_initialize(node_num);

	if (retval == MCC_ERR_DEV)
	{
		return TEST_SETUP_FAILED;
	}
	else
	{
		mcc_get_info(node_num, &info_data);
		printf("version: %s\n", info_data.version_string);
		sleep(2);

		mcc_create_endpoint(&mqx_endpoint_a5, 1); //TODO create macro for port number, avoid use of MCC_RESERVED_PORT_NUMBER
		mcc_create_endpoint(&mqx_endpoint_a5_a2, 3);

		display_queue_lengths();
		return TEST_SETUP_SUCCESSFUL;
	}
}

/*
 * mcc_signal_queue_order_test_teardown
 * Returns 0 - Success, 1 - Failure
 */
void mcc_signal_queue_order_test_teardown(uint_32 node_num)
{
    mcc_destroy(node_num);
}

/*
 * test1_signal_queue_order
 * Returns 0 - Success, 1 - Failure
 *
 * Send Endpoint mqx_endpoint_m4_a2
 * Receive Endpoint mqx_endpoint_a5_a2
 * Send Endpoint mqx_endpoint_m4
 * Receive Endpoint mqx_endpoint a5
 * Repeat in a loop of 100 messages.
 *
 */
int mcc_test1_signal_queue_order()
{
	int i;
    int retval;
    THE_MESSAGE msg;
    THE_MESSAGE* msg_p;
    MCC_MEM_SIZE num_of_received_bytes;

    msg.DATA = 1;

	/* wait until the remote endpoint is created by the other */
	while((retval = mcc_send(&mqx_endpoint_m4_a2, &msg, sizeof(THE_MESSAGE), 0xffffffff)))
	{
		if(retval == MCC_ERR_ENDPOINT)
			sleep(1);
		else
		{
			printf("mcc_send fails. retval=%d\n", retval);
			return TEST_FAILED;
		}
	}

	for(i=0; i<100; i++)
	{

#if 0
        if(mcc_recv_copy(&mqx_endpoint_a5_a2, &msg, sizeof(THE_MESSAGE), &num_of_received_bytes, 0xffffffff))
        {
       		printf("mcc_recv_copy() failed.\n");
       		return TEST_FAILED;
        }

        printf("Message: Size=%x, DATA = %x\n", num_of_received_bytes, msg.DATA);

        msg.DATA++;
#else
    	//TODO: Check if sending the address of the message pointer makes sense...
    	if(mcc_recv_nocopy(&mqx_endpoint_a5_a2, (void **) &msg_p, &num_of_received_bytes, 5000000))
		{
			printf("mcc_recv_copy() failed.\n");
			return TEST_FAILED;
		}

        printf("Message Received mqx_endpoint_a5_a2: Size=%x, DATA = %x.\n", num_of_received_bytes, msg_p->DATA);

        msg.DATA = msg_p->DATA + 1;

        if(mcc_free_buffer(&mqx_endpoint_a5_a2, msg_p))
		{
			printf("mcc_free_buffer() failed.\n");
			return TEST_FAILED;
		}
#endif
		// wait until the remote endpoint is created by the other
		while((retval = mcc_send(&mqx_endpoint_m4, &msg, sizeof(THE_MESSAGE), 0xffffffff)))
		{
			if(retval == MCC_ERR_ENDPOINT)
				sleep(1);
			else
			{
				printf("mcc_send fails. retval=%d\n", retval);
				return TEST_FAILED;
			}
		}

#if 0
        if(mcc_recv_copy(&mqx_endpoint_a5, &msg, sizeof(THE_MESSAGE), &num_of_received_bytes, 0xffffffff))
        {
       		printf("mcc_recv_copy() failed.\n");
       		return TEST_FAILED;
        }

        printf("Message: Size=%x, DATA = %x\n", num_of_received_bytes, msg.DATA);

        msg.DATA++;
#else
    	//TODO: Check if sending the address of the message pointer makes sense...
    	if(mcc_recv_nocopy(&mqx_endpoint_a5, (void **) &msg_p, &num_of_received_bytes, 5000000))
		{
			printf("mcc_recv_copy() failed.\n");
			return TEST_FAILED;
		}

        printf("Message Received mqx_endpoint_a5: Size=%x, DATA = %x.\n", num_of_received_bytes, msg_p->DATA);

        msg.DATA = msg_p->DATA + 1;

        if(mcc_free_buffer(&mqx_endpoint_a5_a2, msg_p))
		{
			printf("mcc_free_buffer() failed.\n");
			return TEST_FAILED;
		}
#endif

		if(mcc_send(&mqx_endpoint_m4_a2, &msg, sizeof(THE_MESSAGE), 5000000))
		{
			printf("mcc_send() mqx_endpoint_m4_a2 failed at\n");
			return TEST_FAILED;
		}

		//TODO: Why was the first arg null in this mcc_free_buffer????
		//mcc_free_buffer(null, data);
		printf("iteration %d\n", i);
	}
	return TEST_SUCCESSFUL;
}

/*
 * test2_signal_queue_order
 * Returns 0 - Success, 1 - Failure
 *
 * Send Endpoint mqx_endpoint_m4_a2
 * Send Endpoint mqx_endpoint_m4
 * Receive Endpoint mqx_endpoint a5
 * Receive Endpoint mqx_endpoint_a5_a2
 * Repeat in a loop of 100 messages.
 *
 */
int mcc_test2_signal_queue_order()
{
	int i;
    int retval;
    THE_MESSAGE msg;
    THE_MESSAGE* msg_p;
    MCC_MEM_SIZE num_of_received_bytes;

    msg.DATA = 1;

	/* wait until the remote endpoint is created by the other */
	while((retval = mcc_send(&mqx_endpoint_m4_a2, &msg, sizeof(THE_MESSAGE), 0xffffffff)))
	{
		if(retval == MCC_ERR_ENDPOINT)
			sleep(1);
		else
		{
			printf("mcc_send fails. retval=%d\n", retval);
			return TEST_FAILED;
		}
	}

	msg.DATA++;

	// wait until the remote endpoint is created by the other
	while((retval = mcc_send(&mqx_endpoint_m4, &msg, sizeof(THE_MESSAGE), 0xffffffff)))
	{
		if(retval == MCC_ERR_ENDPOINT)
			sleep(1);
		else
		{
			printf("mcc_send fails. retval=%d\n", retval);
			return TEST_FAILED;
		}
	}

	for(i=0; i<100; i++)
	{

#if 0
        if(mcc_recv_copy(&mqx_endpoint_a5_a2, &msg, sizeof(THE_MESSAGE), &num_of_received_bytes, 0xffffffff))
        {
       		printf("mcc_recv_copy() failed.\n");
       		return TEST_FAILED;
        }

        printf("Message: Size=%x, DATA = %x\n", num_of_received_bytes, msg.DATA);

        msg.DATA++;
#else
    	//TODO: Check if sending the address of the message pointer makes sense...
    	if(mcc_recv_nocopy(&mqx_endpoint_a5_a2, (void **) &msg_p, &num_of_received_bytes, 5000000))
		{
			printf("mcc_recv_copy() failed.\n");
			return TEST_FAILED;
		}

        printf("Message Received mqx_endpoint_a5_a2: Size=%x, DATA = %x.\n", num_of_received_bytes, msg_p->DATA);

        if(mcc_free_buffer(&mqx_endpoint_a5_a2, msg_p))
		{
			printf("mcc_free_buffer() failed.\n");
			return TEST_FAILED;
		}
#endif

#if 0
        if(mcc_recv_copy(&mqx_endpoint_a5, &msg, sizeof(THE_MESSAGE), &num_of_received_bytes, 0xffffffff))
        {
       		printf("mcc_recv_copy() failed.\n");
       		return TEST_FAILED;
        }

        printf("Message: Size=%x, DATA = %x\n", num_of_received_bytes, msg.DATA);

        msg.DATA++;
#else
    	//TODO: Check if sending the address of the message pointer makes sense...
    	if(mcc_recv_nocopy(&mqx_endpoint_a5, (void **) &msg_p, &num_of_received_bytes, 5000000))
		{
			printf("mcc_recv_copy() failed.\n");
			return TEST_FAILED;
		}

        printf("Message Received mqx_endpoint_a5: Size=%x, DATA = %x.\n", num_of_received_bytes, msg_p->DATA);

        if(mcc_free_buffer(&mqx_endpoint_a5_a2, msg_p))
		{
			printf("mcc_free_buffer() failed.\n");
			return TEST_FAILED;
		}
        msg.DATA = msg_p->DATA + 1;
#endif

		if(mcc_send(&mqx_endpoint_m4_a2, &msg, sizeof(THE_MESSAGE), 5000000))
		{
			printf("mcc_send() mqx_endpoint_m4_a2 failed at\n");
			return TEST_FAILED;
		}

		msg.DATA++;

		if(mcc_send(&mqx_endpoint_m4, &msg, sizeof(THE_MESSAGE), 5000000))
		{
			printf("mcc_send() mqx_endpoint_m4_a2 failed at\n");
			return TEST_FAILED;
		}

		//TODO: Why was the first arg null in this mcc_free_buffer????
		//mcc_free_buffer(null, data);
		printf("iteration %d\n", i);
	}
	return TEST_SUCCESSFUL;
}

int mcc_test_initialize_destroy()
{
	int retval, i;

	// Do init loop for node = 0 to 512
	for(i = 0; i < MAX_NUM_NODES; i++)
	{
		retval = mcc_initialize(i);
		if(retval == MCC_ERR_DEV)
		{
			return TEST_FAILED;
		}
		retval = mcc_destroy(i);
		if(retval == MCC_ERR_DEV)
		{
			return TEST_FAILED;
		}
	}

#if 0
	// Initialize with value of node < 0
	retval = mcc_initialize(-1);
	if(retval != MCC_ERR_DEV)
	{
		return TEST_FAILED;
	}

	retval = mcc_destroy(-1);
	if(retval != MCC_ERR_DEV)
	{
		return TEST_FAILED;
	}

	printf("Completed Initialize with value of node < 0 %d\n", retval);

	// Call more than once in same process
	retval = mcc_initialize(mqx_endpoint_a5.node);
	if(retval == MCC_ERR_DEV)
	{
		return TEST_FAILED;
	}
	retval = mcc_initialize(mqx_endpoint_a5.node);
	if(retval == MCC_ERR_DEV)
	{
		return TEST_FAILED;
	}

	retval = mcc_destroy(mqx_endpoint_a5.node);
	if(retval == MCC_ERR_DEV)
	{
		return TEST_FAILED;
	}

	retval = mcc_destroy(mqx_endpoint_a5.node);
	if(retval == MCC_ERR_DEV)
	{
		return TEST_FAILED;
	}

	printf("Completed calling initialize and destroy twice in the same process %d\n", retval);
#endif

	return TEST_SUCCESSFUL;
}

int mcc_test_create_endpoint(uint_32 node_num)
{
	int retval, i;
	MCC_ENDPOINT endpoint_a5;

	// test setup
	retval = mcc_initialize(node_num);

	if (retval == MCC_ERR_DEV)
	{
		return TEST_SETUP_FAILED;
	}

	// Call with reserved port (0)
	retval = mcc_create_endpoint(&endpoint_a5, 0);
	if(retval != MCC_ERR_ENDPOINT)
	{
		return TEST_FAILED;
	}

	// Call with Endpoint already in use
	// Call with port already in use.
	retval = mcc_create_endpoint(&endpoint_a5, 1);
	if(retval != MCC_SUCCESS)
	{
		return TEST_FAILED;
	}
	retval = mcc_create_endpoint(&endpoint_a5, 1);
	if(retval != MCC_ERR_ENDPOINT)
	{
		return TEST_FAILED;
	}
	retval = mcc_destroy_endpoint(&endpoint_a5);
	if(retval != MCC_SUCCESS)
	{
		return TEST_FAILED;
	}

	// Do create loop iterating port
	for(i = 1; i <= MAX_ENDPOINTS; i++)
	{
		retval = mcc_create_endpoint(&mqx_endpoint_a5, i);
		if(retval != MCC_SUCCESS)
		{
			return TEST_FAILED;
		}
		retval = mcc_destroy_endpoint(&mqx_endpoint_a5);
		if(retval != MCC_SUCCESS)
		{
			return TEST_FAILED;
		}
	}

	// Attempt to create more ep than the maximum specified in the config file
	retval = mcc_create_endpoint(&mqx_endpoint_a5, (MAX_ENDPOINTS + 1));
	if(retval != MCC_ERR_ENDPOINT)
	{
		return TEST_FAILED;
	}

	//TODO: Call with Endpoint not registered.

	// test teardown
	mcc_destroy(node_num);

	return TEST_SUCCESSFUL;
}

int main(int argc, char** argv)
{
    int test_result;

#if 0
    struct timeval t_start, t_end;
    float t_diff;
#endif

    //Test calls
    if(mcc_signal_queue_order_test_setup(mqx_endpoint_a5.node) == 0)
    {
		test_result = mcc_test1_signal_queue_order();

		if(test_result == 0)
			printf("%d, test1_signal_queue_order Passed\n", test_result);
		else
			printf("%d, test1_signal_queue_order Failed\n", test_result);

		mcc_signal_queue_order_test_teardown(mqx_endpoint_a5.node);
    }

    test_result = mcc_test_initialize_destroy();
    if(test_result == 0)
		printf("%d, mcc_test_initialize_destroy Passed\n", test_result);
	else
		printf("%d, mcc_test_initialize_destroy Failed\n", test_result);

    test_result = mcc_test_create_endpoint(mqx_endpoint_a5.node);
    if(test_result == 0)
		printf("%d, mcc_test_create_endpoint Passed\n", test_result);
	else
		printf("%d, mcc_test_create_endpoint Failed\n", test_result);

    return 0;
}
