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
static unsigned int current_timeout_us = 0xFFFFFFFF; //0

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
 * \param[in] timeout_us Timeout in microseconds. 0 = non-blocking call, 0xffffffff = blocking call .
 *
 * TODO handle values besides 0 or 0xffffffff
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

	//check if timeout changed
	if(timeout_us != current_timeout_us)
	{
		if(ioctl(fd, MCC_SET_TIMEOUT, &timeout_us))
			return MCC_ERR_INVAL;

		if((timeout_us == 0) || (current_timeout_us == 0))
		{
			if(fcntl(fd, F_SETFL, new_block_mode))
				return MCC_ERR_INVAL;
		}
		current_timeout_us = timeout_us;
	}

	// do the write
	errno = 0;
    write(fd, msg, msg_size);
    if(errno)
    	perror("mcc_send");

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
 * \param[in] timeout_us Timeout in microseconds. 0 = non-blocking call, 0xffffffff = blocking call
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

	//check if timeout changed
	if(timeout_us != current_timeout_us)
	{
		if(ioctl(fd, MCC_SET_TIMEOUT, &timeout_us))
			return MCC_ERR_INVAL;

		if((timeout_us == 0) || (current_timeout_us == 0))
		{
			if(fcntl(fd, F_SETFL, new_block_mode))
				return MCC_ERR_INVAL;
		}
		current_timeout_us = timeout_us;
	}

	errno = 0;
	*recv_size = read(fd, buffer, buffer_size);
	if(errno)
		perror("mcc_recv_copy");

	return errno != 0 ? MCC_ERR_INVAL : MCC_SUCCESS;
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
 * \param[in] timeout_us Timeout in microseconds. 0 = non-blocking call, 0xffffffff = blocking call .
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
/*!
 * \brief This function returns info structure.
 *
 * Returns implementation specific information.
 *
 * \param[in] node Node number.
 * \param[out] info_data Pointer to the MCC info structure.
 */
int mcc_get_info(MCC_NODE node, MCC_INFO_STRUCT* info_data)
{
	return ioctl(fd, MCC_GET_INFO, info_data);
}

