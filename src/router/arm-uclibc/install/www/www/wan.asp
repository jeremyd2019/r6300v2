<!--
$Copyright Open Broadcom Corporation$

$Id: wan.asp,v 1.17 2011-01-11 18:43:43 willfeng Exp $
-->

<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN">
<html lang="en">
<head>
<title>Broadcom Home Gateway Reference Design: WAN</title>
<meta http-equiv="Content-Type" content="text/html; charset=utf-8">
<link rel="stylesheet" type="text/css" href="style.css" media="screen">
<script language="JavaScript" type="text/javascript" src="overlib.js"></script>
<script language="JavaScript" type="text/javascript">
<!--
/*
#ifdef BCMVISTAROUTER                 
*/  
var ap = "<% nvram_match("router_disable", "1", "1"); %>";  
var ipv6_hint = "Sets the IPv6 Network Prefix for WAN.";
function wan_ipv6_init()
{
	if (ap == "1") {
		ipv6_hint = "Currently IPv6 WAN Prefix is not effective. To Enable, Go to Basic Page and select the Router Mode."
		document.forms[0].wan_ipv6_prefix.disabled = 1;
	}
}
/*
#endif
*/
function wan_pppoe_demand_change()
{
	var wan_pppoe_demand = document.forms[0].wan_pppoe_demand[document.forms[0].wan_pppoe_demand.selectedIndex].value;

	if (wan_pppoe_demand == "1")
		document.forms[0].wan_pppoe_idletime.disabled = 0;
	else
		document.forms[0].wan_pppoe_idletime.disabled = 1;
}
/*
#ifdef __CONFIG_PPTP__                 
*/
function wan_pptp_demand_change()
{
	var wan_pptp_demand = document.forms[0].wan_pptp_demand[document.forms[0].wan_pptp_demand.selectedIndex].value;

	if (wan_pptp_demand == "1")
		document.forms[0].wan_pptp_idletime.disabled = 0;
	else
		document.forms[0].wan_pptp_idletime.disabled = 1;
}
function wan_pptp_static_change()
{
	var wan_pptp_static = document.forms[0].wan_pptp_static[document.forms[0].wan_pptp_static.selectedIndex].value;

	if (wan_pptp_static == "1") {
		document.forms[0].wan_pptp_ipaddr.disabled = 0;
		document.forms[0].wan_pptp_netmask.disabled = 0;
		document.forms[0].wan_pptp_gateway.disabled = 0;
		document.forms[0].wan_pptp_dns0.disabled = 0;
		document.forms[0].wan_pptp_dns1.disabled = 0;
		document.forms[0].wan_pptp_dns2.disabled = 0;
	} else {
		document.forms[0].wan_pptp_ipaddr.disabled = 1;
		document.forms[0].wan_pptp_netmask.disabled = 1;
		document.forms[0].wan_pptp_gateway.disabled = 1;
		document.forms[0].wan_pptp_dns0.disabled = 1;
		document.forms[0].wan_pptp_dns1.disabled = 1;
		document.forms[0].wan_pptp_dns2.disabled = 1;
	}
}
/*
#endif
*/
/*
#ifdef __CONFIG_L2TP__                 
*/
function wan_l2tp_demand_change()
{
	var wan_l2tp_demand = document.forms[0].wan_l2tp_demand[document.forms[0].wan_l2tp_demand.selectedIndex].value;

	if (wan_l2tp_demand == "1")
		document.forms[0].wan_l2tp_idletime.disabled = 0;
	else
		document.forms[0].wan_l2tp_idletime.disabled = 1;
}
function wan_l2tp_static_change()
{
	var wan_l2tp_static = document.forms[0].wan_l2tp_static[document.forms[0].wan_l2tp_static.selectedIndex].value;

	if (wan_l2tp_static == "1") {
		document.forms[0].wan_l2tp_ipaddr.disabled = 0;
		document.forms[0].wan_l2tp_netmask.disabled = 0;
		document.forms[0].wan_l2tp_gateway.disabled = 0;
		document.forms[0].wan_l2tp_dns0.disabled = 0;
		document.forms[0].wan_l2tp_dns1.disabled = 0;
		document.forms[0].wan_l2tp_dns2.disabled = 0;
	} else {
		document.forms[0].wan_l2tp_ipaddr.disabled = 1;
		document.forms[0].wan_l2tp_netmask.disabled = 1;
		document.forms[0].wan_l2tp_gateway.disabled = 1;
		document.forms[0].wan_l2tp_dns0.disabled = 1;
		document.forms[0].wan_l2tp_dns1.disabled = 1;
		document.forms[0].wan_l2tp_dns2.disabled = 1;
	}
}
/*
#endif
*/
function wan_proto_change()
{
	var wan_proto = document.forms[0].wan_proto[document.forms[0].wan_proto.selectedIndex].value;

	if (wan_proto == "pppoe") {
		document.forms[0].wan_pppoe_username.disabled = 0;
		document.forms[0].wan_pppoe_passwd.disabled = 0;
		document.forms[0].wan_pppoe_service.disabled = 0;
		document.forms[0].wan_pppoe_ac.disabled = 0;
		document.forms[0].wan_pppoe_keepalive.disabled = 0;
		document.forms[0].wan_pppoe_demand.disabled = 0;
		document.forms[0].wan_pppoe_idletime.disabled = 0;
		document.forms[0].wan_pppoe_mru.disabled = 0;
		document.forms[0].wan_pppoe_mtu.disabled = 0;

		wan_pppoe_demand_change();
	} else {
		document.forms[0].wan_pppoe_username.disabled = 1;
		document.forms[0].wan_pppoe_passwd.disabled = 1;
		document.forms[0].wan_pppoe_service.disabled = 1;
		document.forms[0].wan_pppoe_ac.disabled = 1;
		document.forms[0].wan_pppoe_keepalive.disabled = 1;
		document.forms[0].wan_pppoe_demand.disabled = 1;
		document.forms[0].wan_pppoe_idletime.disabled = 1;
		document.forms[0].wan_pppoe_mru.disabled = 1;
		document.forms[0].wan_pppoe_mtu.disabled = 1;
	}
/*
#ifdef __CONFIG_PPTP__                 
*/
	if (wan_proto == "pptp") {
		document.forms[0].wan_pptp_server_name.disabled = 0;
		document.forms[0].wan_pptp_static.disabled = 0;
		document.forms[0].wan_pptp_ipaddr.disabled = 0;
		document.forms[0].wan_pptp_netmask.disabled = 0;
		document.forms[0].wan_pptp_gateway.disabled = 0;
		document.forms[0].wan_pptp_dns0.disabled = 0;
		document.forms[0].wan_pptp_dns1.disabled = 0;
		document.forms[0].wan_pptp_dns2.disabled = 0;
		document.forms[0].wan_pptp_username.disabled = 0;
		document.forms[0].wan_pptp_passwd.disabled = 0;
		document.forms[0].wan_pptp_keepalive.disabled = 0;
		document.forms[0].wan_pptp_demand.disabled = 0;
		document.forms[0].wan_pptp_idletime.disabled = 0;
		document.forms[0].wan_pptp_mru.disabled = 0;
		document.forms[0].wan_pptp_mtu.disabled = 0;

		wan_pptp_demand_change();
		wan_pptp_static_change();
	} else {
		document.forms[0].wan_pptp_server_name.disabled = 1;
		document.forms[0].wan_pptp_static.disabled = 1;
		document.forms[0].wan_pptp_ipaddr.disabled = 1;
		document.forms[0].wan_pptp_netmask.disabled = 1;
		document.forms[0].wan_pptp_gateway.disabled = 1;
		document.forms[0].wan_pptp_dns0.disabled = 1;
		document.forms[0].wan_pptp_dns1.disabled = 1;
		document.forms[0].wan_pptp_dns2.disabled = 1;
		document.forms[0].wan_pptp_username.disabled = 1;
		document.forms[0].wan_pptp_passwd.disabled = 1;
		document.forms[0].wan_pptp_keepalive.disabled = 1;
		document.forms[0].wan_pptp_demand.disabled = 1;
		document.forms[0].wan_pptp_idletime.disabled = 1;
		document.forms[0].wan_pptp_mru.disabled = 1;
		document.forms[0].wan_pptp_mtu.disabled = 1;
	}
/*
#endif
*/
/*
#ifdef __CONFIG_L2TP__                 
*/
	if (wan_proto == "l2tp") {
		document.forms[0].wan_l2tp_server_name.disabled = 0;
		document.forms[0].wan_l2tp_static.disabled = 0;
		document.forms[0].wan_l2tp_ipaddr.disabled = 0;
		document.forms[0].wan_l2tp_netmask.disabled = 0;
		document.forms[0].wan_l2tp_gateway.disabled = 0;
		document.forms[0].wan_l2tp_dns0.disabled = 0;
		document.forms[0].wan_l2tp_dns1.disabled = 0;
		document.forms[0].wan_l2tp_dns2.disabled = 0;
		document.forms[0].wan_l2tp_username.disabled = 0;
		document.forms[0].wan_l2tp_passwd.disabled = 0;
		document.forms[0].wan_l2tp_keepalive.disabled = 0;
		document.forms[0].wan_l2tp_demand.disabled = 0;
		document.forms[0].wan_l2tp_idletime.disabled = 0;
		document.forms[0].wan_l2tp_mru.disabled = 0;
		document.forms[0].wan_l2tp_mtu.disabled = 0;

		wan_l2tp_demand_change();
		wan_l2tp_static_change();
	} else {
		document.forms[0].wan_l2tp_server_name.disabled = 1;
		document.forms[0].wan_l2tp_static.disabled = 1;
		document.forms[0].wan_l2tp_ipaddr.disabled = 1;
		document.forms[0].wan_l2tp_netmask.disabled = 1;
		document.forms[0].wan_l2tp_gateway.disabled = 1;
		document.forms[0].wan_l2tp_dns0.disabled = 1;
		document.forms[0].wan_l2tp_dns1.disabled = 1;
		document.forms[0].wan_l2tp_dns2.disabled = 1;
		document.forms[0].wan_l2tp_username.disabled = 1;
		document.forms[0].wan_l2tp_passwd.disabled = 1;
		document.forms[0].wan_l2tp_keepalive.disabled = 1;
		document.forms[0].wan_l2tp_demand.disabled = 1;
		document.forms[0].wan_l2tp_idletime.disabled = 1;
		document.forms[0].wan_l2tp_mru.disabled = 1;
		document.forms[0].wan_l2tp_mtu.disabled = 1;
	}
/*
#endif
*/
	if (wan_proto != "static" && wan_proto != "disabled") {
		document.forms[0].wan_ipaddr.disabled = 1;
		document.forms[0].wan_domain.disabled = 1;
		document.forms[0].wan_netmask.disabled = 1;
		document.forms[0].wan_gateway.disabled = 1;
		document.forms[0].wan_dns0.disabled = 1;
		document.forms[0].wan_dns1.disabled = 1;
		document.forms[0].wan_dns2.disabled = 1;
		document.forms[0].wan_wins0.disabled = 1;
		document.forms[0].wan_wins1.disabled = 1;
		document.forms[0].wan_wins2.disabled = 1;
	} else {
		document.forms[0].wan_ipaddr.disabled = 0;
		document.forms[0].wan_domain.disabled = 0;
		document.forms[0].wan_netmask.disabled = 0;
		document.forms[0].wan_gateway.disabled = 0;
		document.forms[0].wan_dns0.disabled = 0;
		document.forms[0].wan_dns1.disabled = 0;
		document.forms[0].wan_dns2.disabled = 0;
		document.forms[0].wan_wins0.disabled = 0;
		document.forms[0].wan_wins1.disabled = 0;
		document.forms[0].wan_wins2.disabled = 0;
	}
/*
#ifdef BCMVISTAROUTER
*/ 
	wan_ipv6_init();
/*
#endif
*/
}
function wan_ifname_change()
{
	var wan_ifname = document.forms[0].wan_ifname[document.forms[0].wan_ifname.selectedIndex];
	/* text attribute format: ifname (XX:XX:XX:XX:XX:XX) */
	document.forms[0].wan_hwaddr.value = wan_ifname.text.split(/\(/)[1].split(/\)/)[0];
}
//-->
</script>
</head>

<body onLoad="wan_proto_change();">
<div id="overDiv" style="position:absolute; visibility:hidden; z-index:1000;"></div>

<table border="0" cellpadding="0" cellspacing="0" width="100%" bgcolor="#cc0000">
  <% asp_list(); %>
</table>

<table border="0" cellpadding="0" cellspacing="0" width="100%">
  <tr>
    <td colspan="2" class="edge"><img border="0" src="blur_new.jpg" alt=""></td>
  </tr>
  <tr>
    <td><img border="0" src="logo_new.gif" alt=""></td>
    <td width="100%" valign="top">
	<br>
	<span class="title">WAN</span><br>
	<span class="subtitle">This page allows you to configure the
	WAN connections of the router.</span>
    </td>
  </tr>
</table>

<form method="post" action="wan.asp">
<input type="hidden" name="page" value="wan.asp">

<p>
<table border="0" cellpadding="0" cellspacing="0">
  <tr>
    <th width="310"
	onMouseOver="return overlib('Selects which WAN connection to configure.', LEFT);"
	onMouseOut="return nd();">
	Connection:&nbsp;&nbsp;
    </th>
    <td>&nbsp;&nbsp;</td>
    <td>
	<select name="wan_unit" onChange="submit();">
	  <% wan_list(); %>
	</select>
    </td>
    <td>
	<button type="submit" name="action" value="Select">Select</button>
    </td>
  </tr>    
</table>

<p>
<table border="0" cellpadding="0" cellspacing="0">
  <tr>
    <td width="310"></td>
    <td>&nbsp;&nbsp;</td>
    <td>
	<input type="submit" name="action" value="New">
	<input type="submit" name="action" value="Delete">
    </td>
  </tr>
</table>

<p>
<table border="0" cellpadding="0" cellspacing="0">
  <tr>
    <th width="310"
	onMouseOver="return overlib('Sets the connection name.', LEFT);"
	onMouseOut="return nd();">
	Description:&nbsp;&nbsp;
    </th>
    <td>&nbsp;&nbsp;</td>
    <td><input name="wan_desc" value="<% nvram_get("wan_desc"); %>"></td>
  </tr>
  <tr>
    <th width="310"
	onMouseOver="return overlib('Selects the interface used by the connection.', LEFT);"
	onMouseOut="return nd();">
	Interface:&nbsp;&nbsp;
    </th>
    <td>&nbsp;&nbsp;</td>
    <td>
	<select name="wan_ifname" onChange="wan_ifname_change();">
	  <% wan_iflist(); %>
	</select>
    </td>
  </tr>    
  <tr>
    <th width="310"
	onMouseOver="return overlib('Sets the method to use to obtain an IP address for the connection.', LEFT);"
	onMouseOut="return nd();">
	Protocol:&nbsp;&nbsp;
    </th>
    <td>&nbsp;&nbsp;</td>
    <td>
	<select name="wan_proto" onChange="wan_proto_change();">
	  <option value="dhcp" <% nvram_match("wan_proto", "dhcp", "selected"); %>>DHCP</option>
	  <option value="static" <% nvram_match("wan_proto", "static", "selected"); %>>Static</option>
	  <option value="pppoe" <% nvram_match("wan_proto", "pppoe", "selected"); %>>PPPoE</option>
<!--
#ifdef __CONFIG_PPTP__
-->
	  <option value="pptp" <% nvram_match("wan_proto", "pptp", "selected"); %>>PPTP</option>
<!--
#endif
-->
<!--
#ifdef __CONFIG_L2TP__
-->
	  <option value="l2tp" <% nvram_match("wan_proto", "l2tp", "selected"); %>>L2TP</option>
<!--
#endif
-->
	  <option value="disabled" <% nvram_match("wan_proto", "disabled", "selected"); %>>Disabled</option>
	</select>
    </td>
  </tr>
  <tr>
    <th width="310"
	onMouseOver="return overlib('Sets the connection to be primary.', LEFT);"
	onMouseOut="return nd();">
	Primary:&nbsp;&nbsp;
    </th>
    <td>&nbsp;&nbsp;</td>
    <td>
	<select name="wan_primary">
	  <option value="1" <% nvram_match("wan_primary", "1", "selected"); %>>Yes</option>
	  <option value="0" <% nvram_match("wan_primary", "0", "selected"); %>>No</option>
	</select>
    </td>
  </tr>
</table>

<p>
<table border="0" cellpadding="0" cellspacing="0">
  <tr>
    <th width="310"
	onMouseOver="return overlib('Some ISPs require that a host name be provided when requesting an IP address through DHCP.', LEFT);"
	onMouseOut="return nd();">
	Host Name:&nbsp;&nbsp;
    </th>
    <td>&nbsp;&nbsp;</td>
    <td><input name="wan_hostname" value="<% nvram_get("wan_hostname"); %>"></td>
  </tr>
  <tr>
    <th width="310"
	onMouseOver="return overlib('Sets the domain name to be provided to LAN clients who request an IP address through DHCP.', LEFT);"
	onMouseOut="return nd();">
	Domain Name:&nbsp;&nbsp;
    </th>
    <td>&nbsp;&nbsp;</td>
    <td><input name="wan_domain" value="<% nvram_get("wan_domain"); %>"></td>
  </tr>
  <tr>
    <th width="310"
	onMouseOver="return overlib('Some ISPs require that a specific MAC address be used. Also known as MAC address cloning, this feature allows you to set the MAC address of the WAN interface. The MAC address format is XX:XX:XX:XX:XX:XX.', LEFT);"
	onMouseOut="return nd();">
	MAC Address:&nbsp;&nbsp;
    </th>
    <td>&nbsp;&nbsp;</td>
    <td><input name="wan_hwaddr" value="<% nvram_get("wan_hwaddr"); %>" size="17" maxlength="17"></td>
  </tr>
  <tr>
    <th width="310"
	onMouseOver="return overlib('Sets the IP address of the connection.', LEFT);"
	onMouseOut="return nd();">
	IP Address:&nbsp;&nbsp;
    </th>
    <td>&nbsp;&nbsp;</td>
    <td><input name="wan_ipaddr" value="<% nvram_get("wan_ipaddr"); %>" size="15" maxlength="15"></td>
  </tr>
  <tr>
    <th width="310"
	onMouseOver="return overlib('Sets the IP netmask of the connection.', LEFT);"
	onMouseOut="return nd();">
	Subnet Mask:&nbsp;&nbsp;
    </th>
    <td>&nbsp;&nbsp;</td>
    <td><input name="wan_netmask" value="<% nvram_get("wan_netmask"); %>" size="15" maxlength="15"></td>
  </tr>
  <tr>
    <th width="310"
	onMouseOver="return overlib('Sets the IP address of the default gateway to use on the connection.', LEFT);"
	onMouseOut="return nd();">
	Default Gateway:&nbsp;&nbsp;
    </th>
    <td>&nbsp;&nbsp;</td>
    <td><input name="wan_gateway" value="<% nvram_get("wan_gateway"); %>" size="15" maxlength="15"></td>
  </tr>
  <tr>
    <th width="310" valign="top" rowspan="3"
	onMouseOver="return overlib('Sets the IP addresses of the DNS servers to use for resolving host names.', LEFT);"
	onMouseOut="return nd();">
	<input type="hidden" name="wan_dns" value="3">
	DNS Servers:&nbsp;&nbsp;
    </th>
    <td>&nbsp;&nbsp;</td>
    <td><input name="wan_dns0" value="<% nvram_list("wan_dns", 0); %>" size="15" maxlength="15"></td>
  </tr>
  <tr>
    <td>&nbsp;&nbsp;</td>
    <td><input name="wan_dns1" value="<% nvram_list("wan_dns", 1); %>" size="15" maxlength="15"></td>
  </tr>
  <tr>
    <td>&nbsp;&nbsp;</td>
    <td><input name="wan_dns2" value="<% nvram_list("wan_dns", 2); %>" size="15" maxlength="15"></td>
  </tr>
  <tr>
    <th width="310" valign="top" rowspan="3"
	onMouseOver="return overlib('Sets the IP addresses of the WINS servers to use for resolving NetBIOS names.', LEFT);"
	onMouseOut="return nd();">
	<input type="hidden" name="wan_wins" value="3">
	WINS Servers:&nbsp;&nbsp;
    </th>
    <td>&nbsp;&nbsp;</td>
    <td><input name="wan_wins0" value="<% nvram_list("wan_wins", 0); %>" size="15" maxlength="15"></td>
  </tr>
  <tr>
    <td>&nbsp;&nbsp;</td>
    <td><input name="wan_wins1" value="<% nvram_list("wan_wins", 1); %>" size="15" maxlength="15"></td>
  </tr>
  <tr>
    <td>&nbsp;&nbsp;</td>
    <td><input name="wan_wins2" value="<% nvram_list("wan_wins", 2); %>" size="15" maxlength="15"></td>
  </tr>
</table>
<p>
<table border="0" cellpadding="0" cellspacing="0">
  <tr>
    <th width="310"
	onMouseOver="return overlib('Sets the username to use when authenticating with a PPPoE server.', LEFT);"
	onMouseOut="return nd();">
	PPPoE Username:&nbsp;&nbsp;
    </th>
    <td>&nbsp;&nbsp;</td>
    <td><input name="wan_pppoe_username" value="<% nvram_get("wan_pppoe_username"); %>"></td>
  </tr>
  <tr>
    <th width="310"
	onMouseOver="return overlib('Sets the password to use when authenticating with a PPPoE server.', LEFT);"
	onMouseOut="return nd();">
	PPPoE Password:&nbsp;&nbsp;
    </th>
    <td>&nbsp;&nbsp;</td>
    <td><input name="wan_pppoe_passwd" value="<% nvram_get("wan_pppoe_passwd"); %>" type="password"></td>
  </tr>
  <tr>
    <th width="310"
	onMouseOver="return overlib('Sets the PPPoE service name. Required by some ISPs.', LEFT);"
	onMouseOut="return nd();">
	PPPoE Service Name:&nbsp;&nbsp;
    </th>
    <td>&nbsp;&nbsp;</td>
    <td><input name="wan_pppoe_service" value="<% nvram_get("wan_pppoe_service"); %>"></td>
  </tr>
  <tr>
    <th width="310"
	onMouseOver="return overlib('Sets the name of the PPPoE access concentrator. Required by some ISPs.', LEFT);"
	onMouseOut="return nd();">
	PPPoE Access Concentrator:&nbsp;&nbsp;
    </th>
    <td>&nbsp;&nbsp;</td>
    <td><input name="wan_pppoe_ac" value="<% nvram_get("wan_pppoe_ac"); %>"></td>
  </tr>
  <tr>
    <th width="310"
	onMouseOver="return overlib('Sets whether the PPPoE link should be automatically disconnected if no traffic has been observed for the period specified by <b>PPPoE Max Idle Time</b>.', LEFT);"
	onMouseOut="return nd();">
	PPPoE Connect on Demand:&nbsp;&nbsp;
    </th>
    <td>&nbsp;&nbsp;</td>
    <td>
	<select name="wan_pppoe_demand" onChange="wan_pppoe_demand_change();">
	  <option value="1" <% nvram_match("wan_pppoe_demand", "1", "selected"); %>>Enabled</option>
	  <option value="0" <% nvram_match("wan_pppoe_demand", "0", "selected"); %>>Disabled</option>
	</select>
    </td>
  </tr>
  <tr>
    <th width="310"
	onMouseOver="return overlib('Sets the number of seconds to wait before disconnecting the PPPoE link if <b>PPPoE Connect on Demand</b> is <b>Enabled</b>.', LEFT);"
	onMouseOut="return nd();">
	PPPoE Max Idle Time:&nbsp;&nbsp;
    </th>
    <td>&nbsp;&nbsp;</td>
    <td><input name="wan_pppoe_idletime" value="<% nvram_get("wan_pppoe_idletime"); %>" size="4" maxlength="4"></td>
  </tr>
  <tr>
    <th width="310"
	onMouseOver="return overlib('Sets whether the PPPoE link should be automatically restored if it is lost. This setting has no effect if <b>PPPoE Connect on Demand</b> is <b>Enabled</b>.', LEFT);"
	onMouseOut="return nd();">
	PPPoE Keep Alive:&nbsp;&nbsp;
    </th>
    <td>&nbsp;&nbsp;</td>
    <td>
	<select name="wan_pppoe_keepalive">
	  <option value="1" <% nvram_match("wan_pppoe_keepalive", "1", "selected"); %>>Enabled</option>
	  <option value="0" <% nvram_match("wan_pppoe_keepalive", "0", "selected"); %>>Disabled</option>
	</select>
    </td>
  </tr>
  <tr>
    <th width="310"
	onMouseOver="return overlib('Sets the maximum number of bytes that the PPPoE interface will receive in a single Ethernet frame.', LEFT);"
	onMouseOut="return nd();">
	PPPoE MRU:&nbsp;&nbsp;
    </th>
    <td>&nbsp;&nbsp;</td>
    <td><input name="wan_pppoe_mru" value="<% nvram_get("wan_pppoe_mru"); %>" size="4" maxlength="4"></td>
  </tr>
  <tr>
    <th width="310"
	onMouseOver="return overlib('Sets the maximum number of bytes that the PPPoE interface will transmit in a single Ethernet frame.', LEFT);"
	onMouseOut="return nd();">
	PPPoE MTU:&nbsp;&nbsp;
    </th>
    <td>&nbsp;&nbsp;</td>
    <td><input name="wan_pppoe_mtu" value="<% nvram_get("wan_pppoe_mtu"); %>" size="4" maxlength="4"></td>
  </tr>
</table>
<!--
#ifdef __CONFIG_PPTP__
-->
<p>
<table border="0" cellpadding="0" cellspacing="0">
  <tr>
    <th width="310"
	onMouseOver="return overlib('Sets host name or IP address of destination.', LEFT);"
	onMouseOut="return nd();">
	PPTP Server:&nbsp;&nbsp;
    </th>
    <td>&nbsp;&nbsp;</td>
    <td><input name="wan_pptp_server_name" value="<% nvram_get("wan_pptp_server_name"); %>"></td>
  </tr>
  <tr>
    <th width="310"
	onMouseOver="return overlib('Sets the method to use to obtain an IP address for the PPTP connection.', LEFT);"
	onMouseOut="return nd();">
	Protocol of Physical Connection:&nbsp;&nbsp;
    </th>
    <td>&nbsp;&nbsp;</td>
    <td>
	<select name="wan_pptp_static" onChange="wan_pptp_static_change();">
	  <option value="0" <% nvram_match("wan_pptp_static", "0", "selected"); %>>DHCP</option>
	  <option value="1" <% nvram_match("wan_pptp_static", "1", "selected"); %>>Static</option>
	</select>
    </td>
  </tr>
  <tr>
    <th width="310"
	onMouseOver="return overlib('Sets the IP address of the PPTP connection.', LEFT);"
	onMouseOut="return nd();">
	IP Address:&nbsp;&nbsp;
    </th>
    <td>&nbsp;&nbsp;</td>
    <td><input name="wan_pptp_ipaddr" value="<% nvram_get("wan_pptp_ipaddr"); %>" size="15" maxlength="15"></td>
  </tr>
  <tr>
    <th width="310"
	onMouseOver="return overlib('Sets the IP netmask of the PPTP connection.', LEFT);"
	onMouseOut="return nd();">
	Subnet Mask:&nbsp;&nbsp;
    </th>
    <td>&nbsp;&nbsp;</td>
    <td><input name="wan_pptp_netmask" value="<% nvram_get("wan_pptp_netmask"); %>" size="15" maxlength="15"></td>
  </tr>
  <tr>
    <th width="310"
	onMouseOver="return overlib('Sets the IP address of the default gateway to use on the PPTP connection.', LEFT);"
	onMouseOut="return nd();">
	Default Gateway:&nbsp;&nbsp;
    </th>
    <td>&nbsp;&nbsp;</td>
    <td><input name="wan_pptp_gateway" value="<% nvram_get("wan_pptp_gateway"); %>" size="15" maxlength="15"></td>
  </tr>
  <tr>
    <th width="310" valign="top" rowspan="3"
	onMouseOver="return overlib('Sets the IP addresses of the DNS servers to use for resolving host names.', LEFT);"
	onMouseOut="return nd();">
	<input type="hidden" name="wan_pptp_dns" value="3">
	DNS Servers:&nbsp;&nbsp;
    </th>
    <td>&nbsp;&nbsp;</td>
    <td><input name="wan_pptp_dns0" value="<% nvram_list("wan_pptp_dns", 0); %>" size="15" maxlength="15"></td>
  </tr>
  <tr>
    <td>&nbsp;&nbsp;</td>
    <td><input name="wan_pptp_dns1" value="<% nvram_list("wan_pptp_dns", 1); %>" size="15" maxlength="15"></td>
  </tr>
  <tr>
    <td>&nbsp;&nbsp;</td>
    <td><input name="wan_pptp_dns2" value="<% nvram_list("wan_pptp_dns", 2); %>" size="15" maxlength="15"></td>
  </tr> 
  <tr>
    <th width="310"
	onMouseOver="return overlib('Sets the username to use when authenticating with a PPTP server.', LEFT);"
	onMouseOut="return nd();">
	PPTP Username:&nbsp;&nbsp;
    </th>
    <td>&nbsp;&nbsp;</td>
    <td><input name="wan_pptp_username" value="<% nvram_get("wan_pptp_username"); %>"></td>
  </tr>
  <tr>
    <th width="310"
	onMouseOver="return overlib('Sets the password to use when authenticating with a PPTP server.', LEFT);"
	onMouseOut="return nd();">
	PPTP Password:&nbsp;&nbsp;
    </th>
    <td>&nbsp;&nbsp;</td>
    <td><input name="wan_pptp_passwd" value="<% nvram_get("wan_pptp_passwd"); %>" type="password"></td>
  </tr>
  <tr>
    <th width="310"
	onMouseOver="return overlib('Sets whether the PPTP link should be automatically disconnected if no traffic has been observed for the period specified by <b>PPPoE Max Idle Time</b>.', LEFT);"
	onMouseOut="return nd();">
	PPTP Connect on Demand:&nbsp;&nbsp;
    </th>
    <td>&nbsp;&nbsp;</td>
    <td>
	<select name="wan_pptp_demand" onChange="wan_pptp_demand_change();">
	  <option value="1" <% nvram_match("wan_pptp_demand", "1", "selected"); %>>Enabled</option>
	  <option value="0" <% nvram_match("wan_pptp_demand", "0", "selected"); %>>Disabled</option>
	</select>
    </td>
  </tr>
  <tr>
    <th width="310"
	onMouseOver="return overlib('Sets the number of seconds to wait before disconnecting the PPTP link if <b>PPTP Connect on Demand</b> is <b>Enabled</b>.', LEFT);"
	onMouseOut="return nd();">
	PPTP Max Idle Time:&nbsp;&nbsp;
    </th>
    <td>&nbsp;&nbsp;</td>
    <td><input name="wan_pptp_idletime" value="<% nvram_get("wan_pptp_idletime"); %>" size="4" maxlength="4"></td>
  </tr>
  <tr>
    <th width="310"
	onMouseOver="return overlib('Sets whether the PPTP link should be automatically restored if it is lost. This setting has no effect if <b>PPTP Connect on Demand</b> is <b>Enabled</b>.', LEFT);"
	onMouseOut="return nd();">
	PPTP Keep Alive:&nbsp;&nbsp;
    </th>
    <td>&nbsp;&nbsp;</td>
    <td>
	<select name="wan_pptp_keepalive">
	  <option value="1" <% nvram_match("wan_pptp_keepalive", "1", "selected"); %>>Enabled</option>
	  <option value="0" <% nvram_match("wan_pptp_keepalive", "0", "selected"); %>>Disabled</option>
	</select>
    </td>
  </tr>
  <tr>
    <th width="310"
	onMouseOver="return overlib('Sets the maximum number of bytes that the PPTP interface will receive in a single Ethernet frame.', LEFT);"
	onMouseOut="return nd();">
	PPTP MRU:&nbsp;&nbsp;
    </th>
    <td>&nbsp;&nbsp;</td>
    <td><input name="wan_pptp_mru" value="<% nvram_get("wan_pptp_mru"); %>" size="4" maxlength="4"></td>
  </tr>
  <tr>
    <th width="310"
	onMouseOver="return overlib('Sets the maximum number of bytes that the PPTP interface will transmit in a single Ethernet frame.', LEFT);"
	onMouseOut="return nd();">
	PPTP MTU:&nbsp;&nbsp;
    </th>
    <td>&nbsp;&nbsp;</td>
    <td><input name="wan_pptp_mtu" value="<% nvram_get("wan_pptp_mtu"); %>" size="4" maxlength="4"></td>
  </tr>
</table>
<p>
<!--
#endif
-->
<!--
#ifdef __CONFIG_L2TP__
-->
<table border="0" cellpadding="0" cellspacing="0">
  <tr>
    <th width="310"
	onMouseOver="return overlib('Sets host name or IP address of destination.', LEFT);"
	onMouseOut="return nd();">
	L2TP Server:&nbsp;&nbsp;
    </th>
    <td>&nbsp;&nbsp;</td>
    <td><input name="wan_l2tp_server_name" value="<% nvram_get("wan_l2tp_server_name"); %>"></td>
  </tr>
  <tr>
    <th width="310"
	onMouseOver="return overlib('Sets the method to use to obtain an IP address for the L2TP connection.', LEFT);"
	onMouseOut="return nd();">
	Protocol of Physical Connection:&nbsp;&nbsp;
    </th>
    <td>&nbsp;&nbsp;</td>
    <td>
	<select name="wan_l2tp_static" onChange="wan_l2tp_static_change();">
	  <option value="0" <% nvram_match("wan_l2tp_static", "0", "selected"); %>>DHCP</option>
	  <option value="1" <% nvram_match("wan_l2tp_static", "1", "selected"); %>>Static</option>
	</select>
    </td>
  </tr>
  <tr>
    <th width="310"
	onMouseOver="return overlib('Sets the IP address of the L2TP connection.', LEFT);"
	onMouseOut="return nd();">
	IP Address:&nbsp;&nbsp;
    </th>
    <td>&nbsp;&nbsp;</td>
    <td><input name="wan_l2tp_ipaddr" value="<% nvram_get("wan_l2tp_ipaddr"); %>" size="15" maxlength="15"></td>
  </tr>
  <tr>
    <th width="310"
	onMouseOver="return overlib('Sets the IP netmask of the L2TP connection.', LEFT);"
	onMouseOut="return nd();">
	Subnet Mask:&nbsp;&nbsp;
    </th>
    <td>&nbsp;&nbsp;</td>
    <td><input name="wan_l2tp_netmask" value="<% nvram_get("wan_l2tp_netmask"); %>" size="15" maxlength="15"></td>
  </tr>
  <tr>
    <th width="310"
	onMouseOver="return overlib('Sets the IP address of the default gateway to use on the L2TP connection.', LEFT);"
	onMouseOut="return nd();">
	Default Gateway:&nbsp;&nbsp;
    </th>
    <td>&nbsp;&nbsp;</td>
    <td><input name="wan_l2tp_gateway" value="<% nvram_get("wan_l2tp_gateway"); %>" size="15" maxlength="15"></td>
  </tr>
  <tr>
    <th width="310" valign="top" rowspan="3"
	onMouseOver="return overlib('Sets the IP addresses of the DNS servers to use for resolving host names.', LEFT);"
	onMouseOut="return nd();">
	<input type="hidden" name="wan_l2tp_dns" value="3">
	DNS Servers:&nbsp;&nbsp;
    </th>
    <td>&nbsp;&nbsp;</td>
    <td><input name="wan_l2tp_dns0" value="<% nvram_list("wan_l2tp_dns", 0); %>" size="15" maxlength="15"></td>
  </tr>
  <tr>
    <td>&nbsp;&nbsp;</td>
    <td><input name="wan_l2tp_dns1" value="<% nvram_list("wan_l2tp_dns", 1); %>" size="15" maxlength="15"></td>
  </tr>
  <tr>
    <td>&nbsp;&nbsp;</td>
    <td><input name="wan_l2tp_dns2" value="<% nvram_list("wan_pptp_dns", 2); %>" size="15" maxlength="15"></td>
  </tr> 
  <tr>
    <th width="310"
	onMouseOver="return overlib('Sets the username to use when authenticating with a L2TP server.', LEFT);"
	onMouseOut="return nd();">
	L2TP Username:&nbsp;&nbsp;
    </th>
    <td>&nbsp;&nbsp;</td>
    <td><input name="wan_l2tp_username" value="<% nvram_get("wan_l2tp_username"); %>"></td>
  </tr>
  <tr>
    <th width="310"
	onMouseOver="return overlib('Sets the password to use when authenticating with a L2TP server.', LEFT);"
	onMouseOut="return nd();">
	L2TP Password:&nbsp;&nbsp;
    </th>
    <td>&nbsp;&nbsp;</td>
    <td><input name="wan_l2tp_passwd" value="<% nvram_get("wan_l2tp_passwd"); %>" type="password"></td>
  </tr>
  <tr>
    <th width="310"
	onMouseOver="return overlib('Sets whether the L2TP link should be automatically disconnected if no traffic has been observed for the period specified by <b>PPPoE Max Idle Time</b>.', LEFT);"
	onMouseOut="return nd();">
	L2TP Connect on Demand:&nbsp;&nbsp;
    </th>
    <td>&nbsp;&nbsp;</td>
    <td>
	<select name="wan_l2tp_demand" onChange="wan_l2tp_demand_change();">
	  <option value="1" <% nvram_match("wan_l2tp_demand", "1", "selected"); %>>Enabled</option>
	  <option value="0" <% nvram_match("wan_l2tp_demand", "0", "selected"); %>>Disabled</option>
	</select>
    </td>
  </tr>
  <tr>
    <th width="310"
	onMouseOver="return overlib('Sets the number of seconds to wait before disconnecting the PPTP link if <b>L2TP Connect on Demand</b> is <b>Enabled</b>.', LEFT);"
	onMouseOut="return nd();">
	L2TP Max Idle Time:&nbsp;&nbsp;
    </th>
    <td>&nbsp;&nbsp;</td>
    <td><input name="wan_l2tp_idletime" value="<% nvram_get("wan_l2tp_idletime"); %>" size="4" maxlength="4"></td>
  </tr>
  <tr>
    <th width="310"
	onMouseOver="return overlib('Sets whether the L2TP link should be automatically restored if it is lost. This setting has no effect if <b>PPTP Connect on Demand</b> is <b>Enabled</b>.', LEFT);"
	onMouseOut="return nd();">
	L2TP Keep Alive:&nbsp;&nbsp;
    </th>
    <td>&nbsp;&nbsp;</td>
    <td>
	<select name="wan_l2tp_keepalive">
	  <option value="1" <% nvram_match("wan_l2tp_keepalive", "1", "selected"); %>>Enabled</option>
	  <option value="0" <% nvram_match("wan_l2tp_keepalive", "0", "selected"); %>>Disabled</option>
	</select>
    </td>
  </tr>
  <tr>
    <th width="310"
	onMouseOver="return overlib('Sets the maximum number of bytes that the L2TP interface will receive in a single Ethernet frame.', LEFT);"
	onMouseOut="return nd();">
	L2TP MRU:&nbsp;&nbsp;
    </th>
    <td>&nbsp;&nbsp;</td>
    <td><input name="wan_l2tp_mru" value="<% nvram_get("wan_l2tp_mru"); %>" size="4" maxlength="4"></td>
  </tr>
  <tr>
    <th width="310"
	onMouseOver="return overlib('Sets the maximum number of bytes that the L2TP interface will transmit in a single Ethernet frame.', LEFT);"
	onMouseOut="return nd();">
	L2TP MTU:&nbsp;&nbsp;
    </th>
    <td>&nbsp;&nbsp;</td>
    <td><input name="wan_l2tp_mtu" value="<% nvram_get("wan_l2tp_mtu"); %>" size="4" maxlength="4"></td>
  </tr>
</table>
<!--
#endif
-->
<!--
#ifdef BCMVISTAROUTER
-->	
<p>
<table border="0" cellpadding="0" cellspacing="0">
  <tr>
    <th width="310"
	onMouseOver="return overlib(ipv6_hint, LEFT);"
	onMouseOut="return nd();">
	IPv6 WAN Network Prefix:&nbsp;&nbsp;
    </th>
    <td>&nbsp;&nbsp;</td>
    <td><input name="wan_ipv6_prefix" value="<% nvram_get("wan_ipv6_prefix"); %>" size="15" maxlength="43"></td>
  </tr>
</table>
<!--
#endif  /* BCMVISTAROUTER */
-->	

<p>
<table border="0" cellpadding="0" cellspacing="0">
  <tr>
    <th width="310"
	onMouseOver="return overlib('Shows the state of the connection.', LEFT);"
	onMouseOut="return nd();">
	Connection Status:&nbsp;&nbsp;
    </th>
    <td>&nbsp;&nbsp;</td>
    <td><% wan_link(); %></td>
  </tr>
  <tr>
    <th width="310"
	onMouseOver="return overlib('Shows the IP address lease info.', LEFT);"
	onMouseOut="return nd();">
	IP Address Expires In:&nbsp;&nbsp;
    </th>
    <td>&nbsp;&nbsp;</td>
	<td><% wan_lease(); %></td>
  </tr>
</table>

<p>
<table border="0" cellpadding="0" cellspacing="0">
  <tr>
    <th width="310" valign="top" rowspan="6"
	onMouseOver="return overlib('Set up static routes to the given networks.', LEFT);"
	onMouseOut="return nd();">	
	<input type="hidden" name="wan_route" value="5">
	Static Routes:&nbsp;&nbsp;
    </th>
    <td>&nbsp;&nbsp;</td>
    <td class="label">IP Address</td>
    <td></td>
    <td class="label">Subnet Mask</td>
    <td></td>
    <td class="label">Gateway</td>
    <td></td>
    <td class="label">Metric</td>
  </tr>
  <tr>
    <td>&nbsp;&nbsp;</td>
    <td><input name="wan_route_ipaddr0" value="<% wan_route("ipaddr", 0); %>" size="15" maxlength="15"></td>
    <td>&nbsp;</td>
    <td><input name="wan_route_netmask0" value="<% wan_route("netmask", 0); %>" size="15" maxlength="15"></td>
    <td>&nbsp;</td>
    <td><input name="wan_route_gateway0" value="<% wan_route("gateway", 0); %>" size="15" maxlength="15"></td>
    <td>&nbsp;</td>
    <td><input name="wan_route_metric0" value="<% wan_route("metric", 0); %>" size="2" maxlength="2"></td>
  </tr>
  <tr>
    <td>&nbsp;&nbsp;</td>
    <td><input name="wan_route_ipaddr1" value="<% wan_route("ipaddr", 1); %>" size="15" maxlength="15"></td>
    <td>&nbsp;</td>
    <td><input name="wan_route_netmask1" value="<% wan_route("netmask", 1); %>" size="15" maxlength="15"></td>
    <td>&nbsp;</td>
    <td><input name="wan_route_gateway1" value="<% wan_route("gateway", 1); %>" size="15" maxlength="15"></td>
    <td>&nbsp;</td>
    <td><input name="wan_route_metric1" value="<% wan_route("metric", 1); %>" size="2" maxlength="2"></td>
  </tr>
  <tr>
    <td>&nbsp;&nbsp;</td>
    <td><input name="wan_route_ipaddr2" value="<% wan_route("ipaddr", 2); %>" size="15" maxlength="15"></td>
    <td>&nbsp;</td>
    <td><input name="wan_route_netmask2" value="<% wan_route("netmask", 2); %>" size="15" maxlength="15"></td>
    <td>&nbsp;</td>
    <td><input name="wan_route_gateway2" value="<% wan_route("gateway", 2); %>" size="15" maxlength="15"></td>
    <td>&nbsp;</td>
    <td><input name="wan_route_metric2" value="<% wan_route("metric", 2); %>" size="2" maxlength="2"></td>
  </tr>
  <tr>
    <td>&nbsp;&nbsp;</td>
    <td><input name="wan_route_ipaddr3" value="<% wan_route("ipaddr", 3); %>" size="15" maxlength="15"></td>
    <td>&nbsp;</td>
    <td><input name="wan_route_netmask3" value="<% wan_route("netmask", 3); %>" size="15" maxlength="15"></td>
    <td>&nbsp;</td>
    <td><input name="wan_route_gateway3" value="<% wan_route("gateway", 3); %>" size="15" maxlength="15"></td>
    <td>&nbsp;</td>
    <td><input name="wan_route_metric3" value="<% wan_route("metric", 3); %>" size="2" maxlength="2"></td>
  </tr>
  <tr>
    <td>&nbsp;&nbsp;</td>
    <td><input name="wan_route_ipaddr4" value="<% wan_route("ipaddr", 4); %>" size="15" maxlength="15"></td>
    <td>&nbsp;</td>
    <td><input name="wan_route_netmask4" value="<% wan_route("netmask", 4); %>" size="15" maxlength="15"></td>
    <td>&nbsp;</td>
    <td><input name="wan_route_gateway4" value="<% wan_route("gateway", 4); %>" size="15" maxlength="15"></td>
    <td>&nbsp;</td>
    <td><input name="wan_route_metric4" value="<% wan_route("metric", 4); %>" size="2" maxlength="2"></td>
  </tr>
</table>

<p>
<table border="0" cellpadding="0" cellspacing="0">
  <tr>
    <td width="310"></td>
    <td>&nbsp;&nbsp;</td>
    <td>
	<input type="submit" name="action" value="Apply">
	<input type="reset" name="action" value="Cancel">
	<input type="submit" name="action" value="Release">
	<input type="submit" name="action" value="Renew">
    </td>
  </tr>
</table>

</form>

<p class="label">&#169;2001-2012 Broadcom Corporation. All rights reserved.</p>

</body>
</html>

