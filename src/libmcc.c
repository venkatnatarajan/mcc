/*
  * Copyright 2013 Freescale, Inc.
  *
  * Redistribution and use in source and binary forms, with or without
  * modification, are permitted provided that the following conditions
  * are met:
  *
  * - Redistributions of source code must retain the above copyright
  *   notice, this list of conditions and the following disclaimer.
  * - Redistributions in binary form must reproduce the above copyright
  *   notice, this list of conditions and the following disclaimer in the
  *   documentation and/or other materials provided with the
  *   distribution.
  *
  * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
  * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
  * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
  * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
  * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
  * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
  * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
  * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
  * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
  * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY
  * WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
  * POSSIBILITY OF SUCH DAMAGE.
  */

#include "libmcc.h"
#include <sys/ioctl.h>
#include <errno.h>

#include <linux/mcc_linux.h>

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

#define ENDPOINTS_EQUAL(e1, e2) ((e1.core == e2.core) && (e1.node == e2.node) && (e1.port == e2.port))
#define ENDPOINT_IN_USE(e) (e.port != MCC_RESERVED_PORT_NUMBER)

// local, private data
static int fd = -1;	// the file descriptor used throughout

static MCC_CORE this_core = MCC_CORE_NUMBER;
static MCC_NODE this_node;
static MCC_ENDPOINT endpoints[MCC_ATTR_MAX_RECEIVE_ENDPOINTS];
static MCC_ENDPOINT send_endpoint, recv_endpoint;
static int block_mode = 0; //O_NONBLOCK

/*!
 * \brief This function initializes the Multi Core Communication.
 *
 * XXX
 *
 * \param[in] node Node number.
 */
int mcc_initialize(MCC_NODE node)
{
	int i;

	this_node = node;

	for(i=0; i<MCC_ATTR_MAX_RECEIVE_ENDPOINTS; i++)
		endpoints[i].port = MCC_RESERVED_PORT_NUMBER;

	send_endpoint.port = MCC_RESERVED_PORT_NUMBER;
	recv_endpoint.port = MCC_RESERVED_PORT_NUMBER;

	fd = open("/dev/mcc", O_RDWR);

    return fd < 0 ? MCC_ERR_DEV : MCC_SUCCESS;
}

/*!
 * \brief This function de-initializes the Multi Core Communication.
 *
 * Clear local data, clean semaphore and share memory resources ......
 *
 * \param[in] node Node number to be deinitialized.
 */
int mcc_destroy(MCC_NODE node)
{
	int i;

	// delete all endpoints associated with this node
	for(i=0; i<MCC_ATTR_MAX_RECEIVE_ENDPOINTS; i++)
	{
		if(ENDPOINT_IN_USE(endpoints[i]))
			ioctl(fd, MCC_DESTROY_ENDPOINT, &endpoints[i]);
	}

    close(fd);

    return MCC_SUCCESS;
}

/*!
 * \brief This function creates an endpoint.
 *
 * Create an endpoint on the local node with the specified port number.
 *
 * \param[out] endpoint Pointer to the endpoint structure.
 * \param[in] port Port number.
 */
int mcc_create_endpoint(MCC_ENDPOINT *endpoint, MCC_PORT port)
{
	int i;
	int slot = -1;

	endpoint->core = this_core;
	endpoint->node = this_node;
	endpoint->port = port;

	for(i=0; i<MCC_ATTR_MAX_RECEIVE_ENDPOINTS; i++)
	{
		if(!ENDPOINT_IN_USE(endpoints[i]))
		{
			slot = i;
			break;
		}
	}
	if(slot < 0)
		return MCC_ERR_NOMEM;

	if(ioctl(fd, MCC_CREATE_ENDPOINT, endpoint))
		return MCC_ERR_ENDPOINT;

	endpoints[slot] = *endpoint;

    return MCC_SUCCESS;
}

/*!
 * \brief This function destrois an endpoint.
 *
 * Destroy an endpoint on the local node.
 *
 * \param[in] endpoint Pointer to the endpoint structure.
 */
int mcc_destroy_endpoint(MCC_ENDPOINT *endpoint)
{
	int i;

	if(ioctl(fd, MCC_DESTROY_ENDPOINT, endpoint))
		return MCC_ERR_ENDPOINT;

	for(i=0; i<MCC_ATTR_MAX_RECEIVE_ENDPOINTS; i++)
	{
		if(ENDPOINTS_EQUAL(endpoints[i], (*endpoint)))
				endpoints[i].port = MCC_RESERVED_PORT_NUMBER;
	}

    return MCC_SUCCESS;
}

/*!
 * \brief This function sends a message.
 *
 * The message is copied into the MCC buffer and the other core is signaled.
 *
 * \param[in] endpoint Pointer to the endpoint structure.
 * \param[in] msg Pointer to the meassge to be sent.
 * \param[in] msg_size Size of the meassge to be sent.
 * \param[in] timeout_us Timeout in microseconds. 0 = non-blocking call, 0xffff = blocking call .
 *
 * TODO handle values besides 0 or 0xffff
 */
int mcc_send(MCC_ENDPOINT *endpoint, void *msg, MCC_MEM_SIZE msg_size, unsigned int timeout_us)
{
	int new_block_mode = timeout_us == 0 ? O_NONBLOCK : 0;

	// set the endpoint, if not already
	if(!ENDPOINTS_EQUAL(send_endpoint, (*endpoint)))
	{
		if(ioctl(fd, MCC_SET_SEND_ENDPOINT, endpoint))
			return MCC_ERR_ENDPOINT;
		send_endpoint = *endpoint;
	}

	// blocking or not
	if(block_mode != new_block_mode)
	{
		if(fcntl(fd, F_SETFL, new_block_mode))
			return MCC_ERR_INVAL;
		block_mode = new_block_mode;
	}

	// do the write
	errno = 0;
    write(fd, msg, msg_size);
	printf("mcc_send: errno=%d\n", errno);
	perror("mcc_send: perror()");

    return errno == 0 ? MCC_SUCCESS : MCC_ERR_INVAL;
}

/*!
 * \brief This function receives a message. The data is copied into the user-app. buffer.
 *
 * This is the "receive with copy" version of the MCC receive function. This version is simple
 * to use but includes the cost of copying the data from shared memory into the user space buffer.
 * The user has no obligation or burden to manage shared memory buffers.
 *
 * \param[in] endpoint Pointer to the endpoint structure.
 * \param[in] buffer Pointer to the user-app. buffer data will be copied into.
 * \param[in] buffer_size Size of the user-app. buffer data will be copied into.
 * \param[out] recv_size Pointer to the number of received bytes.
 * \param[in] timeout_us Timeout in microseconds. 0 = non-blocking call, 0xffff = blocking call .
 */
int mcc_recv_copy(MCC_ENDPOINT *endpoint, void *buffer, MCC_MEM_SIZE buffer_size, MCC_MEM_SIZE *recv_size, unsigned int timeout_us)
{
	int new_block_mode = timeout_us == 0 ? O_NONBLOCK : 0;

	// set the endpoint if not already
	if(!ENDPOINTS_EQUAL(recv_endpoint, (*endpoint)))
	{
		if(ioctl(fd, MCC_SET_RECEIVE_ENDPOINT, endpoint))
			return MCC_ERR_ENDPOINT;
		recv_endpoint = *endpoint;
	}

	// blocking or not
	if(block_mode != new_block_mode)
	{
		if(fcntl(fd, F_SETFL, new_block_mode))
			return MCC_ERR_INVAL;
		block_mode = new_block_mode;
	}

	*recv_size = read(fd, buffer, buffer_size);
	if(*recv_size < 0)
		perror("recv read error");
	printf("mcc_recv_copy: *recv_size=%d\n", *recv_size);

	return (*recv_size) < 0 ? MCC_ERR_INVAL : MCC_SUCCESS;
}

/*!
 * \brief This function receives a message. The data is NOT copied into the user-app. buffer.
 *
 * This is the "zero-copy receive" version of the MCC receive function. No data is copied, just
 * the pointer to the data is returned. This version is fast, but requires the user to manage
 * buffer allocation. Specifically the user must decide when a buffer is no longer in use and
 * make the appropriate API call to free it.
 *
 * \param[in] endpoint Pointer to the endpoint structure.
 * \param[out] buffer_p Pointer to the MCC buffer of the shared memory where the received data is stored.
 * \param[out] recv_size Pointer to the number of received bytes.
 * \param[in] timeout_us Timeout in microseconds. 0 = non-blocking call, 0xffff = blocking call .
 */
int mcc_recv_nocopy(MCC_ENDPOINT *endpoint, void **buffer_p, MCC_MEM_SIZE *recv_size, unsigned int timeout_us)
{
	// TOOO this is complicated on Linux?
    return MCC_ERR_INVAL;
}

/*!
 * \brief This function returns number of available buffers for the specified endpoint.
 *
 * Checks if messages are available on a receive endpoint. The call only checks the
 * availability of messages and does not dequeue them.
 *
 * \param[in] endpoint Pointer to the endpoint structure.
 * \param[out] num_msgs Pointer to the number of messages that are available in the receive queue and waiting for processing.
 */
int mcc_msgs_available(MCC_ENDPOINT *endpoint, unsigned int *num_msgs)
{
	// TOOO this nneds corresponding ioctl?
    return MCC_ERR_INVAL;
}

/*!
 * \brief This function frees a buffer.
 *
 * Once the zero-copy mechanism of receiving data is used this function
 * has to be called to free a buffer and to make it available for the next data
 * transfer.
 *
 * \param[in] endpoint Pointer to the endpoint structure.
 * \param[in] buffer Pointer to the buffer to be freed.
 */
int mcc_free_buffer(MCC_ENDPOINT *endpoint, void *buffer)
{
	// TOOO (see no copy)
    return MCC_ERR_INVAL;
}
