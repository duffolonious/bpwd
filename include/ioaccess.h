/*******************************************************************************

  ioaccess.h : Low level register definition file for Lanner bypass/watchdog 
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


#include "bp_def.h"

#ifndef DJGPP
#define inportb(x) inb(x)
#define outportb(x, y) outb(y, x)
#define inportl(x) inl(x)
#define outportl(x, y) outl(y, x)
#endif

/* SMBUS timeout setting */
#define MAX_TIMEOUT             1000

/* Intel 801 SMBus address offsets */
#define SMBHSTSTS       (0)
#define SMBHSTCNT       (2)
#define SMBHSTCMD       (3)
#define SMBHSTADD       (4)
#define SMBHSTDAT0      (5)
#define SMBHSTDAT1      (6)
#define SMBBLKDAT       (7)
#define SMBPEC          (8)         /* ICH3 and later */
#define SMBAUXSTS       (12)        /* ICH4 and later */
#define SMBAUXCTL       (13)        /* ICH4 and later */

/* I801 Hosts Status register bits */
#define SMBHSTSTS_BYTE_DONE     0x80
#define SMBHSTSTS_INUSE_STS     0x40
#define SMBHSTSTS_SMBALERT_STS  0x20
#define SMBHSTSTS_FAILED        0x10
#define SMBHSTSTS_BUS_ERR       0x08
#define SMBHSTSTS_DEV_ERR       0x04
#define SMBHSTSTS_INTR          0x02
#define SMBHSTSTS_HOST_BUSY     0x01

/* kill bit for SMBHSTCNT */
#define SMBHSTCNT_KILL          2

/* SMBUS ARP assigned base address */
#define ARP_BASE_ADDR   0x20

/* SMBUS 2.0 ARP UUID definition */
#define ARP_UUID_VID0   2   //Vendor ID
#define ARP_UUID_VID1   3   //Vendor ID
#define ARP_UUID_VSID0  12  //Vendor Specific ID


typedef struct {
	unsigned char bp_addr;
	unsigned char bp_cmd;
	unsigned char bp_pec;
	unsigned char bp_inhibit_err;
	unsigned char bp_buffer[20];
} bp_s;

#define I2C_READ        0x01
#define I2C_WRITE       0x00
/* return 0 mean host ready for transfer, otherwise return -1 */
int host_not_ready(int fd);

/* wait command processed and check error occured */
int host_busy_or_err(int fd, int inhibit_err);
int i2c_smbus_write_byte(int fd, unsigned char i2c_address,
                        unsigned char i2c_command,
                        int pec, unsigned char *pec_byte);
int i2c_smbus_write_byte_data(int fd, unsigned char i2c_address,
                                unsigned char i2c_register, unsigned char value);

int i2c_smbus_read_byte_data(int fd, unsigned char i2c_address,
                             unsigned char i2c_register, unsigned char *ret_val, int inhibit_err);

int i2c_smbus_read_word_data(int fd, unsigned char i2c_address,
                            unsigned char i2c_register, unsigned short *ret_val);

int i2c_smbus_read_block_data(int fd, unsigned char i2c_address,
                            unsigned char i2c_register, unsigned char ret_array[],
                            unsigned char *pec_byte);

int i2c_smbus_write_block_data(int fd, unsigned char i2c_address,
                            unsigned char i2c_register, unsigned char ret_array[],
                            unsigned char *pec_byte);


