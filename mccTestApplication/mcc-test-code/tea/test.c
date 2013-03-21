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
* $FileName: mcc.c$
* $Version : 3.8.2.0$
* $Date    : Oct-2-2012$
*
* Comments:
*
*   This file contains the source for test of the MCC.
*
*END************************************************************************/
#if MCC_TEST_APP == 1

#include "test.h"
#include "EUnit.h"
#include "eunit_crit_secs.h"

#if ! BSPCFG_ENABLE_IO_SUBSYSTEM
#error This application requires BSPCFG_ENABLE_IO_SUBSYSTEM defined non-zero in user_config.h. Please recompile BSP with this option.
#endif


#ifndef BSP_DEFAULT_IO_CHANNEL_DEFINED
#error This application requires BSP_DEFAULT_IO_CHANNEL to be not NULL. Please set corresponding BSPCFG_ENABLE_TTYx to non-zero in user_config.h and recompile BSP with this option.
#endif

const TASK_TEMPLATE_STRUCT  MQX_template_list[] =
{
   /* Task Index,  Function,       Stack,  Priority,  Name,         Attributes,           Param,  Time Slice */
    { MAIN_TASK,   main_task,      2000,   10,        "Main",       MQX_AUTO_START_TASK,  0,      0 },
    { 0 }
};

CORE_MUTEX_PTR  coremutex_app_ptr;
uint_32         msg_counter = 0;
/*TASK*--------------------------------------------------------------------
*
* Task Name    : tc_1_main_task
* Comments     : TEST #1: Testing MCC initialization
*                - MCC is initialized on the TEA side
*                - Control EP is created on the TEA side
*                - Creation of (MCC_ATTR_MAX_RECEIVE_ENDPOINTS-2) EPs on the
*                  UUT side is issued. Error conditions covered (creation of
*                  already created EP, creation of a new EP when the maximum
*                  number of EP reached, creation of the EP with
*                  port==MCC_RESERVED_PORT_NUMBER).
*                - Destroy of all created EP on the UUT side is issue. Error
*                  conditions covered (destroy of not registered EP and the EP
*                  with port==MCC_RESERVED_PORT_NUMBER).
*                - mcc_get_info() functions call is issued on the UUT side.
*
*END*---------------------------------------------------------------------*/
void tc_1_main_task(void)
{
    MCC_INFO_STRUCT mcc_info;
    CONTROL_MESSAGE msg;
    ACKNOWLEDGE_MESSAGE ack_msg;
    MCC_MEM_SIZE    num_of_received_bytes;
    int             ret_value, i;
    CONTROL_MESSAGE_DATA_CREATE_EP_PARAM data_create_ep_param;
    CONTROL_MESSAGE_DATA_DESTROY_EP_PARAM data_destroy_ep_param;
    CONTROL_MESSAGE_DATA_GET_INFO_PARAM data_get_info_param;
    MCC_ENDPOINT    uut_endpoint =  {MCC_UUT_CORE,MCC_UUT_NODE,MCC_UUT_EP_PORT};
    
    data_create_ep_param.uut_endpoint = uut_endpoint;
    data_create_ep_param.uut_app_buffer_size = UUT_APP_BUF_SIZE;
    data_create_ep_param.endpoint_to_ack = tea_control_endpoint;

    data_destroy_ep_param.uut_endpoint = uut_endpoint;
    data_destroy_ep_param.endpoint_to_ack = tea_control_endpoint;

    data_get_info_param.uut_endpoint = uut_endpoint;
    data_get_info_param.endpoint_to_ack = tea_control_endpoint;

#if PRINT_ON
    /* create core mutex used in the app. for accessing the serial console */
    coremutex_app_ptr = _core_mutex_create( 0, 2, MQX_TASK_QUEUE_FIFO );
#endif

    bookeeping_data = (MCC_BOOKEEPING_STRUCT *)MCC_BASE_ADDRESS;
    EU_ASSERT_REF_TC_MSG( tc_1_main_task, bookeeping_data != NULL, "TEST #1: 1.1 Bookeeping_data is not NULL");
    if(bookeeping_data == NULL)
        eunit_test_stop();

    ret_value = mcc_initialize(MCC_TEA_NODE);
    EU_ASSERT_REF_TC_MSG( tc_1_main_task, ret_value == MCC_SUCCESS, "TEST #1: 1.2 Testing mcc_initialize");
    if(ret_value != MCC_SUCCESS)
        eunit_test_stop();

    ret_value = mcc_get_info(MCC_TEA_NODE, &mcc_info);
    EU_ASSERT_REF_TC_MSG( tc_1_main_task, ret_value == MCC_SUCCESS, "TEST #1: 1.3 Testing return value of mcc_get_info");
    if(ret_value != MCC_SUCCESS)
        eunit_test_stop();

    ret_value = strcmp(mcc_info.version_string, MCC_VERSION_STRING);
    EU_ASSERT_REF_TC_MSG( tc_1_main_task, ret_value == MCC_SUCCESS, "TEST #1: 1.4 Testing mcc version_string");
    if(MCC_SUCCESS != ret_value)
        eunit_test_stop();

    /* Create TEA control endpoints */
    ret_value = mcc_create_endpoint(&tea_control_endpoint, tea_control_endpoint.port);
    EU_ASSERT_REF_TC_MSG( tc_1_main_task, ret_value == MCC_SUCCESS, "TEST #1: 1.5 Creating TEA control endpoint");
    if(ret_value != MCC_SUCCESS)
        eunit_test_stop();

    /* EP creation testing */
    /* Create MCC_ATTR_MAX_RECEIVE_ENDPOINTS-2 EPs - consider 2 EPs as TEA and UUT control EPs */
    msg.CMD = CTR_CMD_CREATE_EP;
    msg.ACK_REQUIRED = ACK_REQUIRED_NO;
    mcc_memcpy((void*)&data_create_ep_param, (void*)msg.DATA, (unsigned int)sizeof(struct control_message_data_create_ep_param));
    /* wait until the uut control endpoint is created by the other core */
    while(MCC_ERR_ENDPOINT == mcc_send(&uut_control_endpoint, &msg, sizeof(CONTROL_MESSAGE), 0xffffffff)) {
    }

    /* Creation of the same EP should not be possible - MCC_ERR_ENDPOINT should be returned */
    msg.CMD = CTR_CMD_CREATE_EP;
    msg.ACK_REQUIRED = ACK_REQUIRED_YES;
    mcc_memcpy((void*)&data_create_ep_param, (void*)msg.DATA, (unsigned int)sizeof(struct control_message_data_create_ep_param));
    ret_value = mcc_send(&uut_control_endpoint, &msg, sizeof(CONTROL_MESSAGE), 0xffffffff);
    ret_value = mcc_recv_copy(&tea_control_endpoint, &ack_msg, sizeof(ACKNOWLEDGE_MESSAGE), &num_of_received_bytes, 0xffffffff);
    EU_ASSERT_REF_TC_MSG( tc_1_main_task, CTR_CMD_CREATE_EP == ack_msg.CMD_ACK, "TEST #1: 1.6 Checking correct CTR_CMD_CREATE_EP acknowledge message 'CMD_ACK' content");
    EU_ASSERT_REF_TC_MSG( tc_1_main_task, MCC_ERR_ENDPOINT == ack_msg.RETURN_VALUE, "TEST #1: 1.7 Checking correct CTR_CMD_CREATE_EP acknowledge message 'RETURN_VALUE' content");

    for(i = 0; i < MCC_ATTR_MAX_RECEIVE_ENDPOINTS-3; i++) {
        data_create_ep_param.uut_endpoint.port++;
        msg.CMD = CTR_CMD_CREATE_EP;
        msg.ACK_REQUIRED = ACK_REQUIRED_YES;
        mcc_memcpy((void*)&data_create_ep_param, (void*)msg.DATA, (unsigned int)sizeof(struct control_message_data_create_ep_param));
        ret_value = mcc_send(&uut_control_endpoint, &msg, sizeof(CONTROL_MESSAGE), 0xffffffff);
        ret_value = mcc_recv_copy(&tea_control_endpoint, &ack_msg, sizeof(ACKNOWLEDGE_MESSAGE), &num_of_received_bytes, 0xffffffff);
        EU_ASSERT_REF_TC_MSG( tc_1_main_task, CTR_CMD_CREATE_EP == ack_msg.CMD_ACK, "TEST #1: 1.8 Checking correct CTR_CMD_CREATE_EP acknowledge message 'CMD_ACK' content");
        EU_ASSERT_REF_TC_MSG( tc_1_main_task, MCC_SUCCESS == ack_msg.RETURN_VALUE, "TEST #1: 1.9 Checking correct CTR_CMD_CREATE_EP acknowledge message 'RETURN_VALUE' content");
        EU_ASSERT_REF_TC_MSG( tc_1_main_task, 0 == strncmp((const char *)ack_msg.RESP_DATA, "MEM_OK", 7), "TEST #1: 1.10 Checking correct CTR_CMD_CREATE_EP response message content");
    }

    /* Another EP creation should not be possible - MCC_ERR_NOMEM should be returned */
    data_create_ep_param.uut_endpoint.port++;
    msg.CMD = CTR_CMD_CREATE_EP;
    msg.ACK_REQUIRED = ACK_REQUIRED_YES;
    mcc_memcpy((void*)&data_create_ep_param, (void*)msg.DATA, (unsigned int)sizeof(struct control_message_data_create_ep_param));
    ret_value = mcc_send(&uut_control_endpoint, &msg, sizeof(CONTROL_MESSAGE), 0xffffffff);
    ret_value = mcc_recv_copy(&tea_control_endpoint, &ack_msg, sizeof(ACKNOWLEDGE_MESSAGE), &num_of_received_bytes, 0xffffffff);
    EU_ASSERT_REF_TC_MSG( tc_1_main_task, CTR_CMD_CREATE_EP == ack_msg.CMD_ACK, "TEST #1: 1.11 Checking correct CTR_CMD_CREATE_EP acknowledge message 'CMD_ACK' content");
    EU_ASSERT_REF_TC_MSG( tc_1_main_task, MCC_ERR_NOMEM == ack_msg.RETURN_VALUE, "TEST #1: 1.12 Checking correct CTR_CMD_CREATE_EP acknowledge message 'RETURN_VALUE' content");

    /* EP destroy testing */
    /* Destroying an EP that has not been registered should fail and MCC_ERR_ENDPOINT should be returned */
    msg.CMD = CTR_CMD_DESTROY_EP;
    msg.ACK_REQUIRED = ACK_REQUIRED_YES;
    data_destroy_ep_param.uut_endpoint.port = data_create_ep_param.uut_endpoint.port;
    mcc_memcpy((void*)&data_destroy_ep_param, (void*)msg.DATA, (unsigned int)sizeof(struct control_message_data_destroy_ep_param));
    ret_value = mcc_send(&uut_control_endpoint, &msg, sizeof(CONTROL_MESSAGE), 0xffffffff);
    ret_value = mcc_recv_copy(&tea_control_endpoint, &ack_msg, sizeof(ACKNOWLEDGE_MESSAGE), &num_of_received_bytes, 0xffffffff);
    EU_ASSERT_REF_TC_MSG( tc_1_main_task, CTR_CMD_DESTROY_EP == ack_msg.CMD_ACK, "TEST #1: 1.13 Checking correct CTR_CMD_DESTROY_EP acknowledge message 'CMD_ACK' content");
    EU_ASSERT_REF_TC_MSG( tc_1_main_task, MCC_ERR_ENDPOINT == ack_msg.RETURN_VALUE, "TEST #1: 1.14 Checking correct CTR_CMD_DESTROY_EP acknowledge message 'RETURN_VALUE' content");

    /* Destroy all created EPs except of TEA and UUT control EPs */
    for(i = 0; i < MCC_ATTR_MAX_RECEIVE_ENDPOINTS-2; i++) {
        data_destroy_ep_param.uut_endpoint.port--;
        msg.CMD = CTR_CMD_DESTROY_EP;
        msg.ACK_REQUIRED = ACK_REQUIRED_YES;
        mcc_memcpy((void*)&data_destroy_ep_param, (void*)msg.DATA, (unsigned int)sizeof(struct control_message_data_destroy_ep_param));
        ret_value = mcc_send(&uut_control_endpoint, &msg, sizeof(CONTROL_MESSAGE), 0xffffffff);
        ret_value = mcc_recv_copy(&tea_control_endpoint, &ack_msg, sizeof(ACKNOWLEDGE_MESSAGE), &num_of_received_bytes, 0xffffffff);
        EU_ASSERT_REF_TC_MSG( tc_1_main_task, CTR_CMD_DESTROY_EP == ack_msg.CMD_ACK, "TEST #1: 1.15 Checking correct CTR_CMD_DESTROY_EP acknowledge message 'CMD_ACK' content");
        EU_ASSERT_REF_TC_MSG( tc_1_main_task, MCC_SUCCESS == ack_msg.RETURN_VALUE, "TEST #1: 1.16 Checking correct CTR_CMD_DESTROY_EP acknowledge message 'RETURN_VALUE' content");
        EU_ASSERT_REF_TC_MSG( tc_1_main_task, 0 == strncmp((const char *)ack_msg.RESP_DATA, "MEM_OK", 7), "TEST #1: 1.17 Checking correct CTR_CMD_DESTROY_EP response message content");
    }

    /* Creation of the EP with port==MCC_RESERVED_PORT_NUMBER should not be possible - MCC_ERR_ENDPOINT should be returned */
    msg.CMD = CTR_CMD_CREATE_EP;
    msg.ACK_REQUIRED = ACK_REQUIRED_YES;
    data_create_ep_param.uut_endpoint.port = MCC_RESERVED_PORT_NUMBER;
    mcc_memcpy((void*)&data_create_ep_param, (void*)msg.DATA, (unsigned int)sizeof(struct control_message_data_create_ep_param));
    ret_value = mcc_send(&uut_control_endpoint, &msg, sizeof(CONTROL_MESSAGE), 0xffffffff);
    ret_value = mcc_recv_copy(&tea_control_endpoint, &ack_msg, sizeof(ACKNOWLEDGE_MESSAGE), &num_of_received_bytes, 0xffffffff);
    EU_ASSERT_REF_TC_MSG( tc_1_main_task, CTR_CMD_CREATE_EP == ack_msg.CMD_ACK, "TEST #1: 1.18 Checking correct CTR_CMD_CREATE_EP acknowledge message 'CMD_ACK' content");
    EU_ASSERT_REF_TC_MSG( tc_1_main_task, MCC_ERR_ENDPOINT == ack_msg.RETURN_VALUE, "TEST #1: 1.19 Checking correct CTR_CMD_CREATE_EP acknowledge message 'RETURN_VALUE' content");
        
    /* Destroy of the EP with port==MCC_RESERVED_PORT_NUMBER should not be possible - MCC_ERR_ENDPOINT should be returned */
    msg.CMD = CTR_CMD_DESTROY_EP;
    msg.ACK_REQUIRED = ACK_REQUIRED_YES;
    data_destroy_ep_param.uut_endpoint.port = MCC_RESERVED_PORT_NUMBER;
    mcc_memcpy((void*)&data_destroy_ep_param, (void*)msg.DATA, (unsigned int)sizeof(struct control_message_data_destroy_ep_param));
    ret_value = mcc_send(&uut_control_endpoint, &msg, sizeof(CONTROL_MESSAGE), 0xffffffff);
    ret_value = mcc_recv_copy(&tea_control_endpoint, &ack_msg, sizeof(ACKNOWLEDGE_MESSAGE), &num_of_received_bytes, 0xffffffff);
    EU_ASSERT_REF_TC_MSG( tc_1_main_task, CTR_CMD_DESTROY_EP == ack_msg.CMD_ACK, "TEST #1: 1.20 Checking correct CTR_CMD_DESTROY_EP acknowledge message 'CMD_ACK' content");
    EU_ASSERT_REF_TC_MSG( tc_1_main_task, MCC_ERR_ENDPOINT == ack_msg.RETURN_VALUE, "TEST #1: 1.21 Checking correct CTR_CMD_DESTROY_EP acknowledge message 'RETURN_VALUE' content");

    /* Force mcc_get_info() function to be called on the UUT side and check results */
    msg.CMD = CTR_CMD_GET_INFO;
    msg.ACK_REQUIRED = ACK_REQUIRED_YES;
    mcc_memcpy((void*)&data_get_info_param, (void*)msg.DATA, (unsigned int)sizeof(struct control_message_data_get_info_param));
    ret_value = mcc_send(&uut_control_endpoint, &msg, sizeof(CONTROL_MESSAGE), 0xffffffff);
    ret_value = mcc_recv_copy(&tea_control_endpoint, &ack_msg, sizeof(ACKNOWLEDGE_MESSAGE), &num_of_received_bytes, 0xffffffff);
    EU_ASSERT_REF_TC_MSG( tc_1_main_task, CTR_CMD_GET_INFO == ack_msg.CMD_ACK, "TEST #1: 1.22 Checking correct CTR_CMD_GET_INFO acknowledge message 'CMD_ACK' content");
    EU_ASSERT_REF_TC_MSG( tc_1_main_task, MCC_SUCCESS == ack_msg.RETURN_VALUE, "TEST #1: 1.23 Checking correct CTR_CMD_GET_INFO acknowledge message 'RETURN_VALUE' content");
    EU_ASSERT_REF_TC_MSG( tc_1_main_task, 0 == strncmp((const char *)ack_msg.RESP_DATA, MCC_VERSION_STRING, sizeof(MCC_VERSION_STRING)), "TEST #1: 1.21 Checking correct CTR_CMD_GET_INFO response message content");

#if PRINT_ON
    _core_mutex_lock(coremutex_app_ptr);
    printf("\n\n\nSender task started, MCC version is %s\n", mcc_info.version_string);
    _core_mutex_unlock(coremutex_app_ptr);
#endif
}

/*TASK*--------------------------------------------------------------------
*
* Task Name    : tc_2_main_task
* Comments     : TEST #2: Testing MCC pingpong1
*                - One EP on TEA side is created.
*                - One EP on UUT side is created.
*                - CTR_CMD_RECV cmd is issued with mode == CMD_RECV_MODE_COPY
*                  (i.e. mcc_recv_copy() function has to be used) and with
*                  uut_app_buffer_size == UUT_APP_BUF_SIZE (i.e. UUT APP buffer
*                  of UUT_APP_BUF_SIZE bytes size has to be created on the UUT side).
*                - A message with "aaa" content is sent to UUT EP.
*                - CTR_CMD_SEND cmd is issue  with msg == "abc" (i.e. send
*                  this "payload" back to the TEA EP).
*                - Recv/send sequence is repeated TEST_CNT times.
*                - Attempt to call mcc_recv_no_copy() on the UUT side
*                  with the invalid EP pointer.
*                - Attempt to call mcc_send() on the UUT side
*                  with the invalid EP pointer.
*                - Change the timeout_us parameter of the CTR_CMD_RECV cmd to 0
*                  (i.e. do not block but wait for a new message on the app. level)
*                  and repeat the same loop for TEST_CNT cycles again.
*                - Attempt to call mcc_send() on the UUT side with the invalid msg_size.
*                - Destroy both TEA and UUT EPs.
*
*END*---------------------------------------------------------------------*/
void tc_2_main_task(void)
{
    CONTROL_MESSAGE msg;
    ACKNOWLEDGE_MESSAGE ack_msg;
    MCC_MEM_SIZE    num_of_received_bytes;
    int             ret_value, i;
    CONTROL_MESSAGE_DATA_CREATE_EP_PARAM data_create_ep_param;
    CONTROL_MESSAGE_DATA_DESTROY_EP_PARAM data_destroy_ep_param;
    CONTROL_MESSAGE_DATA_SEND_PARAM data_send_param;
    CONTROL_MESSAGE_DATA_RECV_PARAM data_recv_param;
    MCC_ENDPOINT    uut_endpoint =  {MCC_UUT_CORE,MCC_UUT_NODE,MCC_UUT_EP_PORT};
    MCC_ENDPOINT    tea_endpoint =  {MCC_TEA_CORE,MCC_TEA_NODE,MCC_TEA_EP_PORT};
    char            tea_app_buffer[TEA_APP_BUF_SIZE];
    
    data_create_ep_param.uut_endpoint = uut_endpoint;
    data_create_ep_param.uut_app_buffer_size = UUT_APP_BUF_SIZE;

    data_destroy_ep_param.uut_endpoint = uut_endpoint;
    data_destroy_ep_param.endpoint_to_ack = tea_control_endpoint;
    
    data_send_param.dest_endpoint = tea_endpoint;
    mcc_memcpy("abc", (void*)data_send_param.msg, (unsigned int)4);
    data_send_param.msg_size = CMD_SEND_MSG_SIZE;
    data_send_param.timeout_us = 0xffffffff;
    data_send_param.endpoint_to_ack = tea_control_endpoint;
    data_send_param.repeat_count = 1;
    
    data_recv_param.uut_endpoint = uut_endpoint;
    data_recv_param.uut_app_buffer_size = UUT_APP_BUF_SIZE;
    data_recv_param.timeout_us = 0xffffffff;
    data_recv_param.mode = CMD_RECV_MODE_COPY;
    data_recv_param.endpoint_to_ack = tea_control_endpoint;

    mcc_create_endpoint(&tea_endpoint, tea_endpoint.port);
    
    msg.CMD = CTR_CMD_CREATE_EP;
    msg.ACK_REQUIRED = ACK_REQUIRED_NO;
    mcc_memcpy((void*)&data_create_ep_param, (void*)msg.DATA, (unsigned int)sizeof(struct control_message_data_create_ep_param));
    /* wait until the uut control endpoint is created by the other core */
    while(MCC_ERR_ENDPOINT == mcc_send(&uut_control_endpoint, &msg, sizeof(CONTROL_MESSAGE), 0xffffffff)) {
    }

    for(i = 0; i < TEST_CNT; i++) {
        msg.CMD = CTR_CMD_RECV;
        msg.ACK_REQUIRED = ACK_REQUIRED_NO;
        mcc_memcpy((void*)&data_recv_param, (void*)msg.DATA, (unsigned int)sizeof(struct control_message_data_recv_param));
        mcc_send(&uut_control_endpoint, &msg, sizeof(CONTROL_MESSAGE), 0xffffffff);
        /* wait until the uut endpoint is created by the other core */
        while(MCC_ERR_ENDPOINT == mcc_send(&uut_endpoint, "aaa", 3, 0xffffffff)) {
        }

        msg.CMD = CTR_CMD_SEND;
        msg.ACK_REQUIRED = ACK_REQUIRED_NO;
        mcc_memcpy((void*)&data_send_param, (void*)msg.DATA, (unsigned int)sizeof(struct control_message_data_send_param));
        mcc_send(&uut_control_endpoint, &msg, sizeof(CONTROL_MESSAGE), 0xffffffff);

        ret_value = mcc_recv_copy(&tea_endpoint, &tea_app_buffer, TEA_APP_BUF_SIZE, &num_of_received_bytes, 0xffffffff);
        EU_ASSERT_REF_TC_MSG( tc_2_main_task, ret_value == MCC_SUCCESS, "TEST #2: 2.1 Message successfully received");
        if(MCC_SUCCESS != ret_value) {
#if PRINT_ON
            _core_mutex_lock(coremutex_app_ptr);
            printf("sender task receive error: %i\n", ret_value);
            _core_mutex_unlock(coremutex_app_ptr);
#endif
            eunit_test_stop();
        } else {
#if PRINT_ON
            _core_mutex_lock(coremutex_app_ptr);
            printf("Message from responder task: Size=%x, DATA = %x", num_of_received_bytes, tea_app_buffer);
            _core_mutex_unlock(coremutex_app_ptr);
#endif
        EU_ASSERT_REF_TC_MSG( tc_2_main_task, 0 == strncmp(tea_app_buffer, "abc", 3), "TEST #2: 2.2 Checking correct response message content");
        }
    }

    /* Attempt to call mcc_recv_copy() on the UUT side with the invalid EP pointer */
    data_recv_param.uut_endpoint = null_endpoint;
    msg.CMD = CTR_CMD_RECV;
    msg.ACK_REQUIRED = ACK_REQUIRED_YES;
    mcc_memcpy((void*)&data_recv_param, (void*)msg.DATA, (unsigned int)sizeof(struct control_message_data_recv_param));
    mcc_send(&uut_control_endpoint, &msg, sizeof(CONTROL_MESSAGE), 0xffffffff);
    ret_value = mcc_recv_copy(&tea_control_endpoint, &ack_msg, sizeof(ACKNOWLEDGE_MESSAGE), &num_of_received_bytes, 0xffffffff);
    EU_ASSERT_REF_TC_MSG( tc_2_main_task, CTR_CMD_RECV == ack_msg.CMD_ACK, "TEST #2: 2.3 Checking correct CTR_CMD_RECV acknowledge message 'CMD_ACK' content");
    EU_ASSERT_REF_TC_MSG( tc_2_main_task, MCC_ERR_ENDPOINT == ack_msg.RETURN_VALUE, "TEST #2: 2.4 Checking correct CTR_CMD_RECV acknowledge message 'RETURN_VALUE' content");

    /* Attempt to call mcc_send() on the UUT side with the invalid EP pointer */
    data_send_param.dest_endpoint = null_endpoint;
    msg.CMD = CTR_CMD_SEND;
    msg.ACK_REQUIRED = ACK_REQUIRED_YES;
    mcc_memcpy((void*)&data_send_param, (void*)msg.DATA, (unsigned int)sizeof(struct control_message_data_send_param));
    mcc_send(&uut_control_endpoint, &msg, sizeof(CONTROL_MESSAGE), 0xffffffff);
    ret_value = mcc_recv_copy(&tea_control_endpoint, &ack_msg, sizeof(ACKNOWLEDGE_MESSAGE), &num_of_received_bytes, 0xffffffff);
    EU_ASSERT_REF_TC_MSG( tc_2_main_task, CTR_CMD_SEND == ack_msg.CMD_ACK, "TEST #2: 2.5 Checking correct CTR_CMD_SEND acknowledge message 'CMD_ACK' content");
    EU_ASSERT_REF_TC_MSG( tc_2_main_task, MCC_ERR_ENDPOINT == ack_msg.RETURN_VALUE, "TEST #2: 2.6 Checking correct CTR_CMD_SEND acknowledge message 'RETURN_VALUE' content");



    /*
     * Now, execute non-blocking version of the mcc_recv_copy() function
     * */
    data_recv_param.timeout_us = 0;
    data_recv_param.uut_endpoint = uut_endpoint;
    data_send_param.dest_endpoint = tea_endpoint;

    for(i = 0; i < TEST_CNT; i++) {
        msg.CMD = CTR_CMD_RECV;
        msg.ACK_REQUIRED = ACK_REQUIRED_NO;
        mcc_memcpy((void*)&data_recv_param, (void*)msg.DATA, (unsigned int)sizeof(struct control_message_data_recv_param));
        mcc_send(&uut_control_endpoint, &msg, sizeof(CONTROL_MESSAGE), 0xffffffff);
        /* wait until the uut endpoint is created by the other core */
        while(MCC_ERR_ENDPOINT == mcc_send(&uut_endpoint, "aaa", 3, 0xffffffff)) {
        }

        msg.CMD = CTR_CMD_SEND;
        msg.ACK_REQUIRED = ACK_REQUIRED_NO;
        mcc_memcpy((void*)&data_send_param, (void*)msg.DATA, (unsigned int)sizeof(struct control_message_data_send_param));
        mcc_send(&uut_control_endpoint, &msg, sizeof(CONTROL_MESSAGE), 0xffffffff);

        ret_value = mcc_recv_copy(&tea_endpoint, &tea_app_buffer, TEA_APP_BUF_SIZE, &num_of_received_bytes, 0xffffffff);
        EU_ASSERT_REF_TC_MSG( tc_2_main_task, ret_value == MCC_SUCCESS, "TEST #2: 2.7 Message successfully received");
        if(MCC_SUCCESS != ret_value) {
#if PRINT_ON
            _core_mutex_lock(coremutex_app_ptr);
            printf("sender task receive error: %i\n", ret_value);
            _core_mutex_unlock(coremutex_app_ptr);
#endif
            eunit_test_stop();
        } else {
#if PRINT_ON
            _core_mutex_lock(coremutex_app_ptr);
            printf("Message from responder task: Size=%x, DATA = %x", num_of_received_bytes, tea_app_buffer);
            _core_mutex_unlock(coremutex_app_ptr);
#endif
        EU_ASSERT_REF_TC_MSG( tc_2_main_task, 0 == strncmp(tea_app_buffer, "abc", 3), "TEST #2: 2.8 Checking correct response message content");
        }
    }

    /* Attempt to call mcc_recv_copy() on the UUT side with the invalid EP pointer */
    data_recv_param.uut_endpoint = null_endpoint;
    msg.CMD = CTR_CMD_RECV;
    msg.ACK_REQUIRED = ACK_REQUIRED_YES;
    mcc_memcpy((void*)&data_recv_param, (void*)msg.DATA, (unsigned int)sizeof(struct control_message_data_recv_param));
    mcc_send(&uut_control_endpoint, &msg, sizeof(CONTROL_MESSAGE), 0xffffffff);
    ret_value = mcc_recv_copy(&tea_control_endpoint, &ack_msg, sizeof(ACKNOWLEDGE_MESSAGE), &num_of_received_bytes, 0xffffffff);
    EU_ASSERT_REF_TC_MSG( tc_2_main_task, CTR_CMD_RECV == ack_msg.CMD_ACK, "TEST #2: 2.9 Checking correct CTR_CMD_RECV acknowledge message 'CMD_ACK' content");
    EU_ASSERT_REF_TC_MSG( tc_2_main_task, MCC_ERR_ENDPOINT == ack_msg.RETURN_VALUE, "TEST #2: 2.10 Checking correct CTR_CMD_RECV acknowledge message 'RETURN_VALUE' content");

    /* Attempt to call mcc_send() on the UUT side with the invalid msg_size */
    data_send_param.msg_size = 1 + MCC_ATTR_BUFFER_SIZE_IN_BYTES;
    msg.CMD = CTR_CMD_SEND;
    msg.ACK_REQUIRED = ACK_REQUIRED_YES;
    mcc_memcpy((void*)&data_send_param, (void*)msg.DATA, (unsigned int)sizeof(struct control_message_data_send_param));
    mcc_send(&uut_control_endpoint, &msg, sizeof(CONTROL_MESSAGE), 0xffffffff);
    ret_value = mcc_recv_copy(&tea_control_endpoint, &ack_msg, sizeof(ACKNOWLEDGE_MESSAGE), &num_of_received_bytes, 0xffffffff);
    EU_ASSERT_REF_TC_MSG( tc_2_main_task, CTR_CMD_SEND == ack_msg.CMD_ACK, "TEST #2: 2.11 Checking correct CTR_CMD_SEND acknowledge message 'CMD_ACK' content");
    EU_ASSERT_REF_TC_MSG( tc_2_main_task, MCC_ERR_INVAL == ack_msg.RETURN_VALUE, "TEST #2: 2.12 Checking correct CTR_CMD_SEND acknowledge message 'RETURN_VALUE' content");

    /* Destroy EPs */
    msg.CMD = CTR_CMD_DESTROY_EP;
    msg.ACK_REQUIRED = ACK_REQUIRED_YES;
    mcc_memcpy((void*)&data_destroy_ep_param, (void*)msg.DATA, (unsigned int)sizeof(struct control_message_data_destroy_ep_param));
    mcc_send(&uut_control_endpoint, &msg, sizeof(CONTROL_MESSAGE), 0xffffffff);
    ret_value = mcc_recv_copy(&tea_control_endpoint, &ack_msg, sizeof(ACKNOWLEDGE_MESSAGE), &num_of_received_bytes, 0xffffffff);
    EU_ASSERT_REF_TC_MSG( tc_2_main_task, CTR_CMD_DESTROY_EP == ack_msg.CMD_ACK, "TEST #2: 2.13 Checking correct CTR_CMD_DESTROY_EP acknowledge message 'CMD_ACK' content");
    EU_ASSERT_REF_TC_MSG( tc_2_main_task, MCC_SUCCESS == ack_msg.RETURN_VALUE, "TEST #2: 2.14 Checking correct CTR_CMD_DESTROY_EP acknowledge message 'RETURN_VALUE' content");
    EU_ASSERT_REF_TC_MSG( tc_2_main_task, 0 == strncmp((const char *)ack_msg.RESP_DATA, "MEM_OK", 7), "TEST #2: 2.15 Checking correct CTR_CMD_DESTROY_EP response message content");

    mcc_destroy_endpoint(&tea_endpoint);
}

/*TASK*--------------------------------------------------------------------
*
* Task Name    : tc_3_main_task
* Comments     : TEST #3: Testing MCC pingpong2
*                - One EP on TEA side is created.
*                - One EP on UUT side is created.
*                - CTR_CMD_RECV cmd is issued with mode == CMD_RECV_MODE_NOCOPY
*                  (i.e. mcc_recv_no_copy() function has to be used) and with
*                  uut_app_buffer_size == UUT_APP_BUF_SIZE (i.e. UUT APP buffer
*                  of UUT_APP_BUF_SIZE bytes size has to be created on the UUT side).
*                - A message with "aaa" content is sent to UUT EP.
*                - CTR_CMD_SEND cmd is issue  with msg == "" (i.e. send
*                  the content of the UUT APP buffer to the TEA EP).
*                - Recv/send sequence is repeated TEST_CNT times.
*                - Attempt to call mcc_recv_no_copy() on the UUT side
*                  with the invalid EP pointer.
*                - Attempt to call mcc_send() on the UUT side
*                  with the invalid EP pointer.
*                - Change the timeout_us parameter of the CTR_CMD_RECV cmd to 0
*                  (i.e. do not block but wait for a new message on the app. level)
*                  and repeat the same loop for TEST_CNT cycles again.
*                - Attempt to call mcc_send() on the UUT side with the invalid msg_size.
*                - Destroy both TEA and UUT EPs.
*
*END*---------------------------------------------------------------------*/
void tc_3_main_task(void)
{
    CONTROL_MESSAGE msg;
    ACKNOWLEDGE_MESSAGE ack_msg;
    MCC_MEM_SIZE    num_of_received_bytes;
    int             ret_value, i;
    CONTROL_MESSAGE_DATA_CREATE_EP_PARAM data_create_ep_param;
    CONTROL_MESSAGE_DATA_DESTROY_EP_PARAM data_destroy_ep_param;
    CONTROL_MESSAGE_DATA_SEND_PARAM data_send_param;
    CONTROL_MESSAGE_DATA_RECV_PARAM data_recv_param;
    MCC_ENDPOINT    uut_endpoint =  {MCC_UUT_CORE,MCC_UUT_NODE,MCC_UUT_EP_PORT};
    MCC_ENDPOINT    tea_endpoint =  {MCC_TEA_CORE,MCC_TEA_NODE,MCC_TEA_EP_PORT};
    char            tea_app_buffer[TEA_APP_BUF_SIZE];
    
    data_create_ep_param.uut_endpoint = uut_endpoint;
    data_create_ep_param.uut_app_buffer_size = UUT_APP_BUF_SIZE;

    data_destroy_ep_param.uut_endpoint = uut_endpoint;
    data_destroy_ep_param.endpoint_to_ack = tea_control_endpoint;
    
    data_send_param.dest_endpoint = tea_endpoint;
    mcc_memcpy("", (void*)data_send_param.msg, (unsigned int)4);
    data_send_param.msg_size = CMD_SEND_MSG_SIZE;
    data_send_param.timeout_us = 0xffffffff;
    data_send_param.uut_endpoint = uut_endpoint;
    data_send_param.endpoint_to_ack = tea_control_endpoint;
    data_send_param.repeat_count = 1;
    
    data_recv_param.uut_endpoint = uut_endpoint;
    data_recv_param.uut_app_buffer_size = UUT_APP_BUF_SIZE;
    data_recv_param.timeout_us = 0xffffffff;
    data_recv_param.mode = CMD_RECV_MODE_NOCOPY;
    data_recv_param.endpoint_to_ack = tea_control_endpoint;

    mcc_create_endpoint(&tea_endpoint, tea_endpoint.port);
    
    msg.CMD = CTR_CMD_CREATE_EP;
    msg.ACK_REQUIRED = ACK_REQUIRED_NO;
    mcc_memcpy((void*)&data_create_ep_param, (void*)msg.DATA, (unsigned int)sizeof(struct control_message_data_create_ep_param));
    /* wait until the uut control endpoint is created by the other core */
    while(MCC_ERR_ENDPOINT == mcc_send(&uut_control_endpoint, &msg, sizeof(CONTROL_MESSAGE), 0xffffffff)) {
    }

    for(i = 0; i < TEST_CNT; i++) {
        msg.CMD = CTR_CMD_RECV;
        msg.ACK_REQUIRED = ACK_REQUIRED_NO;
        mcc_memcpy((void*)&data_recv_param, (void*)msg.DATA, (unsigned int)sizeof(struct control_message_data_recv_param));
        mcc_send(&uut_control_endpoint, &msg, sizeof(CONTROL_MESSAGE), 0xffffffff);
        /* wait until the uut endpoint is created by the other core */
        while(MCC_ERR_ENDPOINT == mcc_send(&uut_endpoint, "aaa", 3, 0xffffffff)) {
        }

        msg.CMD = CTR_CMD_SEND;
        msg.ACK_REQUIRED = ACK_REQUIRED_NO;
        mcc_memcpy((void*)&data_send_param, (void*)msg.DATA, (unsigned int)sizeof(struct control_message_data_send_param));
        mcc_send(&uut_control_endpoint, &msg, sizeof(CONTROL_MESSAGE), 0xffffffff);

        ret_value = mcc_recv_copy(&tea_endpoint, &tea_app_buffer, TEA_APP_BUF_SIZE, &num_of_received_bytes, 0xffffffff);
        EU_ASSERT_REF_TC_MSG( tc_2_main_task, ret_value == MCC_SUCCESS, "TEST #3: 3.1 Message successfully received");
        if(MCC_SUCCESS != ret_value) {
#if PRINT_ON
            _core_mutex_lock(coremutex_app_ptr);
            printf("sender task receive error: %i\n", ret_value);
            _core_mutex_unlock(coremutex_app_ptr);
#endif
            eunit_test_stop();
        } else {
#if PRINT_ON
            _core_mutex_lock(coremutex_app_ptr);
            printf("Message from responder task: Size=%x, DATA = %x", num_of_received_bytes, tea_app_buffer);
            _core_mutex_unlock(coremutex_app_ptr);
#endif
        EU_ASSERT_REF_TC_MSG( tc_3_main_task, 0 == strncmp(tea_app_buffer, "aaa", 3), "TEST #3: 3.2 Checking correct response message content");
        }
    }

    /* Attempt to call mcc_recv_no_copy() on the UUT side with the invalid EP pointer */
    data_recv_param.uut_endpoint = null_endpoint;
    msg.CMD = CTR_CMD_RECV;
    msg.ACK_REQUIRED = ACK_REQUIRED_YES;
    mcc_memcpy((void*)&data_recv_param, (void*)msg.DATA, (unsigned int)sizeof(struct control_message_data_recv_param));
    mcc_send(&uut_control_endpoint, &msg, sizeof(CONTROL_MESSAGE), 0xffffffff);
    ret_value = mcc_recv_copy(&tea_control_endpoint, &ack_msg, sizeof(ACKNOWLEDGE_MESSAGE), &num_of_received_bytes, 0xffffffff);
    EU_ASSERT_REF_TC_MSG( tc_3_main_task, CTR_CMD_RECV == ack_msg.CMD_ACK, "TEST #3: 3.3 Checking correct CTR_CMD_RECV acknowledge message 'CMD_ACK' content");
    EU_ASSERT_REF_TC_MSG( tc_3_main_task, MCC_ERR_ENDPOINT == ack_msg.RETURN_VALUE, "TEST #3: 3.4 Checking correct CTR_CMD_RECV acknowledge message 'RETURN_VALUE' content");

    /* Attempt to call mcc_send() on the UUT side with the invalid EP pointer */
    data_send_param.dest_endpoint = null_endpoint;
    msg.CMD = CTR_CMD_SEND;
    msg.ACK_REQUIRED = ACK_REQUIRED_YES;
    mcc_memcpy((void*)&data_send_param, (void*)msg.DATA, (unsigned int)sizeof(struct control_message_data_send_param));
    mcc_send(&uut_control_endpoint, &msg, sizeof(CONTROL_MESSAGE), 0xffffffff);
    ret_value = mcc_recv_copy(&tea_control_endpoint, &ack_msg, sizeof(ACKNOWLEDGE_MESSAGE), &num_of_received_bytes, 0xffffffff);
    EU_ASSERT_REF_TC_MSG( tc_3_main_task, CTR_CMD_SEND == ack_msg.CMD_ACK, "TEST #3: 3.5 Checking correct CTR_CMD_SEND acknowledge message 'CMD_ACK' content");
    EU_ASSERT_REF_TC_MSG( tc_3_main_task, MCC_ERR_ENDPOINT == ack_msg.RETURN_VALUE, "TEST #3: 3.6 Checking correct CTR_CMD_SEND acknowledge message 'RETURN_VALUE' content");



    /*
     * Now, execute non-blocking version of the mcc_recv_no_copy() function
     * */
    data_recv_param.timeout_us = 0;
    data_recv_param.uut_endpoint = uut_endpoint;
    data_send_param.dest_endpoint = tea_endpoint;

    for(i = 0; i < TEST_CNT; i++) {
        msg.CMD = CTR_CMD_RECV;
        msg.ACK_REQUIRED = ACK_REQUIRED_NO;
        mcc_memcpy((void*)&data_recv_param, (void*)msg.DATA, (unsigned int)sizeof(struct control_message_data_recv_param));
        mcc_send(&uut_control_endpoint, &msg, sizeof(CONTROL_MESSAGE), 0xffffffff);
        /* wait until the uut endpoint is created by the other core */
        while(MCC_ERR_ENDPOINT == mcc_send(&uut_endpoint, "aaa", 3, 0xffffffff)) {
        }

        msg.CMD = CTR_CMD_SEND;
        msg.ACK_REQUIRED = ACK_REQUIRED_NO;
        mcc_memcpy((void*)&data_send_param, (void*)msg.DATA, (unsigned int)sizeof(struct control_message_data_send_param));
        mcc_send(&uut_control_endpoint, &msg, sizeof(CONTROL_MESSAGE), 0xffffffff);

        ret_value = mcc_recv_copy(&tea_endpoint, &tea_app_buffer, TEA_APP_BUF_SIZE, &num_of_received_bytes, 0xffffffff);
        EU_ASSERT_REF_TC_MSG( tc_2_main_task, ret_value == MCC_SUCCESS, "TEST #3: 3.7 Message successfully received");
        if(MCC_SUCCESS != ret_value) {
#if PRINT_ON
            _core_mutex_lock(coremutex_app_ptr);
            printf("sender task receive error: %i\n", ret_value);
            _core_mutex_unlock(coremutex_app_ptr);
#endif
            eunit_test_stop();
        } else {
#if PRINT_ON
            _core_mutex_lock(coremutex_app_ptr);
            printf("Message from responder task: Size=%x, DATA = %x", num_of_received_bytes, tea_app_buffer);
            _core_mutex_unlock(coremutex_app_ptr);
#endif
        EU_ASSERT_REF_TC_MSG( tc_3_main_task, 0 == strncmp(tea_app_buffer, "aaa", 3), "TEST #3: 3.8 Checking correct response message content");
        }
    }

    /* Attempt to call mcc_recv_no_copy() on the UUT side with the invalid EP pointer */
    data_recv_param.uut_endpoint = null_endpoint;
    msg.CMD = CTR_CMD_RECV;
    msg.ACK_REQUIRED = ACK_REQUIRED_YES;
    mcc_memcpy((void*)&data_recv_param, (void*)msg.DATA, (unsigned int)sizeof(struct control_message_data_recv_param));
    mcc_send(&uut_control_endpoint, &msg, sizeof(CONTROL_MESSAGE), 0xffffffff);
    ret_value = mcc_recv_copy(&tea_control_endpoint, &ack_msg, sizeof(ACKNOWLEDGE_MESSAGE), &num_of_received_bytes, 0xffffffff);
    EU_ASSERT_REF_TC_MSG( tc_3_main_task, CTR_CMD_RECV == ack_msg.CMD_ACK, "TEST #3: 3.9 Checking correct CTR_CMD_RECV acknowledge message 'CMD_ACK' content");
    EU_ASSERT_REF_TC_MSG( tc_3_main_task, MCC_ERR_ENDPOINT == ack_msg.RETURN_VALUE, "TEST #3: 3.10 Checking correct CTR_CMD_RECV acknowledge message 'RETURN_VALUE' content");

    /* Attempt to call mcc_send() on the UUT side with the invalid msg_size */
    data_send_param.msg_size = 1 + MCC_ATTR_BUFFER_SIZE_IN_BYTES;
    msg.CMD = CTR_CMD_SEND;
    msg.ACK_REQUIRED = ACK_REQUIRED_YES;
    mcc_memcpy((void*)&data_send_param, (void*)msg.DATA, (unsigned int)sizeof(struct control_message_data_send_param));
    mcc_send(&uut_control_endpoint, &msg, sizeof(CONTROL_MESSAGE), 0xffffffff);
    ret_value = mcc_recv_copy(&tea_control_endpoint, &ack_msg, sizeof(ACKNOWLEDGE_MESSAGE), &num_of_received_bytes, 0xffffffff);
    EU_ASSERT_REF_TC_MSG( tc_3_main_task, CTR_CMD_SEND == ack_msg.CMD_ACK, "TEST #3: 3.11 Checking correct CTR_CMD_SEND acknowledge message 'CMD_ACK' content");
    EU_ASSERT_REF_TC_MSG( tc_3_main_task, MCC_ERR_INVAL == ack_msg.RETURN_VALUE, "TEST #3: 3.12 Checking correct CTR_CMD_SEND acknowledge message 'RETURN_VALUE' content");



    /* Destroy EPs */
    msg.CMD = CTR_CMD_DESTROY_EP;
    msg.ACK_REQUIRED = ACK_REQUIRED_YES;
    mcc_memcpy((void*)&data_destroy_ep_param, (void*)msg.DATA, (unsigned int)sizeof(struct control_message_data_destroy_ep_param));
    mcc_send(&uut_control_endpoint, &msg, sizeof(CONTROL_MESSAGE), 0xffffffff);
    ret_value = mcc_recv_copy(&tea_control_endpoint, &ack_msg, sizeof(ACKNOWLEDGE_MESSAGE), &num_of_received_bytes, 0xffffffff);
    EU_ASSERT_REF_TC_MSG( tc_3_main_task, CTR_CMD_DESTROY_EP == ack_msg.CMD_ACK, "TEST #3: 3.13 Checking correct CTR_CMD_DESTROY_EP acknowledge message 'CMD_ACK' content");
    EU_ASSERT_REF_TC_MSG( tc_3_main_task, MCC_SUCCESS == ack_msg.RETURN_VALUE, "TEST #3: 3.14 Checking correct CTR_CMD_DESTROY_EP acknowledge message 'RETURN_VALUE' content");
    EU_ASSERT_REF_TC_MSG( tc_3_main_task, 0 == strncmp((const char *)ack_msg.RESP_DATA, "MEM_OK", 7), "TEST #3: 3.15 Checking correct CTR_CMD_DESTROY_EP response message content");

    mcc_destroy_endpoint(&tea_endpoint);
}

/*TASK*--------------------------------------------------------------------
*
* Task Name    : tc_4_main_task
* Comments     : TEST #4: Testing MCC mcc_msgs_available functionality
*                - One EP on TEA side is created.
*                - CTR_CMD_MSG_AVAIL cmd is issued with uut_endpoint == not
*                  yet registered EP => MCC_ERR_ENDPOINT has to be returned.
*                - One EP on UUT side is created.
*                - Send maximum possible number of messages to the UUT EP,
*                  one buffers has to be left for control messages exchange.
*                - CTR_CMD_MSG_AVAIL cmd is issued again and return value checked.
*                - CTR_CMD_RECV cmd is issued on the UUT side to release all
*                  MCC receive buffers and to clear lwevents (repeats in the loop
*                  for the number of available messages).
*                - Destroy both TEA and UUT EPs.
*
*END*---------------------------------------------------------------------*/
void tc_4_main_task(void)
{
    CONTROL_MESSAGE msg;
    ACKNOWLEDGE_MESSAGE ack_msg;
    MCC_MEM_SIZE    num_of_received_bytes;
    int             ret_value, i;
    unsigned int    sent_msg_num=0;
    CONTROL_MESSAGE_DATA_CREATE_EP_PARAM data_create_ep_param;
    CONTROL_MESSAGE_DATA_DESTROY_EP_PARAM data_destroy_ep_param;
    CONTROL_MESSAGE_DATA_MSG_AVAIL_PARAM data_msg_avail_param;
    CONTROL_MESSAGE_DATA_RECV_PARAM data_recv_param;
    MCC_ENDPOINT    uut_endpoint =  {MCC_UUT_CORE,MCC_UUT_NODE,MCC_UUT_EP_PORT};
    MCC_ENDPOINT    tea_endpoint =  {MCC_TEA_CORE,MCC_TEA_NODE,MCC_TEA_EP_PORT};
    
    data_create_ep_param.uut_endpoint = uut_endpoint;
    data_create_ep_param.uut_app_buffer_size = UUT_APP_BUF_SIZE;
    data_create_ep_param.endpoint_to_ack = tea_control_endpoint;

    data_destroy_ep_param.uut_endpoint = uut_endpoint;
    data_destroy_ep_param.endpoint_to_ack = tea_control_endpoint;
    
    data_msg_avail_param.uut_endpoint = uut_endpoint;
    data_msg_avail_param.endpoint_to_ack = tea_control_endpoint;

    data_recv_param.uut_endpoint = uut_endpoint;
    data_recv_param.uut_app_buffer_size = UUT_APP_BUF_SIZE;
    data_recv_param.timeout_us = 0xffffffff;
    data_recv_param.mode = CMD_RECV_MODE_COPY;
    data_recv_param.endpoint_to_ack = tea_control_endpoint;

    mcc_create_endpoint(&tea_endpoint, tea_endpoint.port);
    
    msg.CMD = CTR_CMD_MSG_AVAIL;
    msg.ACK_REQUIRED = ACK_REQUIRED_YES;
    mcc_memcpy((void*)&data_msg_avail_param, (void*)msg.DATA, (unsigned int)sizeof(struct control_message_data_msg_avail_param));
    /* wait until the uut control endpoint is created by the other core */
    while(MCC_ERR_ENDPOINT == mcc_send(&uut_control_endpoint, &msg, sizeof(CONTROL_MESSAGE), 0xffffffff)) {
    }
    ret_value = mcc_recv_copy(&tea_control_endpoint, &ack_msg, sizeof(ACKNOWLEDGE_MESSAGE), &num_of_received_bytes, 0xffffffff);
    EU_ASSERT_REF_TC_MSG( tc_4_main_task, CTR_CMD_MSG_AVAIL == ack_msg.CMD_ACK, "TEST #4: 4.1 Checking correct CTR_CMD_MSG_AVAIL acknowledge message 'CMD_ACK' content");
    EU_ASSERT_REF_TC_MSG( tc_4_main_task, MCC_ERR_ENDPOINT == ack_msg.RETURN_VALUE, "TEST #4: 4.2 Checking correct CTR_CMD_MSG_AVAIL acknowledge message 'RETURN_VALUE' content");

    msg.CMD = CTR_CMD_CREATE_EP;
    msg.ACK_REQUIRED = ACK_REQUIRED_YES;
    mcc_memcpy((void*)&data_create_ep_param, (void*)msg.DATA, (unsigned int)sizeof(struct control_message_data_create_ep_param));
    mcc_send(&uut_control_endpoint, &msg, sizeof(CONTROL_MESSAGE), 0xffffffff);
    ret_value = mcc_recv_copy(&tea_control_endpoint, &ack_msg, sizeof(ACKNOWLEDGE_MESSAGE), &num_of_received_bytes, 0xffffffff);
    EU_ASSERT_REF_TC_MSG( tc_4_main_task, CTR_CMD_CREATE_EP == ack_msg.CMD_ACK, "TEST #4: 4.3 Checking correct CTR_CMD_CREATE_EP acknowledge message 'CMD_ACK' content");
    EU_ASSERT_REF_TC_MSG( tc_4_main_task, MCC_SUCCESS == ack_msg.RETURN_VALUE, "TEST #4: 4.4 Checking correct CTR_CMD_CREATE_EP acknowledge message 'RETURN_VALUE' content");
    EU_ASSERT_REF_TC_MSG( tc_4_main_task, 0 == strncmp((const char *)ack_msg.RESP_DATA, "MEM_OK", 7), "TEST #4: 4.5 Checking correct CTR_CMD_CREATE_EP response message content");

    /* Send maximum possible number of messages to the uut EP, consider that one buffers has to be left for control messages exchange */
    /* wait until the uut endpoint is created by the other core */
    while(MCC_ERR_ENDPOINT == mcc_send(&uut_endpoint, "aaa", 3, 0xffffffff)) {
    }
    sent_msg_num++;
    for(i = 0; i < MCC_ATTR_NUM_RECEIVE_BUFFERS-2; i++) {
       /* wait until the uut endpoint is created by the other core */
       ret_value = mcc_send(&uut_endpoint, "aaa", 3, 0xffffffff);
       if(MCC_SUCCESS == ret_value) {
           sent_msg_num++;
       }
    }

    EU_ASSERT_REF_TC_MSG( tc_4_main_task, sent_msg_num == MCC_ATTR_NUM_RECEIVE_BUFFERS-1, "TEST #4: 4.6 Checking correct number of sent messages");

    msg.CMD = CTR_CMD_MSG_AVAIL;
    msg.ACK_REQUIRED = ACK_REQUIRED_YES;
    mcc_memcpy((void*)&data_msg_avail_param, (void*)msg.DATA, (unsigned int)sizeof(struct control_message_data_msg_avail_param));
    mcc_send(&uut_control_endpoint, &msg, sizeof(CONTROL_MESSAGE), 0xffffffff);
    ret_value = mcc_recv_copy(&tea_control_endpoint, &ack_msg, sizeof(ACKNOWLEDGE_MESSAGE), &num_of_received_bytes, 0xffffffff);
    EU_ASSERT_REF_TC_MSG( tc_4_main_task, CTR_CMD_MSG_AVAIL == ack_msg.CMD_ACK, "TEST #4: 4.7 Checking CTR_CMD_MSG_AVAIL correct acknowledge message 'CMD_ACK' content");
    EU_ASSERT_REF_TC_MSG( tc_4_main_task, MCC_SUCCESS == ack_msg.RETURN_VALUE, "TEST #4: 4.8 Checking CTR_CMD_MSG_AVAIL correct acknowledge message 'RETURN_VALUE' content");
    EU_ASSERT_REF_TC_MSG( tc_4_main_task, sent_msg_num == *(unsigned int*)ack_msg.RESP_DATA, "TEST #4: 4.9 Checking correct CTR_CMD_MSG_AVAIL acknowledge message 'RESP_DATA' content");
#if PRINT_ON
            _core_mutex_lock(coremutex_app_ptr);
            printf("sent_msg_num =%x, RESP_DATA = %x\n", sent_msg_num, *(unsigned int*)ack_msg.RESP_DATA);
            _core_mutex_unlock(coremutex_app_ptr);
#endif

    /* CTR_CMD_RECV cmd is issued on the UUT side to release all MCC receive buffers and to clear lwevents */
    msg.CMD = CTR_CMD_RECV;
    msg.ACK_REQUIRED = ACK_REQUIRED_YES;
    mcc_memcpy((void*)&data_recv_param, (void*)msg.DATA, (unsigned int)sizeof(struct control_message_data_recv_param));
    for(i = 0; i < sent_msg_num; i++) {
        mcc_send(&uut_control_endpoint, &msg, sizeof(CONTROL_MESSAGE), 0xffffffff);
        ret_value = mcc_recv_copy(&tea_control_endpoint, &ack_msg, sizeof(ACKNOWLEDGE_MESSAGE), &num_of_received_bytes, 0xffffffff);
        EU_ASSERT_REF_TC_MSG( tc_4_main_task, CTR_CMD_RECV == ack_msg.CMD_ACK, "TEST #4: 4.10 Checking correct CTR_CMD_RECV acknowledge message 'CMD_ACK' content");
        EU_ASSERT_REF_TC_MSG( tc_4_main_task, MCC_SUCCESS == ack_msg.RETURN_VALUE, "TEST #4: 4.11 Checking correct CTR_CMD_RECV acknowledge message 'RETURN_VALUE' content");
    }

    /* Destroy EPs */
    msg.CMD = CTR_CMD_DESTROY_EP;
    msg.ACK_REQUIRED = ACK_REQUIRED_YES;
    mcc_memcpy((void*)&data_destroy_ep_param, (void*)msg.DATA, (unsigned int)sizeof(struct control_message_data_destroy_ep_param));
    mcc_send(&uut_control_endpoint, &msg, sizeof(CONTROL_MESSAGE), 0xffffffff);
    ret_value = mcc_recv_copy(&tea_control_endpoint, &ack_msg, sizeof(ACKNOWLEDGE_MESSAGE), &num_of_received_bytes, 0xffffffff);
    EU_ASSERT_REF_TC_MSG( tc_4_main_task, CTR_CMD_DESTROY_EP == ack_msg.CMD_ACK, "TEST #4: 4.12 Checking correct CTR_CMD_DESTROY_EP acknowledge message 'CMD_ACK' content");
    EU_ASSERT_REF_TC_MSG( tc_4_main_task, MCC_SUCCESS == ack_msg.RETURN_VALUE, "TEST #4: 4.13 Checking correct CTR_CMD_DESTROY_EP acknowledge message 'RETURN_VALUE' content");
    EU_ASSERT_REF_TC_MSG( tc_4_main_task, 0 == strncmp((const char *)ack_msg.RESP_DATA, "MEM_OK", 7), "TEST #4: 4.14 Checking correct CTR_CMD_DESTROY_EP response message content");

    mcc_destroy_endpoint(&tea_endpoint);
}

/*TASK*--------------------------------------------------------------------
*
* Task Name    : tc_5_main_task
* Comments     : TEST #5: Testing MCC recv timeouts
*                - One EP on TEA side is created.
*                - Two EPs on UUT side are created.
*                - CTR_CMD_RECV cmd is issued with mode == CMD_RECV_MODE_COPY
*                  (i.e. mcc_recv_copy() function has to be used) and with
*                  timeout_us == CMD_RECV_TIMEOUT (i.e. wait for a new message maximum
*                  2 seconds). No message is sent and the timer expires.
*                - CTR_CMD_RECV cmd is issued again with the same settings.
*                  A message is sent at half of the timeout.
*                - CTR_CMD_RECV cmd is issued again with the same settings.
*                  A message is sent at half of the timeout but to another EP.
*                  CPU-to-CPU interrupt will be generated but should not
*                  wake up the task that is blocked in timeout.
*                - Release the message sent to the second EP.
*                - Repeat the previous sequence with mode == CMD_RECV_MODE_NOCOPY
*                  (i.e. mcc_recv_no_copy() function is used)
*                - Destroy all EPs.
*
*END*---------------------------------------------------------------------*/
void tc_5_main_task(void)
{
    CONTROL_MESSAGE msg;
    ACKNOWLEDGE_MESSAGE ack_msg;
    MCC_MEM_SIZE    num_of_received_bytes;
    int             ret_value;
    CONTROL_MESSAGE_DATA_CREATE_EP_PARAM data_create_ep_param;
    CONTROL_MESSAGE_DATA_DESTROY_EP_PARAM data_destroy_ep_param;
    CONTROL_MESSAGE_DATA_RECV_PARAM data_recv_param;
    MCC_ENDPOINT    uut_endpoint =  {MCC_UUT_CORE,MCC_UUT_NODE,MCC_UUT_EP_PORT};
    MCC_ENDPOINT    tea_endpoint =  {MCC_TEA_CORE,MCC_TEA_NODE,MCC_TEA_EP_PORT};

    data_create_ep_param.uut_endpoint = uut_endpoint;
    data_create_ep_param.uut_app_buffer_size = UUT_APP_BUF_SIZE;

    data_destroy_ep_param.uut_endpoint = uut_endpoint;
    data_destroy_ep_param.endpoint_to_ack = tea_control_endpoint;

    data_recv_param.uut_endpoint = uut_endpoint;
    data_recv_param.uut_app_buffer_size = UUT_APP_BUF_SIZE;
    data_recv_param.timeout_us = CMD_RECV_TIMEOUT_US; /* 2 seconds timeout */
    data_recv_param.mode = CMD_RECV_MODE_COPY;
    data_recv_param.endpoint_to_ack = tea_control_endpoint;

    mcc_create_endpoint(&tea_endpoint, tea_endpoint.port);

    msg.CMD = CTR_CMD_CREATE_EP;
    msg.ACK_REQUIRED = ACK_REQUIRED_NO;
    mcc_memcpy((void*)&data_create_ep_param, (void*)msg.DATA, (unsigned int)sizeof(struct control_message_data_create_ep_param));
    /* wait until the uut control endpoint is created by the other core */
    while(MCC_ERR_ENDPOINT == mcc_send(&uut_control_endpoint, &msg, sizeof(CONTROL_MESSAGE), 0xffffffff)) {
    }

    /* Create second UUT EP */
    data_create_ep_param.uut_endpoint.port += 1;
    msg.CMD = CTR_CMD_CREATE_EP;
    msg.ACK_REQUIRED = ACK_REQUIRED_NO;
    mcc_memcpy((void*)&data_create_ep_param, (void*)msg.DATA, (unsigned int)sizeof(struct control_message_data_create_ep_param));
    /* wait until the uut control endpoint is created by the other core */
    while(MCC_ERR_ENDPOINT == mcc_send(&uut_control_endpoint, &msg, sizeof(CONTROL_MESSAGE), 0xffffffff)) {
    }

    /* Wait for a new message until the timeout expires, no message is sent. */
    msg.CMD = CTR_CMD_RECV;
    msg.ACK_REQUIRED = ACK_REQUIRED_YES;
    mcc_memcpy((void*)&data_recv_param, (void*)msg.DATA, (unsigned int)sizeof(struct control_message_data_recv_param));
    mcc_send(&uut_control_endpoint, &msg, sizeof(CONTROL_MESSAGE), 0xffffffff);
    ret_value = mcc_recv_copy(&tea_control_endpoint, &ack_msg, sizeof(ACKNOWLEDGE_MESSAGE), &num_of_received_bytes, 0xffffffff);
    EU_ASSERT_REF_TC_MSG( tc_5_main_task, CTR_CMD_RECV == ack_msg.CMD_ACK, "TEST #5: 5.1 Checking correct CTR_CMD_RECV acknowledge message 'CMD_ACK' content");
    EU_ASSERT_REF_TC_MSG( tc_5_main_task, MCC_ERR_TIMEOUT == ack_msg.RETURN_VALUE, "TEST #5: 5.2 Checking correct CTR_CMD_RECV acknowledge message 'RETURN_VALUE' content");
    //ret_value = ((1000 * ack_msg.TS2_MSEC) + (1000000 * ack_msg.TS2_SEC) - (1000 * ack_msg.TS1_MSEC) - (1000000 * ack_msg.TS1_SEC));
    EU_ASSERT_REF_TC_MSG( tc_5_main_task, (CMD_RECV_TIMEOUT_US*0.8 < ((1000 * ack_msg.TS2_MSEC) + (1000000 * ack_msg.TS2_SEC) - (1000 * ack_msg.TS1_MSEC) - (1000000 * ack_msg.TS1_SEC)) < CMD_RECV_TIMEOUT_US*1.2), "TEST #5: 5.3 Checking correct recv timeout");

    /* Wait for a new message until the timeout expires, a message is sent at half of the timeout. */
    msg.CMD = CTR_CMD_RECV;
    msg.ACK_REQUIRED = ACK_REQUIRED_YES;
    mcc_memcpy((void*)&data_recv_param, (void*)msg.DATA, (unsigned int)sizeof(struct control_message_data_recv_param));
    mcc_send(&uut_control_endpoint, &msg, sizeof(CONTROL_MESSAGE), 0xffffffff);
    _time_delay(CMD_RECV_TIMEOUT_US/1000/2);
    mcc_send(&uut_endpoint, "aaa", 3, 0xffffffff);
    ret_value = mcc_recv_copy(&tea_control_endpoint, &ack_msg, sizeof(ACKNOWLEDGE_MESSAGE), &num_of_received_bytes, 0xffffffff);
    EU_ASSERT_REF_TC_MSG( tc_5_main_task, CTR_CMD_RECV == ack_msg.CMD_ACK, "TEST #5: 5.4 Checking correct CTR_CMD_RECV acknowledge message 'CMD_ACK' content");
    EU_ASSERT_REF_TC_MSG( tc_5_main_task, MCC_SUCCESS == ack_msg.RETURN_VALUE, "TEST #5: 5.5 Checking correct CTR_CMD_RECV acknowledge message 'RETURN_VALUE' content");
    //ret_value = ((1000 * ack_msg.TS2_MSEC) + (1000000 * ack_msg.TS2_SEC) - (1000 * ack_msg.TS1_MSEC) - (1000000 * ack_msg.TS1_SEC));
    EU_ASSERT_REF_TC_MSG( tc_5_main_task, (CMD_RECV_TIMEOUT_US/2*0.8 < ((1000 * ack_msg.TS2_MSEC) + (1000000 * ack_msg.TS2_SEC) - (1000 * ack_msg.TS1_MSEC) - (1000000 * ack_msg.TS1_SEC)) < CMD_RECV_TIMEOUT_US/2*1.2), "TEST #5: 5.6 Checking correct recv timeout");

    /* Wait for a new message until the timeout expires, a message is sent at half of the timeout but to the second EP.
     * CPU-to-CPU interrupt will be generated but should not wake up the task that is blocked in timeout. */
    uut_endpoint.port += 1;
    msg.CMD = CTR_CMD_RECV;
    msg.ACK_REQUIRED = ACK_REQUIRED_YES;
    mcc_memcpy((void*)&data_recv_param, (void*)msg.DATA, (unsigned int)sizeof(struct control_message_data_recv_param));
    mcc_send(&uut_control_endpoint, &msg, sizeof(CONTROL_MESSAGE), 0xffffffff);
    _time_delay(CMD_RECV_TIMEOUT_US/1000/2);
    mcc_send(&uut_endpoint, "aaa", 3, 0xffffffff);
    ret_value = mcc_recv_copy(&tea_control_endpoint, &ack_msg, sizeof(ACKNOWLEDGE_MESSAGE), &num_of_received_bytes, 0xffffffff);
    EU_ASSERT_REF_TC_MSG( tc_5_main_task, CTR_CMD_RECV == ack_msg.CMD_ACK, "TEST #5: 5.7 Checking correct CTR_CMD_RECV acknowledge message 'CMD_ACK' content");
    EU_ASSERT_REF_TC_MSG( tc_5_main_task, MCC_ERR_TIMEOUT == ack_msg.RETURN_VALUE, "TEST #5: 5.8 Checking correct CTR_CMD_RECV acknowledge message 'RETURN_VALUE' content");
    //ret_value = ((1000 * ack_msg.TS2_MSEC) + (1000000 * ack_msg.TS2_SEC) - (1000 * ack_msg.TS1_MSEC) - (1000000 * ack_msg.TS1_SEC));
    EU_ASSERT_REF_TC_MSG( tc_5_main_task, (CMD_RECV_TIMEOUT_US*0.8 < ((1000 * ack_msg.TS2_MSEC) + (1000000 * ack_msg.TS2_SEC) - (1000 * ack_msg.TS1_MSEC) - (1000000 * ack_msg.TS1_SEC)) < CMD_RECV_TIMEOUT_US*1.2), "TEST #5: 5.9 Checking correct recv timeout");

    /* Release the message sent to the second EP now */
    data_recv_param.uut_endpoint.port += 1;
    msg.CMD = CTR_CMD_RECV;
    msg.ACK_REQUIRED = ACK_REQUIRED_YES;
    mcc_memcpy((void*)&data_recv_param, (void*)msg.DATA, (unsigned int)sizeof(struct control_message_data_recv_param));
    mcc_send(&uut_control_endpoint, &msg, sizeof(CONTROL_MESSAGE), 0xffffffff);
    ret_value = mcc_recv_copy(&tea_control_endpoint, &ack_msg, sizeof(ACKNOWLEDGE_MESSAGE), &num_of_received_bytes, 0xffffffff);
    EU_ASSERT_REF_TC_MSG( tc_5_main_task, CTR_CMD_RECV == ack_msg.CMD_ACK, "TEST #5: 5.10 Checking correct CTR_CMD_RECV acknowledge message 'CMD_ACK' content");
    EU_ASSERT_REF_TC_MSG( tc_5_main_task, MCC_SUCCESS == ack_msg.RETURN_VALUE, "TEST #5: 5.11 Checking correct CTR_CMD_RECV acknowledge message 'RETURN_VALUE' content");


    /*
     * Repeat the previous sequence with mode == CMD_RECV_MODE_NOCOPY (i.e. mcc_recv_no_copy() function is used)
     * */
    data_recv_param.mode = CMD_RECV_MODE_NOCOPY;
    uut_endpoint.port -= 1;
    data_recv_param.uut_endpoint.port -= 1;

    /* Wait for a new message until the timeout expires, no message is sent. */
    msg.CMD = CTR_CMD_RECV;
    msg.ACK_REQUIRED = ACK_REQUIRED_YES;
    mcc_memcpy((void*)&data_recv_param, (void*)msg.DATA, (unsigned int)sizeof(struct control_message_data_recv_param));
    mcc_send(&uut_control_endpoint, &msg, sizeof(CONTROL_MESSAGE), 0xffffffff);
    ret_value = mcc_recv_copy(&tea_control_endpoint, &ack_msg, sizeof(ACKNOWLEDGE_MESSAGE), &num_of_received_bytes, 0xffffffff);
    EU_ASSERT_REF_TC_MSG( tc_5_main_task, CTR_CMD_RECV == ack_msg.CMD_ACK, "TEST #5: 5.12 Checking correct CTR_CMD_RECV acknowledge message 'CMD_ACK' content");
    EU_ASSERT_REF_TC_MSG( tc_5_main_task, MCC_ERR_TIMEOUT == ack_msg.RETURN_VALUE, "TEST #5: 5.13 Checking correct CTR_CMD_RECV acknowledge message 'RETURN_VALUE' content");
    //ret_value = ((1000 * ack_msg.TS2_MSEC) + (1000000 * ack_msg.TS2_SEC) - (1000 * ack_msg.TS1_MSEC) - (1000000 * ack_msg.TS1_SEC));
    EU_ASSERT_REF_TC_MSG( tc_5_main_task, (CMD_RECV_TIMEOUT_US*0.8 < ((1000 * ack_msg.TS2_MSEC) + (1000000 * ack_msg.TS2_SEC) - (1000 * ack_msg.TS1_MSEC) - (1000000 * ack_msg.TS1_SEC)) < CMD_RECV_TIMEOUT_US*1.2), "TEST #5: 5.14 Checking correct recv timeout");

    /* Wait for a new message until the timeout expires, a message is sent at half of the timeout. */
    msg.CMD = CTR_CMD_RECV;
    msg.ACK_REQUIRED = ACK_REQUIRED_YES;
    mcc_memcpy((void*)&data_recv_param, (void*)msg.DATA, (unsigned int)sizeof(struct control_message_data_recv_param));
    mcc_send(&uut_control_endpoint, &msg, sizeof(CONTROL_MESSAGE), 0xffffffff);
    _time_delay(CMD_RECV_TIMEOUT_US/1000/2);
    mcc_send(&uut_endpoint, "aaa", 3, 0xffffffff);
    ret_value = mcc_recv_copy(&tea_control_endpoint, &ack_msg, sizeof(ACKNOWLEDGE_MESSAGE), &num_of_received_bytes, 0xffffffff);
    EU_ASSERT_REF_TC_MSG( tc_5_main_task, CTR_CMD_RECV == ack_msg.CMD_ACK, "TEST #5: 5.15 Checking correct CTR_CMD_RECV acknowledge message 'CMD_ACK' content");
    EU_ASSERT_REF_TC_MSG( tc_5_main_task, MCC_SUCCESS == ack_msg.RETURN_VALUE, "TEST #5: 5.16 Checking correct CTR_CMD_RECV acknowledge message 'RETURN_VALUE' content");
    //ret_value = ((1000 * ack_msg.TS2_MSEC) + (1000000 * ack_msg.TS2_SEC) - (1000 * ack_msg.TS1_MSEC) - (1000000 * ack_msg.TS1_SEC));
    EU_ASSERT_REF_TC_MSG( tc_5_main_task, (CMD_RECV_TIMEOUT_US/2*0.8 < ((1000 * ack_msg.TS2_MSEC) + (1000000 * ack_msg.TS2_SEC) - (1000 * ack_msg.TS1_MSEC) - (1000000 * ack_msg.TS1_SEC)) < CMD_RECV_TIMEOUT_US/2*1.2), "TEST #5: 5.17 Checking correct recv timeout");

    /* Wait for a new message until the timeout expires, a message is sent at half of the timeout but to the second EP.
     * CPU-to-CPU interrupt will be generated but should not wake up the task that is blocked in timeout. */
    uut_endpoint.port += 1;
    msg.CMD = CTR_CMD_RECV;
    msg.ACK_REQUIRED = ACK_REQUIRED_YES;
    mcc_memcpy((void*)&data_recv_param, (void*)msg.DATA, (unsigned int)sizeof(struct control_message_data_recv_param));
    mcc_send(&uut_control_endpoint, &msg, sizeof(CONTROL_MESSAGE), 0xffffffff);
    _time_delay(CMD_RECV_TIMEOUT_US/1000/2);
    mcc_send(&uut_endpoint, "aaa", 3, 0xffffffff);
    ret_value = mcc_recv_copy(&tea_control_endpoint, &ack_msg, sizeof(ACKNOWLEDGE_MESSAGE), &num_of_received_bytes, 0xffffffff);
    EU_ASSERT_REF_TC_MSG( tc_5_main_task, CTR_CMD_RECV == ack_msg.CMD_ACK, "TEST #5: 5.18 Checking correct CTR_CMD_RECV acknowledge message 'CMD_ACK' content");
    EU_ASSERT_REF_TC_MSG( tc_5_main_task, MCC_ERR_TIMEOUT == ack_msg.RETURN_VALUE, "TEST #5: 5.19 Checking correct CTR_CMD_RECV acknowledge message 'RETURN_VALUE' content");
    //ret_value = ((1000 * ack_msg.TS2_MSEC) + (1000000 * ack_msg.TS2_SEC) - (1000 * ack_msg.TS1_MSEC) - (1000000 * ack_msg.TS1_SEC));
    EU_ASSERT_REF_TC_MSG( tc_5_main_task, (CMD_RECV_TIMEOUT_US*0.8 < ((1000 * ack_msg.TS2_MSEC) + (1000000 * ack_msg.TS2_SEC) - (1000 * ack_msg.TS1_MSEC) - (1000000 * ack_msg.TS1_SEC)) < CMD_RECV_TIMEOUT_US*1.2), "TEST #5: 5.20 Checking correct recv timeout");

    /* Release the message sent to the second EP now */
    data_recv_param.uut_endpoint.port += 1;
    msg.CMD = CTR_CMD_RECV;
    msg.ACK_REQUIRED = ACK_REQUIRED_YES;
    mcc_memcpy((void*)&data_recv_param, (void*)msg.DATA, (unsigned int)sizeof(struct control_message_data_recv_param));
    mcc_send(&uut_control_endpoint, &msg, sizeof(CONTROL_MESSAGE), 0xffffffff);
    ret_value = mcc_recv_copy(&tea_control_endpoint, &ack_msg, sizeof(ACKNOWLEDGE_MESSAGE), &num_of_received_bytes, 0xffffffff);
    EU_ASSERT_REF_TC_MSG( tc_5_main_task, CTR_CMD_RECV == ack_msg.CMD_ACK, "TEST #5: 5.21 Checking correct CTR_CMD_RECV acknowledge message 'CMD_ACK' content");
    EU_ASSERT_REF_TC_MSG( tc_5_main_task, MCC_SUCCESS == ack_msg.RETURN_VALUE, "TEST #5: 5.22 Checking correct CTR_CMD_RECV acknowledge message 'RETURN_VALUE' content");


    /* Destroy EPs */
    msg.CMD = CTR_CMD_DESTROY_EP;
    msg.ACK_REQUIRED = ACK_REQUIRED_YES;
    mcc_memcpy((void*)&data_destroy_ep_param, (void*)msg.DATA, (unsigned int)sizeof(struct control_message_data_destroy_ep_param));
    mcc_send(&uut_control_endpoint, &msg, sizeof(CONTROL_MESSAGE), 0xffffffff);
    ret_value = mcc_recv_copy(&tea_control_endpoint, &ack_msg, sizeof(ACKNOWLEDGE_MESSAGE), &num_of_received_bytes, 0xffffffff);
    EU_ASSERT_REF_TC_MSG( tc_5_main_task, CTR_CMD_DESTROY_EP == ack_msg.CMD_ACK, "TEST #5: 5.23 Checking correct CTR_CMD_DESTROY_EP acknowledge message 'CMD_ACK' content");
    EU_ASSERT_REF_TC_MSG( tc_5_main_task, MCC_SUCCESS == ack_msg.RETURN_VALUE, "TEST #5: 5.24 Checking correct CTR_CMD_DESTROY_EP acknowledge message 'RETURN_VALUE' content");
    EU_ASSERT_REF_TC_MSG( tc_5_main_task, 0 == strncmp((const char *)ack_msg.RESP_DATA, "MEM_OK", 7), "TEST #5: 5.25 Checking correct CTR_CMD_DESTROY_EP response message content");

    msg.CMD = CTR_CMD_DESTROY_EP;
    data_destroy_ep_param.uut_endpoint.port += 1;
    msg.ACK_REQUIRED = ACK_REQUIRED_YES;
    mcc_memcpy((void*)&data_destroy_ep_param, (void*)msg.DATA, (unsigned int)sizeof(struct control_message_data_destroy_ep_param));
    mcc_send(&uut_control_endpoint, &msg, sizeof(CONTROL_MESSAGE), 0xffffffff);
    ret_value = mcc_recv_copy(&tea_control_endpoint, &ack_msg, sizeof(ACKNOWLEDGE_MESSAGE), &num_of_received_bytes, 0xffffffff);
    EU_ASSERT_REF_TC_MSG( tc_5_main_task, CTR_CMD_DESTROY_EP == ack_msg.CMD_ACK, "TEST #5: 5.26 Checking correct CTR_CMD_DESTROY_EP acknowledge message 'CMD_ACK' content");
    EU_ASSERT_REF_TC_MSG( tc_5_main_task, MCC_SUCCESS == ack_msg.RETURN_VALUE, "TEST #5: 5.27 Checking correct CTR_CMD_DESTROY_EP acknowledge message 'RETURN_VALUE' content");
    EU_ASSERT_REF_TC_MSG( tc_5_main_task, 0 == strncmp((const char *)ack_msg.RESP_DATA, "MEM_OK", 7), "TEST #5: 5.28 Checking correct CTR_CMD_DESTROY_EP response message content");

    mcc_destroy_endpoint(&tea_endpoint);
}

/*TASK*--------------------------------------------------------------------
*
* Task Name    : tc_6_main_task
* Comments     : TEST #6: Testing MCC send timeouts
*                - One EP on TEA side is created.
*                - One EP on UUT side are created.
*                - CTR_CMD_SEND cmd is issued with
*                  repeat_count = MCC_ATTR_NUM_RECEIVE_BUFFERS+1
*                  (i.e. more than maximum configured number of messages
*                  will be sent to TEA EP) and with
*                  timeout_us == CMD_SEND_TIMEOUT_US (i.e. wait for a free
*                  buffer maximum 1 second). No buffer is freed on the TEA side
*                  and the timer expires.
*                - Now, when only one buffer is free, issue two CTR_CMD_SEND
*                  commands on the UUT side. UUT side waits for a free buffer
*                  until the timeout expires, one buffer is freed at half
*                  of the timeout on the TEA side.
*                - Release all messages sent to the TEA EP.
*                - Destroy all EPs.
*
*END*---------------------------------------------------------------------*/
void tc_6_main_task(void)
{
    CONTROL_MESSAGE msg;
    ACKNOWLEDGE_MESSAGE ack_msg;
    MCC_MEM_SIZE    num_of_received_bytes;
    int             ret_value;
    CONTROL_MESSAGE_DATA_CREATE_EP_PARAM data_create_ep_param;
    CONTROL_MESSAGE_DATA_DESTROY_EP_PARAM data_destroy_ep_param;
    CONTROL_MESSAGE_DATA_SEND_PARAM data_send_param;
    MCC_ENDPOINT    uut_endpoint =  {MCC_UUT_CORE,MCC_UUT_NODE,MCC_UUT_EP_PORT};
    MCC_ENDPOINT    tea_endpoint =  {MCC_TEA_CORE,MCC_TEA_NODE,MCC_TEA_EP_PORT};
    char            tea_app_buffer[TEA_APP_BUF_SIZE];

    data_create_ep_param.uut_endpoint = uut_endpoint;
    data_create_ep_param.uut_app_buffer_size = UUT_APP_BUF_SIZE;

    data_destroy_ep_param.uut_endpoint = uut_endpoint;
    data_destroy_ep_param.endpoint_to_ack = tea_control_endpoint;

    data_send_param.dest_endpoint = tea_endpoint;
    mcc_memcpy("xxx", (void*)data_send_param.msg, (unsigned int)4);
    data_send_param.msg_size = CMD_SEND_MSG_SIZE;
    data_send_param.timeout_us = CMD_SEND_TIMEOUT_US; /* 1 seconds timeout */
    data_send_param.uut_endpoint = uut_endpoint;
    data_send_param.endpoint_to_ack = tea_control_endpoint;
    data_send_param.repeat_count = MCC_ATTR_NUM_RECEIVE_BUFFERS+1;

    mcc_create_endpoint(&tea_endpoint, tea_endpoint.port);

    msg.CMD = CTR_CMD_CREATE_EP;
    msg.ACK_REQUIRED = ACK_REQUIRED_NO;
    mcc_memcpy((void*)&data_create_ep_param, (void*)msg.DATA, (unsigned int)sizeof(struct control_message_data_create_ep_param));
    /* wait until the uut control endpoint is created by the other core */
    while(MCC_ERR_ENDPOINT == mcc_send(&uut_control_endpoint, &msg, sizeof(CONTROL_MESSAGE), 0xffffffff)) {
    }

    /* UUT side waits for a free buffer until the timeout expires, no buffer is freed on the TEA side. */
    msg.CMD = CTR_CMD_SEND;
    msg.ACK_REQUIRED = ACK_REQUIRED_YES;
    mcc_memcpy((void*)&data_send_param, (void*)msg.DATA, (unsigned int)sizeof(struct control_message_data_send_param));
    mcc_send(&uut_control_endpoint, &msg, sizeof(CONTROL_MESSAGE), 0xffffffff);
    _time_delay(CMD_SEND_TIMEOUT_US/1000*2);
    ret_value = mcc_recv_copy(&tea_endpoint, &tea_app_buffer, TEA_APP_BUF_SIZE, &num_of_received_bytes, 0xffffffff);
    EU_ASSERT_REF_TC_MSG( tc_6_main_task, 0 == strncmp(tea_app_buffer, "xxx", 3), "TEST #6: 6.1 Checking correct response message content");

    ret_value = mcc_recv_copy(&tea_control_endpoint, &ack_msg, sizeof(ACKNOWLEDGE_MESSAGE), &num_of_received_bytes, 0xffffffff);
    EU_ASSERT_REF_TC_MSG( tc_6_main_task, CTR_CMD_SEND == ack_msg.CMD_ACK, "TEST #6: 6.2 Checking correct CTR_CMD_SEND acknowledge message 'CMD_ACK' content");
    EU_ASSERT_REF_TC_MSG( tc_6_main_task, MCC_ERR_TIMEOUT == ack_msg.RETURN_VALUE, "TEST #6: 6.3 Checking correct CTR_CMD_SEND acknowledge message 'RETURN_VALUE' content");
    //ret_value = ((1000 * ack_msg.TS2_MSEC) + (1000000 * ack_msg.TS2_SEC) - (1000 * ack_msg.TS1_MSEC) - (1000000 * ack_msg.TS1_SEC));
    EU_ASSERT_REF_TC_MSG( tc_6_main_task, (CMD_SEND_TIMEOUT_US*0.8 < ((1000 * ack_msg.TS2_MSEC) + (1000000 * ack_msg.TS2_SEC) - (1000 * ack_msg.TS1_MSEC) - (1000000 * ack_msg.TS1_SEC)) < CMD_SEND_TIMEOUT_US*1.2), "TEST #6: 6.4 Checking correct send timeout");


    /* Now, when only one buffer is free, issue two CTR_CMD_SEND commands on the UUT side.
     * UUT side waits for a free buffer until the timeout expires, one buffer is freed at half of the timeout on the TEA side. */
    data_send_param.repeat_count = 2;
    msg.CMD = CTR_CMD_SEND;
    msg.ACK_REQUIRED = ACK_REQUIRED_YES;
    mcc_memcpy((void*)&data_send_param, (void*)msg.DATA, (unsigned int)sizeof(struct control_message_data_send_param));
    mcc_send(&uut_control_endpoint, &msg, sizeof(CONTROL_MESSAGE), 0xffffffff);
    _time_delay(CMD_SEND_TIMEOUT_US/1000/2);
    /* This read frees one buffer for send timeout wakeup. */
    ret_value = mcc_recv_copy(&tea_endpoint, &tea_app_buffer, TEA_APP_BUF_SIZE, &num_of_received_bytes, 0xffffffff);
    EU_ASSERT_REF_TC_MSG( tc_6_main_task, 0 == strncmp(tea_app_buffer, "xxx", 3), "TEST #6: 6.5 Checking correct response message content");
    /* This read frees one buffer for ACK message to be sent back to the TEA. */
    ret_value = mcc_recv_copy(&tea_endpoint, &tea_app_buffer, TEA_APP_BUF_SIZE, &num_of_received_bytes, 0xffffffff);
    EU_ASSERT_REF_TC_MSG( tc_6_main_task, 0 == strncmp(tea_app_buffer, "xxx", 3), "TEST #6: 6.6 Checking correct response message content");

    ret_value = mcc_recv_copy(&tea_control_endpoint, &ack_msg, sizeof(ACKNOWLEDGE_MESSAGE), &num_of_received_bytes, 0xffffffff);
    EU_ASSERT_REF_TC_MSG( tc_6_main_task, CTR_CMD_SEND == ack_msg.CMD_ACK, "TEST #6: 6.7 Checking correct CTR_CMD_SEND acknowledge message 'CMD_ACK' content");
    EU_ASSERT_REF_TC_MSG( tc_6_main_task, MCC_SUCCESS == ack_msg.RETURN_VALUE, "TEST #6: 6.8 Checking correct CTR_CMD_SEND acknowledge message 'RETURN_VALUE' content");
    //ret_value = ((1000 * ack_msg.TS2_MSEC) + (1000000 * ack_msg.TS2_SEC) - (1000 * ack_msg.TS1_MSEC) - (1000000 * ack_msg.TS1_SEC));
    EU_ASSERT_REF_TC_MSG( tc_6_main_task, (CMD_SEND_TIMEOUT_US/2*0.8 < ((1000 * ack_msg.TS2_MSEC) + (1000000 * ack_msg.TS2_SEC) - (1000 * ack_msg.TS1_MSEC) - (1000000 * ack_msg.TS1_SEC)) < CMD_SEND_TIMEOUT_US/2*1.2), "TEST #6: 6.9 Checking correct send timeout");


    /* Release all messages sent to the TEA EP */
    while (MCC_SUCCESS == mcc_recv_copy(&tea_endpoint, &tea_app_buffer, TEA_APP_BUF_SIZE, &num_of_received_bytes, 0)) {
        EU_ASSERT_REF_TC_MSG( tc_6_main_task, 0 == strncmp(tea_app_buffer, "xxx", 3), "TEST #6: 6.10 Checking correct response message content");
    }

    /* Destroy EPs */
    msg.CMD = CTR_CMD_DESTROY_EP;
    msg.ACK_REQUIRED = ACK_REQUIRED_YES;
    mcc_memcpy((void*)&data_destroy_ep_param, (void*)msg.DATA, (unsigned int)sizeof(struct control_message_data_destroy_ep_param));
    mcc_send(&uut_control_endpoint, &msg, sizeof(CONTROL_MESSAGE), 0xffffffff);
    ret_value = mcc_recv_copy(&tea_control_endpoint, &ack_msg, sizeof(ACKNOWLEDGE_MESSAGE), &num_of_received_bytes, 0xffffffff);
    EU_ASSERT_REF_TC_MSG( tc_6_main_task, CTR_CMD_DESTROY_EP == ack_msg.CMD_ACK, "TEST #6: 3.11 Checking correct CTR_CMD_DESTROY_EP acknowledge message 'CMD_ACK' content");
    EU_ASSERT_REF_TC_MSG( tc_6_main_task, MCC_SUCCESS == ack_msg.RETURN_VALUE, "TEST #6: 6.12 Checking correct CTR_CMD_DESTROY_EP acknowledge message 'RETURN_VALUE' content");
    EU_ASSERT_REF_TC_MSG( tc_6_main_task, 0 == strncmp((const char *)ack_msg.RESP_DATA, "MEM_OK", 7), "TEST #6: 6.13 Checking correct CTR_CMD_DESTROY_EP response message content");

    mcc_destroy_endpoint(&tea_endpoint);
}

/*TASK*--------------------------------------------------------------------
*
* Task Name    : tc_7_main_task
* Comments     : TEST #7: Testing MCC deinitialization
*                - TEA control endpoint is destroyed.
*                - MCC is destroyed.
*
*END*---------------------------------------------------------------------*/
void tc_7_main_task(void)
{
    int           ret_value;

    /* Destroy TEA control endpoint */
    ret_value = mcc_destroy_endpoint(&tea_control_endpoint);
    EU_ASSERT_REF_TC_MSG( tc_7_main_task, ret_value == MCC_SUCCESS, "TEST #7: 7.1 Destroying TEA control endpoint");
    ret_value = mcc_destroy(MCC_TEA_NODE);
    EU_ASSERT_REF_TC_MSG( tc_7_main_task, ret_value == MCC_SUCCESS, "TEST #7: 7.2 Destroying MCC");
}

//------------------------------------------------------------------------
// Define Test Suite
 EU_TEST_SUITE_BEGIN(suite_mcc)
    EU_TEST_CASE_ADD(tc_1_main_task, " TEST #1 - Testing MCC initialization"),
    EU_TEST_CASE_ADD(tc_2_main_task, " TEST #2 - Testing MCC pingpong1"),
    EU_TEST_CASE_ADD(tc_3_main_task, " TEST #3 - Testing MCC pingpong2"),
    EU_TEST_CASE_ADD(tc_4_main_task, " TEST #4 - Testing MCC mcc_msgs_available functionality"),
    EU_TEST_CASE_ADD(tc_5_main_task, " TEST #5 - Testing MCC recv timeouts"),
    EU_TEST_CASE_ADD(tc_6_main_task, " TEST #6 - Testing MCC send timeouts"),
    EU_TEST_CASE_ADD(tc_7_main_task, " TEST #7 - Testing MCC deinitialization"),
 EU_TEST_SUITE_END(suite_mcc)
// Add test suites
 EU_TEST_REGISTRY_BEGIN()
        EU_TEST_SUITE_ADD(suite_mcc, " - MCC task suite")
 EU_TEST_REGISTRY_END()

void main_task
   (
      /* [IN] The MQX task parameter */
      uint_32 param
   )
{ /* Body */
   eunit_mutex_init();
   EU_RUN_ALL_TESTS(eunit_tres_array);
   eunit_test_stop();
} /* Endbody */

#endif
