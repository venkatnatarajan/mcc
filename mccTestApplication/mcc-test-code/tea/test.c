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
    MCC_ENDPOINT    uut_endpoint =  {MCC_UUT_CORE,MCC_UUT_NODE,MCC_UUT_EP_PORT};
    
    data_create_ep_param.uut_endpoint = uut_endpoint;
    data_create_ep_param.uut_app_buffer_size = 30;
    data_create_ep_param.endpoint_to_ack = tea_control_endpoint;

    data_destroy_ep_param.uut_endpoint = uut_endpoint;
    data_destroy_ep_param.endpoint_to_ack = tea_control_endpoint;

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
    EU_ASSERT_REF_TC_MSG( tc_1_main_task, CTR_CMD_CREATE_EP == ack_msg.CMD_ACK, "TEST #1: 1.6 Checking correct acknowledge message 'CMD_ACK' content");
    EU_ASSERT_REF_TC_MSG( tc_1_main_task, MCC_ERR_ENDPOINT == ack_msg.RETURN_VALUE, "TEST #1: 1.7 Checking correct acknowledge message 'RETURN_VALUE' content");

    data_create_ep_param.uut_endpoint.port++;

    for(i = 0; i < MCC_ATTR_MAX_RECEIVE_ENDPOINTS-3; i++) {
        msg.CMD = CTR_CMD_CREATE_EP;
        msg.ACK_REQUIRED = ACK_REQUIRED_YES;
        mcc_memcpy((void*)&data_create_ep_param, (void*)msg.DATA, (unsigned int)sizeof(struct control_message_data_create_ep_param));
        ret_value = mcc_send(&uut_control_endpoint, &msg, sizeof(CONTROL_MESSAGE), 0xffffffff);
        ret_value = mcc_recv_copy(&tea_control_endpoint, &ack_msg, sizeof(ACKNOWLEDGE_MESSAGE), &num_of_received_bytes, 0xffffffff);
        EU_ASSERT_REF_TC_MSG( tc_1_main_task, CTR_CMD_CREATE_EP == ack_msg.CMD_ACK, "TEST #1: 1.8 Checking correct acknowledge message 'CMD_ACK' content");
        EU_ASSERT_REF_TC_MSG( tc_1_main_task, MCC_SUCCESS == ack_msg.RETURN_VALUE, "TEST #1: 1.9 Checking correct acknowledge message 'RETURN_VALUE' content");
        data_create_ep_param.uut_endpoint.port++;
    }

    /* Another EP creation should not be possible - MCC_ERR_NOMEM should be returned */
    msg.CMD = CTR_CMD_CREATE_EP;
    msg.ACK_REQUIRED = ACK_REQUIRED_YES;
    mcc_memcpy((void*)&data_create_ep_param, (void*)msg.DATA, (unsigned int)sizeof(struct control_message_data_create_ep_param));
    ret_value = mcc_send(&uut_control_endpoint, &msg, sizeof(CONTROL_MESSAGE), 0xffffffff);
    ret_value = mcc_recv_copy(&tea_control_endpoint, &ack_msg, sizeof(ACKNOWLEDGE_MESSAGE), &num_of_received_bytes, 0xffffffff);
    EU_ASSERT_REF_TC_MSG( tc_1_main_task, CTR_CMD_CREATE_EP == ack_msg.CMD_ACK, "TEST #1: 1.10 Checking correct acknowledge message 'CMD_ACK' content");
    EU_ASSERT_REF_TC_MSG( tc_1_main_task, MCC_ERR_NOMEM == ack_msg.RETURN_VALUE, "TEST #1: 1.11 Checking correct acknowledge message 'RETURN_VALUE' content");

    /* EP destroy testing */
    /* Destroying an EP that has not been registered should fail and MCC_ERR_ENDPOINT should be returned */
    msg.CMD = CTR_CMD_DESTROY_EP;
    msg.ACK_REQUIRED = ACK_REQUIRED_YES;
    data_destroy_ep_param.uut_endpoint.port = data_create_ep_param.uut_endpoint.port;
    mcc_memcpy((void*)&data_destroy_ep_param, (void*)msg.DATA, (unsigned int)sizeof(struct control_message_data_destroy_ep_param));
    ret_value = mcc_send(&uut_control_endpoint, &msg, sizeof(CONTROL_MESSAGE), 0xffffffff);
    ret_value = mcc_recv_copy(&tea_control_endpoint, &ack_msg, sizeof(ACKNOWLEDGE_MESSAGE), &num_of_received_bytes, 0xffffffff);
    EU_ASSERT_REF_TC_MSG( tc_1_main_task, CTR_CMD_DESTROY_EP == ack_msg.CMD_ACK, "TEST #1: 1.12 Checking correct acknowledge message 'CMD_ACK' content");
    EU_ASSERT_REF_TC_MSG( tc_1_main_task, MCC_ERR_ENDPOINT == ack_msg.RETURN_VALUE, "TEST #1: 1.13 Checking correct acknowledge message 'RETURN_VALUE' content");

    /* Destroy all created EPs except of TEA and UUT control EPs */
    for(i = 0; i < MCC_ATTR_MAX_RECEIVE_ENDPOINTS-2; i++) {
        data_destroy_ep_param.uut_endpoint.port--;
        msg.CMD = CTR_CMD_DESTROY_EP;
        msg.ACK_REQUIRED = ACK_REQUIRED_YES;
        mcc_memcpy((void*)&data_destroy_ep_param, (void*)msg.DATA, (unsigned int)sizeof(struct control_message_data_destroy_ep_param));
        ret_value = mcc_send(&uut_control_endpoint, &msg, sizeof(CONTROL_MESSAGE), 0xffffffff);
        ret_value = mcc_recv_copy(&tea_control_endpoint, &ack_msg, sizeof(ACKNOWLEDGE_MESSAGE), &num_of_received_bytes, 0xffffffff);
        EU_ASSERT_REF_TC_MSG( tc_1_main_task, CTR_CMD_DESTROY_EP == ack_msg.CMD_ACK, "TEST #1: 1.13 Checking correct acknowledge message 'CMD_ACK' content");
        EU_ASSERT_REF_TC_MSG( tc_1_main_task, MCC_SUCCESS == ack_msg.RETURN_VALUE, "TEST #1: 1.14 Checking correct acknowledge message 'RETURN_VALUE' content");
    }

    /* Creation of the EP with port==MCC_RESERVED_PORT_NUMBER should not be possible - MCC_ERR_ENDPOINT should be returned */
    msg.CMD = CTR_CMD_CREATE_EP;
    msg.ACK_REQUIRED = ACK_REQUIRED_YES;
    data_create_ep_param.uut_endpoint.port = MCC_RESERVED_PORT_NUMBER;
    mcc_memcpy((void*)&data_create_ep_param, (void*)msg.DATA, (unsigned int)sizeof(struct control_message_data_create_ep_param));
    ret_value = mcc_send(&uut_control_endpoint, &msg, sizeof(CONTROL_MESSAGE), 0xffffffff);
    ret_value = mcc_recv_copy(&tea_control_endpoint, &ack_msg, sizeof(ACKNOWLEDGE_MESSAGE), &num_of_received_bytes, 0xffffffff);
    EU_ASSERT_REF_TC_MSG( tc_1_main_task, CTR_CMD_CREATE_EP == ack_msg.CMD_ACK, "TEST #1: 1.15 Checking correct acknowledge message 'CMD_ACK' content");
    EU_ASSERT_REF_TC_MSG( tc_1_main_task, MCC_ERR_ENDPOINT == ack_msg.RETURN_VALUE, "TEST #1: 1.16 Checking correct acknowledge message 'RETURN_VALUE' content");
        
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
    char            tea_app_buffer[50];
    
    data_create_ep_param.uut_endpoint = uut_endpoint;
    data_create_ep_param.uut_app_buffer_size = 30;

    data_destroy_ep_param.uut_endpoint = uut_endpoint;
    data_destroy_ep_param.endpoint_to_ack = tea_control_endpoint;
    
    data_send_param.dest_endpoint = tea_endpoint;
    mcc_memcpy("abc", (void*)data_send_param.msg, (unsigned int)4);
    data_send_param.msg_size = 20;
    data_send_param.timeout_us = 0xffffffff;
    
    data_recv_param.uut_endpoint = uut_endpoint;
    data_recv_param.uut_app_buffer_size = 30;
    data_recv_param.timeout_us = 0xffffffff;
    data_recv_param.mode = CMD_RECV_MODE_COPY;

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

        ret_value = mcc_recv_copy(&tea_endpoint, &tea_app_buffer, 50, &num_of_received_bytes, 0xffffffff);
        EU_ASSERT_REF_TC_MSG( tc_2_main_task, ret_value == MCC_SUCCESS, "TEST #2: 2.1 Message successfuly recieved");
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

    msg.CMD = CTR_CMD_DESTROY_EP;
    msg.ACK_REQUIRED = ACK_REQUIRED_YES;
    mcc_memcpy((void*)&data_destroy_ep_param, (void*)msg.DATA, (unsigned int)sizeof(struct control_message_data_destroy_ep_param));
    mcc_send(&uut_control_endpoint, &msg, sizeof(CONTROL_MESSAGE), 0xffffffff);
    ret_value = mcc_recv_copy(&tea_control_endpoint, &ack_msg, sizeof(ACKNOWLEDGE_MESSAGE), &num_of_received_bytes, 0xffffffff);
    EU_ASSERT_REF_TC_MSG( tc_2_main_task, CTR_CMD_DESTROY_EP == ack_msg.CMD_ACK, "TEST #2: 2.3 Checking correct acknowledge message 'CMD_ACK' content");
    EU_ASSERT_REF_TC_MSG( tc_2_main_task, MCC_SUCCESS == ack_msg.RETURN_VALUE, "TEST #2: 2.4 Checking correct acknowledge message 'RETURN_VALUE' content");

    mcc_destroy_endpoint(&tea_endpoint);
}

/*TASK*--------------------------------------------------------------------
*
* Task Name    : tc_3_main_task
* Comments     : TEST #3: Testing MCC pingpong2
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
    char            tea_app_buffer[50];
    
    data_create_ep_param.uut_endpoint = uut_endpoint;
    data_create_ep_param.uut_app_buffer_size = 30;

    data_destroy_ep_param.uut_endpoint = uut_endpoint;
    data_destroy_ep_param.endpoint_to_ack = tea_control_endpoint;
    
    data_send_param.dest_endpoint = tea_endpoint;
    mcc_memcpy("", (void*)data_send_param.msg, (unsigned int)4);
    data_send_param.msg_size = 20;
    data_send_param.timeout_us = 0xffffffff;
    data_send_param.uut_endpoint = uut_endpoint;
    
    data_recv_param.uut_endpoint = uut_endpoint;
    data_recv_param.uut_app_buffer_size = 30;
    data_recv_param.timeout_us = 0xffffffff;
    data_recv_param.mode = CMD_RECV_MODE_NOCOPY;

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

        ret_value = mcc_recv_copy(&tea_endpoint, &tea_app_buffer, 50, &num_of_received_bytes, 0xffffffff);
        EU_ASSERT_REF_TC_MSG( tc_2_main_task, ret_value == MCC_SUCCESS, "TEST #3: 3.1 Message successfuly recieved");
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

    msg.CMD = CTR_CMD_DESTROY_EP;
    msg.ACK_REQUIRED = ACK_REQUIRED_YES;
    mcc_memcpy((void*)&data_destroy_ep_param, (void*)msg.DATA, (unsigned int)sizeof(struct control_message_data_destroy_ep_param));
    mcc_send(&uut_control_endpoint, &msg, sizeof(CONTROL_MESSAGE), 0xffffffff);
    ret_value = mcc_recv_copy(&tea_control_endpoint, &ack_msg, sizeof(ACKNOWLEDGE_MESSAGE), &num_of_received_bytes, 0xffffffff);
    EU_ASSERT_REF_TC_MSG( tc_3_main_task, CTR_CMD_DESTROY_EP == ack_msg.CMD_ACK, "TEST #3: 3.3 Checking correct acknowledge message 'CMD_ACK' content");
    EU_ASSERT_REF_TC_MSG( tc_3_main_task, MCC_SUCCESS == ack_msg.RETURN_VALUE, "TEST #3: 3.4 Checking correct acknowledge message 'RETURN_VALUE' content");

    mcc_destroy_endpoint(&tea_endpoint);
}

/*TASK*--------------------------------------------------------------------
*
* Task Name    : tc_4_main_task
* Comments     : TEST #4: Testing MCC mcc_msgs_available functionality
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
    MCC_ENDPOINT    uut_endpoint =  {MCC_UUT_CORE,MCC_UUT_NODE,MCC_UUT_EP_PORT};
    MCC_ENDPOINT    tea_endpoint =  {MCC_TEA_CORE,MCC_TEA_NODE,MCC_TEA_EP_PORT};
    
    data_create_ep_param.uut_endpoint = uut_endpoint;
    data_create_ep_param.uut_app_buffer_size = 30;
    data_create_ep_param.endpoint_to_ack = tea_control_endpoint;

    data_destroy_ep_param.uut_endpoint = uut_endpoint;
    data_destroy_ep_param.endpoint_to_ack = tea_control_endpoint;
    
    data_msg_avail_param.uut_endpoint = uut_endpoint;
    data_msg_avail_param.endpoint_to_ack = tea_control_endpoint;

    mcc_create_endpoint(&tea_endpoint, tea_endpoint.port);
    
    msg.CMD = CTR_CMD_MSG_AVAIL;
    msg.ACK_REQUIRED = ACK_REQUIRED_YES;
    mcc_memcpy((void*)&data_msg_avail_param, (void*)msg.DATA, (unsigned int)sizeof(struct control_message_data_msg_avail_param));
    /* wait until the uut control endpoint is created by the other core */
    while(MCC_ERR_ENDPOINT == mcc_send(&uut_control_endpoint, &msg, sizeof(CONTROL_MESSAGE), 0xffffffff)) {
    }
    ret_value = mcc_recv_copy(&tea_control_endpoint, &ack_msg, sizeof(ACKNOWLEDGE_MESSAGE), &num_of_received_bytes, 0xffffffff);
    EU_ASSERT_REF_TC_MSG( tc_4_main_task, CTR_CMD_MSG_AVAIL == ack_msg.CMD_ACK, "TEST #4: 4.1 Checking correct acknowledge message 'CMD_ACK' content");
    EU_ASSERT_REF_TC_MSG( tc_4_main_task, MCC_ERR_ENDPOINT == ack_msg.RETURN_VALUE, "TEST #4: 4.2 Checking correct acknowledge message 'RETURN_VALUE' content");

    msg.CMD = CTR_CMD_CREATE_EP;
    msg.ACK_REQUIRED = ACK_REQUIRED_YES;
    mcc_memcpy((void*)&data_create_ep_param, (void*)msg.DATA, (unsigned int)sizeof(struct control_message_data_create_ep_param));
    mcc_send(&uut_control_endpoint, &msg, sizeof(CONTROL_MESSAGE), 0xffffffff);
    ret_value = mcc_recv_copy(&tea_control_endpoint, &ack_msg, sizeof(ACKNOWLEDGE_MESSAGE), &num_of_received_bytes, 0xffffffff);
    EU_ASSERT_REF_TC_MSG( tc_4_main_task, CTR_CMD_CREATE_EP == ack_msg.CMD_ACK, "TEST #4: 4.3 Checking correct acknowledge message 'CMD_ACK' content");
    EU_ASSERT_REF_TC_MSG( tc_4_main_task, MCC_SUCCESS == ack_msg.RETURN_VALUE, "TEST #4: 4.4 Checking correct acknowledge message 'RETURN_VALUE' content");

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

    EU_ASSERT_REF_TC_MSG( tc_4_main_task, sent_msg_num == MCC_ATTR_NUM_RECEIVE_BUFFERS-1, "TEST #4: 4.5 Checking correct number of sent messages");

    msg.CMD = CTR_CMD_MSG_AVAIL;
    msg.ACK_REQUIRED = ACK_REQUIRED_YES;
    mcc_memcpy((void*)&data_msg_avail_param, (void*)msg.DATA, (unsigned int)sizeof(struct control_message_data_msg_avail_param));
    mcc_send(&uut_control_endpoint, &msg, sizeof(CONTROL_MESSAGE), 0xffffffff);
    ret_value = mcc_recv_copy(&tea_control_endpoint, &ack_msg, sizeof(ACKNOWLEDGE_MESSAGE), &num_of_received_bytes, 0xffffffff);
    EU_ASSERT_REF_TC_MSG( tc_4_main_task, CTR_CMD_MSG_AVAIL == ack_msg.CMD_ACK, "TEST #4: 4.6 Checking correct acknowledge message 'CMD_ACK' content");
    EU_ASSERT_REF_TC_MSG( tc_4_main_task, MCC_SUCCESS == ack_msg.RETURN_VALUE, "TEST #4: 4.7 Checking correct acknowledge message 'RETURN_VALUE' content");
    EU_ASSERT_REF_TC_MSG( tc_4_main_task, sent_msg_num == *(unsigned int*)ack_msg.RESP_DATA, "TEST #4: 4.8 Checking correct acknowledge message 'RESP_DATA' content");
#if PRINT_ON
            _core_mutex_lock(coremutex_app_ptr);
            printf("sent_msg_num =%x, RESP_DATA = %x\n", sent_msg_num, *(unsigned int*)ack_msg.RESP_DATA);
            _core_mutex_unlock(coremutex_app_ptr);
#endif

    msg.CMD = CTR_CMD_DESTROY_EP;
    msg.ACK_REQUIRED = ACK_REQUIRED_YES;
    mcc_memcpy((void*)&data_destroy_ep_param, (void*)msg.DATA, (unsigned int)sizeof(struct control_message_data_destroy_ep_param));
    mcc_send(&uut_control_endpoint, &msg, sizeof(CONTROL_MESSAGE), 0xffffffff);
    ret_value = mcc_recv_copy(&tea_control_endpoint, &ack_msg, sizeof(ACKNOWLEDGE_MESSAGE), &num_of_received_bytes, 0xffffffff);
    EU_ASSERT_REF_TC_MSG( tc_4_main_task, CTR_CMD_DESTROY_EP == ack_msg.CMD_ACK, "TEST #4: 4.9 Checking correct acknowledge message 'CMD_ACK' content");
    EU_ASSERT_REF_TC_MSG( tc_4_main_task, MCC_SUCCESS == ack_msg.RETURN_VALUE, "TEST #4: 4.10 Checking correct acknowledge message 'RETURN_VALUE' content");

    mcc_destroy_endpoint(&tea_endpoint);
}

/*TASK*--------------------------------------------------------------------
*
* Task Name    : tc_5_main_task
* Comments     : TEST #5: Testing MCC deinitialization
*
*END*---------------------------------------------------------------------*/
void tc_5_main_task(void)
{
    int           ret_value;

    /* Destroy TEA control endpoint */
    ret_value = mcc_destroy_endpoint(&tea_control_endpoint);
    EU_ASSERT_REF_TC_MSG( tc_5_main_task, ret_value == MCC_SUCCESS, "TEST #5: 5.1 Destroying TEA control endpoint");
    ret_value = mcc_destroy(MCC_TEA_NODE);
    EU_ASSERT_REF_TC_MSG( tc_5_main_task, ret_value == MCC_SUCCESS, "TEST #5: 5.2 Destroying MCC");
}

//------------------------------------------------------------------------
// Define Test Suite
 EU_TEST_SUITE_BEGIN(suite_mcc)
    EU_TEST_CASE_ADD(tc_1_main_task, " TEST #1 - Testing MCC initialization"),
    EU_TEST_CASE_ADD(tc_2_main_task, " TEST #2 - Testing MCC pingpong1"),
    EU_TEST_CASE_ADD(tc_3_main_task, " TEST #3 - Testing MCC pingpong2"),
    EU_TEST_CASE_ADD(tc_4_main_task, " TEST #4 - Testing MCC mcc_msgs_available functionality"),
    EU_TEST_CASE_ADD(tc_5_main_task, " TEST #5 - Testing MCC deinitialization"),
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
