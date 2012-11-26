#include "mcc_config.h"
#include "mcc_common.h"

MCC_BOOKEEPING_STRUCT * bookeeping_data;

/*!
 * \brief This function registers an endpoint.
 *
 * Register an endpoint with specified structure / params (core, node and port).
 *
 * \param[in] endpoint Pointer to the endpoint structure.
 * \param[Out] Status
 */
int mcc_register_endpoint(MCC_ENDPOINT endpoint)
{
	int i = 0;
	for(i = 0; i < MCC_ATTR_MAX_RECEIVE_ENDPOINTS; i++) {

		if(bookeeping_data->endpoint_table[i].endpoint.port == MCC_RESERVED_PORT_NUMBER) {
			bookeeping_data->endpoint_table[i].endpoint.core = endpoint.core;
			bookeeping_data->endpoint_table[i].endpoint.node = endpoint.node;
			bookeeping_data->endpoint_table[i].endpoint.port = endpoint.port;
			bookeeping_data->endpoint_table[i].list = &bookeeping_data->r_lists[i];
			return MCC_SUCCESS;
		}
	}
	return MCC_ERR_ENDPOINT;
}

/*!
 * \brief This function removes an endpoint.
 *
 * Removes an endpoint with specified structure / params (core, node and port).
 *
 * \param[in] endpoint Pointer to the endpoint structure.
 * \param[Out] Status
 */
int mcc_remove_endpoint(MCC_ENDPOINT endpoint)
{
	int i=0;
	for(i = 0; i < MCC_ATTR_MAX_RECEIVE_ENDPOINTS; i++) {

		if(	(bookeeping_data->endpoint_table[i].endpoint.core == endpoint.core) &&
			(bookeeping_data->endpoint_table[i].endpoint.node == endpoint.node) &&
			(bookeeping_data->endpoint_table[i].endpoint.port == endpoint.port)) {

			// clear the queue
			MCC_RECEIVE_BUFFER * buffer = mcc_dequeue_buffer(bookeeping_data->endpoint_table[i].list);
			while(buffer) {
				mcc_queue_buffer(&bookeeping_data->free_list, buffer);
				buffer = mcc_dequeue_buffer(bookeeping_data->endpoint_table[i].list);
			}

			// indicate free
			bookeeping_data->endpoint_table[i].endpoint.port = MCC_RESERVED_PORT_NUMBER;
			return MCC_SUCCESS;
		}
	}
	return MCC_ERR_ENDPOINT;
}

/*!
 * \brief This function dequeues the buffer.
 *
 * Dequeues the buffer from the list.
 *
 * \param[in] Pointer to the MCC_RECEIVE_LIST structure.
 * \param[Out] Pointer to MCC_RECEIVE_BUFFER
 */
MCC_RECEIVE_BUFFER * mcc_dequeue_buffer(MCC_RECEIVE_LIST *list)
{
	MCC_RECEIVE_BUFFER * next_buf = list->head;
	if(next_buf) {
		list->head = next_buf->next;
		if(list->tail == next_buf)
			list->tail = null;
	}

	return next_buf;
}

/*!
 * \brief This function queues the buffer.
 *
 * Queues the buffer in the list.
 *
 * \param[in] Pointer to the MCC_RECEIVE_LIST structure and pointer to MCC_RECEIVE_BUFFER.
 * \param[Out] none
 */
void mcc_queue_buffer(MCC_RECEIVE_LIST *list, MCC_RECEIVE_BUFFER * r_buffer)
{
	MCC_RECEIVE_BUFFER * last_buf = list->tail;
	if(last_buf)
		last_buf->next = r_buffer;
	else
		list->head = r_buffer;
	r_buffer->next = null;
	list->tail = r_buffer;
}

/*!
 * \brief This function returns the endpoint list.
 *
 * Returns the MCC_RECEIVE_LIST respective to the endpoint structure provided.
 *
 * \param[in] Pointer to the MCC_ENDPOINT structure.
 * \param[Out] MCC_RECEIVE_LIST / null.
 */
MCC_RECEIVE_LIST * mcc_get_endpoint_list(MCC_ENDPOINT endpoint)
{
	int i=0;
	for(i = 0; i<MCC_ATTR_MAX_RECEIVE_ENDPOINTS; i++) {

		if( (bookeeping_data->endpoint_table[i].endpoint.core == endpoint.core) &&
			(bookeeping_data->endpoint_table[i].endpoint.node == endpoint.node) &&
			(bookeeping_data->endpoint_table[i].endpoint.port == endpoint.port))
			return bookeeping_data->endpoint_table[i].list;
	}
	return null;
}


