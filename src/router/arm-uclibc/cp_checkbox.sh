#!/bin/sh

# Created on September 20, 2012
#
# Copyright (c) 2012, NETGEAR, Inc.
# 350 East Plumeria, San Jose California, 95134, U.S.A.
# All rights reserved.
#
# This software is the confidential and proprietary information of
# NETGEAR, Inc. ("Confidential Information").  You shall not
# disclose such Confidential Information and shall use it only in
# accordance with the terms of the license agreement you entered into
# with NETGEAR.

# cp_checkbox.sh - script to check if a device is Ecosystem ready.
# 
#   usage:
#       cp_checkbox.sh <path-for-eco.env> <path-for-currentsetting.htm>
#   e.g.
#       cp_checkbox.sh /tmp /www
#

#################################################################
# System Resource Requirements
#################################################################
MIN_NAND_SIZE=90000	# Minimum non-volatile storage size in Kb
WARN_NAND_SIZE=96000	# Warning size for non-volatile storage
MIN_RAM_SIZE=70000	# Minimum free RAM size in Kb
WARN_RAM_SIZE=80000	# Warning size for RAM size
BROWSER_FILE="currentsetting.htm"
SETTING_ENTRY="SmartNetworkSupported=1"
WGET_URL=http://updates.netgear.com/ecosystem/scripts
WGET_FILE=pkg.tar.gz

#################################################################
# Command parameters
#################################################################
PATH_ECO_ENV=${1}
PATH_CURRENT_SETTING=${2}
if [ -z "${PATH_ECO_ENV}" ] || [ -z "${PATH_CURRENT_SETTING}" ]; then
    echo "Usage:"
    echo "    ${0} <path-for-eco.env> <path-for-currentsetting.htm>"
    echo " e.g."
    echo "    ${0} /tmp /www"
    exit 1
fi

# Source the eco.env file
. ${PATH_ECO_ENV}/eco.env

SAVE_DIR=$PWD
THIS_SCRIPT="cp_checkbox.sh"
LOGFILE="/tmp/cp_checkbox.log"
TESTFILE="test.txt"
TESTMSG="Hello World"
if [ -f ${LOGFILE} ]; then
    mv -f ${LOGFILE} ${LOGFILE}.old
fi
echo "# Log file of test results of command:" > ${LOGFILE}
echo "${0} ${1} ${2}" >> ${LOGFILE}

COUNT_PASS=0
COUNT_FAIL=0
COUNT_WARN=0

####################################################################
#  Functions
####################################################################
# cp_log_header()
#   input:  ${1} = header to be logged
cp_log_header() {
    echo -n "${1} " >> ${LOGFILE}
    return 0
}

####################################################################
# cp_log_newline()
cp_log_newline() {
    echo "" >> ${LOGFILE}
}

####################################################################
# cp_log_uptime()
cp_log_uptime() {
    echo "" >> ${LOGFILE}
    echo " `uptime`" >> ${LOGFILE}
}

################################################
# cp_log_result()
#   input:  ${1} = result:
#	       0 = passed
#	     > 0 = failed
#	     < 0 = warning
cp_log_result() {
    if [ ${1} -eq 0 ]; then
	echo "passed" >> ${LOGFILE}
	COUNT_PASS=`expr $COUNT_PASS + 1`
    elif [ ${1} -gt 0 ]; then
	echo "${1} *** failed ***" >> ${LOGFILE}
	COUNT_FAIL=`expr $COUNT_FAIL + 1`
    else
	echo "${1} * warning *" >> ${LOGFILE}
	COUNT_WARN=`expr $COUNT_WARN + 1`
    fi
    return 0
}

################################################
# cp_check_command()
#
#   function for shell command verification
#   input:  ${1} = shell command
#	    ${2}..${9} = command parameters
#   output: result (passed/failed) - in $LOGFILE
cp_check_command() {
    cp_log_header "Checking command $1 $2 $3 $4 $5 $6 $7 $8 $9..."
    ${1} ${2} ${3} ${4} ${5} ${6} ${7} ${8} ${9}
    CMD_RESULT=$?
    cp_log_result ${CMD_RESULT}
    return ${CMD_RESULT}
}

################################################
# cp_check_size()
#
#   function for checking a size:
#   input: $1 = size to check
#	   $2 = minimum size
#	   $3 = warning size
#	   $4 = Header string to print
#   output 0 = passed
#	   1 = failed
#	  -1 = warning
cp_check_size() {
    cp_log_header "Checking $4 size $1"
    if [ $1 -ge $2 ]; then
	cp_log_header ">= $2"
	cp_log_result 0
    elif [ $1 -lt $3 ]; then
	cp_log_header "< $3"
	cp_log_result 1
    else
	cp_log_header "falls between $2 and $3"
	cp_log_result -1
    fi
}

####################################################################
# Execute the platform dependent script, if ODM provides one. 
####################################################################
SCRIPT_PATH=`echo "${0}" | sed -e"s/${THIS_SCRIPT}//"`
cp_log_uptime
if [ -x ${SCRIPT_PATH}cp_platform.sh ]; then
    echo "Executing ${SCRIPT_PATH}cp_platform.sh ..." >> ${LOGFILE}
    . ${SCRIPT_PATH}cp_platform.sh
else
    echo "No ${SCRIPT_PATH}cp_platform.sh" >> ${LOGFILE}
fi

####################################################################
# System resource check:
####################################################################
# - check if /bin is in $PATH
cp_log_uptime
cp_log_header "Checking /bin in PATH ..."
echo ${PATH} | grep ":/bin"
BIN_IN_PATH=$?
if [ $BIN_IN_PATH -eq 1 ]; then
    echo ${PATH} | grep "^/bin"
    BIN_IN_PATH=$?
fi
cp_log_result $BIN_IN_PATH

# - check non-volatile storage file create/delete
cp_log_header "Non-volatile storage file create/delete ..."
if [ -z $APPS_MOUNT_POINT ]; then
    cp_log_result 1
else
    rm -f $APPS_MOUNT_POINT/$TESTFILE
    if [ -f $APPS_MOUNT_POINT/$TESTFILE ]; then
	cp_log_header "rm"
	cp_log_result 2	# Failed to remove file
    else
    # - create a file
	echo "$TESTMSG" > $APPS_MOUNT_POINT/$TESTFILE
	if [ ! -f $APPS_MOUNT_POINT/$TESTFILE ]; then
	    cp_log_header "echo"
	    cp_log_result 3		# Failed to create file
	else
	    # - check file content
	    if [ "`cat $APPS_MOUNT_POINT/$TESTFILE`" != "$TESTMSG" ]; then
		cp_log_header "$TESTMSG"
		cp_log_result 4	# The file content is different
	    else
	        # -delete file
		cp_log_newline
		cp_check_command rm $APPS_MOUNT_POINT/$TESTFILE
	    fi
	fi
    fi
fi

# - check non-volatile storage size
NAND_SIZE=`df -k $APPS_MOUNT_POINT | grep "[0-9]" | grep -v 1k | awk '{ print $2 }'`
cp_check_size $NAND_SIZE $MIN_NAND_SIZE $WARN_NAND_SIZE "non-volatile storage"

# - check free RAM size
RAM_SIZE=`free | grep "Mem:" | awk '{ print $4 }'`
cp_check_size $RAM_SIZE $MIN_RAM_SIZE $WARN_RAM_SIZE "free RAM"

# - check web browser file
cp_check_command grep "$SETTING_ENTRY" $PATH_CURRENT_SETTING/$BROWSER_FILE

####################################################################
# Check environment variables defined in eco.env 
####################################################################
cp_log_uptime
echo "Checking eco.env ..." >> ${LOGFILE}
cat ${PATH_ECO_ENV}/eco.env | grep "^export" | sed -e's/=/ /' \
		| awk '{ print $2 " " $3 }' > /tmp/eco.env.test
while read line; do
    ENV_VAR_NAME=`echo $line | awk '{ print $1 }'`
    ENV_VAR_VALUE=`echo $line | awk '{ print $2 }'`
    cp_log_header "  $ENV_VAR_NAME=$ENV_VAR_VALUE ... "
    RESULT=0
    if [ -z $ENV_VAR_VALUE ]; then
	RESULT=1
	cp_log_header "(empty) "
    elif [ "$ENV_VAR_NAME" = "DEVICE_MODEL_NAME" ]; then
	DEV_MODEL_UPCASE=`echo $ENV_VAR_VALUE | tr '[:lower:]' '[:upper:]'`
	if [ "$DEV_MODEL_UPCASE" = "$ENV_VAR_VALUE" ]; then
	    RESULT=1
	    cp_log_header "(uppercase) "
	fi
    fi
    cp_log_result $RESULT
done < /tmp/eco.env.test


####################################################################
# Check shell commands
#   Some shell commands are already used in this script so we do
# not need to check them.  Some cannot be checked by the function
# cp_check_command(), e.g, for loop, case statement, etc.
####################################################################
cp_log_uptime
echo "Checking shell commands ..." >> ${LOGFILE} 
cp_check_command uptime
cp_check_command let x=1+2
cp_check_command cp ${PATH_ECO_ENV}/eco.env /tmp/eco.env.test
cp_check_command mkdir $APPS_MOUNT_POINT/testdir
cd $APPS_MOUNT_POINT
cp_check_command find -name testdir -print
cd $SAVE_DIR
cp_check_command rmdir $APPS_MOUNT_POINT/testdir
cp_check_command md5sum ${PATH_ECO_ENV}/eco.env 
cp_check_command cut -d'=' -f1 /tmp/eco.env.test
cp_check_command sed -e's/=/@/' /tmp/eco.env.test
cp_check_command tail /tmp/eco.env.test
cp_check_command wc -l /tmp/eco.env.test
cp_check_command sleep 1
cp_check_command ping -c 1 127.0.0.1
# - check expr
cp_log_header "Checking command expr ..."
X=0
X=`expr $X + 1`
if [ $X -eq 1 ]; then
    cp_log_result 0
else
    cp_log_result 1
fi
# - check whether 'ps' or 'ps -ef' is supported
PS_CMD="ps"
PS_LIST=`${PS_CMD}`
if [ $? -ne 0 ]; then
    PS_CMD="ps -ef"
fi
cp_check_command ${PS_CMD}
# - check crond
cp_log_header "Checking command crond ..."
CROND=`${PS_CMD} | grep " crond " | grep -v grep | wc -l`
if [ ${CROND} -eq 0 ]; then
    crond -b
    CROND=`${PS_CMD} | grep " crond " | grep -v grep | wc -l`
fi
RESULT=`expr 1 - ${CROND}`
cp_log_result $RESULT
# - check crondtab
crontab -l > /tmp/cp_crontab
cp_check_command crontab /tmp/cp_crontab
# - check case statement and for loop 
cp_log_header "Checking case statement in for loop ..."
X=1         
for X in 1 2 3 ; do
    case $X in
	1 | 2 )
	    echo "X=$X le 2";;
	3 )
	    echo "X=$X eq 3";;
    esac
done
cp_log_result $?

####################################################################
# - check wget and tar commands
WGET_OUTPUT=/tmp/$WGET_FILE
cp_check_command rm -f $WGET_OUTPUT
cp_check_command wget $WGET_URL/$DEVICE_MODEL_NAME/$WGET_FILE \
		-P/tmp --delete-after
cp_check_command wget -4 $WGET_URL/$DEVICE_MODEL_NAME/$WGET_FILE \
		-T 10 --tries=1 -O $WGET_OUTPUT
cd /tmp
cp_log_header "Checking tar ..."
cp_check_command tar -zxf $WGET_FILE

####################################################################
cp_log_uptime
cd $SAVE_DIR
echo "All checking done." >> ${LOGFILE}
echo " $COUNT_PASS passed" >> ${LOGFILE}
echo " $COUNT_FAIL failed" >> ${LOGFILE}
echo " $COUNT_WARN warnings" >> ${LOGFILE}
echo "" >> ${LOGFILE}
echo "Shell commands not checked but used in this script:" >> ${LOGFILE}
echo "  cd, tr, while, awk, echo, mv, read" >> ${LOGFILE}
echo "Shell commands not checked or used in this script:" >> ${LOGFILE}
echo "  vi" >> ${LOGFILE}

echo "${0} - All checking done - Results in ${LOGFILE}"

exit 0

