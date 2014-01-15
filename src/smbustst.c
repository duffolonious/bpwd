/*******************************************************************************

  smbustst.c: main application for Lanner bypass/watchdog module access code

  Lanner bypass/watchdog module access code
  Copyright(c) 2012 Lanner Electronics Inc.

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
#include "../include/version.h"

/* standard include file */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#ifdef DJGPP

/* For DOS DJGPP */
#include <dos.h>
#include <inlines/pc.h>

#ifndef DIRECT_IO_ACCESS
#error ***Error: define DIRECT_IO_ACCESS in config.h for DOS ***
#endif

#else

/* For Linux */
#include <sys/io.h>
#include <sys/stat.h>
#include <dirent.h>
#include <time.h>
#include <stdint.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include "../include/bpwd_ioctl.h"

#ifdef LINUX_KERNEL_DRIVER
#include <linux/i2c.h>

#ifdef LINUX_24
struct i2c_smbus_ioctl_data {
        char read_write;
        __u8 command;
        int size;
        union i2c_smbus_data *data;
};
#else
#include <linux/i2c-dev.h>
#endif

#endif

#define inportb(x) inb(x)
#define outportb(x, y) outb(y, x)
#define inportl(x) inl(x)
#define outportl(x, y) outl(y, x)
#define delay(x) usleep(x)

#ifdef DIRECT_IO_ACCESS
#warning ***** Note: You build with DIRECT_IO_ACCESS defined *****
#warning ***** Note: undefine this to build for driver code *****
#endif

#endif

/* local include file */
#include "../include/bp_def.h"
#include "../include/crc8.h"

/* pci configuration space definition */
#define PCI_INDEX_PORT 	0xCF8
#define PCI_DATA_PORT   0xCFC
#define	PCIADDR		0x80000000

/* PMIO for AMD  */
#define PMIO_INDEX_PORT 0xCD6
#define PMIO_DATA_PORT 0xCD7
#define SmBus0En  0x2C

/* SMBUS ARP assigned base address */
#define ARP_BASE_ADDR   0x20

/* SMBUS 2.0 ARP UUID definition */
#define ARP_UUID_VID0   2   //Vendor ID
#define ARP_UUID_VID1   3   //Vendor ID
#define ARP_UUID_VSID0  12  //Vendor Specific ID

int i2c_smbus_read_byte_data(int fd, unsigned char i2c_address, 
                             unsigned char i2c_register, unsigned char *ret_val, int inhibit_err);
int i2c_smbus_read_word_data(int fd, unsigned char i2c_address, 
                                      unsigned char i2c_register, unsigned short *ret_val);
int i2c_smbus_read_block_data(int fd, unsigned char i2c_address, 
                            unsigned char i2c_register, unsigned char ret_array[],
                            unsigned char *pec_byte);
int i2c_smbus_write_byte_data(int fd, unsigned char i2c_address, 
                                unsigned char i2c_register, unsigned char value);
int i2c_smbus_write_byte(int fd, unsigned char i2c_address,
                        unsigned char i2c_command,
                        int pec, unsigned char *pec_byte);
int i2c_smbus_write_block_data(int fd, unsigned char i2c_address, 
                            unsigned char i2c_register, unsigned char ret_array[],
                            unsigned char *pec_byte);

void crcInit(void);                                
CRC_TYPE crcUpdate( unsigned char c );
static int tblInitialized = 0;
static CRC_TYPE crcReg;
static CRC_TYPE crcTbl[ 256 ];


/* Scan i2c bus and print out device */
void scan_i2c_bus( int fd );

#ifdef LINUX_KERNEL_DRIVER
/* Enable use of the PEC */
void i2c_pec_enable( int fd );

/* Disable use of the PEC */
void i2c_pec_disable( int fd );

int multi_i2c_busses(int *guess_bus);
#endif


char *program_name;

/*
 * adapter_number ==> smbus adapter number, enumerate by i2c subsystem
 */ 
int adapter_number=-1;

/*
 * scan_mode=0 ==> SMBUS read/write 
 * scan_mode=1 ==> Try to scan SMBUS
 * scan_mode=2 ==> Try to do ARP
 * scan_mode=3 ==> Show Lanner Bypass module information
 * scan_mode=4 ==> Send command I2C switch to get throug bus operation
 */ 
int scan_mode=-1;

/*
 * inhibit_error=1 ==> Do not report device error
 */ 
int inhibit_error=0;

/* 
 * rw_mode=0 ==> write
 * rw_mode=1 ==> read
 */
int rw_mode;

/* 
 * byte_word=0 ==> byte
 * byte_word=1 ==> word
 */
int byte_word;

/*
 * motherboard=0 ==> no i2c switch
 * motherboard=1 ==> with i2c switch
 */
int motherboard=0;

 
int verbose = 0;

/*
 * model_name
 */ 
char model_name[16];

/* 
 * slave_address ==> 7-bit address
 */ 
unsigned int slave_address = 0x00;
unsigned int reg_no = 0xffff;
unsigned int write_data;
int adapter_no;

void print_help(void);

/*
 * Parse argument, if error, return -1
 */ 
int parse_argument(int argc, char** argv)
{
	int write_data_available=0;
	int ret_val=0;
	int opt;
	int argument_available = 0;

    program_name = argv[0];
    
    while ((opt = getopt(argc, argv, "+SIvrwh:M:d:c:o:a:")) != -1) {
        argument_available = 1;
        switch (opt) {
            case 'S':
                scan_mode = 1;
                inhibit_error = 1;
                break;
            case 'M':
                sscanf(optarg, "%s", model_name);
				//scan_mode = 4;
				motherboard=1;
                break;
            case 'I':
                scan_mode = 3;
                break;
            case 'r':
                scan_mode = 0;
                rw_mode = 1;
                byte_word =1;
                break;    
            case 'w':
                scan_mode = 0;
                rw_mode = 0;
                byte_word =0;
                break;    
            case 'd':
                sscanf(optarg, "%x", &slave_address);
                break;    
            case 'c':
                sscanf(optarg, "%x", &reg_no);
                break;    
            case 'o':
                sscanf(optarg, "%x", &write_data);
                write_data_available=1;
                break;
            case 'a':
                sscanf(optarg, "%d", &adapter_number);
                break;
            case 'v':
                verbose = 1;
                break;    
            case 'h':
                return -1;
                break;    
            default:
                fprintf(stderr, "Unknown argument \"%c\"\n", opt);
                ret_val = -1;
        }
    }
//debug +
//	printf("scan_mode=%x rw_mode=%x byte_word=%x slave_address=%x command=%x write_data=%x adapter_num=%d\n", 
//		scan_mode, rw_mode, byte_word, slave_address, reg_no, write_data, adapter_number);
//debug -
    if (!argument_available) return -1;
    if ( scan_mode == -1 ) {
        ret_val=-1;
        fprintf(stderr, "Error: Unknown operation to do, try -r | -w\n");
    }
    if ( (scan_mode==0) && (rw_mode==0) && (write_data_available==0)) {
        ret_val = -1;
        fprintf(stderr, "Error: No output data for write function, try -o [byte_data]\n");
	}
	if (((scan_mode==0) && (slave_address == 0x00)) || 
        ((scan_mode==3) && (slave_address == 0x00))){
        ret_val=-1;
        fprintf(stderr, "Error: Unknown device address, try -d [i2c_address]\n");
	}
	if ( (scan_mode == 0) && (reg_no == 0xffff)) {
        ret_val=-1;
        fprintf(stderr, "Error: Unknown command, try -c [command]\n");
	}
	return ret_val;
       
}
unsigned char pec_table[50];         

unsigned char calculate_pec(unsigned char str[], int len)
{
    int i;
    unsigned char tmp;
    
    crcInit();

    for( i=0; i<len; i++ ){
        tmp = crcUpdate( str[i] );
        printf( "str[%d]=%2.2x residual=0x" CRC_PRN "\n",\
           i, (unsigned char)str[i], tmp );
    }
    return tmp;            
}

typedef struct {
    char model[16];
    unsigned char switch_addr;
    unsigned char switch_data;
} model_method_t;

#define MODEL_GET_THROUGH_NUM	6
model_method_t model_get_through_tbl[MODEL_GET_THROUGH_NUM+1]= {
	//{"MB-887X", 0x73, 0x0F},
	{"MB-887X"   , 0x73, 0x00},
	{"MB-9655"   , 0x70, 0x00},
	{"MB-8895"   , 0x72, 0x00},
	{"MB-8865EXT", 0x73, 0x00},
	{"MB-7582EXT", 0x73, 0x00},
	{"MB-7583"   , 0x73, 0x00},
	{0, 0, 0}
};

int get_through_i2c_bus( int fd ,int slave_addr, int v)
{

    model_method_t *model_ptr;
    unsigned char second_layer_addr;
	
    model_ptr = model_get_through_tbl;
    while ( model_ptr->switch_addr != 0 ) {
		if(!strcmp(model_name, model_ptr->model)) {
                                                              
          if(strcmp(model_ptr->model,"MB-7583")==0) 
			{
				if((slave_addr==0x30))
				{                     
                  model_ptr->switch_data=0x01;     
                  }
                                                                      
            else
				{
				    if (v) {
						fprintf(stderr, "MB-7583 wrong Slave Address\n");
					}
					return -1;
				}
	/*		    	if(i2c_smbus_write_byte(fd, second_layer_addr, model_ptr->switch_data, 0, NULL)) {
			            if (v)
				        fprintf(stderr, "Get through second I2C BUS FAILED\n");
				    return -1;
			    	}                                          */
		    		if (v)
				    fprintf(stdout, "Get through second I2C Bus successful\n");
				return 0;
				
			} 			
  //===========================================================================
                                                                                                                                                                                                                                                                        
                                                            
		  else if(strcmp(model_ptr->model,"MB-7582EXT")==0)  //PCA9545
			{
				if((slave_addr==0x30)||(slave_addr==0x31)||(slave_addr==0x32))
				{
				    /* Slot 1~3 is located at bottom module, left to right */
				    if (slave_addr == 0x30) {
				    	model_ptr->switch_data=0x01;  //channel-00
				    } else if (slave_addr == 0x31) {
				    	model_ptr->switch_data=0x02;  //channel-01
				    } else {
				    	model_ptr->switch_data=0x04;  //channel-02
				    }
				    if(i2c_smbus_write_byte(fd, model_ptr->switch_addr, model_ptr->switch_data, 0, NULL)) {
			            	if (v)
				        	fprintf(stderr, "Get through first I2C BUS FAILED\n");
				    		return -1;
			        	}  
		    			if (v)
				    		fprintf(stdout, "Get through first I2C Bus successful\n");
					return 0;
				}
				else if((slave_addr==0x33)||(slave_addr==0x34)||(slave_addr==0x35))
				{
				    /* Slot 4~6 is located at top module, left to right */
					model_ptr->switch_data=0x08;  //channel-03
				}
				else
				{
				    if (v) {
						fprintf(stderr, "MB-7582EXT wrong Slave Address\n");
					}
					return -1;
				}
				if(i2c_smbus_write_byte(fd, model_ptr->switch_addr, model_ptr->switch_data, 0, NULL)) {
			            if (v)
				        fprintf(stderr, "Get through first I2C BUS FAILED\n");
				    return -1;
			        }  
		    		if (v)
				    fprintf(stdout, "Get through first I2C Bus successful\n");
				    
				second_layer_addr=0x72;  //PCA9545 address
				if (slave_addr==0x33)
				{
				    /* Slot 4 is located at top module, left */
					model_ptr->switch_data=0x04;  //channel-02
				}
				else if(slave_addr==0x34)
				{
				    /* Slot 5 is located at top module, middle */
					model_ptr->switch_data=0x02;  //channel-01
				}
				else if(slave_addr==0x35)
				{
				    /* Slot 6 is located at top module, right */
					model_ptr->switch_data=0x01;  //channel-00
				}
				else
				{
				    if (v) {
						fprintf(stderr, "MB-7582EXT wrong Slave Address\n");
					}
					return -1;
				}
			    	if(i2c_smbus_write_byte(fd, second_layer_addr, model_ptr->switch_data, 0, NULL)) {
			            if (v)
				        fprintf(stderr, "Get through second I2C BUS FAILED\n");
				    return -1;
			    	}
		    		if (v)
				    fprintf(stdout, "Get through second I2C Bus successful\n");
				return 0;
				
			} else
//================================================

			if(strcmp(model_ptr->model,"MB-8865EXT")==0)  //PCA9545
			{
				if((slave_addr==0x30)||(slave_addr==0x31)||(slave_addr==0x32))
				{
				    /* Slot 1~3 is located at bottom module, left to right */
					model_ptr->switch_data=0x02;  //channel-02
				}
				else if((slave_addr==0x33)||(slave_addr==0x34)||(slave_addr==0x35))
				{
				    /* Slot 4~6 is located at top module, left to right */
					model_ptr->switch_data=0x01;  //channel-01
				}
				else
				{
				    if (v) {
						fprintf(stderr, "MB-8865EXT wrong Slave Address\n");
					}
					return -1;
				}
				if(i2c_smbus_write_byte(fd, model_ptr->switch_addr, model_ptr->switch_data, 0, NULL)) {
			            if (v)
				        fprintf(stderr, "Get through first I2C BUS FAILED\n");
				    return -1;
			        }  
		    		if (v)
				    fprintf(stdout, "Get through first I2C Bus successful\n");
				    
				second_layer_addr=0x71;  //PCA9545 address
				if(slave_addr==0x30)
				{
				    /* Slot 1 is located at bottom module, left */
					model_ptr->switch_data=0x02;  //channel-02
//					second_layer_addr=0x73;  //PCA9545 address
				}
				else if(slave_addr==0x31)
				{
				    /* Slot 2 is located at bottom module, middle */
					model_ptr->switch_data=0x01;  //channel-01
//					second_layer_addr=0x73;  //PCA9545 address
				}
				else if(slave_addr==0x32)
				{
				    /* Slot 3 is located at bottom module, right */
					model_ptr->switch_data=0x04;  //channel-03
//					second_layer_addr=0x73;  //PCA9545 address
				}
				else if(slave_addr==0x33)
				{
				    /* Slot 4 is located at top module, left */
					model_ptr->switch_data=0x01;  //channel-01
				}
				else if(slave_addr==0x34)
				{
				    /* Slot 5 is located at top module, middle */
					model_ptr->switch_data=0x02;  //channel-02
				}
				else if(slave_addr==0x35)
				{
				    /* Slot 6 is located at top module, right */
					model_ptr->switch_data=0x04;  //channel-03
				}
				else
				{
				    if (v) {
						fprintf(stderr, "MB-8865EXT wrong Slave Address\n");
					}
					return -1;
				}
			    	if(i2c_smbus_write_byte(fd, second_layer_addr, model_ptr->switch_data, 0, NULL)) {
			            if (v)
				        fprintf(stderr, "Get through second I2C BUS FAILED\n");
				    return -1;
			    	}
		    		if (v)
				    fprintf(stdout, "Get through second I2C Bus successful\n");
				return 0;
				
			}
			else if(strcmp(model_ptr->model,"MB-887X")==0)  //PCA9545
			{
				if(slave_addr==48)  //0x30
				{
					model_ptr->switch_data=0x01;  //channel-01
				}
				else if(slave_addr==49)  //0x31
				{
					model_ptr->switch_data=0x02;  //channel-02
				}
				else if(slave_addr==50)  //0x32
				{
					model_ptr->switch_data=0x04;  //channel-03
				}
				else if(slave_addr==51)  //0x33
				{
					model_ptr->switch_data=0x08;  //channel-04
				}
				else
				{
				    	if (v) {
						fprintf(stderr, "MB-887X wrong Slave Address\n");
					}
					return -1;
				}
			}
			else if(strcmp(model_ptr->model,"MB-9655")==0)  //PCA9548
			{
				//model_ptr->switch_data=0x20;
				if(slave_addr==48)  //0x30
				{
					model_ptr->switch_data=0x20;  //channel05
				}
				else if(slave_addr==49)  //0x31
				{
					model_ptr->switch_data=0x40;  //channel-06
				}
				else if(slave_addr==50)  //0x32
				{
					model_ptr->switch_data=0x80;  //channel-07
				}
				else
				{
				    	if (v) {
						fprintf(stderr, "MB-9655 wrong Slave Address\n");
					}
					return -1;
				}
			}
			else if(strcmp(model_ptr->model,"MB-8895")==0)  //PCA9548(1111-0100)
			{
				if(slave_addr==48)  //0x30
				{
					model_ptr->switch_data=0x01;  //channel01
				}
				else if(slave_addr==49)  //0x31
				{
					model_ptr->switch_data=0x02;  //channel02
				}
				else if(slave_addr==50)  //0x32
				{
					model_ptr->switch_data=0x04;  //channel03
				}
				else if(slave_addr==51)  //0x33
				{
					model_ptr->switch_data=0x08;  //channel04
				}
				else if(slave_addr==52)  //0x34
				{
					model_ptr->switch_data=0x10;  //channel05
				}
				else if(slave_addr==53)  //0x35
				{
					model_ptr->switch_data=0x20;  //channel06
				}
				else if(slave_addr==54)  //0x36
				{
					model_ptr->switch_data=0x40;  //channel07
				}
				else if(slave_addr==55)  //0x37
				{
					model_ptr->switch_data=0x80;  //channel08
				}
				else
				{
				    	if (v) {
						fprintf(stderr, "MB-8895 wrong Slave Address\n");
					}
					return -1;
				}
			}
			else
			{
				if (v) {
					fprintf(stderr, "Fail to Set Switch Data\n");
				}
				return -1;
			}
			//model_ptr->switch_data=0xff;
			if(0)
			{
				fprintf(stdout, "model name : %s\n",model_ptr->model);
				fprintf(stdout, "model addr : %x\n",model_ptr->switch_addr);
				fprintf(stdout, "model data : %x\n",model_ptr->switch_data);
				fprintf(stdout, "addr : %x\n",slave_addr);
			}
			if(i2c_smbus_write_byte(fd, model_ptr->switch_addr, model_ptr->switch_data, 0, NULL)) {
			    if (v)
				fprintf(stderr, "Get through I2C BUS FAILED\n");
				return -1;
			}
		    	if (v)
				fprintf(stdout, "Get through I2C Bus successful\n");
			return 0;
		}
		model_ptr++;
    }
    return -1;
}

int main( int argc, char** argv )
{
	int fd;
	int i, j, len;
	char str[20];
	CRC_TYPE tmp;
	unsigned char pec_byte;
	unsigned char ret_val_8;
	unsigned short ret_val_16;
	unsigned char ret_array[50];

#if ( defined(DIRECT_IO_ACCESS) && !defined(DJGPP) ) 
	iopl(3);
#endif

    if ( verbose ) {
	   fprintf(stdout, "=== Lanner Electronics Inc.===\n");;
	   fprintf(stdout, "Bypass module access utility "CODE_VERSION"\n\n");;
	}   

	if ( parse_argument(argc, argv)) {
		print_help();
		return -1;
	}
#ifdef DIRECT_IO_ACCESS
  /* 1. adapter_number is meaning for base address of SMBUS controller */
  fd = probe_smbus_controller();
  //fd = 0xb20;
  if ( fd == 0 ) {
    fprintf(stderr, "Failed to probe SMBUS controller\n");
    exit(1);
  }
  if (verbose)
    fprintf(stdout, "Found SMBUS controller, base address = 0x%4x\n", fd);
#else

#ifdef LANNER_DRIVER	
	/* 1. check lanner driver available or not */
	strcpy(str, "/dev/bpwd_drv");
	fd = open( str, O_RDWR );
	if( fd == -1 )
	{
		fprintf( stderr, "Failed to open %s\n", str );
		fprintf( stderr, "Please make sure Lanner bpwd driver is loaded\n" );
		exit(1);
	}

#else

	/* Use Linux i2c-dev driver */

	/* try to probe how many i2c bus on system
 	 * Show warning message if multi adapter presented and no "-a" argument input
 	 */ 
	if (adapter_number < 0) {
	    int guess_bus=0;
	    if ( multi_i2c_busses(&guess_bus)) {
		fprintf( stderr, "!!!Warning!!!: Multi i2c-dev presented in system\n"
			         "!!!            Specific i2c device number by \"-a [dev_no]\"\n"
				 "!!!            eg: -a 0 : use i2c-0\n"
				 "!!!            eg: -a 8 : use i2c-8\n"
				 "!!! Now program use \"i2c-%d\" if no \"-a [dev_no] \" argument input\n", guess_bus);
		fprintf( stderr, "!!! Press NETRE to continue...");
		fgetc(stdin);
				
		adapter_number=guess_bus;
		
	    } else {
		adapter_number=guess_bus;
	    }
	}

	/* 1. check smbus adapter available or not */
	sprintf(str,"/dev/i2c-%d", adapter_number);
	fd = open( str, O_RDWR );
	if( fd == -1 )
	{
		fprintf( stderr, "Failed to open %s\n", str );
		fprintf( stderr, "Please make sure i2c adapter, i2c-dev driver is loaded\n" );
		fprintf( stderr, "and proper device node is created(/dev/i2c-0 c 89 0, /dev/i2c-1 c 89 1 ...) \n" );
		exit(1);
	}
#endif //LANNER_DRIVER	

#endif //DIRECT_IO_ACCESS

	if (scan_mode == 1) {
        /* Try to scan i2c bus */ 
		scan_i2c_bus(fd);
		return 0;
	} 
    if (scan_mode == 2) {
        /* Try to do SMBUS 2.0 ARP */
        return arp_i2c_bus(fd);
    }
    if (scan_mode == 3) {
        /* Try to do SMBUS 2.0 ARP */
        return get_information(fd);
    }
	/* i2c read */
    if (rw_mode == 1) {
		if(motherboard==1)
		{
			if (get_through_i2c_bus(fd, slave_address, 1)) return -1;
			slave_address=0x30;
		}
		//slave_address=0x30;
	    fprintf(stdout,"READ ADDRESS:0x%2x Command:0x%2x ...", slave_address, reg_no);
		if (byte_word == 0 ) {        
			
		    if (i2c_smbus_read_byte_data(fd, slave_address, reg_no, &ret_val_8, 0))  
                fprintf(stdout, "FAIL\n");
            else 
                fprintf(stdout, "OK, DATA= 0x%2.2x\n", ret_val_8);
		}    
		if (byte_word == 1 ) {        
		    if (i2c_smbus_read_word_data(fd, slave_address, reg_no, &ret_val_16)) 
				fprintf(stdout, "FAIL\n");
			else 
				fprintf(stdout, "OK, DATA = 0x%4.4x\n", ret_val_16);
		}    
	}
	/* i2c write */
    if ((rw_mode == 0) && (slave_address != 0x50)) {
		if(motherboard==1)
		{
			if ( get_through_i2c_bus(fd, slave_address, 1)) return -1;
			slave_address=0x30;
		}
		//slave_address=0x30;
		fprintf(stdout,"WRITE ADDRESS:0x%2x  Command:0x%2x  Data:0x%2x...", slave_address, reg_no, write_data); 
		if (i2c_smbus_write_byte_data(fd, slave_address, reg_no, write_data))
          fprintf(stdout, "FAIL\n");
        else
          fprintf(stdout, "OK\n");
    }
	
#ifndef DIRECT_IO_ACCESS	
	close(fd);
#endif	
	return 0;
}

void scan_i2c_bus( int fd )
{
	int i;
	unsigned char ret_val;
	unsigned char i2c_ret_val;
	unsigned char scan_addr;
/* 
 * Almost all device support read byte or read byte data command
 * Try to probe known device
 */ 	
	unsigned char known_address[]={	
//	0x2C, //W83792D HW monitor
//	0x2D, //W83792D HW monitor
//	0x2E, //W83792D HW monitor
//      0x2F, //W83792D HW monitor
        0x30, //Lanner Bypass Module
        0x31, //Lanner Bypass Module
        0x32, //Lanner Bypass Module
        0x33, //Lanner Bypass Module
        0x34, //Lanner Bypass Module
        0x35, //Lanner Bypass Module
        0x36, //Lanner Bypass Module
        0x37, //Lanner Bypass Module
//	0x50, //EEPROM
//	0x51, //EEPROM
//	0x52, //EEPROM
//	0x53, //EEPROM
//	0x6A  //clock gen
	};
	for ( i = 0; i < sizeof(known_address); i++) {
		scan_addr = known_address[i];
		if ( (scan_addr <= 0x37) && (scan_addr >=0x30)) {
			if (motherboard == 1) {
				if ( get_through_i2c_bus(fd, scan_addr, 0)) return;
				scan_addr = 0x30;
			}
		}
		if (i2c_smbus_read_byte_data(fd, scan_addr, 0, &ret_val, 1)) 			  			 {
        		fprintf(stdout, "Slot %d (Address:0x%x) No response\n",i+1, known_address[i]);
		}
		else 
		{
        		fprintf(stdout, "Slot %d (Address:0x%x) response with read byte command, value = 0x%x\n",i+1, known_address[i], ret_val);
  		}	
  	}
	return;
}
/* try to do ARP 
 * Code will based on SMBUS 2.0 ARP protocol to handle 
 * slave address(0x61) device with following procedures:
 * 1. Send Prepare to ARP command
 * 2. Get UUID 
 * 3. assign address
 * 4. loop over setp2 until no response
 * 5. Show results
*/
#define ARP_ADDRESS         0x61
#define ARP_PREPARE_TO_ARP  1
#define ARP_GET_UUID        3
#define ARP_ASSIGN_ADDRESS  4
#define MAX_ARP_NUM         10

int arp_i2c_bus( int fd )
{
	int i, j, len, arp_device;
	unsigned char value, pec_byte;
    unsigned char block_data[MAX_ARP_NUM][50];
	unsigned char str[]={0xc2, 0x01};
	unsigned char assigned_address;

    if(verbose) {
        fprintf(stdout, "Start to ARP on SMBUS, maximum support %d device\n", MAX_ARP_NUM);
        fprintf(stdout, "Assigned BAse address = 0x%x\n", ARP_BASE_ADDR);
    }
    /* Step1: Send Prepare to ARP command */
    len = 2;
    pec_byte = calculate_pec(str, len);
    if(i2c_smbus_write_byte(fd, ARP_ADDRESS, ARP_PREPARE_TO_ARP, 1, &pec_byte)) {
        fprintf(stderr, "Send Prepare to ARP Fail\n");
        return -1;
	}
    /* Step2: Get UUID */
    arp_device = 0;
    while (!(i2c_smbus_read_block_data(fd, ARP_ADDRESS, ARP_GET_UUID, block_data[arp_device], &pec_byte)))
    {
        /* PEC include address_w+command+address_r+length+block15+block14+.....+block0+slave_address */
        pec_table[0]=0xc2;
        pec_table[1]=0x03;
        pec_table[2]=0xc3;
        i=block_data[arp_device][0]+1; //include length byte
        j=0;
        while (i-- > 0) {pec_table[j+3] = block_data[arp_device][j]; j++;}
        len = block_data[arp_device][0]+1+3;
        if ( verbose )
            fprintf(stdout, "Found one device, Check PEC...");
        if ( pec_byte == calculate_pec(pec_table, len)) {
            if (verbose) fprintf(stdout, "OK\n");
        } else {
            if (verbose) fprintf(stdout, "FAIL\n");
            break;
        }    
        /* Step 3: assign address */
        assigned_address = (ARP_BASE_ADDR<<1) |((arp_device)<<1);
        if(verbose)
            fprintf(stdout, "Assign to address %x\n\n", assigned_address>>1);
        block_data[arp_device][17]=assigned_address; //assign address
        pec_table[0]=0xc2;
        pec_table[1]=0x04;
        i=block_data[arp_device][0]+1; //include length byte
        j=0;
        while (i-- > 0) {pec_table[j+2] = block_data[arp_device][j]; j++;}
        len = block_data[arp_device][0]+1+2;
        pec_byte = calculate_pec(pec_table, len);
        i2c_smbus_write_block_data(fd, ARP_ADDRESS, ARP_ASSIGN_ADDRESS, block_data[arp_device], &pec_byte);
        arp_device++;    
           
    }
    fprintf(stdout, "ARP done with %d device address assigned",arp_device);
    if(arp_device) {
        for ( j =0; j< arp_device; j++){
            fprintf(stdout, "\nAssigned address: 0x%2.2x",(ARP_BASE_ADDR | j));
            if ( (block_data[j][ARP_UUID_VID0+1] == 0x00)&&
                 (block_data[j][ARP_UUID_VID1+1] == 0x90)&&
                 (block_data[j][ARP_UUID_VSID0+1] == 0x0b)) {
                 /* Lanner device ID
                  * Errata: Once ARP is done, need to write byte data twice with
                  *         command =0 data=0.
                  *         In this stage, device error is expected, just inhibit
                  *         error reporting.                  
                  */
                    fprintf(stdout, " <<Lanner Bypass module>>");
                    inhibit_error = 1;
                    (void)i2c_smbus_write_byte_data(fd, (ARP_BASE_ADDR | j), 0, 0);
                    (void)i2c_smbus_write_byte_data(fd, (ARP_BASE_ADDR | j), 0, 0);
            }     
            if(verbose) {
                fprintf(stdout, "  UUID data as below..\n"); 
                for(i=1; i<18; i++) {   //byte0 = block length, donot show it
                    fprintf(stdout, "%2.2x ", block_data[j][i]);
                }
            }    
        }
        printf("\n");
    }  
     
	return 0;
}
typedef struct {
    unsigned char cmd;
    char cmd_desc[50];
} cmd_t;

cmd_t info_tbl[]={
    {MAJOR_VER_CMD      ,"FW major version: "},                     
    {MINOR_VER_CMD      ,"FW minor version: "},
    {MODULE_CAP_CMD     ,"Module capability: \n"},                     
    {SO_BP_CAP_CMD      ,"System-Off Bypass capability: Support "},                     
    {JO_BP_CAP_CMD      ,"Just-On Bypass capability: Support "},                     
    {RT_BP_CAP_CMD      ,"Run-Time Bypass capability: Support "},                     
    {RT_WDT1_CAP_CMD    ,"Run-Time watchdog1 timer capability: 1~"},                     
    {RT_WDT2_CAP_CMD    ,"Run-Time watchdog2 timer capability: 1~"},                     
    {JO_WDT3_CAP_CMD    ,"Just-On watchdog3 timer capability: 5~"},                     
    {SO_BP_SET_CMD      ,"System-Off Bypass setting:\n"},                    
    {JO_BP_SET_CMD      ,"Just-On Bypass setting:\n"},                     
    {RT_BP_SET_CMD      ,"Run-Time Bypass setting:\n"},                     
    {RT_WDT1_STS_CMD    ,"Run-Time watchdog1 timer status: "},                     
    {RT_WDT1_PAIR_CMD   ,"Run-Time watchdog1 pair setting:\n"},                     
    {RT_WDT1_TIMER_CMD  ,"Run-Time watchdog1 timer count: "},                     
    {RT_WDT2_STS_CMD    ,"Run-Time watchdog2 timer status: "},                     
    {RT_WDT2_PAIR_CMD   ,"Run-Time watchdog2 pair setting:\n"},                     
    {RT_WDT2_TIMER_CMD  ,"Run-Time watchdog2 timer count: "},                     
    {JO_WDT3_STS_CMD    ,"Just-On watchdog3 timer status: "},                     
    {JO_WDT3_PAIR_CMD   ,"Just-On watchdog3 pair setting:\n"},                     
    {JO_WDT3_TIMER_CMD  ,"Just-On watchdog3 timer count: "},
    {0, 0},                     
}; 



int get_information( int fd )
{
 	int i;
	unsigned short ret_val_16;
	unsigned char ret_val, temp;
	cmd_t *info_tbl_ptr;
	unsigned char so_bp_cap, jo_bp_cap, rt_bp_cap;
	//slave_address=0x30;
	if(motherboard==1)
	{
		if (get_through_i2c_bus(fd,slave_address, 1)) return -1;
		slave_address=0x30;
	}
	else
	{
		slave_address=0x37;
	}
	info_tbl_ptr = info_tbl;
	while ( info_tbl_ptr->cmd != 0 ) {
        if (i2c_smbus_read_word_data(fd, slave_address, info_tbl_ptr->cmd, &ret_val_16)) { 
            fprintf(stdout, "Command Fail, check -d [i2c_address} is correct\n");
            break;
        }
        if ( (ret_val_16 & 0x00ff) != (info_tbl_ptr->cmd | CMD_SUCCESS)) { 
            fprintf(stdout, "Un-support Command, check specification\n");
        } else {
            ret_val = (ret_val_16 >> 8);    
            fprintf(stdout, "(cmd 0x%2.2x)%s", info_tbl_ptr->cmd, info_tbl_ptr->cmd_desc);
            switch ( info_tbl_ptr->cmd ) {
                case MAJOR_VER_CMD:                     
                case MINOR_VER_CMD:
                    fprintf(stdout, "%d\n", (ret_val));
                    break;
                case MODULE_CAP_CMD:
                    fprintf(stdout, "\tSystem-Off bypass ");                     
                    if ( !(ret_val & MODULE_CAP_SO_BIT)) 
                        fprintf(stdout, "Not ");
                    fprintf(stdout, "Supported\n");
                                             
                    fprintf(stdout, "\tJust-On bypass ");                     
                    if ( !(ret_val & MODULE_CAP_JO_BIT)) 
                        fprintf(stdout, "Not ");
                    fprintf(stdout, "Supported\n");
                                             
                    fprintf(stdout, "\tRun-Time bypass ");                     
                    if ( !(ret_val & MODULE_CAP_RT_BIT)) 
                        fprintf(stdout, "Not ");
                    fprintf(stdout, "Supported\n");
                                             
                    fprintf(stdout, "\tRun-Time Watchdog1 timer ");                     
                    if ( !(ret_val & MODULE_CAP_WDT1_BIT)) 
                        fprintf(stdout, "Not ");
                    fprintf(stdout, "Supported\n");
                                             
                    fprintf(stdout, "\tRun-Time Watchdog2 timer ");                     
                    if ( !(ret_val & MODULE_CAP_WDT2_BIT)) 
                        fprintf(stdout, "Not ");
                    fprintf(stdout, "Supported\n");      
                                       
                    fprintf(stdout, "\tJust-On Watchdog3 timer ");                     
                    if ( !(ret_val & MODULE_CAP_WDT3_BIT)) 
                        fprintf(stdout, "Not ");
                    fprintf(stdout, "Supported\n"); 
                    break;                        
                case SO_BP_CAP_CMD:              
                    so_bp_cap = ret_val;
                    i=0;       
                    while(ret_val&0x01) {i++; ret_val>>=1;}                           
                    fprintf(stdout, "%d bypass pairs\n", i);                     
                    break;                        
                case JO_BP_CAP_CMD:                           
                    jo_bp_cap = ret_val;
                    i=0;       
                    while(ret_val&0x01) {i++; ret_val>>=1;}                           
                    fprintf(stdout, "%d bypass pairs\n", i);                     
                    break;                        
                case RT_BP_CAP_CMD:
                    rt_bp_cap = ret_val;       
                    i=0;
                    while(ret_val&0x01) {i++; ret_val>>=1;}                           
                    fprintf(stdout, "%d bypass pairs\n", i);                     
                    break;                        
                case RT_WDT1_CAP_CMD:                         
                case RT_WDT2_CAP_CMD:
                    fprintf(stdout, "%d seconds\n", (int)ret_val);
                    break;                     
                case JO_WDT3_CAP_CMD:                     
                    fprintf(stdout, "%d seconds ( 1 scale = 5 seconds)\n", (int)ret_val*5);
                    break;                     
                case SO_BP_SET_CMD:                    
                    i=1;
                    temp = so_bp_cap;
                    while (temp & 0x01 ) {
                        fprintf(stdout,"\tPair %d bypass ", i);
                        if (ret_val & 0x01)
                            fprintf(stdout,"Enable\n");
                        else    
                            fprintf(stdout,"Disbale\n");
                        temp >>=1;
                        ret_val >>=1;
                        i++;
                    }
                    break;
                case JO_BP_SET_CMD:                     
                    i=1;
                    temp = jo_bp_cap;
                    while (temp & 0x01 ) {
                        fprintf(stdout,"\tPair %d bypass ", i);
                        if (ret_val & 0x01)
                            fprintf(stdout,"Enable\n");
                        else    
                            fprintf(stdout,"Disbale\n");
                        temp >>=1;
                        ret_val >>=1;
                        i++;
                    }
                    break;
                case RT_BP_SET_CMD:
                    i=1;
                    temp = rt_bp_cap;
                    while (temp & 0x01 ) {
                        fprintf(stdout,"\tPair %d bypass ", i);
                        if (ret_val & 0x01)
                            fprintf(stdout,"Enable\n");
                        else    
                            fprintf(stdout,"Disbale\n");
                        temp >>=1;
                        ret_val >>=1;
                        i++;
                    }
                    break;
                case RT_WDT1_STS_CMD:                     
                case RT_WDT2_STS_CMD:                     
                case JO_WDT3_STS_CMD:
                    switch (ret_val) {
                        case WDT_STS_STOP:
                            fprintf(stdout, "Timer Stop\n");
                            break;
                        case WDT_STS_RUNNING:
                            fprintf(stdout, "Timer Running\n");
                            break;
                        case WDT_STS_EXPIRED:
                            fprintf(stdout, "Timer Expired\n");
                            break;
                        default:
                            fprintf(stdout, "Unknown state\n");
                            break;
                    }
                    break;                 
                case RT_WDT1_PAIR_CMD:                     
                case RT_WDT2_PAIR_CMD:                     
                case JO_WDT3_PAIR_CMD:
                    i=1;
                    if ( ret_val == 0) {
                        fprintf(stdout,"\tNo Pair setting for timeout\n");
                    } else {
                        temp = rt_bp_cap;
                        while (temp & 0x01 ) {
                            if (ret_val & 0x01)
                                fprintf(stdout,"\tPair %d bypass will Enable while timeout\n", i);
                            temp >>=1;
                            ret_val >>=1;
                            i++;
                        }
                    }    
                    break;
                                     
                case RT_WDT1_TIMER_CMD:                     
                case RT_WDT2_TIMER_CMD:                     
                    fprintf(stdout, " %d seconds\n", (int)ret_val);
                    break;                     
                case JO_WDT3_TIMER_CMD:
                    fprintf(stdout, " %d seconds ( 1 scale = 5 seconds)\n", (int)ret_val*5);
                    break;                     
            }
        }       
        info_tbl_ptr++;   
    }
    /* Get board ID */
    /* first, write byte data to reset board id pointer, 
       then read word data 3 times sequential*/
    if (i2c_smbus_write_byte_data(fd, slave_address, BOARD_ID_CMD, 0)) {
        return -1;
    }
    fprintf(stdout, "(cmd 0x%2.2x)Get Board ID:", BOARD_ID_CMD);
    for ( i=0; i<3; i++) {
        if (i2c_smbus_read_word_data(fd, slave_address, BOARD_ID_CMD, &ret_val_16)) { 
            fprintf(stdout, "\nCommand Fail, check -d [i2c_address} is correct\n");
            return -1;
        }
        fprintf(stdout, "%2.2x %2.2x ", (ret_val_16&0x00ff), (ret_val_16>>8));
    }
    fprintf(stdout, "\n");    
    return 0;
}

void print_help(void)
{
	int i; 
    	model_method_t *model_ptr;
    	model_ptr = model_get_through_tbl;

	fprintf(stdout,"Lanner Bypass/Watchdog Access Code:" CODE_VERSION"\n");;
//	fprintf(stdout,
//        "Usage: %s -M [model_name]\n"\
//		"\tGet through I2C bus for [model_name].\n"\
//		"\tOr\n", program_name);
//	fprintf(stdout,
//        "Usage: %s -M [model_name] -d [i2c_address]\n"\
//		"\tGet through I2C bus for [model_name].\n", program_name);
//	fprintf(stdout, 
//		"\tmodel_name:");
//	for(i=0; i<MODEL_GET_THROUGH_NUM; i++, model_ptr++) {
//	    fprintf(stdout, 
//		    "%s ",model_ptr->model); 
//	}
//	fprintf(stdout,
//		"\n\tOr\n");
	fprintf(stdout,
        "Usage: %s -S [-M model_name ]\n"\
		"\tScan known device.\n"\
		"\tOr\n", program_name);
	fprintf(stdout,
		"Usage: %s -r -d [i2c_address] -c [command] [-M model_name]\n"\
		"\tSMBUS read word data protocol\n"\
        "\tOr\n", program_name);
	fprintf(stdout,	
		"Usage: %s -w -d [i2c_address] -c [command] -o [byte_data] [-M model_name]\n"\
		"\tSMBUS write byte data protocol\n"\
	"\tOr\n", program_name);
	fprintf(stdout,	
		"Usage: %s -I -d [i2c_address] [-M model_name]\n"\
		"\tGet Lanner bypass module information\n", program_name);
	fprintf(stdout,
        "Global argument:\n"\
        "\t-v: be verbose\n"\
        "\t-h: help\n"); 	
    	fprintf(stdout, 
		"Support %d model_name:\n", MODEL_GET_THROUGH_NUM);
	for(i=0; i<MODEL_GET_THROUGH_NUM; i++, model_ptr++) {
	    fprintf(stdout, 
		    "\t %s \n",model_ptr->model); 
	}
		
#ifdef LINUX_KERNEL_DRIVER
	fprintf(stdout,	
		"Note: Add -a [i2c_bus_no] to specific i2c bus number while using kernel driver\n");
#endif
}


#ifdef DIRECT_IO_ACCESS

/* Try to get Base/Sub class code from pci configuration register */
/* In this case I Just probe Intel ICH Bus0 Dev1F Fun3 */
int probe_smbus_controller(void) 
{
  unsigned long addr, data_tmp,amd_base;
  unsigned char ret_val;
  int ret_value=0;
	//check if the smbus is AMD
	addr= (PCIADDR) | ((0x14 <<3)<<8) | (0x00<<8 | 0x00);
	outportl(PCI_INDEX_PORT, addr);
	data_tmp=inportl(PCI_DATA_PORT);
	if(data_tmp == 0x43851002)	
	{
		outportb(PMIO_INDEX_PORT,SmBus0En+1);
		data_tmp = inportb(PMIO_DATA_PORT);
		amd_base=data_tmp;
		//fprintf(stdout, "AMD base address:0x%x\n",amd_base);
		outportb(PMIO_INDEX_PORT,SmBus0En+2);
		data_tmp = inportb(PMIO_DATA_PORT);
		amd_base=(amd_base << 4) | data_tmp;
		//fprintf(stdout, "AMD base address:0x%x\n",amd_base);
		outportb(PMIO_INDEX_PORT,SmBus0En+3);
		data_tmp = inportb(PMIO_DATA_PORT);
		amd_base=(amd_base << 4) | data_tmp;
		//fprintf(stdout, "AMD base address:0x%x\n",amd_base);
		//check the amd i2c base address
		ret_value = i2c_smbus_read_byte_data(amd_base, 0x37, 0, &ret_val, 1);
		if(ret_value == 0)
			return amd_base;
		amd_base=(amd_base | 0x20);
		ret_value = i2c_smbus_read_byte_data(amd_base, 0x37, 0, &ret_val, 1);
		if(ret_value == 0)
			return amd_base;
		return 0;
	}
	else
	{
		//check the chipset smbus controller is working?
		addr= (PCIADDR) | ((0x1F <<3)<<8) | (0x03<<8 | 0x08);
		outportl(PCI_INDEX_PORT, addr);
		data_tmp=inportl(PCI_DATA_PORT);
		if((data_tmp!=0xffffffff)&&( (data_tmp>>16)==(0x0c05)))
		{
			//try to get smbus base address
			addr=(PCIADDR) | ((0x1F <<3)<<8) | (0x03<<8)|0x20;
			outportl(PCI_INDEX_PORT, addr);
			data_tmp=inportl(PCI_DATA_PORT);
			return (data_tmp&0x0000fffe);
		}
		else
			return 0;  
	}
	//check the chipset smbus controller is working?
	//addr= (PCIADDR) | ((0x1F <<3)<<8) | (0x03<<8 | 0x08);
	//outportl(PCI_INDEX_PORT, addr);
	//data_tmp=inportl(PCI_DATA_PORT);
	//if((data_tmp!=0xffffffff)&&( (data_tmp>>16)==(0x0c05)))
	//{
	//	//try to get smbus base address
	//	addr=(PCIADDR) | ((0x1F <<3)<<8) | (0x03<<8)|0x20;
	//	outportl(PCI_INDEX_PORT, addr);
	//	data_tmp=inportl(PCI_DATA_PORT);
	//	return (data_tmp&0x0000fffe);
	//}
	//else
	//	return 0;  
}

#else
typedef struct {
        unsigned char bp_addr;
        unsigned char bp_cmd;
        unsigned char bp_pec;
	unsigned char bp_inhibit_err;
        unsigned char bp_buffer[20];
} bp_s;

#ifdef LANNER_DRIVER
int i2c_smbus_write_byte(int fd, unsigned char i2c_address,
                        unsigned char i2c_command,
                        int pec, unsigned char *pec_byte)
{
	bp_s bp;

	bp.bp_addr = i2c_address;
	bp.bp_cmd = i2c_command;
	if(pec_byte!=NULL)
	{
		bp.bp_pec = *pec_byte;
	}
	else
	{
		bp.bp_pec = i2c_command;
	}
	//bp.bp_pec = *pec_byte;
	memset(bp.bp_buffer, 0, sizeof(bp.bp_buffer));

	return ( ioctl(fd, IOCTL_WRITE_BYTE_CMD, &bp));
}

int i2c_smbus_write_byte_data(int fd, unsigned char i2c_address, 
                                unsigned char i2c_command, unsigned char value)
{

	bp_s bp;

	bp.bp_addr = i2c_address;
	bp.bp_cmd = i2c_command;
	bp.bp_pec = 0;
	memset(bp.bp_buffer, 0, sizeof(bp.bp_buffer));
	bp.bp_buffer[0] = value;

	return ( ioctl(fd, IOCTL_WRITE_BYTE_DATA_CMD, &bp));
}


int i2c_smbus_read_byte_data(int fd, unsigned char i2c_address,
                             unsigned char i2c_command, unsigned char *ret_val, int inhibit_err)
{

	bp_s 	bp;
	int	ret;

	bp.bp_addr = i2c_address;
	bp.bp_cmd = i2c_command;
	bp.bp_pec = 0;
	bp.bp_inhibit_err = inhibit_err;
	memset(bp.bp_buffer, 0, sizeof(bp.bp_buffer));

	ret =  ioctl(fd, IOCTL_READ_BYTE_DATA_CMD, &bp);
	*ret_val = bp.bp_buffer[0];
	return ret;
}

int i2c_smbus_read_word_data(int fd, unsigned char i2c_address,
                            unsigned char i2c_command, unsigned short *ret_val)
{

	bp_s 	bp;
	int	ret;
	unsigned short tmp;

	bp.bp_addr = i2c_address;
	bp.bp_cmd = i2c_command;
	bp.bp_pec = 0;
	memset(bp.bp_buffer, 0, sizeof(bp.bp_buffer));

	ret =  ioctl(fd, IOCTL_READ_WORD_DATA_CMD, &bp);
	tmp = bp.bp_buffer[1];
	tmp <<= 8;
	tmp |= bp.bp_buffer[0];

	*ret_val = tmp;
	
	return ret;

}

int i2c_smbus_read_block_data(int fd, unsigned char i2c_address,
                            unsigned char i2c_command, unsigned char ret_array[],
                            unsigned char *pec_byte)
{
	bp_s 	bp;
	int	ret;

	bp.bp_addr = i2c_address;
	bp.bp_cmd = i2c_command;
	bp.bp_pec = 0;
	memset(bp.bp_buffer, 0, sizeof(bp.bp_buffer));

	ret =  ioctl(fd, IOCTL_READ_BLOCK_DATA_CMD, &bp);
	memcpy(ret_array, bp.bp_buffer, sizeof(bp.bp_buffer));
	*pec_byte = bp.bp_pec;
	return ret;
}

int i2c_smbus_write_block_data(int fd, unsigned char i2c_address, 
                            unsigned char i2c_command, unsigned char ret_array[],
                            unsigned char *pec_byte)

{
	bp_s 	bp;
	int	ret;

	bp.bp_addr = i2c_address;
	bp.bp_cmd = i2c_command;
	bp.bp_pec = *pec_byte;
	memcpy(bp.bp_buffer, ret_array, sizeof(bp.bp_buffer));

	ret =  ioctl(fd, IOCTL_WRITE_BLOCK_DATA_CMD, &bp);
	return ret;
}
#endif

#ifdef LINUX_KERNEL_DRIVER
int i2c_smbus_access(int fd, char read_write, unsigned char command,
                     int size, union i2c_smbus_data *data, int pec)
{
        struct i2c_smbus_ioctl_data args;
	int ret;
#ifndef LINUX_24
//LINUX 2.4 did not support I2C_PEC
	ret = ioctl( fd, I2C_PEC, pec);
	if (!ret) {
#endif
        	args.read_write = read_write;
	        args.command = command;
        	args.size = size;
	        args.data = data;
		ret = ioctl(fd, I2C_SMBUS, &args);
        	return ret;
#ifndef LINUX_24
	}
	return ret;
#endif
}


int set_i2c_slave_address(int fd, unsigned char i2c_address)
{
	int ret;
	ret = ioctl( fd, I2C_SLAVE, i2c_address);
	if(ret < 0 ) {
        	fprintf( stderr, "Failed to set slave address\n");
        	fprintf( stderr, "Maybe occupted by device driver\n");
        }
	return ret;
}

int i2c_smbus_write_byte(int fd, unsigned char i2c_address,
                        unsigned char i2c_command,
                        int pec, unsigned char *pec_byte)
{
	int ret; 
	ret = set_i2c_slave_address(fd, i2c_address);

	if (!ret) {
        	ret = i2c_smbus_access(fd, I2C_SMBUS_WRITE, i2c_command,
	                                I2C_SMBUS_BYTE, NULL, pec);
	}
	return ret;
}

int i2c_smbus_write_byte_data(int fd, unsigned char i2c_address, 
                                unsigned char i2c_command, unsigned char value)
{
	int ret; 
	union i2c_smbus_data data;

	ret = set_i2c_slave_address(fd, i2c_address);
	if ( !ret) {
	        data.byte = value;
        	return i2c_smbus_access(fd,I2C_SMBUS_WRITE,i2c_command,
                                I2C_SMBUS_BYTE_DATA, &data, 0);
	}
}


int i2c_smbus_read_byte_data(int fd, unsigned char i2c_address,
                             unsigned char i2c_command, unsigned char *ret_val, int inhibit_err)
{

	int ret; 
	union i2c_smbus_data data;

	ret = set_i2c_slave_address(fd, i2c_address);
	if ( !ret) {
		ret = i2c_smbus_access(fd, I2C_SMBUS_READ, i2c_command,
                             I2C_SMBUS_BYTE_DATA, &data, 0);
		*ret_val = data.byte;
	}
	return ret;
}

int i2c_smbus_read_word_data(int fd, unsigned char i2c_address,
                            unsigned char i2c_command, unsigned short *ret_val)
{
	int ret; 
	union i2c_smbus_data data;

	ret = set_i2c_slave_address(fd, i2c_address);
	if ( !ret) {
		ret = i2c_smbus_access(fd, I2C_SMBUS_READ, i2c_command,
                             I2C_SMBUS_WORD_DATA, &data, 0);
		*ret_val = data.word;
	}
	return ret;

}
/* 2.6.18 donot support block read */
/* 2.6.25 support it */
int i2c_smbus_read_block_data(int fd, unsigned char i2c_address,
                            unsigned char i2c_command, unsigned char ret_array[],
                            unsigned char *pec_byte)
{
	int ret, i; 
	union i2c_smbus_data data;

	data.block[0]=0x11+2; //block length
	ret = set_i2c_slave_address(fd, i2c_address);
	if ( !ret) {
		ret = i2c_smbus_access(fd, I2C_SMBUS_READ, i2c_command,
                             I2C_SMBUS_I2C_BLOCK_DATA, &data, 1);
		if(!ret) {
//			for(i=0;i<data.block[0]; i++){
//				printf("data[%d]=%x\n", i, data.block[i]);
//			}
			for(i=0;i<=data.block[1]; i++){
				ret_array[i] = data.block[i+1];	
				printf("ret_array[%d]=%x\n", i, ret_array[i]);
			}
		*pec_byte = data.block[i+1];
//		printf("pec_byte = %x\n", *pec_byte);
		}
	}
	return ret;

}

int i2c_smbus_write_block_data(int fd, unsigned char i2c_address, 
                            unsigned char i2c_command, unsigned char ret_array[],
                            unsigned char *pec_byte)

{

	int ret, i; 
	union i2c_smbus_data data;

        for (i = 1; i <= 0x11; i++)
                data.block[i] = ret_array[i-1];
        data.block[0] = 0x13;

	ret = set_i2c_slave_address(fd, i2c_address);
	if ( !ret) {
		ret = i2c_smbus_access(fd, I2C_SMBUS_WRITE, i2c_command,
                             I2C_SMBUS_I2C_BLOCK_DATA, &data, 1);
	}
	return ret;
}

/*
 * Check more than 1 i2c busses presented
 * Return: 	1: more than 1 i2c busses presented
 * 		0: only one bus or not detected(elder or newer kernel?)
 */
int multi_i2c_busses(int *guess_bus) 
{
	char sysfs[1024], n[1024], s[120];
	DIR *dir;
	struct dirent *de;
	FILE *f;
	char *px;
	int bus_count=0;

	sprintf(sysfs,"/sys/class/i2c-adapter");
	if (!(dir = opendir(sysfs)))
		goto done;
	while ((de = readdir(dir)) !=NULL) {
                if (!strcmp(de->d_name, "."))
                        continue;
                if (!strcmp(de->d_name, ".."))
                        continue;
		sprintf(n, "%s/%s/name", sysfs, de->d_name);
		f = fopen(n, "r");
		if (f == NULL) continue;
		px = fgets(s, 120, f);
		fclose(f);
		sscanf(de->d_name, "i2c-%d", guess_bus);
		if ( (strstr(s, "801")) && (bus_count > 1)) {
        		fprintf( stdout, "\n*** Best guess: \"%s\" name=%s\n", de->d_name, s);
		}
 		bus_count++;
	}

done:
	return (bus_count > 1) ? 1 : 0;
}

#endif	//LINUX_KERNEL_DRIVER

#endif

CRC_TYPE crcUpdate( unsigned char c )
{
   crcReg = crcTbl[ crcReg ^ c ];
   return( crcReg );
}


void crcInit( void )
{
unsigned long int i, j, k;

   if( !tblInitialized ){
      for( j=0; j < 256; j++ ){
         k = j;
         for( i=0; i < 8; i++ ){
            k = k & 0x80? (k << 1) ^ POLY : k << 1;
         }
         crcTbl[ j ] = (CRC_TYPE)k;
      }
      tblInitialized = 1;
   }
   crcReg = CRC_INIT;
}




