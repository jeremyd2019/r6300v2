/*
 * Broadcom IGD module, soap_x_layer3forwarding.c
 *
 * Copyright (C) 2011, Broadcom Corporation
 * All Rights Reserved.
 * 
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 *
 * $Id: soap_x_layer3forwarding.c 241182 2011-02-17 21:50:03Z $
 */
#include <upnp.h>
#include <InternetGatewayDevice.h>

/*
 * WARNNING: PLEASE IMPLEMENT YOUR CODES AFTER 
 *          "<< USER CODE START >>"
 * AND DON'T REMOVE TAG :
 *          "<< AUTO GENERATED FUNCTION: "
 *          ">> AUTO GENERATED FUNCTION"
 *          "<< USER CODE START >>"
 */

/* << AUTO GENERATED FUNCTION: statevar_DefaultConnectionService() */
static int
statevar_DefaultConnectionService
(
	UPNP_CONTEXT * context,
	UPNP_SERVICE * service,
	UPNP_TLV * tlv
)
{
	HINT(STR::tlv)

	/* << USER CODE START >> */
	UPNP_SERVICE *wan_service = 0;
	UPNP_ADVERTISE *wan_advertise = 0;
	char *buf = upnp_buffer(context);
	
	wan_service = upnp_get_service_by_control_url(context, "/control?WANIPConnection");
	if (!wan_service)
		return SOAP_ACTION_FAILED;

	wan_advertise = upnp_get_advertise_by_name(context, wan_service->name);
	if (!wan_advertise)
		return SOAP_ACTION_FAILED;
    
	/* Construct out string */
	sprintf(buf, "uuid:%s:WANConnectionDevice:1,urn:upnp-org:serviceId:%s",
		wan_advertise->uuid,
		wan_service->service_id);

	/* Update tlv */
	upnp_tlv_set(tlv, (int)buf);
	return OK;
}
/* >> AUTO GENERATED FUNCTION */

/* << AUTO GENERATED FUNCTION: action_SetDefaultConnectionService() */
static int
action_SetDefaultConnectionService
(
	UPNP_CONTEXT * context,
	UPNP_SERVICE *service,
	IN_ARGUMENT *in_argument,
	OUT_ARGUMENT *out_argument
)
{
	HINT( STR:UPNP_TLV *in_NewDefaultConnectionService = UPNP_IN_TLV("NewDefaultConnectionService"); )

	/* << USER CODE START >> */
	/* modify default connection service */
	return OK;
}
/* >> AUTO GENERATED FUNCTION */

/* << AUTO GENERATED FUNCTION: action_GetDefaultConnectionService() */
static int
action_GetDefaultConnectionService
(
	UPNP_CONTEXT * context,
	UPNP_SERVICE *service,
	IN_ARGUMENT *in_argument,
	OUT_ARGUMENT *out_argument
)
{
	HINT( STR::UPNP_TLV *out_NewDefaultConnectionService = UPNP_OUT_TLV("NewDefaultConnectionService"); )

	/* << USER CODE START >> */
	UPNP_TLV *out_NewDefaultConnectionService = UPNP_OUT_TLV("NewDefaultConnectionService");

	return statevar_DefaultConnectionService(context, service, out_NewDefaultConnectionService);
}
/* >> AUTO GENERATED FUNCTION */


/* << TABLE BEGIN */
/*
 * WARNNING: DON'T MODIFY THE FOLLOWING TABLES
 * AND DON'T REMOVE TAG :
 *          "<< TABLE BEGIN"
 *          ">> TABLE END"
 */

#define	STATEVAR_DEFAULTCONNECTIONSERVICE   0

/* State Variable Table */
UPNP_STATE_VAR statevar_x_layer3forwarding[] =
{
	{0, "DefaultConnectionService", UPNP_TYPE_STR,  &statevar_DefaultConnectionService, 1},
	{0, 0,                          0,              0,                                  0}
};

/* Action Table */
static ACTION_ARGUMENT arg_in_SetDefaultConnectionService [] =
{
	{"NewDefaultConnectionService", UPNP_TYPE_STR,  STATEVAR_DEFAULTCONNECTIONSERVICE}
};

static ACTION_ARGUMENT arg_out_GetDefaultConnectionService [] =
{
	{"NewDefaultConnectionService", UPNP_TYPE_STR,  STATEVAR_DEFAULTCONNECTIONSERVICE}
};

UPNP_ACTION action_x_layer3forwarding[] =
{
	{"GetDefaultConnectionService", 0,  0,                                  1,  arg_out_GetDefaultConnectionService,    &action_GetDefaultConnectionService},
	{"SetDefaultConnectionService", 1,  arg_in_SetDefaultConnectionService, 0,  0,                                      &action_SetDefaultConnectionService},
	{0,                             0,  0,                                  0,  0,                                      0}
};
/* >> TABLE END */
