#!/bin/sh
##############################################################################
# Linux sample code to Get board ID of Lanner bypass/watchdog modules
# 
# This script get back board ID. 
#
# Note: Below example is for on-board configuration
#       Developer should add "-M [model_name]" for add-on card configuration
#
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

#Write dummy data to Lanner bypass/watchdog module to reset report counter
./${PROGRAM_NAME} -d ${DEV_ADDR} -c 0x0c -o 0 -w
#Subsequent Read 3 times
./${PROGRAM_NAME} -d ${DEV_ADDR} -c 0x0c -r
./${PROGRAM_NAME} -d ${DEV_ADDR} -c 0x0c -r
./${PROGRAM_NAME} -d ${DEV_ADDR} -c 0x0c -r
