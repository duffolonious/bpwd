/*******************************************************************************

  bpwd_main.c : Linux driver for Lanner bypass/watchdog module access code

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


/* Standard in kernel modules */
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/sched.h>
#include <linux/pci.h>
#include <asm/io.h>
#include <linux/delay.h>
#include <linux/spinlock.h>
#include <asm/uaccess.h>
#include "../include/bpwd_ioctl.h"
#include "../include/ioaccess.h"
#include "../include/version.h"

/*
 * Device Major Number
 */
#define BPWD_MAJOR 247 
/*
 * IO spcae size
 */
#define SMBUS_IO_SIZE 0x20

/*
 * Add support for linux kernel over 2.6.36
 */
#ifndef LINUX_VERSION_CODE
#include <linux/version.h>
#else
#define KERNEL_VERSION(a,b,c) (((a) << 16) + ((b) << 8) + (c))
#endif

/*
 * Add for AMD
 */
#define PCI_INDEX_PORT	0xCF8
#define PCI_DATA_PORT	0xCFC
#define PCIADDR		0x80000000

#define PMIO_INDEX_PORT	0xCD6
#define PMIO_DATA_PORT	0xCD7
#define SmBus0En	0x2C


/*
 * Is the device opened right now?
 * Prevent to access the device in the same time
 */
static int Device_Open = 0;
static struct pci_dev *smbus_dev;
static u16 io_base;

//static DEFINE_SPINLOCK(bpwd_lock);

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,36))
static int bpwd_ioctl(struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg)
#else
static long bpwd_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
#endif
{
#if defined(OLDKERNEL)
	char *up =(void *)arg;
#else
	char __user *up = (void __user *)arg;
#endif
	bp_s bp; 
	

	if ( copy_from_user(&bp, up, sizeof(bp_s)))
		return -EFAULT;
//printk("Before addr= %x cmd= %x, bp_buffer=%x %x, pec=%x\n", bp.bp_addr, bp.bp_cmd, bp.bp_buffer[0], bp.bp_buffer[1], bp.bp_pec);
	switch(cmd)
	{
		case IOCTL_READ_BYTE_DATA_CMD:
			if (i2c_smbus_read_byte_data(io_base, bp.bp_addr, bp.bp_cmd, 
						&bp.bp_buffer[0], bp.bp_inhibit_err))
                		return -EFAULT;
			break;
		case IOCTL_READ_WORD_DATA_CMD:
			if (i2c_smbus_read_word_data(io_base, bp.bp_addr, bp.bp_cmd, 
							(unsigned short*)&bp.bp_buffer[0]))
                		return -EFAULT;
			break;
		case IOCTL_WRITE_BYTE_CMD:
			if (i2c_smbus_write_byte(io_base, bp.bp_addr, bp.bp_cmd, 1,
							 &bp.bp_pec))
                		return -EFAULT;
			break;
		case IOCTL_WRITE_BYTE_DATA_CMD:
			if (i2c_smbus_write_byte_data(io_base, bp.bp_addr, bp.bp_cmd, 
							bp.bp_buffer[0]))
                		return -EFAULT;
			break;
		case IOCTL_READ_BLOCK_DATA_CMD:
			printk("Start read block\n");
			if (i2c_smbus_read_block_data(io_base, bp.bp_addr, bp.bp_cmd, 
							bp.bp_buffer, &bp.bp_pec))
                		return -EFAULT;
			break;
		case IOCTL_WRITE_BLOCK_DATA_CMD:
			if (i2c_smbus_write_block_data(io_base, bp.bp_addr, bp.bp_cmd, 
							bp.bp_buffer, &bp.bp_pec))
                		return -EFAULT;
			break;
		default:
			return -EFAULT;
	}
//printk("After addr= %x cmd= %x, bp_buffer=%x %x, pec=%x\n", bp.bp_addr, bp.bp_cmd, bp.bp_buffer[0], bp.bp_buffer[1], bp.bp_pec);
	if (copy_to_user(up, &bp, sizeof(bp_s)))
{
		printk("copy error\n");
		return -EFAULT;
}
	else
		return 0;
}

/*
 * This function is called whenever a process attempts to
 * open the device file
 */
static int bpwd_open(struct inode * inode, struct file * file)
{
	/* we don't want to talk to two processes at the same time */
	if(Device_Open) return -EBUSY;
	Device_Open++;
	/* Make sure that the module isn't removed while the file
	 * is open by incrementing the usage count (the number of
	 * opened references to the module,if it's zero emmod will
	 * fail)
	 */
//	printk("Lanner Watchdog Driver Opened\n");
	return 0;
}

/*
 * This function is called when a process closes the device file.
 */
static int bpwd_release(struct inode * inode, struct file * file)
{
	/* ready for next caller */
	Device_Open--;
	/* Decrement the usage count, otherwise once you opened the file
	 * you'll never get rid of the module.
	 */
//	printk("Lanner Watchdog Driver Closed\n");
	return 0;
}

/*
 * This structure will hold the functions to be called
 * when a process does something to the device we created.
 * Since a pointer to this structure is kept in the
 * devices table, it can't be local to init_module.
 * NULL is for unimplemented functions.
 */
#if defined(OLDKERNEL)
static struct file_operations bpwd_fops = {
	owner:		THIS_MODULE,
	read:		NULL,
	write:		NULL,
	ioctl:		bpwd_ioctl,
	open:		bpwd_open,
	release:	bpwd_release,
};
#else
static const struct file_operations bpwd_fops = {
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,36))
	.ioctl		= bpwd_ioctl,
#else
	.unlocked_ioctl		= bpwd_ioctl,
#endif
	.open		= bpwd_open,
	.release	= bpwd_release,
};
#endif

static int probe_device(void)
{
	int err=0,ret_value=0;
	unsigned long addr, data_tmp, amd_base;
	unsigned char tmp,ret_val;

	addr = (PCIADDR) | (0x14 << 3 <<8) | (0x00 << 8 | 0x00);
	outportl(PCI_INDEX_PORT,addr);
	data_tmp=inportl(PCI_DATA_PORT);
	printk("AMD Vendor ID: 0x%lx\n",data_tmp);
	if(data_tmp == 0x43851002)
	{
		printk("Match vendor ID !!\n");
		//Get AMD smbus base address
		outportb(PMIO_INDEX_PORT, SmBus0En+1);
		tmp=inportb(PMIO_DATA_PORT);
		amd_base = (amd_base) | tmp;
		outportb(PMIO_INDEX_PORT, SmBus0En+2);
		tmp=inportb(PMIO_DATA_PORT);
		amd_base = (amd_base << 4) | tmp;
		outportb(PMIO_INDEX_PORT, SmBus0En+3);
		tmp=inportb(PMIO_DATA_PORT);
		amd_base = (amd_base << 4) | tmp;
		//printk("Base address %d : %x\n",i,tmp);
		//printk("AMD Base Address: 0x%lx\n",amd_base);
		io_base = amd_base;
		ret_value = i2c_smbus_read_byte_data(io_base, 0x37, 0,&ret_val,1);
		if(ret_value == 0)
			return err;	
		//printk("Return Value : %d\n",ret_value);
		io_base = io_base | 0x20;
		ret_value = i2c_smbus_read_byte_data(io_base, 0x37, 0,&ret_val,1);
		if(ret_value == 0)
			return err;
		err= -ENODEV;
		return err;
		//printk("Return Value : %d\n",ret_value);
		//printk("---AMD Base Address: 0x%x\n",io_base);
	}
	else
	{
#if defined(OLDKERNEL)
	smbus_dev = pci_find_slot(0, PCI_DEVFN(0x1F, 3));
#else
	smbus_dev = pci_get_bus_and_slot(0, PCI_DEVFN(0x1F, 3));
#endif
		printk("Probe SMBUS controller..");
		if (!smbus_dev) {
			printk("Cannot locate Intel SMBUS controller\n");
			err = -ENODEV;
			goto probe_exit;
		}

		/* enable pci device */
#if defined(OLDKERNEL)
                err = pci_enable_device_bars(smbus_dev, 1<<4);
#else
		err = pci_enable_device_io(smbus_dev);
#endif
        	if (err) {
                	printk("Failed to enable SMBus PCI device (%d)\n",err);
                	err= -EBUSY;
			goto probe_exit;
        	}

		/* Get IO resource */
		pci_read_config_word(smbus_dev, 0x20, &io_base);
		io_base &= 0xfffe;
		if ( !io_base ) {
			printk("No resource for SMBUS controller\n");
			err =  -ENODEV;
			goto probe_exit;
		} else {
			printk("Got SMBUS IO = 0x%x\n", io_base);
		}
	}
	if (!request_region(io_base, SMBUS_IO_SIZE, "lanner_bypass")) {
                printk("SMBus region 0x%x already in use! "
			" Try to unload i2c_i801 driver\n", io_base);
		err = -EBUSY;
		goto probe_exit;
	}

probe_exit:
#if !defined(OLDKERNEL)
	pci_dev_put(smbus_dev);
#endif
	
	return err;
}

int bpwd_init(void)
{
	/*
	 * Register the character device
	 */
	if(register_chrdev(BPWD_MAJOR, "bpwd_drv", &bpwd_fops))
	{
		printk("bpwd_drv : unable to get major %d\n", BPWD_MAJOR);
		return -EIO;
	}
	if ( probe_device()) {
		unregister_chrdev(BPWD_MAJOR, "bpwd_drv");
		return -EIO;
	}
	//spin_lock_init(&bpwd_lock);
	printk("Lanner Bypass/Watchdog Module Driver Version %s -- loaded\n", CODE_VERSION);
	return 0;
}

/*
 * Cleanup - unregister the appropriate file from /proc
 */
void bpwd_exit(void)
{
	/* Unregister the device */
	unregister_chrdev(BPWD_MAJOR, "bpwd_drv");
	/* release request region */
	release_region(io_base, SMBUS_IO_SIZE);
	/* If there's an error, report it */
	printk("Lanner Bypass/Watchdog Module Driver -- Unloaded\n");
}

module_init(bpwd_init);
module_exit(bpwd_exit);

MODULE_AUTHOR("Lanner SW");
MODULE_DESCRIPTION("Lanner Bypass/Watchdog Module Driver");
MODULE_LICENSE("Dual BSD/GPL");
