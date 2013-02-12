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

#ifndef PINGPOMG_H_
#define PINGPOMG_H_
/**********************************************************************
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
*   Dec-12-2012 Modified for Linux
*
**********************************************************************/

#define MCC_MQX_NODE_A5 1
#define MCC_MQX_NODE_M4 2

#define uint_32 unsigned int

typedef struct the_message
{
//   MESSAGE_HEADER_STRUCT  HEADER;
   uint_32                DATA;
} THE_MESSAGE, * THE_MESSAGE_PTR;

#endif /* PINGPOMG_H_ */
