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
#include <stddef.h>

#include <linux/mcc_common.h>
#include <linux/mcc_linux.h>

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>

// local, private data
static int fd = -1;	// the file descriptor used throughout
static unsigned int bookeeping_p;

static MCC_CORE this_core = MCC_CORE_NUMBER;
static MCC_NODE this_node;
static MCC_ENDPOINT send_endpoint, recv_endpoint;
static unsigned int current_timeout_us = 0xFFFFFFFF; //0
static MCC_READ_MODE current_read_mode = MCC_READ_MODE_UNDEFINED;

#define MCC_ENDPOINT_IN_USE(e) (e.port != MCC_RESERVED_PORT_NUMBER)

/*!
 * \brief This function initializes the Multi Core Communication subsystem for a given node.
 *
 * This should only be called once per node (once in MQX, once per process in Linux).
 *
 * \param[in] node Node number that will be used in endpoints created by this process.
 *
 * \return MCC_SUCCESS
 * \return MCC_ERR_DEV (cant open /dev/mcc - kernel module not loaded)
 * \return MCC_ERR_NOMEM (Cant map shared memory)
*/
int mcc_initialize(MCC_NODE node)
{
	int i;

	this_node = node;

	send_endpoint.port = MCC_RESERVED_PORT_NUMBER;
	recv_endpoint.port = MCC_RESERVED_PORT_NUMBER;

	fd = open("/dev/mcc", O_RDWR);
	if(fd < 0)
		return MCC_ERR_DEV;

	bookeeping_p = (unsigned int) mmap(0, sizeof(MCC_BOOKEEPING_STRUCT), PROT_READ, MAP_SHARED, fd, 0);
	if(bookeeping_p < 0)
		return MCC_ERR_NOMEM;

	if(ioctl(fd, MCC_SET_NODE, &node) < 0)
	{
		if(errno == EFAULT)
			return MCC_ERR_DEV;
	}


    return MCC_SUCCESS;
}

/*!
 * \brief This function de-initializes the Multi Core Communication subsystem for a given node.
 *
 * Frees all resources of the node. Deletes all endpoints and frees any buffers that may have been queued there.
 *
 * \param[in] node Node number to be deinitialized.
 *
 * \return MCC_SUCCESS
 * \return MCC_ERR_DEV (internal driver error)
 * \return MCC_ERR_ENDPOINT (invalid endpoint)
 * \return MCC_ERR_SEMAPHORE (semaphore handling error)
 */
int mcc_destroy(MCC_NODE node)
{
    close(fd);

    return MCC_SUCCESS;
}

/*!
 * \brief This function creates an endpoint.
 *
 * Create an endpoint on the local node with the specified port number.
 * The core and node provided in endpoint must match the callers core and
 * node and the port argument must match the endpoint port.
 *
 * \param[out] endpoint Pointer to the endpoint triplet to be created.
 * \param[in] port Port number.
 *
 * \return MCC_SUCCESS
 * \return MCC_ERR_DEV (internal driver error)
 * \return MCC_ERR_NOMEM (maximum number of endpoints exceeded)
 * \return MCC_ERR_ENDPOINT (invalid value for core, node, or port)
 * \return MCC_ERR_SEMAPHORE (semaphore handling error)
 */
int mcc_create_endpoint(MCC_ENDPOINT *endpoint, MCC_PORT port)
{
	endpoint->core = this_core;
	endpoint->node = this_node;
	endpoint->port = port;

	if(ioctl(fd, MCC_CREATE_ENDPOINT, endpoint) < 0)
	{
		if(errno == EFAULT)
			return MCC_ERR_DEV;
		if(errno == EBUSY)
			return MCC_ERR_SEMAPHORE;
		if(errno == EINVAL)
			return MCC_ERR_ENDPOINT;
	}

    return MCC_SUCCESS;
}

/*!
  * \brief This function destroys an endpoint.
 *
 * Destroy an endpoint on the local node and frees any buffers that may be queued.
 *
 * \param[in] endpoint Pointer to the endpoint triplet to be deleted.
 *
 * \return MCC_SUCCESS
 * \return MCC_ERR_DEV (internal driver error)
 * \return MCC_ERR_ENDPOINT (the endpoint doesn't exist)
 * \return MCC_ERR_SEMAPHORE (semaphore handling error)
*/
int mcc_destroy_endpoint(MCC_ENDPOINT *endpoint)
{
	if(endpoint->node != this_node)
		return MCC_ERR_ENDPOINT;

	if(ioctl(fd, MCC_DESTROY_ENDPOINT, endpoint) < 0)
	{
		if(errno == EFAULT)
			return MCC_ERR_DEV;
		if(errno == EBUSY)
			return MCC_ERR_SEMAPHORE;
		if(errno == EINVAL)
			return MCC_ERR_ENDPOINT;
	}

    return MCC_SUCCESS;
}

/*
* routine to set modes - common tasks to send, recv copy and nocopy modes
*/
int set_io_modes(int set_endpoint_command, MCC_ENDPOINT *current_endpoint, MCC_ENDPOINT *new_endpoint, unsigned int timeout_us)
{
	int new_block_mode = timeout_us == 0 ? O_NONBLOCK : 0;

	// set the endpoint if not already
	if(!MCC_ENDPOINTS_EQUAL((*current_endpoint), (*new_endpoint)))
	{
		if(ioctl(fd, set_endpoint_command, new_endpoint) < 0)
		{
			if(errno == EFAULT)
				return MCC_ERR_DEV;
			if(errno == EBUSY)
				return MCC_ERR_SEMAPHORE;
			if(errno == EINVAL)
				return MCC_ERR_ENDPOINT;
		}

		*current_endpoint = *new_endpoint;
	}

	//check if timeout changed
	if(timeout_us != current_timeout_us)
	{
		if(ioctl(fd, MCC_SET_TIMEOUT, &timeout_us) < 0)
			return MCC_ERR_DEV;

		if((timeout_us == 0) || (current_timeout_us == 0))
		{
			if(fcntl(fd, F_SETFL, new_block_mode))
				return MCC_ERR_DEV;
		}
		current_timeout_us = timeout_us;
	}

	return 0;
}

/*!
 * \brief This function sends a message to an endpoint.
 *
 * The message is copied into the MCC buffer and the destination core is signaled.
 *
 * \param[in] endpoint Pointer to the receiving endpoint to send to.
 * \param[in] msg Pointer to the message to be sent.
 * \param[in] msg_size Size of the message to be sent in bytes.
 * \param[in] timeout_us Timeout, in microseconds, to wait for a free buffer. A value of 0 means dont wait (non-blocking call), 0xffffffff means wait forever (blocking call).
 *
 * \return MCC_SUCCESS
 * \return MCC_ERR_DEV (internal driver error)
 * \return MCC_ERR_ENDPOINT (the endpoint does not exist)
 * \return MCC_ERR_SEMAPHORE (semaphore handling error)
 * \return MCC_ERR_INVAL (an invalid address has been supplied for msg or the msg_size exceeds the size of a data buffer)
 * \return MCC_ERR_TIMEOUT (timeout exceeded before a buffer became available)
 * \return MCC_ERR_NOMEM (no free buffer available and timeout_us set to 0)
 * \return MCC_ERR_SQ_FULL (signal queue is full)
 */
int mcc_send(MCC_ENDPOINT *endpoint, void *msg, MCC_MEM_SIZE msg_size, unsigned int timeout_us)
{
	int retval = set_io_modes(MCC_SET_SEND_ENDPOINT, &send_endpoint, endpoint, timeout_us);
	if(retval)
		return retval;

	// do the write
	errno = 0;
	write(fd, msg, msg_size);

	switch (errno)
	{
	case -EFAULT:
		return MCC_ERR_DEV;
	case -EBUSY:
		return MCC_ERR_SEMAPHORE;
	case -EINVAL:
		return MCC_ERR_ENDPOINT;
	case -ETIME:
		return MCC_ERR_TIMEOUT;
	case -ENOMEM:
		return MCC_ERR_NOMEM;
	case 0:
		return MCC_SUCCESS;
	}
	return MCC_ERR_DEV;
}


/*!
* \brief This function receives a message from the specified endpoint if one is available.
 *        The data will be copied from the receive buffer into the user supplied buffer.
 *
 * This is the "receive with copy" version of the MCC receive function. This version is simple
 * to use but includes the cost of copying the data from shared memory into the user space buffer.
 * The user has no obligation or burden to manage shared memory buffers.
 *
 * \param[in] endpoint Pointer to the receiving endpoint to receive from.
 * \param[in] buffer Pointer to the user-app. buffer data will be copied into.
 * \param[in] buffer_size The maximum number of bytes to copy.
 * \param[out] recv_size Pointer to an MCC_MEM_SIZE that will contain the number of bytes actually copied into the buffer.
 * \param[in] timeout_us Timeout, in microseconds, to wait for a free buffer. A value of 0 means dont wait (non-blocking call), 0xffffffff means wait forever (blocking call).
 *
 * \return MCC_SUCCESS
 * \return MCC_ERR_DEV (internal driver error)
 * \return MCC_ERR_ENDPOINT (the endpoint does not exist)
 * \return MCC_ERR_SEMAPHORE (semaphore handling error)
 * \return MCC_ERR_TIMEOUT (timeout exceeded before a buffer became available)
 */
int mcc_recv_copy(MCC_ENDPOINT *endpoint, void *buffer, MCC_MEM_SIZE buffer_size, MCC_MEM_SIZE *recv_size, unsigned int timeout_us)
{
	int retval = set_io_modes(MCC_SET_RECEIVE_ENDPOINT, &recv_endpoint, endpoint, timeout_us);
	if(retval)
		return retval;

	//check copy or no copy mode
	MCC_READ_MODE read_mode = MCC_READ_MODE_COPY;
	if(read_mode != current_read_mode)
	{
		if(ioctl(fd, MCC_SET_READ_MODE, &read_mode))
			return MCC_ERR_INVAL;
		current_read_mode = read_mode;
	}

	errno = 0;
	*recv_size = read(fd, buffer, buffer_size);

	switch (errno)
	{
	case -EFAULT:
		return MCC_ERR_DEV;
	case -EBUSY:
		return MCC_ERR_SEMAPHORE;
	case -EINVAL:
		return MCC_ERR_ENDPOINT;
	case -ETIME:
		return MCC_ERR_TIMEOUT;
	case -ENOMEM:
		return MCC_ERR_NOMEM;
	case 0:
		return MCC_SUCCESS;
	}
	return MCC_ERR_DEV;
}

/*!
 * \brief This function receives a message from the specified endpoint if one is available. The data is NOT copied into the user-app. buffer.
 *
 * This is the "zero-copy receive" version of the MCC receive function. No data is copied, just
 * the pointer to the data is returned. This version is fast, but requires the user to manage
 * buffer allocation. Specifically the user must decide when a buffer is no longer in use and
 * make the appropriate API call to free it.
 *
 * \param[in] endpoint Pointer to the receiving endpoint to receive from.
 * \param[out] buffer_p Pointer to the MCC buffer of the shared memory where the received data is stored.
 * \param[out] recv_size Pointer to an MCC_MEM_SIZE that will contain the number of valid bytes in the buffer.
 * \param[in] timeout_us Timeout, in microseconds, to wait for a free buffer. A value of 0 means dont wait (non-blocking call), 0xffffffff means wait forever (blocking call).
 *
 * \return MCC_SUCCESS
 * \return MCC_ERR_DEV (internal driver error)
 * \return MCC_ERR_ENDPOINT (the endpoint does not exist)
 * \return MCC_ERR_SEMAPHORE (semaphore handling error)
 * \return MCC_ERR_TIMEOUT (timeout exceeded before a buffer became available)
 */
int mcc_recv_nocopy(MCC_ENDPOINT *endpoint, void **buffer_p, MCC_MEM_SIZE *recv_size, unsigned int timeout_us)
{
	unsigned int offset;

	int retval = set_io_modes(MCC_SET_RECEIVE_ENDPOINT, &recv_endpoint, endpoint, timeout_us);
	if(retval)
		return retval;

	//check copy or no copy mode
	MCC_READ_MODE read_mode = MCC_READ_MODE_NOCOPY;
	if(read_mode != current_read_mode)
	{
		if(ioctl(fd, MCC_SET_READ_MODE, &read_mode))
			return MCC_ERR_INVAL;
		current_read_mode = read_mode;
	}

	errno = 0;
	*recv_size = read(fd, &offset, 4);

	switch (errno)
	{
	case -EFAULT:
		return MCC_ERR_DEV;
	case -EBUSY:
		return MCC_ERR_SEMAPHORE;
	case -EINVAL:
		return MCC_ERR_ENDPOINT;
	case -ETIME:
		return MCC_ERR_TIMEOUT;
	case -ENOMEM:
		return MCC_ERR_NOMEM;
	case 0:
		// data pointer = bookeeping offset of the base of the buffer + offset into the data part of the buffer
		*buffer_p = (void *)(offset + bookeeping_p + offsetof(MCC_RECEIVE_BUFFER, data));
		return MCC_SUCCESS;
	}
	return MCC_ERR_DEV;
}

/*!
 * \brief This function returns number of buffers currently queued at the endpoint.
 *
 * Checks if messages are available on a receive endpoint. The call only checks the
 * availability of messages and does not dequeue them.
 *
 * \param[in] endpoint Pointer to the endpoint structure.
 * \param[out] num_msgs Pointer to an int that will contain the number of buffers queued.
 *
 * \return MCC_SUCCESS
 * \return MCC_ERR_ENDPOINT (the endpoint does not exist)
 * \return MCC_ERR_SEMAPHORE (semaphore handling error)
 */
int mcc_msgs_available(MCC_ENDPOINT *endpoint, unsigned int *num_msgs)
{
	struct mcc_queue_info_struct q_info;

	q_info.endpoint.core = endpoint->core;
	q_info.endpoint.node = endpoint->node;
	q_info.endpoint.port = endpoint->port;

	if(ioctl(fd, MCC_GET_QUEUE_INFO, &q_info) < 0)
		return MCC_ERR_ENDPOINT;

	*num_msgs = q_info.current_queue_length;

	return MCC_SUCCESS;
}

/*!
  * \brief This function frees a buffer previously returned by mcc_recv_nocopy().
 *
 * Once the zero-copy mechanism of receiving data is used this function
 * has to be called to free a buffer and to make it available for the next data
 * transfer.
 *
 * \param[in] buffer Pointer to the buffer to be freed.
 *
 * \return MCC_SUCCESS
 * \return MCC_ERR_ENDPOINT (the endpoint does not exist)
 * \return MCC_ERR_SEMAPHORE (semaphore handling error)
 */
int mcc_free_buffer(void *buffer)
{
	unsigned int offset = (unsigned int)buffer - (bookeeping_p + offsetof(MCC_RECEIVE_BUFFER, data));
	if(ioctl(fd, MCC_FREE_RECEIVE_BUFFER, &offset) < 0)
		return MCC_ERR_INVAL;
	return MCC_SUCCESS;
}

/*!
  * \brief This function returns information about the MCC sub system.
 *
 * Returns implementation specific information.
 *
 * \param[in] node Node number.
 * \param[out] info_data Pointer to the MCC_INFO_STRUCT structure to hold returned data.
 *
 * \return MCC_SUCCESS
 * \return MCC_ERR_DEV (internal driver error)
 * \return MCC_ERR_SEMAPHORE (semaphore handling error)
 */
int mcc_get_info(MCC_NODE node, MCC_INFO_STRUCT* info_data)
{
		if(ioctl(fd, MCC_GET_INFO, info_data) < 0)
		{
			if(errno == EFAULT)
				return MCC_ERR_DEV;
			if(errno == EBUSY)
				return MCC_ERR_SEMAPHORE;
			if(errno == EINVAL)
				return MCC_ERR_ENDPOINT;
		}
		return MCC_SUCCESS;
}

