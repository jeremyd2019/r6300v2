<!--
$Copyright Open Broadcom Corporation$

$Id: filter.asp,v 1.35 2011-01-11 18:43:43 willfeng Exp $
-->

<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN">
<html lang="en">
<head>
<title>Broadcom Home Gateway Reference Design: Filters</title>
<meta http-equiv="Content-Type" content="text/html; charset=utf-8">
<link rel="stylesheet" type="text/css" href="style.css" media="screen">
<script language="JavaScript" type="text/javascript" src="overlib.js"></script>
</head>

<body>
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
	<span class="title">FILTERS</span><br>
	<span class="subtitle">This page allows you to configure LAN
	filters for the router. The LAN machines affected by the filters
	will not be able to communicate through the WAN but will be able
	to communicate with each other and with the router itself.</span>
    </td>
  </tr>
</table>

<form method="post" action="apply.cgi">
<input type="hidden" name="page" value="filter.asp">

<p>
<table border="0" cellpadding="0" cellspacing="0">
  <tr>
    <th width="310"
	onMouseOver="return overlib('Selects whether clients with the specified MAC address are allowed or denied access to the router and the WAN.', LEFT);"
	onMouseOut="return nd();">
	LAN MAC Filter Mode:&nbsp;&nbsp;
    </th>
    <td>&nbsp;&nbsp;</td>
    <td>
	<select name="filter_macmode">
	  <option value="disabled" <% nvram_match("filter_macmode", "disabled", "selected"); %>>Disabled</option>
	  <option value="allow" <% nvram_match("filter_macmode", "allow", "selected"); %>>Allow</option>
	  <option value="deny" <% nvram_match("filter_macmode", "deny", "selected"); %>>Deny</option>
	</select>
    </td>
  </tr>
  <tr>
    <th width="310" valign="top" rowspan="5"
	onMouseOver="return overlib('Filter packets from LAN machines with the specified MAC addresses. The MAC address format is XX:XX:XX:XX:XX:XX.', LEFT);"
	onMouseOut="return nd();">
	<input type="hidden" name="filter_maclist" value="5">
	LAN MAC Filters:&nbsp;&nbsp;
    </th>
    <td>&nbsp;&nbsp;</td>
    <td><input name="filter_maclist0" value="<% nvram_list("filter_maclist", 0); %>" size="17" maxlength="17"></td>
  </tr>
  <tr>
    <td>&nbsp;&nbsp;</td>
    <td><input name="filter_maclist1" value="<% nvram_list("filter_maclist", 1); %>" size="17" maxlength="17"></td>
  </tr>
  <tr>
    <td>&nbsp;&nbsp;</td>
    <td><input name="filter_maclist2" value="<% nvram_list("filter_maclist", 2); %>" size="17" maxlength="17"></td>
  </tr>
  <tr>
    <td>&nbsp;&nbsp;</td>
    <td><input name="filter_maclist3" value="<% nvram_list("filter_maclist", 3); %>" size="17" maxlength="17"></td>
  </tr>
  <tr>
    <td>&nbsp;&nbsp;</td>
    <td><input name="filter_maclist4" value="<% nvram_list("filter_maclist", 4); %>" size="17" maxlength="17"></td>
  </tr>
</table>

<p>
<table border="0" cellpadding="0" cellspacing="0">
  <tr>
    <th width="310" valign="top" rowspan="11"
	onMouseOver="return overlib('Filter packets from IP addresses destined to certain port ranges during the specified times.', LEFT);"
	onMouseOut="return nd();">
	<input type="hidden" name="filter_client" value="10">
	LAN Client Filters:&nbsp;&nbsp;
    </th>
    <td>&nbsp;&nbsp;</td>
    <td class="label" colspan="3">LAN IP Address Range</td>
    <td>&nbsp;</td>
    <td class="label">Protocol</td>
    <td>&nbsp;</td>
    <td class="label" colspan="3">Destination<br>Port Range</td>
    <td>&nbsp;</td>
    <td class="label">From<br>Day</td>
    <td></td>
    <td class="label">To<br>Day</td>
    <td>&nbsp;</td>
    <td class="label">From<br>Hour</td>
    <td></td>
    <td class="label">To<br>Hour</td>
    <td>&nbsp;</td>
    <td class="label">Enabled</td>
  </tr>
  <% filter_client(0, 9); %>
</table>

<% filter_url(0, 9); %>

<p>
<table border="0" cellpadding="0" cellspacing="0">
    <tr>
      <td width="310"></td>
      <td>&nbsp;&nbsp;</td>
      <td>
	  <input type=submit name="action" value="Apply">
	  <input type=reset name="action" value="Cancel">
      </td>
    </tr>
</table>

</form>

<p class="label">&#169;2001-2012 Broadcom Corporation. All rights reserved.</p>

</body>
</html>
