/*******************************************************************************

  ioaccess.c: IO access code for Lanner bypass/watchdog module access code

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



#include "../include/config.h"


#ifdef DJGPP

/* standard include file */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
/* For DOS DJGPP */
#include <dos.h>
#include <inlines/pc.h>

#else //DJGPP
/* For Linux */

#define inportb(x) inb(x)
#define outportb(x, y) outb(y, x)
#define inportl(x) inl(x)
#define outportl(x, y) outl(y, x)


#ifdef DIRECT_IO_ACCESS
/* For Linux direct io access code */
/* standard include file */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/io.h>
#include <time.h>
#include <stdint.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#define delay(x) usleep(x)
#endif

#ifdef LINUX_KERNEL_DRIVER
#include <linux/i2c.h>
#ifndef LINUX_24
#include <linux/i2c-dev.h>
#endif
#endif


#ifdef MODULE

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <asm/io.h>
#include <linux/delay.h>

#undef delay
#define delay(x) mdelay(x)
#undef fprintf
#define fprintf(S, A)  printk(A)

#endif //MODULE


#endif

/* local include file */
#include "../include/bp_def.h"
#include "../include/crc8.h"
#include "../include/ioaccess.h"

#if (defined(MODULE) || defined(DIRECT_IO_ACCESS))

int wait_bit_clear(int fd, unsigned char bit_mask) {
    int loop_count=0x100;
    while (loop_count-- > 0) {
        if ( !(inportb(fd) & bit_mask)) return 0;
        outportb(fd, inportb(fd));
        delay(1);    
    }
    return -1;
}
int wait_bit_raise(int fd, unsigned char bit_mask) {
    int loop_count=0x100;
    while (loop_count-- > 0) {
        delay(1);
        if ( inportb(fd) & bit_mask) return 0;
    }
    return -1;
}

#define I2C_READ	0x01
#define I2C_WRITE	0x00
/* return 0 mean host ready for transfer, otherwise return -1 */
int host_not_ready(int fd)
{
    unsigned char temp;
    int timeout = 0;
    
    outportb(fd,0xFF);		// Host Status Register (00)
    delay(1);			
    temp = inportb(fd);
    
    /* Make sure the SMBus host is ready to start transmitting */
    /* 0x1f = Failed, Bus_Err, Dev_Err, Intr, Host_Busy */
    if ((temp = (0x1f & inportb(fd + SMBHSTSTS))) != 0x00) {
        do {
            outportb(fd + SMBHSTSTS, temp);
            delay(1);
            temp = inportb(fd + SMBHSTSTS);
        } while (( (temp & 0x1f) != 0x00) && (timeout++ < MAX_TIMEOUT));
//        printf("temp = %x timeout = %x\n", temp, timeout);
        if ( timeout >= MAX_TIMEOUT) {
//            printf("Temp = %x\n", temp);
            return -1;
        }    
    }
    return 0;
}

/* wait command processed and check error occured */
int host_busy_or_err(int fd, int inhibit_error)
{
    unsigned char temp;
    int     timeout = 0;
    
    /* We will always wait for a fraction of a second! */
    do {
        delay(1);
        temp = inportb(fd + SMBHSTSTS);
    } while ((temp & SMBHSTSTS_HOST_BUSY) && (timeout++ < MAX_TIMEOUT));

    /* If the SMBus is still busy, we give up */
    if (timeout >= MAX_TIMEOUT) {
        fprintf(stderr, "SMBus Timeout!\n");
        outportb(fd+ SMBHSTCNT, inportb(fd + SMBHSTCNT) | SMBHSTCNT_KILL);
        delay(1);
        outportb(fd+ SMBHSTCNT, inportb(fd + SMBHSTCNT) & (~SMBHSTCNT_KILL));
        return -1;
    }
    
    if ( temp & (SMBHSTSTS_FAILED | SMBHSTSTS_BUS_ERR | SMBHSTSTS_DEV_ERR)) {
        if(inhibit_error != 1) /* do not show error message for scan mode */
	{
            fprintf(stderr, "SMBus Error: ");
    	    if ( temp & SMBHSTSTS_FAILED) 
            	fprintf(stderr, " Transaction failed ");
    	    if ( temp & SMBHSTSTS_DEV_ERR) 
            	fprintf(stderr, " No response ");
    	    if ( temp & SMBHSTSTS_BUS_ERR) 
            	fprintf(stderr, " Lost arbitration ");
	    
       	    fprintf(stderr, "\n");
	}
	
        return -1;
    }   
     
    if ((inportb( fd + SMBHSTSTS) & 0x1f) != 0x00)
        outportb(fd + SMBHSTSTS, inportb(fd+SMBHSTSTS));

    
    return 0;
}
int i2c_smbus_write_byte(int fd, unsigned char i2c_address,
                        unsigned char i2c_command,
                        int pec, unsigned char *pec_byte)
{
    
    if ( host_not_ready(fd) ) return -1;
    
    	outportb(fd + SMBHSTCMD, i2c_command);        // Host Command Register (03)
	outportb(fd + SMBHSTADD, (i2c_address<<1|I2C_WRITE)); //Host Address Register (04)
	if ( pec ) {
        	outportb(fd + SMBPEC, *pec_byte);      // PEC Register (08)
        	outportb(fd + SMBHSTCNT, 0x44 | 0x80);    // Host Control Register (02)
                                        // Write byte with PEC and Start
    	} else
        	outportb(fd + SMBHSTCNT, 0x44); // Host Control Register (02)
                                        // Write byte protocol and Start
    if ( host_busy_or_err(fd, 0)) 
        return -1;
    else
        return 0;		                    
} 
int i2c_smbus_write_byte_data(int fd, unsigned char i2c_address, 
                                unsigned char i2c_register, unsigned char value)
{
    
    if ( host_not_ready(fd) ) return -1;
							
	outportb(fd + SMBHSTCMD, i2c_register);	// Host Command Register (03)
	outportb(fd + SMBHSTADD,(i2c_address<<1|I2C_WRITE)); //Host Address Register (04)
	outportb(fd + SMBHSTDAT0, value);	   // Host Data 0 Register (05)
	outportb(fd + SMBHSTCNT, 0x48);        // Host Control Register (02)
                                           // Read byte protocol and Start
    if ( host_busy_or_err(fd, 0)) 
        return -1;
    else
        return 0;		                    
} 

int i2c_smbus_read_byte_data(int fd, unsigned char i2c_address, 
                             unsigned char i2c_register, unsigned char *ret_val, int inhibit_error)
{
		
    if ( host_not_ready(fd) ) return -1;
    					
	outportb(fd + SMBHSTCMD, i2c_register);  // Host Command Register (03)
	outportb(fd + SMBHSTADD,(i2c_address<<1|I2C_READ)); //Host Address Register (04)	 // Set the I2C address
	outportb(fd + SMBHSTCNT, 0x48);          // Host Control Register (02)
				                             // Read byte protocol and Start

    if ( host_busy_or_err(fd, inhibit_error)) return -1;
    
    *ret_val = inportb(fd + SMBHSTDAT0);
    
	return 0;  //read OK
}
	
int i2c_smbus_read_word_data(int fd, unsigned char i2c_address, 
                            unsigned char i2c_register, unsigned short *ret_val)
{
		
    if ( host_not_ready(fd) ) return -1;
    					
	outportb(fd + SMBHSTCMD, i2c_register);  // Host Command Register (03)
	outportb(fd + SMBHSTADD,(i2c_address<<1|I2C_READ)); //Host Address Register (04)	 // Set the I2C address
	outportb(fd + SMBHSTCNT, 0x4C);          // Host Control Register (02)
				                             // Read word protocol and Start

    if ( host_busy_or_err(fd, 0)) return -1;
    
    *ret_val = ((inportb(fd + SMBHSTDAT1) << 8) | inportb(fd + SMBHSTDAT0));
    
	return 0;  //read OK
}

int i2c_smbus_read_block_data(int fd, unsigned char i2c_address, 
                            unsigned char i2c_register, unsigned char ret_array[],
                            unsigned char *pec_byte)
{
    unsigned char   i;
    unsigned int    j;
    int cnt1=0,cnt2=0;
    
    if ( host_not_ready(fd) ) return -1;
    i=inportb(fd);
    delay(1);
	outportb(fd,i);	//clear status again
    
					
    outportb(fd + SMBHSTCMD, i2c_register);	// Host Command Register (03)
    outportb(fd + SMBHSTADD,(i2c_address<<1|I2C_READ)); //Host Address Register (04)
    outportb(fd + SMBPEC, 0x00);       //clear PEC byte
    outportb(fd + SMBHSTDAT0, 0x00);       //clear byte count							
	outportb(fd + SMBHSTCNT, 0x54|0x80);	// Host Control Register (02)
				                // Read block protocol with PEC and Start
	delay(1);				    // delay 1 ms
	j=0;
	if (wait_bit_raise(fd, 0x80)) return -1;
    do {
        i=inportb(fd + SMBHSTSTS);
//        if (i & 0x01) 				//;;termination of command ??
//		{ 
            cnt2=inportb(fd + SMBHSTDAT0);
            if(cnt2!=0) {
//                    printf("cnt2=%x\n", cnt2);
            ret_array[0]=cnt2;
            cnt1=1;
		    ret_array[cnt1++] = inportb(fd + SMBBLKDAT);
//                    printf("cnt2=%x fd+7=%x\n", cnt2, inportb(fd+7));
		    outportb(fd + SMBHSTSTS, inportb(fd+0));
		    cnt2--;
		    cnt2--;
            delay(1);
		    while (cnt2-- > 0) {
                wait_bit_raise(fd, 0x80); 
//               		while (!(inportb(fd) & 0x03)) delay (100);
                ret_array[cnt1++] = inportb(fd + SMBBLKDAT);
//                    printf("cnt2= %x fd+7=%x\n", cnt2, inportb(fd+7));
                outportb(fd + SMBHSTSTS, inportb(fd + SMBHSTSTS));
			    delay(1);
            } 
            outportb(fd + SMBHSTSTS, inportb(fd + SMBHSTSTS));
            delay(1);
		    ret_array[cnt1++] = inportb(fd + SMBBLKDAT);
            outportb(fd + SMBHSTSTS, inportb(fd + SMBHSTSTS));
            delay(1);
		    *pec_byte = inportb(fd + SMBPEC);
//		    printf("pec_byte = %x\n", *pec_byte);		            
			return 0;
//        } else {
//            return -1;
//        }
	}			
	j++;
	delay(1);
    } while(j<=0x100);
	return -1;  //;read fail
}

int i2c_smbus_write_block_data(int fd, unsigned char i2c_address, 
                            unsigned char i2c_register, unsigned char ret_array[],
                            unsigned char *pec_byte)
{
    unsigned char	i;
    unsigned int j;		//;tadd++
    int cnt2=0;
    
//printf("Start write block\n");
    if ( host_not_ready(fd) ) return -1;
    i=inportb(fd + SMBHSTSTS);
    delay(1);
	outportb(fd + SMBHSTSTS,i);	//clear status again
		
	cnt2=ret_array[0];
	outportb(fd + SMBHSTCMD, i2c_register);	// Host Command Register (03)
			                          	// Set the register number
	outportb(fd + SMBHSTADD,(i2c_address<<1|I2C_WRITE)); //Host Address Register (04)
	outportb(fd + SMBBLKDAT, ret_array[1]); //Block data Register (07)
//    printf("fd+7=%x\n", inportb(fd+7));
    outportb(fd + SMBHSTDAT0, ret_array[0]);
    cnt2--;
    outportb(fd + SMBPEC, *pec_byte);
	outportb(fd + SMBHSTCNT, 0x54|0x80);	// Host Control Register (02)
				                            // Write block protocol and Start
	delay(1);				                // delay 1 ms
	for(j=0;j<cnt2;j++) {
        if ( wait_bit_raise(fd, 0x03) ) return -1;
//        printf("fd+7=%x\n", ret_array[j+2]);
        outportb(fd + SMBBLKDAT, ret_array[j+2]);
        outportb(fd + SMBHSTSTS, inportb(fd + SMBHSTSTS));
        delay(1);
	}
    outportb(fd + SMBHSTSTS, inportb(fd + SMBHSTSTS)); //clear status
    delay(1);
    outportb(fd + SMBHSTSTS, inportb(fd + SMBHSTSTS)); //clear status
	return 0;  //;write ok
}
#endif
