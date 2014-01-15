
/*******************************************************************************

  crc8.h : CRC8 definition file for Lanner bypass/watchdog module access code

  Lanner bypass/watchdog module access code
  Copyright(c) 2010 Lanner Electronics Inc.

  This program is free software; you can redistribute it and/or modify it
  under the terms and conditions of the GNU General Public License,
  version 2, as published by the Free Software Foundation.

  This program is distributed in the hope it will be useful, but WITHOUT
  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
  more details.

  You should have received a copy of the GNU General Public License along with
  this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.

  The full GNU General Public License is included in this distribution in
  the file called "COPYING".

*******************************************************************************/


/* Type define */
typedef unsigned char  UINT8;
typedef unsigned short UINT16;
typedef unsigned long  UINT32;
typedef UINT8      CRC_TYPE;


#define LEN 0x15
#define CRC_PRN "%2.2x"
/*CRC-8     x^8 + x^2 + x + 1 */
#define POLY       0x0107
#define CRC_INIT   0x00



