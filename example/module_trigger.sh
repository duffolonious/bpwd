#!/bin/sh
##############################################################################
# Linux sample code to re-trigger watchdog1 timer
# 
# This script intends to demostrate how to setup watchdog1 related command to 
# force bypass enabled while timer expired.
#
# In this case, User want to setup LAN pair1 enable while watchdog1 timer 
# timeout, and set watchdog1 timer counter to 5 seconds, forever loop will 
# re-trigger watchdog in every 3 second to keep watchdog1 timer alive.
# Once you Ctrl-C to stop this script, because no program to re-trigger 
# watchdog1 timer, cause bypass will enable within 5 seconds.
#  
# Note: Below example is for on-board configuration
#       Developer should add "-M [model_name]" for add-on card configuration
#
# Command Usage:
# 	get currect watchdog1 status by command 0x20
# 	setup bypass pair1 enable while watchdog1 timer expired by command 0x21, 
# 	setup watchdog1 timer counter by command 0x22
# 	start and re-trigger watchdog1 timer by command 0x24
# More command instructions, See bp_def.h and document for details
# 
##############################################################################

PROGRAM_NAME=bpwd_tst
#change DEV_ADDR to proper address
DEV_ADDR=30

if [ ! -x ${PROGRAM_NAME} ] ; then
echo "${PROGRAM_NAME} NOT FOUND"
exit
fi


#Setup LAN pair1 bypass will enable while watchdog1 timer expired
./${PROGRAM_NAME} -d ${DEV_ADDR} -c 21 -o 1 -w

#Set watchdog1 timer to 5 second
./${PROGRAM_NAME} -d ${DEV_ADDR} -c 22 -o 5 -w

#forever loop
while true; do

#Get current watchdog timer status
./${PROGRAM_NAME} -d ${DEV_ADDR} -c 20 -r

#Start watchdog timer, re-trigger watchdog timer as well
./${PROGRAM_NAME} -d ${DEV_ADDR} -c 24 -o 0 -w

#sleep 3 second
sleep 3 

done;
