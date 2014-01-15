/*******************************************************************************

  bp_def.h : command definition file for Lanner bypass/watchdog module access code

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

/* Return code definitions */
#define CMD_SUCCESS 0x80

/* command definitions */

/* CMD = 0x01: major version */
#define MAJOR_VER_CMD       0x01

/* CMD = 0x02: minor version */
#define MINOR_VER_CMD       0x02

/* CMD = 0x03: module capability */
#define MODULE_CAP_CMD      0x03
/*      bit definition */
#define MODULE_CAP_SO_BIT   0x01
#define MODULE_CAP_JO_BIT   0x02
#define MODULE_CAP_RT_BIT   0x04
#define MODULE_CAP_WDT1_BIT 0x08
#define MODULE_CAP_WDT2_BIT 0x10
#define MODULE_CAP_WDT3_BIT 0x20

/* CMD = 0x04: System-Off bypass capability */
#define SO_BP_CAP_CMD       0x04
/*      variable definition */
#define BYPASS_CAP_1PAIR    0x01
#define BYPASS_CAP_2PAIR    0x03
#define BYPASS_CAP_3PAIR    0x07
#define BYPASS_CAP_4PAIR    0x0F
#define BYPASS_CAP_5PAIR    0x1F
#define BYPASS_CAP_6PAIR    0x3F
#define BYPASS_CAP_7PAIR    0x7F
#define BYPASS_CAP_8PAIR    0xFF

/* CMD = 0x05: Just-On bypass capability */
#define JO_BP_CAP_CMD       0x05

/* CMD = 0x06: Run-Time bypass capability */
#define RT_BP_CAP_CMD       0x06

/* CMD = 0x07: watchdog 1(Run-Time) capability */
#define RT_WDT1_CAP_CMD     0x07

/* CMD = 0x08: watchdog 2(Run-Time) capability */
#define RT_WDT2_CAP_CMD     0x08

/* CMD = 0x09: watchdog 3(Just-On) capability */
#define JO_WDT3_CAP_CMD     0x09
    
/* CMD = 0x0A: Load factory default */
#define LOAD_DEFAULT_CMD    0x0A
    
/* CMD = 0x0B: Save current setting */
#define SAVE_SETTING_CMD    0x0B
    
/* CMD = 0x0C: Board ID */
#define BOARD_ID_CMD        0x0C

/*================ Bypass ==================*/
    
/* CMD = 0x10: System-Off bypass setting */
#define SO_BP_SET_CMD       0x10
/*      bit definition */
#define BYPASS_PAIR_1_BIT 0x01
#define BYPASS_PAIR_2_BIT 0x02
#define BYPASS_PAIR_3_BIT 0x04
#define BYPASS_PAIR_4_BIT 0x08
#define BYPASS_PAIR_5_BIT 0x10
#define BYPASS_PAIR_6_BIT 0x20
#define BYPASS_PAIR_7_BIT 0x40
#define BYPASS_PAIR_8_BIT 0x80

/* CMD = 0x11: Just-On bypass setting */
#define JO_BP_SET_CMD       0x11

/* CMD = 0x12: Run-Time bypass setting */
#define RT_BP_SET_CMD       0x12

/*================ Watchdog 1 (Run-Time) ==================*/

/* CMD = 0x20: watchdog1(Run-Time) status */
#define RT_WDT1_STS_CMD     0x20
/*      variable definition */
#define WDT_STS_STOP        0x00
#define WDT_STS_RUNNING     0x01
#define WDT_STS_EXPIRED     0x02

/* CMD = 0x21: watchdog1(Run-Time) pair setting for timeout */
#define RT_WDT1_PAIR_CMD    0x21

/* CMD = 0x22: watchdog1(Run-Time) timer counter */
#define RT_WDT1_TIMER_CMD   0x22

/* CMD = 0x23: watchdog1(Run-Time) estimated time */
#define RT_WDT1_ESTIMATE_CMD 0x23

/* CMD = 0x24: watchdog1(Run-Time) start */
#define RT_WDT1_START_CMD    0x24

/* CMD = 0x25: watchdog1(Run-Time) stop */
#define RT_WDT1_STOP_CMD    0x25

/*================ Watchdog 2 (Run-Time) ==================*/

/* CMD = 0x30: watchdog2(Run-Time) status */
#define RT_WDT2_STS_CMD     0x30

/* CMD = 0x31: watchdog2(Run-Time) pair setting for timeout */
#define RT_WDT2_PAIR_CMD    0x31

/* CMD = 0x32: watchdog1(Run-Time) timer counter */
#define RT_WDT2_TIMER_CMD   0x32

/* CMD = 0x33: watchdog2(Run-Time) estimated time */
#define RT_WDT2_ESTIMATE_CMD 0x33

/* CMD = 0x34: watchdog2(Run-Time) start */
#define RT_WDT2_START_CMD    0x34

/* CMD = 0x35: watchdog2(Run-Time) stop */
#define RT_WDT2_STOP_CMD    0x35

/*================ Watchdog 3 (Just-On) ==================*/

/* CMD = 0x40: watchdog3(Just-On) status */
#define JO_WDT3_STS_CMD     0x40

/* CMD = 0x41: watchdog3(Just-On) pair setting for timeout */
#define JO_WDT3_PAIR_CMD    0x41

/* CMD = 0x42: watchdog3(Just-On) timer counter */
#define JO_WDT3_TIMER_CMD   0x42

/* CMD = 0x43: watchdog3(Just-On) estimated time */
#define JO_WDT3_ESTIMATE_CMD 0x43

/* CMD = 0x45: watchdog3(Just-On) stop */
#define JO_WDT3_STOP_CMD    0x45



