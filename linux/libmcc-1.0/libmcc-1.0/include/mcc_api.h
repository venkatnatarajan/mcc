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

#ifndef __MCC_API__
#define __MCC_API__

int mcc_initialize(MCC_NODE);
int mcc_destroy(MCC_NODE);
int mcc_create_endpoint(MCC_ENDPOINT*, MCC_PORT);
int mcc_destroy_endpoint(MCC_ENDPOINT*);
int mcc_send(MCC_ENDPOINT*, void*, MCC_MEM_SIZE, unsigned int);
int mcc_recv_copy(MCC_ENDPOINT*, void*, MCC_MEM_SIZE, MCC_MEM_SIZE*, unsigned int);
int mcc_recv_nocopy(MCC_ENDPOINT*, void**, MCC_MEM_SIZE*, unsigned int);
int mcc_msgs_available(MCC_ENDPOINT*, unsigned int*);
int mcc_free_buffer(void*);
int mcc_get_info(MCC_NODE, MCC_INFO_STRUCT*);

#endif /* __MCC_API__ */

