/**HEADER********************************************************************
*
* Copyright (c) 2012 Freescale Semiconductor;
* All Rights Reserved
*
***************************************************************************
*
* THIS SOFTWARE IS PROVIDED BY FREESCALE "AS IS" AND ANY EXPRESSED OR
* IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
* OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
* IN NO EVENT SHALL FREESCALE OR ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
* INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
* (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
* SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
* HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
* STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
* IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
* THE POSSIBILITY OF SUCH DAMAGE.
*
**************************************************************************
*
* $FileName: responder.c$
* $Version : 4.0.1.0$
* $Date    : Feb-4-2013$
*
* Comments:
*
*   This file contains the source for responder to test application.
*
*END************************************************************************/

#if MCC_TEST_APP == 0

#include "test.h"

#if ! BSPCFG_ENABLE_IO_SUBSYSTEM
#error This application requires BSPCFG_ENABLE_IO_SUBSYSTEM defined non-zero in user_config.h. Please recompile BSP with this option.
#endif


#ifndef BSP_DEFAULT_IO_CHANNEL_DEFINED
#error This application requires BSP_DEFAULT_IO_CHANNEL to be not NULL. Please set corresponding BSPCFG_ENABLE_TTYx to non-zero in user_config.h and recompile BSP with this option.
#endif

const TASK_TEMPLATE_STRUCT  MQX_template_list[] =
{
   /* Task Index,  Function,       Stack,  Priority,  Name,         Attributes,           Param,  Time Slice */
    { RESP_TASK,   responder_task, 10000,  9,         "Responder",  MQX_AUTO_START_TASK,  0,      0 },
    { 0 }
};

/*TASK*----------------------------------------------------------
*
* Task Name : responder_task
* Comments  :
*     This task creates a message queue then waits for a message.
*     Upon receiving the message the data is incremented before
*     the message is returned to the sender.
*END*-----------------------------------------------------------*/

void responder_task(uint_32 dummy)
{
    CONTROL_MESSAGE msg;
    ACKNOWLEDGE_MESSAGE ack_msg;
    MCC_MEM_SIZE    num_of_received_bytes;
    CORE_MUTEX_PTR  coremutex_app_ptr;
    MCC_INFO_STRUCT mcc_info;
    int             ret_value;
    CONTROL_MESSAGE_DATA_CREATE_EP_PARAM_PTR data_create_ep_param;
    CONTROL_MESSAGE_DATA_DESTROY_EP_PARAM_PTR data_destroy_ep_param;
    CONTROL_MESSAGE_DATA_SEND_PARAM_PTR data_send_param;
    CONTROL_MESSAGE_DATA_RECV_PARAM_PTR data_recv_param;
    CONTROL_MESSAGE_DATA_MSG_AVAIL_PARAM_PTR data_msg_avail_param;
    CONTROL_MESSAGE_DATA_GET_INFO_PARAM_PTR data_get_info_param;
    void*           uut_app_buffer_ptr[255];
    void*           uut_temp_buffer_ptr;
    TIME_STRUCT     uut_time;

#if PRINT_ON
    /* create core mutex used in the app. for accessing the serial console */
    coremutex_app_ptr = _core_mutex_create( 0, 2, MQX_TASK_QUEUE_FIFO );
#endif

    bookeeping_data = (MCC_BOOKEEPING_STRUCT *)MCC_BASE_ADDRESS;
    ret_value = mcc_initialize(MCC_UUT_NODE);
    ret_value = mcc_get_info(MCC_UUT_NODE, &mcc_info);
    if(MCC_SUCCESS == ret_value && (strcmp(mcc_info.version_string, MCC_VERSION_STRING) != 0)) {
#if PRINT_ON
        _core_mutex_lock(coremutex_app_ptr);
        printf("\n\n\nError, attempting to use different versions of the MCC library on each core! Application is stopped now.\n");
        _core_mutex_unlock(coremutex_app_ptr);
#endif
        mcc_destroy(MCC_UUT_NODE);
        _task_block();
    }
    ret_value = mcc_create_endpoint(&uut_control_endpoint, uut_control_endpoint.port);
#if PRINT_ON
    _core_mutex_lock(coremutex_app_ptr);
    printf("\n\n\nResponder task started, MCC version is %s\n", mcc_info.version_string);
    _core_mutex_unlock(coremutex_app_ptr);
#endif
    while (TRUE) {
        ret_value = mcc_recv_copy(&uut_control_endpoint, &msg, sizeof(CONTROL_MESSAGE), &num_of_received_bytes, 0xffffffff);
        if(MCC_SUCCESS != ret_value) {
#if PRINT_ON
            _core_mutex_lock(coremutex_app_ptr);
            printf("Responder task receive error: %i\n", ret_value);
            _core_mutex_unlock(coremutex_app_ptr);
#endif
        } else {
#if PRINT_ON
            _core_mutex_lock(coremutex_app_ptr);
            printf("Responder task received a msg\n");
            printf("Message: Size=%x, CMD = %x, DATA = %x\n", num_of_received_bytes, msg.CMD, msg.DATA);
            _core_mutex_unlock(coremutex_app_ptr);
#endif

            switch(msg.CMD){
            case CTR_CMD_CREATE_EP:
                data_create_ep_param = (CONTROL_MESSAGE_DATA_CREATE_EP_PARAM_PTR)&msg.DATA;
                uut_app_buffer_ptr[data_create_ep_param->uut_endpoint.port] = _mem_alloc(data_create_ep_param->uut_app_buffer_size);
                ret_value = mcc_create_endpoint(&data_create_ep_param->uut_endpoint, data_create_ep_param->uut_endpoint.port);
                if(MCC_SUCCESS != ret_value) {
                    _mem_free((void*)uut_app_buffer_ptr[data_create_ep_param->uut_endpoint.port]);
                }
                if(ACK_REQUIRED_YES == msg.ACK_REQUIRED) {
                    ack_msg.CMD_ACK = CTR_CMD_CREATE_EP;
                    ack_msg.RETURN_VALUE = ret_value;
                    ret_value = mcc_send(&data_create_ep_param->endpoint_to_ack, &ack_msg, sizeof(ACKNOWLEDGE_MESSAGE), 0xFFFFFFFF);
                }
                break;
            case CTR_CMD_RECV:
                data_recv_param = (CONTROL_MESSAGE_DATA_RECV_PARAM_PTR)&msg.DATA;
                if(CMD_RECV_MODE_COPY == data_recv_param->mode) {
                    if(0xffffffff == data_recv_param->timeout_us) {
                        ret_value = mcc_recv_copy(&data_recv_param->uut_endpoint, uut_app_buffer_ptr[data_recv_param->uut_endpoint.port], data_recv_param->uut_app_buffer_size, &num_of_received_bytes, data_recv_param->timeout_us);
                    }
                    else if(0 == data_recv_param->timeout_us) {
                        do {
                            ret_value = mcc_recv_copy(&data_recv_param->uut_endpoint, uut_app_buffer_ptr[data_recv_param->uut_endpoint.port], data_recv_param->uut_app_buffer_size, &num_of_received_bytes, data_recv_param->timeout_us);
                        } while(MCC_ERR_TIMEOUT == ret_value);
                    }
                    else {
                        _time_get(&uut_time);
                        ack_msg.TS1_SEC = uut_time.SECONDS;
                        ack_msg.TS1_MSEC = uut_time.MILLISECONDS;
                        ret_value = mcc_recv_copy(&data_recv_param->uut_endpoint, uut_app_buffer_ptr[data_recv_param->uut_endpoint.port], data_recv_param->uut_app_buffer_size, &num_of_received_bytes, data_recv_param->timeout_us);
                        _time_get(&uut_time);
                        ack_msg.TS2_SEC = uut_time.SECONDS;
                        ack_msg.TS2_MSEC = uut_time.MILLISECONDS;
                    }
                }
                else if(CMD_RECV_MODE_NOCOPY == data_recv_param->mode) {
                    if(0xffffffff == data_recv_param->timeout_us) {
                        ret_value = mcc_recv_nocopy(&data_recv_param->uut_endpoint, (void**)&uut_temp_buffer_ptr, &num_of_received_bytes, data_recv_param->timeout_us);
                    }
                    else if(0 == data_recv_param->timeout_us) {
                        do {
                            ret_value = mcc_recv_nocopy(&data_recv_param->uut_endpoint, (void**)&uut_temp_buffer_ptr, &num_of_received_bytes, data_recv_param->timeout_us);
                        } while(MCC_ERR_TIMEOUT == ret_value);
                    }
                    else {
                        _time_get(&uut_time);
                        ack_msg.TS1_SEC = uut_time.SECONDS;
                        ack_msg.TS1_MSEC = uut_time.MILLISECONDS;
                        ret_value = mcc_recv_nocopy(&data_recv_param->uut_endpoint, (void**)&uut_temp_buffer_ptr, &num_of_received_bytes, data_recv_param->timeout_us);
                        _time_get(&uut_time);
                        ack_msg.TS2_SEC = uut_time.SECONDS;
                        ack_msg.TS2_MSEC = uut_time.MILLISECONDS;
                    }
                    if(MCC_SUCCESS == ret_value) {
                        mcc_memcpy((void*)uut_temp_buffer_ptr, (void*)uut_app_buffer_ptr[data_recv_param->uut_endpoint.port], (unsigned int)num_of_received_bytes);
                        mcc_free_buffer(&data_recv_param->uut_endpoint, uut_temp_buffer_ptr);
                    }
                }
                if(ACK_REQUIRED_YES == msg.ACK_REQUIRED) {
                    ack_msg.CMD_ACK = CTR_CMD_RECV;
                    ack_msg.RETURN_VALUE = ret_value;
                    ret_value = mcc_send(&data_recv_param->endpoint_to_ack, &ack_msg, sizeof(ACKNOWLEDGE_MESSAGE), 0xFFFFFFFF);
                }
                break;
            case CTR_CMD_SEND:
                data_send_param = (CONTROL_MESSAGE_DATA_SEND_PARAM_PTR)&msg.DATA;
                if((0 != data_send_param->timeout_us) && (0xffffffff != data_send_param->timeout_us)) {
                    _time_get(&uut_time);
                    ack_msg.TS1_SEC = uut_time.SECONDS;
                    ack_msg.TS1_MSEC = uut_time.MILLISECONDS;
                }
                if(0 == strncmp((const char *)data_send_param->msg, "", 1)) {
                    /* send the content of the uut_app_buffer_ptr */
                    ret_value = mcc_send(&data_send_param->dest_endpoint, uut_app_buffer_ptr[data_send_param->uut_endpoint.port], data_send_param->msg_size, data_send_param->timeout_us);
                }
                else {
                    /* send what received in the control command */
                    ret_value = mcc_send(&data_send_param->dest_endpoint, data_send_param->msg, data_send_param->msg_size, data_send_param->timeout_us);
                }
                if((0 != data_send_param->timeout_us) && (0xffffffff != data_send_param->timeout_us)) {
                    _time_get(&uut_time);
                    ack_msg.TS2_SEC = uut_time.SECONDS;
                    ack_msg.TS2_MSEC = uut_time.MILLISECONDS;
                }
                if(ACK_REQUIRED_YES == msg.ACK_REQUIRED) {
                    ack_msg.CMD_ACK = CTR_CMD_SEND;
                    ack_msg.RETURN_VALUE = ret_value;
                    ret_value = mcc_send(&data_send_param->endpoint_to_ack, &ack_msg, sizeof(ACKNOWLEDGE_MESSAGE), 0xFFFFFFFF);
                }
                break;
            case CTR_CMD_MSG_AVAIL:
                data_msg_avail_param = (CONTROL_MESSAGE_DATA_MSG_AVAIL_PARAM_PTR)&msg.DATA;
                ret_value = mcc_msgs_available(&data_msg_avail_param->uut_endpoint, (unsigned int*)uut_app_buffer_ptr[data_msg_avail_param->uut_endpoint.port]);
                if(ACK_REQUIRED_YES == msg.ACK_REQUIRED) {
                    ack_msg.CMD_ACK = CTR_CMD_MSG_AVAIL;
                    ack_msg.RETURN_VALUE = ret_value;
                    mcc_memcpy((void*)uut_app_buffer_ptr[data_msg_avail_param->uut_endpoint.port], (void*)ack_msg.RESP_DATA, sizeof(unsigned int));
                    ret_value = mcc_send(&data_msg_avail_param->endpoint_to_ack, &ack_msg, sizeof(ACKNOWLEDGE_MESSAGE), 0xFFFFFFFF);
                }
                break;
            case CTR_CMD_DESTROY_EP:
                data_destroy_ep_param = (CONTROL_MESSAGE_DATA_DESTROY_EP_PARAM_PTR)&msg.DATA;
                ret_value = mcc_destroy_endpoint(&data_destroy_ep_param->uut_endpoint);
                _mem_free((void*)uut_app_buffer_ptr[data_destroy_ep_param->uut_endpoint.port]);
                if(ACK_REQUIRED_YES == msg.ACK_REQUIRED) {
                    ack_msg.CMD_ACK = CTR_CMD_DESTROY_EP;
                    ack_msg.RETURN_VALUE = ret_value;
                    ret_value = mcc_send(&data_destroy_ep_param->endpoint_to_ack, &ack_msg, sizeof(ACKNOWLEDGE_MESSAGE), 0xFFFFFFFF);
                }
                break;
            case CTR_CMD_GET_INFO:
            	data_get_info_param = (CONTROL_MESSAGE_DATA_GET_INFO_PARAM_PTR)&msg.DATA;
                ret_value = mcc_get_info(data_get_info_param->uut_endpoint.node, (MCC_INFO_STRUCT*)&ack_msg.RESP_DATA);
                if(ACK_REQUIRED_YES == msg.ACK_REQUIRED) {
                    ack_msg.CMD_ACK = CTR_CMD_GET_INFO;
                    ack_msg.RETURN_VALUE = ret_value;
                    ret_value = mcc_send(&data_get_info_param->endpoint_to_ack, &ack_msg, sizeof(ACKNOWLEDGE_MESSAGE), 0xFFFFFFFF);
                }
                break;
            }
        }
    }
}

#endif
