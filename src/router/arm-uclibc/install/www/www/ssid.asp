<!--
$Copyright Open Broadcom Corporation$
$ID$-->

<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN">
<html lang="en">
<head>
<title>Broadcom Home Gateway Reference Design: SSID</title>
<meta http-equiv="Content-Type" content="text/html; charset=utf-8">
<link rel="stylesheet" type="text/css" href="style.css" media="screen">
<script language="JavaScript" type="text/javascript" src="overlib.js"></script>
<script language="JavaScript" type="text/javascript">
<!--
function wl_recalc()
{
   var ure_disable = "<% nvram_get("ure_disable"); %>";
   var wme_enabled = "<% nvram_get("wl_wme"); %>";
   var mode = "<% nvram_get("wl_mode"); %>";

   document.forms[0].wl_mode.disabled = 1;
   if (ure_disable == "0") {
		document.forms[0].wl_bridge.disabled = 1;
   }

   if (wme_enabled != "on") {
		document.forms[0].wl_wme_bss_disable.disabled = 1;
   }
   if (mode == "sta") {
		document.forms[0].wl_wmf_bss_enable.disabled = 1;
		document.forms[0].wl_bss_maxassoc.disabled = 1;
   }
   if(mode == "ap") {
		document.forms[0].wl_mcast_regen_bss_enable.disabled = 1;
   }
}

function wl_closed_check()
{
/*
#ifdef BCMWPS
*/
<% wps_closed_check_display(); %>
/*
#endif
*/
}

function wl_macmode_check()
{
/*
#ifdef BCMWPS
*/
<% wps_macmode_check_display(); %>
/*
#endif
*/
}

//-->
</script>
</head>

<body onLoad="wl_recalc();">
<div id="overDiv" style="position:absolute; visibility:hidden; z-index:1000;"></div>
<input type=hidden name="wl_mode_changed" value=0>
<input type=hidden name="wl_ure_changed" value=0>

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
	<span class="title">SSID</span><br>
	<span class="subtitle">This page allows you to configure the Virtual interfaces for each
	Physical interface.</span>
    </td>
  </tr>
</table>

<form method="post" action="ssid.asp">
<input type="hidden" name="page" value="ssid.asp">


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
</table>

<p>
<table border="0" cellpadding="0" cellspacing="0"> <tr>
    <th width="310"
	onMouseOver="return overlib('Selects desired BSS to configure.', LEFT);"
	onMouseOut="return nd();">
	BSS-MAC (SSID):&nbsp;&nbsp;
    </th>
    <td>&nbsp;&nbsp;</td>
    <td>
	<select name="wl_bssid" onChange="submit();">
	  <% wl_bssid_list(); %>
	</select>
    </td>
  </tr>
  <tr>
    <th width="310"
	onMouseOver="return overlib('It shows whether the BSSID is AP, WirelessBridge, WET or STA ', LEFT);"
	onMouseOut="return nd();">
	Mode:&nbsp;&nbsp;
    </th>
    <td>&nbsp;&nbsp;</td>
    <td>
	<select name="wl_mode">
	<% wl_mode_list();%>
	</select>
    </td>
  </tr>
  
  <tr>
    <th width="310"
	onMouseOver="return overlib('Enables or disables this SSID .', LEFT);"
	onMouseOut="return nd();">
	BSS Enabled:&nbsp;&nbsp;
    </th>
    <td>&nbsp;&nbsp;</td>
    <td>
	<select name="wl_bss_enabled">
	  <option value="0" <% nvram_match("wl_bss_enabled", "0", "selected"); %>>Disabled</option> 
	  <option value="1" <% nvram_match("wl_bss_enabled", "1", "selected"); %>>Enabled</option>
	</select>
    </td>
  </tr>  
  <tr>
    <th width="310"
	onMouseOver="return overlib('Sets the Network Name (also known as SSID) of this network.', LEFT);"
	onMouseOut="return nd();">
	Network Name (SSID):&nbsp;&nbsp;
    </th>
    <td>&nbsp;&nbsp;</td>
    <td><input name="wl_ssid" value="<% nvram_get("wl_ssid"); %>" size="32" maxlength="32"></td>
  </tr>  
    
  <tr>
    <th width="310"
	onMouseOver="return overlib('Selecting <b>Closed</b> hides the network from active scans. Selecting <b>Open</b> reveals the network to active scans.', LEFT);"
	onMouseOut="return nd();">
	Network Type:&nbsp;&nbsp;
    </th>
    <td>&nbsp;&nbsp;</td>
    <td>
    <select name="wl_closed" onChange="wl_closed_check();">
	  <option value="0" <% nvram_match("wl_closed", "0", "selected"); %>>Open</option>
	  <option value="1" <% nvram_match("wl_closed", "1", "selected"); %>>Closed</option>
	</select>
    </td>
  </tr>
  <tr>
    <th width="310"
	onMouseOver="return overlib('Selecting <b>On</b> enables AP isolation mode.  When enabled, STAs associated to the AP will not be able to communicate with each other.', LEFT);"
	onMouseOut="return nd();">
	AP Isolation:&nbsp;&nbsp;
    </th>
    <td>&nbsp;&nbsp;</td>
    <td>
	<select name="wl_ap_isolate">
	  <option value="0" <% nvram_match("wl_ap_isolate", "0", "selected"); %>>Off</option>
	  <option value="1" <% nvram_match("wl_ap_isolate", "1", "selected"); %>>On</option>
	</select>
    </td>
  </tr>
  <tr>
    <th width="310"
	onMouseOver="return overlib('Selects desired Bridge br0 or br1 .', LEFT);"
	onMouseOut="return nd();">
	Network Bridge:&nbsp;&nbsp;
    </th>
    <td>&nbsp;&nbsp;</td>
    <td>  <select name="wl_bridge" >  <% wl_get_bridge(); %> </select> 	</td>
  </tr>  
  <tr>
    <th width="310"
	onMouseOver="return overlib('Sets the maximum associations the for this BSS.', LEFT);"
	onMouseOut="return nd();">
	BSS Max Associations Limit:&nbsp;&nbsp;
    </th>
    <td>&nbsp;&nbsp;</td>
    <td><input name="wl_bss_maxassoc" value="<% nvram_get("wl_bss_maxassoc"); %>" size="4" maxlength="4"></td>
  </tr>
  <tr>
    <th width="310"
	onMouseOver="return overlib('When WMM is enabled for the radio, selecting <b>On</b> allows WMM to be advertised in beacons and probes for this BSS.  <b>Off</b> disables advertisement of WMM in beacons and probes', LEFT);"
	onMouseOut="return nd();">
	WMM Advertise:&nbsp;&nbsp;
    </th>
    <td>&nbsp;&nbsp;</td>
    <td>
	<select name="wl_wme_bss_disable">
	  <option value="0" <% nvram_match("wl_wme_bss_disable", "0", "selected"); %>>Advertise</option>
	  <option value="1" <% nvram_match("wl_wme_bss_disable", "1", "selected"); %>>Do Not Advertise</option>
	</select>
    </td>
  </tr>
  <tr>
    <th width="310"
	onMouseOver="return overlib('Choose <b>On</b> to enable Wireless Multicast Forwarding on this BSS.  <b>Off</b> disables the feature', LEFT);"
	onMouseOut="return nd();">
	WMF:&nbsp;&nbsp;
    </th>
    <td>&nbsp;&nbsp;</td>
    <td>
	<select name="wl_wmf_bss_enable">
	  <option value="1" <% nvram_match("wl_wmf_bss_enable", "1", "selected"); %>>On</option>
	  <option value="0" <% nvram_match("wl_wmf_bss_enable", "0", "selected"); %>>Off</option>
	</select>
    </td>
  </tr>
  <tr>
    <th width="310"
	onMouseOver="return overlib('Choose <b>On</b> to enable Multicast Reverse Translation on this sta.  <b>Off</b> disables the feature', LEFT);"
	onMouseOut="return nd();">
	MCAST_REGEN:&nbsp;&nbsp;
    </th>
    <td>&nbsp;&nbsp;</td>
    <td>
	<select name="wl_mcast_regen_bss_enable">
	  <option value="1" <% nvram_match("wl_mcast_regen_bss_enable", "1", "selected"); %>>On</option>
	  <option value="0" <% nvram_match("wl_mcast_regen_bss_enable", "0", "selected"); %>>Off</option>
	</select>
    </td>
  </tr>
  <tr>
    <th width="310"
	onMouseOver="return overlib('Set operational capabilities required for stations to associate to the BSS supported by the interface: <b>none<b> - no requirements on joining devices. <b>erp<b> - devices must advertise ERP (11g) capabilities to be allowed on a 2G band BSS. <b>ht<b> - devices must advertise HT (11n) capabilities to be allowed on the BSS. <b>vht<b> - devices must advertise VHT (11ac) capabilities to be allowed on the BSS', LEFT);"
	onMouseOut="return nd();">
	Operational capabilities mode required:&nbsp;&nbsp;
    </th>
    <td>&nbsp;&nbsp;</td>
    <td>
	<select name="wl_bss_opmode_cap_reqd">
	  <option value="0" <% nvram_match("wl_bss_opmode_cap_reqd", "0", "selected"); %>>none</option>
	  <option value="1" <% nvram_match("wl_bss_opmode_cap_reqd", "1", "selected"); %>>ERP</option>
	  <option value="2" <% nvram_match("wl_bss_opmode_cap_reqd", "2", "selected"); %>>HT</option>
	  <option value="3" <% nvram_match("wl_bss_opmode_cap_reqd", "3", "selected"); %>>VHT</option>
	</select>
    </td>
  </tr>
</table>
<p>
<table border="0" cellpadding="0" cellspacing="0">
  <tr>
    <th width="310"
	onMouseOver="return overlib('Selects whether clients with the specified MAC address are allowed or denied wireless access.', LEFT);"
	onMouseOut="return nd();">
	MAC Restrict Mode:&nbsp;&nbsp;
    </th>
    <td>&nbsp;&nbsp;</td>
    <td>
	<select name="wl_macmode" onChange="wl_macmode_check();">
	  <option value="disabled" <% nvram_match("wl_macmode", "disabled", "selected"); %>>Disabled</option>
	  <option value="allow" <% nvram_match("wl_macmode", "allow", "selected"); %>>Allow</option>
	  <option value="deny" <% nvram_match("wl_macmode", "deny", "selected"); %>>Deny</option>
 	</select>
    </td>
  </tr>
  <tr>	
    <th width="310" valign="top" rowspan="8"
	onMouseOver="return overlib('Allows or denies wireless access to clients with the specified MAC addresses. The MAC address format is XX:XX:XX:XX:XX:XX.', LEFT);"
	onMouseOut="return nd();">
	<input type="hidden" name="wl_maclist" value="16">
	MAC Addresses:&nbsp;&nbsp;
    </th>
    <td>&nbsp;&nbsp;</td>
    <td><input name="wl_maclist0" value="<% nvram_list("wl_maclist", 0); %>" size="17" maxlength="17"></td>
    <td><input name="wl_maclist1" value="<% nvram_list("wl_maclist", 1); %>" size="17" maxlength="17"></td>
  </tr>
  <tr>
    <td>&nbsp;&nbsp;</td>
    <td><input name="wl_maclist2" value="<% nvram_list("wl_maclist", 2); %>" size="17" maxlength="17"></td>
    <td><input name="wl_maclist3" value="<% nvram_list("wl_maclist", 3); %>" size="17" maxlength="17"></td>
  </tr>
  <tr>
    <td>&nbsp;&nbsp;</td>
    <td><input name="wl_maclist4" value="<% nvram_list("wl_maclist", 4); %>" size="17" maxlength="17"></td>
    <td><input name="wl_maclist5" value="<% nvram_list("wl_maclist", 5); %>" size="17" maxlength="17"></td>
  </tr>
  <tr>
    <td>&nbsp;&nbsp;</td>
    <td><input name="wl_maclist6" value="<% nvram_list("wl_maclist", 6); %>" size="17" maxlength="17"></td>
    <td><input name="wl_maclist7" value="<% nvram_list("wl_maclist", 7); %>" size="17" maxlength="17"></td>
  </tr>
  <tr>
    <td>&nbsp;&nbsp;</td>
    <td><input name="wl_maclist8" value="<% nvram_list("wl_maclist", 8); %>" size="17" maxlength="17"></td>
    <td><input name="wl_maclist9" value="<% nvram_list("wl_maclist", 9); %>" size="17" maxlength="17"></td>
  </tr>
  <tr>
    <td>&nbsp;&nbsp;</td>
    <td><input name="wl_maclist10" value="<% nvram_list("wl_maclist", 10); %>" size="17" maxlength="17"></td>
    <td><input name="wl_maclist11" value="<% nvram_list("wl_maclist", 11); %>" size="17" maxlength="17"></td>
  </tr>
  <tr>
    <td>&nbsp;&nbsp;</td>
    <td><input name="wl_maclist12" value="<% nvram_list("wl_maclist", 12); %>" size="17" maxlength="17"></td>
    <td><input name="wl_maclist13" value="<% nvram_list("wl_maclist", 13); %>" size="17" maxlength="17"></td>
  </tr>
  <tr>
    <td>&nbsp;&nbsp;</td>
    <td><input name="wl_maclist14" value="<% nvram_list("wl_maclist", 14); %>" size="17" maxlength="17"></td>
    <td><input name="wl_maclist15" value="<% nvram_list("wl_maclist", 15); %>" size="17" maxlength="17"></td>
  </tr>
</table>

<p>
<table border="0" cellpadding="0" cellspacing="0">
  <tr>
    <th width="310" valign="top"
	onMouseOver="return overlib('Stations currently authenticated to this AP.', LEFT);"
	onMouseOut="return nd();">
	Authenticated Stations:&nbsp;&nbsp;
    </th>
    <td>&nbsp;&nbsp;</td>
    <td>
	<table border="1" cellpadding="2" cellspacing="0">
	  <tr align="center">
	    <td class="label">MAC<br>Address</td>
	    <td class="label">Association<br>Time</td>
	    <td class="label">Authorized</td>
	    <td class="label">WMM<br>Link</td>
	    <td class="label">Power<br>Save</td>
	    <td class="label">APSD<br>Default</td>
	  </tr>
	  <% wl_auth_list(); %>
	</table>
    </td>
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
      </td>
    </tr>
</table>

</form>

<p class="label">&#169;2001-2012 Broadcom Corporation. All rights
reserved. 54g and XPress are trademarks of Broadcom Corporation.</p>

</body>
</html>
