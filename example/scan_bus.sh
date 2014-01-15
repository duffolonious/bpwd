#!/bin/sh
##############################################################################
# Linux sample code to scan Lanner bypass device
# 
# Note: Below example is for on-board configuration
#       Developer should add "-M [model_name]" for add-on card configuration
#
# 
# More command instructions, See bp_def.h and document for details
# 
##############################################################################

PROGRAM_NAME=bpwd_tst

if [ ! -x ${PROGRAM_NAME} ] ; then
echo "${PROGRAM_NAME} NOT FOUND"
exit
fi

./${PROGRAM_NAME} -S
