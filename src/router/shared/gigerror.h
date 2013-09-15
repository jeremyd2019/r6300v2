/**
 * \addtogroup common
 */
/*@{*/

/***************************************************
 * Header name: gigerror.h
 *
 * Copyright (C) 2011, Broadcom Corporation
 * All Rights Reserved.
 * 
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 *
 * First written on 13/07/2006 by I. Barrera
 *
 ***************************************************/
/** \file gigerror.h
 *
 * \brief This contains the definition of errors
 *
 * All error definitions for Concorde modules must
 * be contained here
 *
 * $Id: gigerror.h 299852 2011-12-01 03:46:47Z $
 **************************************************/

/* FILE-CSTYLED */

#ifndef GIGERROR_H_
#define GIGERROR_H_


/***************************************************
 *                 Public Constants Section
 ***************************************************/

/** \brief Error codes. */
enum {
  GIG_ERR_OK             =   0, /**< Result is OK */
  GIG_ERR_ERROR          =  -1, /**< Result is a generic (undefined) error */
  GIG_ERR_MQUEUE_FULL    =  -2, /**< Full queue error. Used when sending to a mqueue (GigOS) */
  GIG_ERR_MQUEUE_EMPTY   =  -3, /**< Empty queue error. Used when receiving from a mqueue (GigOS) */
  GIG_ERR_INV            =  -4, /**< Invalid parameter */
  GIG_ERR_DISCARDED      =  -5, /**< Discarded log (log level not high enough to be displayed - GigLog) */
  GIG_ERR_EOF            =  -6, /**< End of input hit (only used by the GigLog parser so far) */
  GIG_ERR_SYNTAX         =  -7, /**< Syntax error of a command (GigLog) */
  GIG_ERR_NG             =  -8, /**< Wrong result of a command (GigLog) */
  GIG_ERR_PENDING        =  -9, /**< Operation didn't finish, still pending. */
  GIG_ERR_PARTIAL        = -10, /**< Partially fit (Bwmgr) */
  GIG_ERR_NOBANDWIDTH    = -11, /**< No bandwidth available (Bwmgr) */
  GIG_ERR_NORESOURCES    = -12, /**< No resources available (Bwmgr) */
  GIG_ERR_REQS_INCORRECT = -13, /**< Incorrect requirements (Bwmgr) */
  GIG_ERR_NOT_SUPPORTED  = -14, /**< Not supported function. */
  GIG_ERR_BUSY           = -15, /**< The requested operation can't be accepted because target module is busy */
  GIG_ERR_HOLD           = -16, /**< The received packet wasn't processed by its registered client: HPAV will keep it */
  GIG_ERR_NOT_FOUND      = -17, /**< Not found (Tonemapboss). */
  GIG_ERR_TIMEOUT        = -18 /**< Timeout */
};

/***************************************************
 *                 Public Defines Section
 ***************************************************/

/** \brief Type for the returned error code from functions. */
typedef int t_gig_error;

#else /*GIGERROR_H_*/
#error "Header file __FILE__ has already been included!"
#endif /*GIGERROR_H_*/

/*@}*/
