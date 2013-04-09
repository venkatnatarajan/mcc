#ifndef __test_h__
#define __test_h__
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
* $FileName: mcc.h$
* $Version : 3.8.1.0$
* $Date    : Aug-1-2012$
*
* Comments:
*
*   This file contains the source for the common definitions for the
*   MCC example
*
*END************************************************************************/

#include <linux/mcc_config.h>
#include <linux/mcc_common.h>
#include <mcc_api.h>
#include <linux/mcc_linux.h>
#include <stdint.h>

//#include <mqx.h>
//#include <bsp.h>
//#include "mcc_config.h"
//#include "mcc_common.h"
//#include "mcc_api.h"
//#include "mcc_mqx.h"
//#include <string.h>

//#include <core_mutex.h>
#define uint_32 uint32_t
#define PRINT_ON              (1)
#define _PTR_ *

#define MAIN_TASK             (10)
#define RESP_TASK             (12)

#define TEST_CNT              (10)
#define UUT_APP_BUF_SIZE      (30)
#define TEA_APP_BUF_SIZE      (50)
#define CMD_SEND_MSG_SIZE     (20)
#define CMD_SEND_TIMEOUT_US   (1000000)
#define CMD_RECV_TIMEOUT_US   (2000000)

#define CTR_CMD_CREATE_EP     (10)
#define CTR_CMD_DESTROY_EP    (11)
#define CTR_CMD_SEND          (12)
#define CTR_CMD_RECV          (13)
#define CTR_CMD_MSG_AVAIL     (14)
#define CTR_CMD_GET_INFO      (15)

/* recv command modes */
#define CMD_RECV_MODE_COPY     (1)
#define CMD_RECV_MODE_NOCOPY   (2)

/* acknowledge required? */
#define ACK_REQUIRED_YES       (1)
#define ACK_REQUIRED_NO        (0)

#define PSP_MQX_CPU_IS_VYBRID_A5 
#define MCC_TEST_APP 0

#ifdef PSP_MQX_CPU_IS_VYBRID_A5
#if (MCC_TEST_APP == 1)
  #define MCC_TEA_CORE         (0)
  #define MCC_TEA_NODE         (0)
  #define MCC_TEA_CTRL_EP_PORT (5)
  #define MCC_TEA_EP_PORT      (19)
  #define MCC_UUT_CORE         (1)
  #define MCC_UUT_NODE         (0)
  #define MCC_UUT_CTRL_EP_PORT (6)
  #define MCC_UUT_EP_PORT      (20)
#elif (MCC_TEST_APP == 0)
  #define MCC_TEA_CORE         (1)
  #define MCC_TEA_NODE         (0)
  #define MCC_TEA_CTRL_EP_PORT (5)
  #define MCC_TEA_EP_PORT      (19)
  #define MCC_UUT_CORE         (0)
  #define MCC_UUT_NODE         (0)
  #define MCC_UUT_CTRL_EP_PORT (6)
  #define MCC_UUT_EP_PORT      (20)
#endif
#elif PSP_MQX_CPU_IS_VYBRID_M4
#if (MCC_TEST_APP == 0)
  #define MCC_TEA_CORE         (0)
  #define MCC_TEA_NODE         (0)
  #define MCC_TEA_CTRL_EP_PORT (5)
  #define MCC_TEA_EP_PORT      (19)
  #define MCC_UUT_CORE         (1)
  #define MCC_UUT_NODE         (0)
  #define MCC_UUT_CTRL_EP_PORT (6)
  #define MCC_UUT_EP_PORT      (20)
#elif (MCC_TEST_APP == 1)
  #define MCC_TEA_CORE         (1)
  #define MCC_TEA_NODE         (0)
  #define MCC_TEA_CTRL_EP_PORT (5)
  #define MCC_TEA_EP_PORT      (19)
  #define MCC_UUT_CORE         (0)
  #define MCC_UUT_NODE         (0)
  #define MCC_UUT_CTRL_EP_PORT (6)
  #define MCC_UUT_EP_PORT      (20)
#endif
#endif

MCC_ENDPOINT    tea_control_endpoint = {MCC_TEA_CORE,MCC_TEA_NODE,MCC_TEA_CTRL_EP_PORT};
MCC_ENDPOINT    uut_control_endpoint = {MCC_UUT_CORE,MCC_UUT_NODE,MCC_UUT_CTRL_EP_PORT};
MCC_ENDPOINT    null_endpoint = {0,0,0};

#if defined(__IAR_SYSTEMS_ICC__)
__packed
#endif
struct control_message
{
    uint_32    CMD;
    uint_32    ACK_REQUIRED;
    uint_32    DATA[100];
#if defined(__IAR_SYSTEMS_ICC__)
};
#else
}__attribute__((packed));
#endif
typedef struct control_message CONTROL_MESSAGE, _PTR_ CONTROL_MESSAGE_PTR;

#if defined(__IAR_SYSTEMS_ICC__)
__packed
#endif
struct acknowledge_message
{
    uint_32 CMD_ACK;
    int    RETURN_VALUE;
    int    TS1_SEC;
    int    TS1_MSEC;
    int    TS2_SEC;
    int    TS2_MSEC;
    uint_32 RESP_DATA[100];
#if defined(__IAR_SYSTEMS_ICC__)
};
#else
}__attribute__((packed));
#endif
typedef struct acknowledge_message ACKNOWLEDGE_MESSAGE, _PTR_ ACKNOWLEDGE_MESSAGE_PTR;

#if defined(__IAR_SYSTEMS_ICC__)
__packed
#endif
struct control_message_data_create_ep_param
{
    MCC_ENDPOINT uut_endpoint;
    MCC_MEM_SIZE uut_app_buffer_size;
    MCC_ENDPOINT endpoint_to_ack;
#if defined(__IAR_SYSTEMS_ICC__)
};
#else
}__attribute__((packed));
#endif
typedef struct control_message_data_create_ep_param CONTROL_MESSAGE_DATA_CREATE_EP_PARAM, _PTR_ CONTROL_MESSAGE_DATA_CREATE_EP_PARAM_PTR;

#if defined(__IAR_SYSTEMS_ICC__)
__packed
#endif
struct control_message_data_destroy_ep_param
{
    MCC_ENDPOINT uut_endpoint;
    MCC_ENDPOINT endpoint_to_ack;
#if defined(__IAR_SYSTEMS_ICC__)
};
#else
}__attribute__((packed));
#endif
typedef struct control_message_data_destroy_ep_param CONTROL_MESSAGE_DATA_DESTROY_EP_PARAM, _PTR_ CONTROL_MESSAGE_DATA_DESTROY_EP_PARAM_PTR;

#if defined(__IAR_SYSTEMS_ICC__)
__packed
#endif
struct control_message_data_send_param
{
    MCC_ENDPOINT dest_endpoint;
    uint_32       msg[50]; //if empty send the content of the uup_app_buffer
    MCC_MEM_SIZE msg_size;
    unsigned int timeout_us;
    MCC_ENDPOINT uut_endpoint; //in case of uup_app_buffer has to be used, specify the UUT endpoint
    MCC_ENDPOINT endpoint_to_ack;
    unsigned int repeat_count;
#if defined(__IAR_SYSTEMS_ICC__)
};
#else
}__attribute__((packed));
#endif
typedef struct control_message_data_send_param CONTROL_MESSAGE_DATA_SEND_PARAM, _PTR_ CONTROL_MESSAGE_DATA_SEND_PARAM_PTR;

#if defined(__IAR_SYSTEMS_ICC__)
__packed
#endif
struct control_message_data_recv_param
{
    MCC_ENDPOINT uut_endpoint;
    MCC_MEM_SIZE uut_app_buffer_size;
    unsigned int timeout_us;
    uint_32       mode;
    MCC_ENDPOINT endpoint_to_ack;
#if defined(__IAR_SYSTEMS_ICC__)
};
#else
}__attribute__((packed));
#endif
typedef struct control_message_data_recv_param CONTROL_MESSAGE_DATA_RECV_PARAM, _PTR_ CONTROL_MESSAGE_DATA_RECV_PARAM_PTR;

#if defined(__IAR_SYSTEMS_ICC__)
__packed
#endif
struct control_message_data_msg_avail_param
{
    MCC_ENDPOINT uut_endpoint;
    MCC_ENDPOINT endpoint_to_ack;
#if defined(__IAR_SYSTEMS_ICC__)
};
#else
}__attribute__((packed));
#endif
typedef struct control_message_data_msg_avail_param CONTROL_MESSAGE_DATA_MSG_AVAIL_PARAM, _PTR_ CONTROL_MESSAGE_DATA_MSG_AVAIL_PARAM_PTR;

#if defined(__IAR_SYSTEMS_ICC__)
__packed
#endif
struct control_message_data_get_info_param
{
    MCC_ENDPOINT uut_endpoint;
    MCC_ENDPOINT endpoint_to_ack;
#if defined(__IAR_SYSTEMS_ICC__)
};
#else
}__attribute__((packed));
#endif
typedef struct control_message_data_get_info_param CONTROL_MESSAGE_DATA_GET_INFO_PARAM, _PTR_ CONTROL_MESSAGE_DATA_GET_INFO_PARAM_PTR;

extern void main_task(uint_32);
extern void tc_1_main_task(void);
extern void tc_2_main_task(void);
extern void tc_3_main_task(void);
extern void tc_4_main_task(void);
extern void tc_5_main_task(void);
extern void tc_6_main_task(void);
extern void tc_7_main_task(void);

extern void responder_task(uint_32);


typedef struct time_struct
{

   /*! \brief The number of seconds in the time. */
   uint_32     SECONDS;

   /*! \brief The number of milliseconds in the time. */
   uint_32     MILLISECONDS;

} TIME_STRUCT;

typedef void * pointer;

#define MQX_OK                              (0)

#define TRUE (1)

void _time_get(TIME_STRUCT * time);

#endif
