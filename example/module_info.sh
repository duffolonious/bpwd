#!/bin/sh
##############################################################################
# Linux sample code to get Lanner bypass/watchdog module information
# 
# This script intends to demostrate how to get module information. 
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

#Get Lanner bypass/watchdog module 
./${PROGRAM_NAME} -d ${DEV_ADDR} -I
