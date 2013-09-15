/**
 * \addtogroup common
 */
/*@{*/

/***************************************************
 * Header name: gigle_version.h
 *
 * Copyright (C) 2011, Broadcom Corporation
 * All Rights Reserved.
 * 
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 *
 * First written on 18/10/2010 by Toni Homedes
 *
 ***************************************************/
/** \file gigle_version.h
 *
 * \brief This contains the version number and description
 *
 * Version 0.0.1 - 20/10/2010
 *       First compilable version
 *
 * $Id: gigle_version.h 299852 2011-12-01 03:46:47Z $
 ****************************************************/

/* FILE-CSTYLED */

#ifndef VERSION_H_
#define VERSION_H_

/***************************************************
 *                 Public Defines Section
 ***************************************************/

/** \brief Version branch */
#define VERSION_BRANCH  (6) // WIFI

/** \brief Version major */
#define VERSION_MAJOR   (2)
 
/** \brief Version revison */
#define VERSION_REVISION (0)

/** \brief Version minor */
#define VERSION_MINOR   (9)

/*************** HW version defines ******************/
// only the lower 16 bits can be used to identify hw version

//Gigle-WiFi
#define HWVER_RELEASE_MII_GGL541AC 0x80000701
#define HWVER_RELEASE_WBROADCOM_A2XMIIHPAV 0x80000703

/************** Release defines ***********************/

#ifdef RELEASE
  /* */
#endif

#define RAW_SOCKET_INPUT_IF            "vlan7"
#define RAW_SOCKET_OUTPUT_IF           "vlan7"


//#define PRECONFIGURE_NVRAM_ON_GIGLED

#define FW_VERSION (VERSION_BRANCH << 24 | VERSION_MAJOR << 16 | VERSION_REVISION << 8 | VERSION_MINOR)

#else /*VERSION_H_*/
#error "Header file __FILE__ has already been included!"
#endif /*VERSION_H_*/


/*@}*/
