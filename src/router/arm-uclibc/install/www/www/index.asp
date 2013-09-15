<!--
$Copyright Open Broadcom Corporation$

$Id: index.asp,v 1.59 2011-01-11 18:43:43 willfeng Exp $
-->

<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN">
<html lang="en">
<head>
<title>Broadcom Home Gateway Reference Design: Basic</title>
<meta http-equiv="Content-Type" content="text/html; charset=utf-8">
<link rel="stylesheet" type="text/css" href="style.css" media="screen">
<script language="JavaScript" type="text/javascript" src="overlib.js"></script>
<script language="JavaScript" type="text/javascript">
<!--
function firewall_change()
{
	var fw_disable = document.forms[0].fw_disable[document.forms[0].fw_disable.selectedIndex].value;

	if (fw_disable == "1" || document.forms[0].fw_disable.disabled == 1) {
		document.forms[0].http_wanport.disabled = 1;
	}
	else {
		document.forms[0].http_wanport.disabled = 0;
	}
}
function mode_change()
{
	var router_disable = document.forms[0].router_disable[document.forms[0].router_disable.selectedIndex].value;
	
	if (router_disable == "1") {
		document.forms[0].fw_disable.disabled = 1;
	}
	else {
		document.forms[0].fw_disable.disabled = 0;
	}
	
	firewall_change();
}

function basicLoad()
{
	var ure_enabled = <% wl_ure_any_enabled(); %>

	if (ure_enabled == "1") {
		document.forms[0].router_disable.disabled = 1;
		document.forms[0].ses_cl_enable.disabled = 1;
	}
	else {
		document.forms[0].router_disable.disabled = 0;
		document.forms[0].ses_cl_enable.disabled = 0;
	}
}
//-->
</script>
</head>

<body onLoad="basicLoad();">
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
	<span class="title">BASIC</span><br>
	<span class="subtitle">This page allows you to configure the
	basic operation of the router.</span>
    </td>
  </tr>
</table>

<form method="post" action="apply.cgi">
<input type="hidden" name="page" value="index.asp">

<p>
<table border="0" cellpadding="0" cellspacing="0">
  <tr>
    <th width="310"
	onMouseOver="return overlib('Shows the local time as kept by the router.', LEFT);"
	onMouseOut="return nd();">
	Local Time:&nbsp;&nbsp;
    </th>
    <td>&nbsp;&nbsp;</td>
    <td><% localtime(); %></td>
  </tr>
</table>

<p>
<table border="0" cellpadding="0" cellspacing="0">
  <tr>
    <th width="310"
	onMouseOver="return overlib('Sets the user name for access to these configuration pages. Leave this field and <b>Router Password</b> blank to disable access control.', LEFT);"
	onMouseOut="return nd();">
	Router Username:&nbsp;&nbsp;
    </th>
    <td>&nbsp;&nbsp;</td>
    <td><input name="http_username" value="<% nvram_get("http_username"); %>"></td>
  </tr>
  <tr>
    <th width="310"
	onMouseOver="return overlib('Sets the password for access to these configuration pages. Leave this field and <b>Router Username</b> blank to disable access control.', LEFT);"
	onMouseOut="return nd();">
	Router Password:&nbsp;&nbsp;
    </th>
    <td>&nbsp;&nbsp;</td>
    <td><input name="http_passwd" value="<% nvram_get("http_passwd"); %>" type="password"></td>
  </tr>
</table>

<p>
<table border="0" cellpadding="0" cellspacing="0">
  <tr>
    <th width="310"
	onMouseOver="return overlib('<b>Access Point</b> mode is equivalent to disabling <b>LAN DHCP Server</b>, <b>LAN Spanning Tree Protocol</b>, and <b>WAN Protocol</b>.', LEFT);"
	onMouseOut="return nd();">
	Router Mode:&nbsp;&nbsp;
    </th>
    <td>&nbsp;&nbsp;</td>
    <td>
	<select name="router_disable" onChange="mode_change();">
	  <option value="0" <% nvram_match("router_disable", "0", "selected"); %>>Router</option>
	  <option value="1" <% nvram_match("router_disable", "1", "selected"); %>>Access Point</option>
	</select>
    </td>
  </tr>
  <tr>
    <th width="310"
	onMouseOver="return overlib('Sets whether the firewall should be disabled. Connections from the WAN are allowed if the firewall is disabled.', LEFT);"
	onMouseOut="return nd();">
	Firewall:&nbsp;&nbsp;
    </th>
    <td>&nbsp;&nbsp;</td>
    <td>
	<select name="fw_disable" onChange="firewall_change();">
	  <option value="0" <% nvram_match("fw_disable", "0", "selected"); %>>Enabled</option>
	  <option value="1" <% nvram_match("fw_disable", "1", "selected"); %>>Disabled</option>
	</select>
    </td>
  </tr>
  <tr>
    <th width="310"
	onMouseOver="return overlib('Sets the HTTP port to use for remote access to these configuration pages. Leave this field blank to disable remote access.', LEFT);"
	onMouseOut="return nd();">	
	WAN HTTP Port:&nbsp;&nbsp;
    </th>
    <td>&nbsp;&nbsp;</td>
    <td><input name="http_wanport" value="<% nvram_get("http_wanport"); %>" size="5" maxlength="5"></td>
  </tr>
</table>
  
<p>
<table border="0" cellpadding="0" cellspacing="0">
  <tr>
    <th width="310"
	onMouseOver="return overlib('Sets the time zone of this locale.', LEFT);"
	onMouseOut="return nd();">
	Time Zone:&nbsp;&nbsp;
    </th>
    <td>&nbsp;&nbsp;</td>
    <td>
	<select name="time_zone">
	  <option value="PST8PDT" <% nvram_match("time_zone", "PST8PDT", "selected"); %>>Pacific Time</option>
	  <option value="MST7MDT" <% nvram_match("time_zone", "MST7MDT", "selected"); %>>Mountain Time</option>
	  <option value="CST6CDT" <% nvram_match("time_zone", "CST6CDT", "selected"); %>>Central Time</option>
	  <option value="EST5EDT" <% nvram_match("time_zone", "EST5EDT", "selected"); %>>Eastern Time</option>
	</select>
    </td>
  </tr>
  <tr>
    <th width="310" valign="top" rowspan="3"
	onMouseOver="return overlib('Sets the IP addresses of the NTP servers to use for time synchronization.', LEFT);"
	onMouseOut="return nd();">
	<input type="hidden" name="ntp_server" value="3">
	NTP Servers:&nbsp;&nbsp;
    </th>
    <td>&nbsp;&nbsp;</td>
    <td><input name="ntp_server0" value="<% nvram_list("ntp_server", 0); %>" size="15" maxlength="15"></td>
  </tr>
  <tr>
    <td>&nbsp;&nbsp;</td>
    <td><input name="ntp_server1" value="<% nvram_list("ntp_server", 1); %>" size="15" maxlength="15"></td>
  </tr>
  <tr>
    <td>&nbsp;&nbsp;</td>
    <td><input name="ntp_server2" value="<% nvram_list("ntp_server", 2); %>" size="15" maxlength="15"></td>
  </tr>
</table>

<p>
<table border="0" cellpadding="0" cellspacing="0">
  <tr>
    <th width="310"
	onMouseOver="return overlib('System log messages will be sent to this IP address.', LEFT);"
	onMouseOut="return nd();">
	Syslog IP Address:&nbsp;&nbsp;
    </th>
    <td>&nbsp;&nbsp;</td>
    <td><input name="log_ipaddr" value="<% nvram_get("log_ipaddr"); %>" size="15" maxlength="15"></td>
  </tr>
</table>

<!--
#ifdef __CONFIG_SES__
-->
<p>
<table border="0" cellpadding="0" cellspacing="0">
  <tr>
    <th width="310"
	onMouseOver="return overlib('Sets whether SecureEasySetup (SES) is enabled.', LEFT);"
	onMouseOut="return nd();">
	SES:&nbsp;&nbsp;
    </th>
    <td>&nbsp;&nbsp;</td>
    <td>
	<select name="ses_enable">
	  <option value="1" <% nvram_match("ses_enable", "1", "selected"); %>>Enabled</option>
	  <option value="0" <% nvram_match("ses_enable", "0", "selected"); %>>Disabled</option>
	</select>
    </td>
  </tr>  
  <tr>
    <th width="310"
	onMouseOver="return overlib('Sets WDS configuration mode for SES.', LEFT);"
	onMouseOut="return nd();">
	SES WDS Mode:&nbsp;&nbsp;
    </th>
    <td>&nbsp;&nbsp;</td>
    <td>
	<select name="ses_wds_mode">
	<% ses_wds_mode_list();%>
	</select>
    </td>
  </tr>  
    <% ses_button_display(); %>
</table>
<!--
#endif
-->    

<!--
#ifdef __CONFIG_SES_CL__
-->
<p>
<table border="0" cellpadding="0" cellspacing="0">
  <tr>
    <th width="310"
	onMouseOver="return overlib('Sets whether SecureEasySetup (SES) WET client is enabled.', LEFT);"
	onMouseOut="return nd();">
	SES Client:&nbsp;&nbsp;
    </th>
    <td>&nbsp;&nbsp;</td>
    <td>
	<select name="ses_cl_enable">
	  <option value="1" <% nvram_match("ses_cl_enable", "1", "selected"); %>>Enabled</option>
	  <option value="0" <% nvram_match("ses_cl_enable", "0", "selected"); %>>Disabled</option>
	</select>
    </td>
  </tr>  
    <% ses_cl_button_display(); %>
</table>
<!--
#endif
-->    

<p>
<table border="0" cellpadding="0" cellspacing="0">

<p>
<table border="0" cellpadding="0" cellspacing="0">
  <tr>
    <th width="310"
	onMouseOver="return overlib('Sets whether Universal Plug and Play (UPnP) is enabled.', LEFT);"
	onMouseOut="return nd();">
	UPnP:&nbsp;&nbsp;
    </th>
    <td>&nbsp;&nbsp;</td>
    <td>
	<select name="upnp_enable">
	  <option value="1" <% nvram_match("upnp_enable", "1", "selected"); %>>Enabled</option>
	  <option value="0" <% nvram_match("upnp_enable", "0", "selected"); %>>Disabled</option>
	</select>
    </td>
  </tr>  
  <tr>
    <th width="310"
	onMouseOver="return overlib('Sets which connections through the router should be logged. Selecting <b>Denied</b> enables logging of denied connections. Selecting <b>Accepted</b> enables logging of accepted connections. Select <b>Both</b> enables logging of both denied and accepted connections.', LEFT);"
	onMouseOut="return nd();">
	Connection Logging:&nbsp;&nbsp;
    </th>
    <td>&nbsp;&nbsp;</td>
    <td>
	<select name="log_level">
	  <option value="0" <% nvram_match("log_level", "0", "selected"); %>>Disabled</option>
	  <option value="1" <% nvram_match("log_level", "1", "selected"); %>>Denied</option>
	  <option value="2" <% nvram_match("log_level", "2", "selected"); %>>Accepted</option>
	  <option value="3" <% nvram_match("log_level", "3", "selected"); %>>Both</option>
	</select>
    </td>
  </tr>
</table>

<p>
<table border="0" cellpadding="0" cellspacing="0">

<p>
<table border="0" cellpadding="0" cellspacing="0">
  <tr>
    <th width="310"
	onMouseOver="return overlib('Sets the coma mode interval in seconds before reset.', LEFT);"
	onMouseOut="return nd();">
	Coma Mode Sleep Time:&nbsp;&nbsp;
    </th>
    <td>&nbsp;&nbsp;</td>
    <td><input name="coma_sleep" value="<% nvram_get("coma_sleep"); %>"></td>    
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
	<input type="submit" name="action" value="Restore Defaults">
	<input type="submit" name="action" value="Reboot">
    </td>
  </tr>
</table>

</form>

<p class="label">&#169;2001-2012 Broadcom Corporation. All rights reserved.</p>

</body>
</html>
