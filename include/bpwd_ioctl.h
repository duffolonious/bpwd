/*******************************************************************************

  bpwd_ioctl.h : Linux IOCTL code definition file for Lanner bypass/watchdog 
		 module access code

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

#include <linux/ioctl.h>

#define MAGIC_CHAR 'B'

/* READ BYTE DATA COMMAND */
#define IOCTL_READ_BYTE_DATA_CMD	_IOW(MAGIC_CHAR, 20, int)

/* READ WORD DATA COMMAND */
#define IOCTL_READ_WORD_DATA_CMD	_IOW(MAGIC_CHAR, 21, int)

/* WRITE BYTE COMMAND */
#define IOCTL_WRITE_BYTE_CMD		_IOW(MAGIC_CHAR, 22, int)

/* WRITE BYTE DATA COMMAND */
#define IOCTL_WRITE_BYTE_DATA_CMD	_IOW(MAGIC_CHAR, 23, int)

/* READ BLOCK COMMAND*/
#define IOCTL_READ_BLOCK_DATA_CMD	_IOW(MAGIC_CHAR, 24, int)

/* WRITE BLOCK COMMAND*/
#define IOCTL_WRITE_BLOCK_DATA_CMD	_IOW(MAGIC_CHAR, 25, int)

