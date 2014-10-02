#!/bin/sh

# Created on April 13, 2012
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

# cp_installer for all devices
# 
#   usage:
#       cp_installer <server-url> <cp-install-dir> <path-to-eco.env> <certificate>
#   e.g.
#       ./cp_installer.sh  http://updates.netgear.com/ecosystem/pkg-repo  .  .
#       ./cp_installer.sh  https://updates.netgear.com/ecosystem/pkg-repo /media/nand .
#       ./cp_installer.sh  https://updates.netgear.com/ecosystem/pkg-repo /media/nand . /etc/ca/CAs.txt
#       ./cp_installer.sh  updates.netgear.com/ecosystem/pkg-repo /media/nand
#
#
# This script file installs the Control Point package as
#       <cp-install-dir>/cp.d

REPO_URL=${1}
LOCAL_DIR=${2}
CP_INSTALL_DIR=${LOCAL_DIR}/cp.d

PATH_ECO_ENV=${3}
if [ -z ${PATH_ECO_ENV} ] || [ ${PATH_ECO_ENV} = "." ]; then
    PATH_ECO_ENV=$PWD
else
    # Check if PATH_ECO_ENV is an absolute path, get the first char
    ABSOLUTE_PATH=`echo "${PATH_ECO_ENV}" | cut -c1`
    if [ "${ABSOLUTE_PATH}" != "/" ]; then
        PATH_ECO_ENV=${PWD}/${PATH_ECO_ENV}
    fi
fi

CA_FILE=${4}
HTTPS_FLAGS=""

#. ./cp.d/cpinst/cp_dbg.sh
cp_timing_debug() {
    if [ -n "${TIMING_DEBUG}" ] && [ ${TIMING_DEBUG} -eq 1 ]; then
	CURR_TIME=`uptime | cut -d " " -f2`
	echo " ___ ${CURR_TIME} ___ ${1}"
    fi
    return 0
}

ENV_EXISTS=0		# eco.env file exists: 0=no, 1=yes
# source the env file, if it's in the same directory
# otherwise the caller must do it before calling this script
if [ -r ${PATH_ECO_ENV}/eco.env ]; then
  echo "sourcing  ${PATH_ECO_ENV}/eco.env ..."
  . ${PATH_ECO_ENV}/eco.env
  ENV_EXISTS=1
fi

if [ "X${DEVICE_MODEL_NAME}" = "X" ]; then
  echo "eco env var DEVICE_MODEL_NAME not defined. exiting ${0}"
  exit 1
fi
echo "DEVICE_MODEL_NAME is $DEVICE_MODEL_NAME"
TARGET_ID=${DEVICE_MODEL_NAME}

if [ "X${FIRMWARE_VERSION}" = "X" ]; then
  echo "        -- eco env var FIRMWARE_VERSION not defined. exiting ${0}"
  exit 1
fi


# wait_mount_nand()
#   wait for the nand partition $APPS_MOUNT_POINT 
#   to be mounted in the system.
#   The environment variable APPS_MOUNT_POINT must be 
#   defined and exported before this script is called.

wait_mount_nand()
{
    OUTFILE=/tmp/mtab.txt
    MTABLE=/proc/mounts
    NAND=$APPS_MOUNT_POINT
    
    if [ "X${NAND}" = "X" ]; then
      echo "        -- eco env var APPS_MOUNT_POINT not defined. exiting ${0}"
      exit 1
    fi
    
    echo "APPS_MOUNT_POINT is $NAND"
    
    # let's put 60 seconds timeout
    count=60
    
    while [ $count -ne 0 ] ;
    do
        # dump mtab to file for pipe
        awk '{ print $2 }' $MTABLE > $OUTFILE

        while read MOUNT
        do
            if [ "$MOUNT" = "$NAND" ]
            then
                echo "$NAND mounted ok."
                rm $OUTFILE
                return 0
            fi
        done < $OUTFILE
        sleep 1
        echo "wait_mount, retry"
        count=`expr $count - 1`
    done

    echo "        -- ${0} timedout"
    return 1
}

echo "	-- ${0}, `uptime`"
if [ -z ${INITIAL_WAIT} ]; then
    if [ ${ENV_EXISTS} -eq 0 ]; then
	# eco.env file does not exist, do not sleep
	INITIAL_WAIT=0
    else
	INITIAL_WAIT=75
    fi
fi
echo "  -- ${0}, sleep INITIAL_WAIT=${INITIAL_WAIT} seconds"
sleep ${INITIAL_WAIT}
wait_mount_nand
RESULT=${?}
echo "	-- ${0}, `uptime`"

if [ ${RESULT} -ne 0 ]; then 
    echo "        -- wait_mount_nand failed ... exiting ${0}"
    exit ${RESULT}
fi

normalized_string=""
normalize_version() 
{
    NUMSIZE=5
    substring=""
    normalized_string=""

    startnum=-1
    chars=`echo ${1} | awk -v ORS="" '{ gsub(/./,"&\n") ; print }'`
    for char in $chars; do 
        case $char in
            "1" | "2" | "3" | "4" | "5" | "6" | "7" | "8" | "9" | "0")
                substring="${substring}${char}"
                ;;
            *)
                if [ ${#substring} -gt 0 ]; then
                    pack_accum=${#substring}
                    while [ ${pack_accum} -lt ${NUMSIZE} ]; do
                        #pack_accum=$((${pack_accum}+1))
                        pack_accum=`expr ${pack_accum} + 1`
                        normalized_string="${normalized_string}0"
                    done
                    normalized_string="${normalized_string}${substring}"
                    substring=""
                fi
                normalized_string="${normalized_string}${char}"
                ;;
        esac
    done

    if [ ${#substring} -gt 0 ]; then
        pack_accum=${#substring}
        while [ ${pack_accum} -lt ${NUMSIZE} ]; do
            #pack_accum=$((${pack_accum}+1))
            pack_accum=`expr ${pack_accum} + 1`
            normalized_string="${normalized_string}0"
        done
        normalized_string="${normalized_string}${substring}"
    fi
}


#   determine the best fit upgrade version
UPDATE_FIRMWARE_VERSION=""
TEMP_FILE=/tmp/tmp_pkglists


get_https_flags()
{
    # get the URL scheme (http or https)
    SCHEME=`echo ${REPO_URL} | cut -d ':' -f1`
    # if no scheme in the URL, prepend "https://"
    if [ "${SCHEME}" != "http" ] && [ "${SCHEME}" != "https" ]; then
        REPO_URL="https://${REPO_URL}"
        SCHEME=https
    fi
    if [ "${SCHEME}" != "http" ]; then
        if [ "${CA_FILE}" != "" ]; then
            CERTIFICATE=${CA_FILE}
            if [ "${CERTIFICATE}" = "" ]; then
                CERTIFICATE=/etc/ca/CAs.txt
            fi
        fi
        HTTPS_FLAGS="--secure-protocol=auto  --ca-certificate=${CERTIFICATE}"
    fi
}


check_update_server()
{
    # each ping = sleep 10 sec (if internet is down), or
    #		= default deadline 20 sec + sleep 10 sec
    # max no. of pings = 15
    # max ping time = 150 sec (internet down), or
    #		    = 450 sec (internet up)
    # max wait time = initial wait (75 sec) + total ping
    #		    = 225 sec (internet down), or
    #		    = 525 sec (internet up)
    SLEEP_TIME=10
    ping_count=15

    SERVER_NAME=`echo ${REPO_URL} | cut -d "/" -f3`
    echo "      -- checking server ${SERVER_NAME}"
    while [ ${ping_count} -ne 0 ] ;
    do
        PING_STAT=`((ping -c1 updates.netgear.com) > /dev/null 2>&1) && echo "up" || echo "down"`
        if [ ${PING_STAT} = "up" ]; then
            return 0
        fi
        ping_count=`expr $ping_count - 1`
        sleep $SLEEP_TIME
        echo -n "#"
    done

    # Server is not reachable.
    echo " -- Unable to ping server ${SERVER_NAME}"
    return 1
}


validate_cp_pkg()
{
    PACKAGE_DB=${CP_INSTALL_DIR}/"pkgdb"
    PKG_LIST=${PACKAGE_DB}/"pkg_list"

    if [ ! -f ${PKG_LIST} ]; then 
        echo "  -- ${PKG_LIST} not found"
        return 1
    fi
    #   iterate through the packages in the pkg_list file
    while read line; do
        PKG_NAME=`echo ${line} | cut -d " " -f1`
        PKG_STATE_FILE=${PACKAGE_DB}/${PKG_NAME}/"state"
        if [ ! -f ${PKG_STATE_FILE} ]; then 
            echo "      -- ${PKG_STATE_FILE} not found"
            return 1
        fi
        PKG_STATE=`cat ${PKG_STATE_FILE}`
        echo "  -- ${PKG_STATE_FILE} = ${PKG_STATE}"
        if [ ${PKG_STATE} != "validated" ]; then
            return 1
        fi
    done < ${PKG_LIST}
    # All packages validated OK
    return 0
}


get_update_version()
{
    check_update_server
    # result: 0=up / 1=down
    SERVER_STAT=${?}
    CP_VALIDATE_RESULT=0

    DONE=0
    while [ $DONE -eq 0 ]; do
        if [ -f ${TEMP_FILE} ]; then
            rm -f ${TEMP_FILE}
        fi
        
	cp_timing_debug "${0} get_update_version wget ### > > >"
        wget -4 ${HTTPS_FLAGS} ${REPO_URL}/${TARGET_ID} -T 30 --tries=1 -O ${TEMP_FILE}
        RESULT=${?}
	cp_timing_debug "get_update_version wget < < < ###"
        if [ ${RESULT} -ne 0 ]; then 
            # If CP was downloaded before, but the server is down,
            # then validate the existing CP package.
            # The validation will need to be run once.
            if [ -d ${CP_INSTALL_DIR}/cpinst ] && [ ${SERVER_STAT} -eq 1 ]; then
                if [ ${CP_VALIDATE_RESULT} -eq 0 ]; then
                    validate_cp_pkg
                    CP_VALIDATE_RESULT=${?}
                    # result: 0=validated / 1=not
                fi
                if [ ${CP_VALIDATE_RESULT} -eq 0 ]; then
                    echo "        -- update server offline, but a cp is present!"
                    return 1
                fi
            fi
            echo "        -- Unable to connect to package server. ... retrying in 1 minute ${0}"
            sleep 60
        else
            DONE=1
        fi
    done
    
    #   always update cpinst if can contact the update server
    if [ -d ./cpinst ]; then
        rm -rf ./cpinst
    fi
    
    # get a normalized version of the firmware version
    normalize_version ${FIRMWARE_VERSION}
    normalized_firmware_version=${normalized_string}
    normalized_update_version=""

    for repo_dir in `grep "pkg_cont-" $TEMP_FILE | sed -e 's/.*href=\"//' -e 's/\/\">.*$//'`; do
        #   extract version
        repo_version=`echo ${repo_dir} | sed -e 's/.*pkg_cont\-//'`
        normalize_version ${repo_version}
        normalized_repo_version=${normalized_string}
        
        normalize_version `echo ${repo_version} | sed 's/-[^-]*$//'`
        min_target_version=${normalized_string}
        if [ ! "${min_target_version}" \> "${normalized_firmware_version}" ]; then
            if [ -z "${UPDATE_FIRMWARE_VERSION}" -o ! "${normalized_repo_version}" \< "${normalized_update_version}" ]; then
                UPDATE_FIRMWARE_VERSION=${repo_version}
                normalized_update_version=${normalized_repo_version}
            fi
        fi
    done 

    #   clean up the temp file
    if [ -f ${TEMP_FILE} ]; then
        rm -f ${TEMP_FILE}
    fi

    echo "INSTALLED_FIRMWARE_VERSION = \"${FIRMWARE_VERSION}\""
    if [ -z "${UPDATE_FIRMWARE_VERSION}" ]; then
        echo "no corresponding version, exiting ... !"
        exit 1
    fi
    echo "UPDATE_FIRMWARE_VERSION = \"${UPDATE_FIRMWARE_VERSION}\""
    
    return 0
}


install_cpinst()
{
    get_https_flags
    get_update_version
    RESULT=${?}
    if [ $RESULT -ne 0 ]; then
        echo "        -- revert to installed cpinst!"
        return 0
    fi

    #   fetch the cp startup scripts from the repository
    DONE=0
    while [ $DONE -eq 0 ]; do
        # delete partially downloaded file just in case
        rm -f /tmp/cpinst.tar.gz

	cp_timing_debug "${0} install_cpinst wget === > > >"
        wget -4 ${HTTPS_FLAGS} ${REPO_URL}/${TARGET_ID}/pkg_cont-${UPDATE_FIRMWARE_VERSION}/packages/cpinst.tar.gz -O /tmp/cpinst.tar.gz
        RESULT=${?}
	cp_timing_debug "install_cpinst wget < < < ==="
        if [ ${RESULT} -ne 0 ]; then 
            echo "        -- Unable to connect to package server. ... retrying in 1 minute ${0}"
            sleep 60
        else
            DONE=1
        fi
    done

    tar -zxf /tmp/cpinst.tar.gz
    rm -rf ./META-INF
    rm -rf /tmp/cpinst.tar.gz
}

#   call the core startup script
if [ -d ${LOCAL_DIR} ]; then
    if [ ! -d ${CP_INSTALL_DIR} ]; then
        mkdir ${CP_INSTALL_DIR}
    fi
    cd ${CP_INSTALL_DIR}
    
    install_cpinst

    export HTTPS_FLAGS

    if [ -x ./cpinst/cp_startup.sh ]; then
        ./cpinst/cp_startup.sh ${TARGET_ID} ${FIRMWARE_VERSION} ${REPO_URL} ${PATH_ECO_ENV}
    else
        echo "        -- startup script doesn't exist ... exiting ${0}"
        exit 1
    fi
fi

exit 0

