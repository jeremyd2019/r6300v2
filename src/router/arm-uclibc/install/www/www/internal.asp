<!--
$Copyright Open Broadcom Corporation$

$Id: internal.asp,v 1.42 2011-01-11 18:43:43 willfeng Exp $
-->

<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN">
<html lang="en">
<head>
<title>Broadcom Home Gateway Reference Design: Internal</title>
<meta http-equiv="Content-Type" content="text/html; charset=utf-8">
<link rel="stylesheet" type="text/css" href="style.css" media="screen">
<script language="JavaScript" type="text/javascript" src="overlib.js"></script>
<script language="JavaScript" type="text/javascript">

<!--
function nfs_proto_change() {
/*
#ifdef BCMINTERNAL
*/
	var nfs_proto = document.forms[0].nfs_proto[document.forms[0].nfs_proto.selectedIndex].value;

	if (nfs_proto == "dhcp") {
		document.forms[0].nfs_ipaddr.disabled = 1;
		document.forms[0].nfs_netmask.disabled = 1;
		document.forms[0].nfs_gateway.disabled = 1;
	} else {
		document.forms[0].nfs_ipaddr.disabled = 0;
		document.forms[0].nfs_netmask.disabled = 0;
		document.forms[0].nfs_gateway.disabled = 0;
	}
/*
#endif
*/
}
//-->
</script>
</head>

<body onLoad="nfs_proto_change();">

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
	<span class="title">INTERNAL</span><br>
	<span class="subtitle">This screen is for Broadcom internal
	usage only.</span>
    </td>
  </tr>
</table>

<form method="post" action="internal.asp">
<input type="hidden" name="page" value="internal.asp">

<!--
#ifdef BCMINTERNAL
-->	
<p>
<table border="0" cellpadding="0" cellspacing="0">
  <tr>
    <th width="310">Upgrade Server:&nbsp;&nbsp;</th>
    <td>&nbsp;&nbsp;</td>
    <td><input name="os_server" value="<% nvram_get("os_server"); %>"></td>
  </tr>
  <tr>
    <th width="310">Upgrade Version:&nbsp;&nbsp;</th>
    <td>&nbsp;&nbsp;</td>
    <td><input name="os_version"></td>
  </tr>
  <tr>
    <th width="310">Statistics Server:&nbsp;&nbsp;</th>
    <td>&nbsp;&nbsp;</td>
    <td><input name="stats_server" value="<% nvram_get("stats_server"); %>"></td>
  </tr>
  <tr>
    <th width="310">Timer Interval:&nbsp;&nbsp;</th>
    <td>&nbsp;&nbsp;</td>
    <td><input name="timer_interval" value="<% nvram_get("timer_interval"); %>"></td>
  </tr>
</table>
<!--
#endif
-->	

<p>
<table border="0" cellpadding="0" cellspacing="0">
  <tr>
    <th width="310"
	onMouseOver="return overlib('Selects which wireless interface to configure.', LEFT);"
	onMouseOut="return nd();">
	Wireless Interface:&nbsp;&nbsp;
    </th>
    <td>&nbsp;&nbsp;</td>
    <td>
	<select name="wl_unit" onChange="submit();">
	  <% wl_list(); %>
	</select>
    </td>
  </tr>
  <tr>
    <th width="310">54g Only Mode:&nbsp;&nbsp;</th>
    <td>&nbsp;&nbsp;</td>
    <td>
	<select name="wl_gmode">
	  <option value="1" <% nvram_invmatch("wl_gmode", "2", "selected"); %>>Disabled</option>
	  <option value="2" <% nvram_match("wl_gmode", "2", "selected"); %>>Enabled</option>
	</select>
    </td>
  </tr>
   <tr>
     <th width="310"
 	onMouseOver="return overlib('Sets whether system log messages will be saved in RAM for showing in web.', LEFT);"
 	onMouseOut="return nd();">
 	Syslog in RAM:&nbsp;&nbsp;
     </th>
     <td>&nbsp;&nbsp;</td>
     <td>
 	<select name="log_ram_enable">
 	  <option value="0" <% nvram_match("log_ram_enable", "0", "selected"); %>>Disabled</option>
 	  <option value="1" <% nvram_match("log_ram_enable", "1", "selected"); %>>Enabled</option>
 	</select>
     </td>
   </tr>
</table>

<!--
#ifdef BCMINTERNAL
-->
<p>
<table border="0" cellpadding="0" cellspacing="0">
  <tr>
    <th width="310"
	onMouseOver="return overlib('Shows the interface hardware address.', LEFT);"
	onMouseOut="return nd();">
	All Interfaces:&nbsp;&nbsp;
    </th>
    <td>&nbsp;&nbsp;</td>
    <td>
	<select name="all_iflist">
	  <% all_iflist(); %>
	</select>
    </td>
  </tr>    
  <tr>
    <th width="310"
	onMouseOver="return overlib('Sets the LAN interface.', LEFT);"
	onMouseOut="return nd();">
	Internal Network Interface:&nbsp;&nbsp;
    </th>
    <td>&nbsp;&nbsp;</td>
    <td><input name="lan_ifname" value="<% nvram_get("lan_ifname"); %>"></td>
  </tr>
  <tr>
    <th width="310"
	onMouseOver="return overlib('Sets the Guest LAN 1 interface. This is hardwired to lan1_ifname', LEFT);"
	onMouseOut="return nd();">
	Guest Network Interface:&nbsp;&nbsp;
    </th>
    <td>&nbsp;&nbsp;</td>
    <td><input name="lan_guest_ifname" value="<% nvram_get("lan1_ifname"); %>"></td>
  </tr>
  <tr>
    <th width="310"
	onMouseOver="return overlib('Specifies the bridged interfaces if the LAN interface is bridge.', LEFT);"
	onMouseOut="return nd();">
	br0 Interfaces:&nbsp;&nbsp;
    </th>
    <td>&nbsp;&nbsp;</td>
    <td><input name="lan_ifnames" value="<% nvram_get("lan_ifnames"); %>"></td>
  </tr>   <tr>
    <th width="310"
	onMouseOver="return overlib('Specifies the bridged interfaces if the LAN interface is bridge.', LEFT);"
	onMouseOut="return nd();">
	br1 Interfaces:&nbsp;&nbsp;
    </th>
    <td>&nbsp;&nbsp;</td>
    <td><input name="lan1_ifnames" value="<% nvram_get("lan1_ifnames"); %>"></td>
  </tr>
  <tr>
    <th width="310"
	onMouseOver="return overlib('Sets the WAN interfaces.', LEFT);"
	onMouseOut="return nd();">
	WAN Interfaces:&nbsp;&nbsp;
    </th>
    <td>&nbsp;&nbsp;</td>
    <td><input name="wan_ifnames" value="<% nvram_get("wan_ifnames"); %>"></td>
  </tr>
</table>

<p>
<table border="0" cellpadding="0" cellspacing="0">
  <tr>
    <th width="310"
	onMouseOver="return overlib('Sets the syslogd log level.', LEFT);"
	onMouseOut="return nd();">
	console Log Level:&nbsp;&nbsp;
    </th>
    <td>&nbsp;&nbsp;</td>
    <td><input name="console_loglevel" value="<% nvram_get("console_loglevel"); %>"></td>
  </tr>
  <tr>
    <th width="310"
	onMouseOver="return overlib('Sets the syslogd mark interval.', LEFT);"
	onMouseOut="return nd();">
	syslogd Mark Interval:&nbsp;&nbsp;
    </th>
    <td>&nbsp;&nbsp;</td>
    <td><input name="syslogd_mark" value="<% nvram_get("syslogd_mark"); %>"></td>
  </tr>
  <tr>
    <th width="310"
	onMouseOver="return overlib('Sets the klogd log level.', LEFT);"
	onMouseOut="return nd();">
	syslogd Log Facility:&nbsp;&nbsp;
    </th>
    <td>&nbsp;&nbsp;</td>
    <td><input name="syslogd_facility" value="<% nvram_get("syslogd_facility"); %>"></td>
  </tr>
</table>

<p>
<table border="0" cellpadding="0" cellspacing="0">
  <tr>
    <th width="310"
	onMouseOver="return overlib('Sets the wireless driver message level.', LEFT);"
	onMouseOut="return nd();">
	wl_msglevel Message Level:&nbsp;&nbsp;
    </th>
    <td>&nbsp;&nbsp;</td>
    <td><input name="wl_msglevel" value="<% nvram_get("wl_msglevel"); %>"></td>
  </tr>
</table>

<p>
<table border="0" cellpadding="0" cellspacing="0">
  <tr>
    <th width="310"
	onMouseOver="return overlib('Sets the SES debug level(0xFFFF for all).', LEFT);"
	onMouseOut="return nd();">
	SES Debug Level:&nbsp;&nbsp;
    </th>
    <td>&nbsp;&nbsp;</td>
    <td><input name="ses_debug_level" value="<% nvram_get("ses_debug_level"); %>"></td>
  </tr>
</table>

<p>
<table border="0" cellpadding="0" cellspacing="0">
  <tr>
    <th width="310"
	onMouseOver="return overlib('Sets the interface to use for mounting the root filesystem over NFS.', LEFT);"
	onMouseOut="return nd();">
	NFS Interface:&nbsp;&nbsp;
    </th>
    <td>&nbsp;&nbsp;</td>
    <td><input name="nfs_ifname" value="<% nvram_get("nfs_ifname"); %>"></td>
  </tr>
  <tr>
    <th width="310"
	onMouseOver="return overlib('Sets the IP address of the NFS server.', LEFT);"
	onMouseOut="return nd();">
	NFS Server:&nbsp;&nbsp;
    </th>
    <td>&nbsp;&nbsp;</td>
    <td><input name="nfs_server" value="<% nvram_get("nfs_server"); %>" size="15" maxlength="15"></td>
  </tr>
  <tr>
    <th width="310"
	onMouseOver="return overlib('Sets the path to the NFS root filesystem.', LEFT);"
	onMouseOut="return nd();">
	NFS Root:&nbsp;&nbsp;
    </th>
    <td>&nbsp;&nbsp;</td>
    <td><input name="nfs_root" value="<% nvram_get("nfs_root"); %>"></td>
  </tr>
  <tr>
    <th width="310"
	onMouseOver="return overlib('Sets the method to use to obtain an IP address for the NFS interface.', LEFT);"
	onMouseOut="return nd();">
	NFS Protocol:&nbsp;&nbsp;
    </th>
    <td>&nbsp;&nbsp;</td>
    <td>
	<select name="nfs_proto" onChange="nfs_proto_change();">
	  <option value="dhcp" <% nvram_match("nfs_proto", "dhcp", "selected"); %>>DHCP</option>
	  <option value="static" <% nvram_match("nfs_proto", "static", "selected"); %>>Static</option>
	  <option value="disabled" <% nvram_match("nfs_proto", "disabled", "selected"); %>>Disabled</option>
	</select>
    </td>
  </tr>
  <tr>
    <th width="310"
	onMouseOver="return overlib('Sets the IP address of the NFS interface.', LEFT);"
	onMouseOut="return nd();">
	NFS IP Address:&nbsp;&nbsp;
    </th>
    <td>&nbsp;&nbsp;</td>
    <td><input name="nfs_ipaddr" value="<% nvram_get("nfs_ipaddr"); %>" size="15" maxlength="15"></td>
  </tr>
  <tr>
    <th width="310"
	onMouseOver="return overlib('Sets the IP netmask of the NFS interface.', LEFT);"
	onMouseOut="return nd();">
	NFS Subnet Mask:&nbsp;&nbsp;
    </th>
    <td>&nbsp;&nbsp;</td>
    <td><input name="nfs_netmask" value="<% nvram_get("nfs_netmask"); %>" size="15" maxlength="15"></td>
  </tr>
  <tr>
    <th width="310"
	onMouseOver="return overlib('Sets the IP address of the default gateway to use for the NFS interface.', LEFT);"
	onMouseOut="return nd();">
	NFS Default Gateway:&nbsp;&nbsp;
    </th>
    <td>&nbsp;&nbsp;</td>
    <td><input name="nfs_gateway" value="<% nvram_get("nfs_gateway"); %>" size="15" maxlength="15"></td>
  </tr>
</table>

<table border="0" cellpadding="0" cellspacing="0">
  <tr>
    <th width="310" valign="top"
	onMouseOver="return overlib('Show System Log.', LEFT);"
	onMouseOut="return nd();">
	SysLog:&nbsp;&nbsp;
    </th>
    <td>&nbsp;&nbsp;</td>
    <td><% syslog(); %></td>
  </tr>
</table>
<!--
#endif
-->

<p>
<table border="0" cellpadding="0" cellspacing="0">
  <tr>
    <td width="310"></td>
    <td>&nbsp;&nbsp;</td>
    <td>
	<input type="submit" name="action" value="Apply">
	<input type="reset" name="action" value="Cancel">
	<input type="submit" name="action" value="Upgrade">
	<input type="submit" name="action" value="Stats">
	<% wl_radio_roam_option(); %>
    </td>
  </tr>
</table>

</form>

<p class="label">&#169;2001-2012 Broadcom Corporation. All rights reserved. 

</body>
</html>
