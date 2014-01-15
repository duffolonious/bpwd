Lanner Bypass/Watchdog Module Access Code
=========================================

Jan 10, 2013

Contents
========
- In This Release
- Building 
- Installation
- HOW-TO Access
- Support model
- License


In This Release
===============
This file describes the Lanner bypass/watchdog module acces code.  
This package provides program and driver for both DOS and Linux.
Separated makefile available as well.(Makefile.dos or Makefile.linux)

For DOS environment, Use DJGPP asncompiler, use DJGPP and Makefile.dos.

For Linux, supports kernel versions 2.4.x, 2.6.x and 3.x.x and Makefile.linux. 


This release provide 3 ways to access Lanner bypass/watchdog module: 

1. DIRECT_IO_ACCESS:

   To access Lanner bypass/watchdog module by userland application direct
   accessing. This mode JUST FOR providing quick way for user to realize 
   Lannner watchdog/bypass function. This mode supported on DOS and Linux.

   Note: This mode is enabled by set DIRECT_IO_ACCESS=1 in Makefile.


2. LANNER_DRIVER:

   To access Lanner bypass/watchdog module through lanner driver. 
   Lanner driver(bpwd_drv.ko) is available in bin sub-directory after make.
   This driver is only supported as a loadable module at this time.  Lanner is
   not supplying patches against the kernel source to allow for static linking
   of the driver. 

   Note: This mode is enabled by set DIRECT_IO_ACCESS=0 and LANNER_DRIVER=1 in 
   Makefile.


3. LINUX_KERNEL_DRIVER:

   To access Lanner bypass/watchdog module through linux i2c dev interface. 
   i2c-dev.ko and i2c-i801.ko are need to be loader for normal access.
   /dev/i2c-0 c 89 0 node is need to be created as well.

   Note1: This mode is enabled by set DIRECT_IO_ACCESS=0 and LANNER_DRIVER=0 in 
          Makefile.
   Note2: Linux kernel driver(i2c-i801.c) is lacked for block accessing. 
          Varied kernel version may get different results. 
          For example, 2.6.18(CentOS-54) do not support SMBUS block function.
	  2.6.25(FC9) block write function fail. 
          Since Lanner bypass/watchdog ARP function relied on BLOCK+PEC 
          accessing, this cause ARP function fail.
   Note3: MB-5330 is using AMD Structure system, and we only support 
          DIRECT_IO_ACCESS and LANNER_DRIVER in source code.
          


Building
========
To build, 3 steps to completed:

1. Identify current OS and select proper Makefile. 
   Copy Makefile.(os) to Makefile.

2. Select access mode(DIRECT_IO_ACCESS=[0|1] and LANNER_DRIVER=[0|1] by 
   edit Makefile.

3. Just type make to build, once completed, bin sub-directory contains.



Installation
============
To installation, depend on what's access mode you set:

If DIRECT_IO_ACCESS=1, no driver is need. Just execute bpwd_tst program to
to access Lanner bypass/watchdog module.

If DIRECT_IO_ACCESS=0 and LANNER_DRIVER=1, driver is needed. Insert module and
create node in /dev as below example:

	#insmod bpwd_drv.[k]o

	#mknod /dev/bpwd_drv c 247 0


If DIRECT_IO_ACCESS=0 and LANNER_DRIVER=0, linux kernel driver is needed. 
i2c subsystem: i2c-core, i2c-dev and busses/i2c-i801 need to be loaded before
access Lanner bypass/watchdog module, dev node need to create as below:

	#mknod /dev/i2c-0 c 89 0
	
	Note: '0' in here is for example, check proper bus number of kernel driver 
              populated. 


HOW-TO ACCESS
=============
Once build completed, application(and driver) is available in bin sub-directory.
Main access program is bpwd_tst. just run it without argument will list help.

Lanner Bypass/Watchdog Access Code:V1.1.5 2012-08-14
Usage: ./bpwd_tst -S [-M [model_name]]
        Scan Lanner bypass module.
        Or
Usage: ./bpwd_tst -r -d [i2c_address] -c [command] [-M [module_name]]
        SMBUS read word data protocol
        Or
Usage: ./bpwd_tst -w -d [i2c_address] -c [command] -o [byte_data] [-M [module_name]]
        SMBUS write byte data protocol
        Or
Usage: ./bpwd_tst -I -d [i2c_address] [-M [module_name]]
        Get Lanner bypass module information
Global argument:
        -v: be verbose
        -h: help


For multiple card configuration
-------------------------------
  Since multiple bypass add-on card maybe populate on motherboard, isolate bypass 
card by I2C switch is implemented in following motherboard:

	model_name	Lanner model 
	=====================================================
	MB-887X         MB-8875, MB-8876, MB-8865, MB-8866
        MB-9655         MB-9655
        MB-8895         MB-8895, MB-8893
        MB-8865EXT      MB-8865(with RC-8865 expansion card, 6 slots)
        MB-7582EXT      MB-7582(with RC-7582 expansion card, 6 slots)
        MB-7583         MB-7583         
 
Program need to specific with -M [model_name] for dedicate motherboard and
device address [specific by -d] is fixed as below:
	0x30: slot1
	0x31: slot2
	0x32: slot3
	0x33: slot4
	0x34: slot5
	0x35: slot6
	0x36: slot7

If bypass controller is add-on card, below using i2c address 0x30 at first slot on the motherboard. 
Following is example within -M command:

     MB-8875 as example, access slot1( address is :0x30)
     Write:
	bpwd_tst -w -d 0x30 -c 0x12 -o 0x03 -M MB-887X 
     Read:
	bpwd_tst -r -d 0x30 -c 0x12 -M MB-887X  
     Scan Bus:
	bpwd_tst -S -M MB-887X


For on-board configuration
--------------------------
If bypass controller is embedded in motherboard, no -M parameter is needed and i2c address is fixed to 0x37.
Following is example without -M command:

     MB-8765 as exampple
     Write
	bpwd_tst -w -d 0x37 -c 0x12 -o 0x03
     Read:
	bpwd_tst -r -d 0x37 -c 0x12
     Scan Bus:
	bpwd_tst -S

Example sub-directory presented you how to access module. 
You may add -M parameter for proper motherboard.



Support model
=============
Following is the list of Lanner's model which support 3rd bypass.

3rd bypass add-on card, which connect to motherboard with i2c switch to control each device.
You should have to use -M command to control each slot.
    model_name(-M)      Lanner model
    ===========================================================
    MB-887X             MB-8875, MB-8876, MB-8865, MB-8866
    MB-9655             MB-9655
    MB-8895	        MB-8895, MB-8893
    MB-8865EXT          MB-8865(with RC-8865 expansion card, 6 slots)
    MB-7582EXT      	MB-7582(with RC-7582 expansion card, 6 slots)
    MB-7583             MB-7583  

3rd bypass embedded on motherboard, which only use i2c address 0x37 to control the micro-processor.
No need to use -M command to control micro-processor.       	 
    Lanner model
    ====================================
    MB-8756
    MB-8770
    MB-7582
    MB-5330	( There is only support lanner driver and direct i/o access to MB-5330 )	
    MB-7583


License
=======
Lanner bypass/watchdog module access code
Copyright(c) 2013 Lanner Electronics Inc.

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





