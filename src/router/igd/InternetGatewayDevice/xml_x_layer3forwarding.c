/*
 * Broadcom IGD module, xml_x_layer3forwarding.c
 *
 * Copyright (C) 2011, Broadcom Corporation
 * All Rights Reserved.
 * 
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 *
 * $Id: xml_x_layer3forwarding.c 241182 2011-02-17 21:50:03Z $
 */

char xml_x_layer3forwarding[]=
	"<?xml version=\"1.0\"?>\r\n"
	"<scpd xmlns=\"urn:schemas-upnp-org:service-1-0\">\r\n"
	"\t<specVersion>\r\n"
	"\t\t<major>1</major>\r\n"
	"\t\t<minor>0</minor>\r\n"
	"\t</specVersion>\r\n"
	"\t<actionList>\r\n"
	"\t\t<action>\r\n"
	"\t\t\t<name>SetDefaultConnectionService</name>\r\n"
	"\t\t\t\t<argumentList>\r\n"
	"\t\t\t\t\t<argument>\r\n"
	"\t\t\t\t\t\t<name>NewDefaultConnectionService</name>\r\n"
	"\t\t\t\t\t\t<direction>in</direction>\r\n"
	"\t\t\t\t\t\t<relatedStateVariable>DefaultConnectionService</relatedStateVariable>\r\n"
	"\t\t\t\t\t</argument>\r\n"
	"\t\t\t\t</argumentList>\r\n"
	"\t\t</action>\r\n"
	"\t\t<action>\r\n"
	"\t\t\t<name>GetDefaultConnectionService</name>\r\n"
	"\t\t\t\t<argumentList>\r\n"
	"\t\t\t\t\t<argument>\r\n"
	"\t\t\t\t\t\t<name>NewDefaultConnectionService</name>\r\n"
	"\t\t\t\t\t\t<direction>out</direction>\r\n"
	"\t\t\t\t\t\t<relatedStateVariable>DefaultConnectionService</relatedStateVariable>\r\n"
	"\t\t\t\t\t</argument>\r\n"
	"\t\t\t\t</argumentList>\r\n"
	"\t\t</action>\r\n"
	"\t</actionList>\r\n"
	"\t<serviceStateTable>\r\n"
	"\t\t<stateVariable sendEvents=\"yes\">\r\n"
	"\t\t\t<name>DefaultConnectionService</name>\r\n"
	"\t\t\t<dataType>string</dataType>\r\n"
	"\t\t</stateVariable>\r\n"
	"\t</serviceStateTable>\r\n"
	"</scpd>\r\n"
	"\r\n";
