<!--
$Copyright Open Broadcom Corporation$

$Id: forward.asp,v 1.32 2009-01-05 19:46:52 bbatchel Exp $
-->

<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN">
<html lang="en">
<head>
<title>Broadcom Home Gateway Reference Design: Routing</title>
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
	<span class="title">ROUTING</span><br>
	<span class="subtitle">This page allows you to configure port
	forwarding for the router. Requests to the specified WAN port
	range will be forwarded to the port range of the LAN
	machine. You may also configure static routes here.</span>
    </td>
  </tr>
</table>

<form method="post" action="apply.cgi">
<input type="hidden" name="page" value="forward.asp">

<p>
<table border="0" cellpadding="0" cellspacing="0">
  <tr>
    <th width="310" valign="top" rowspan="11"
	onMouseOver="return overlib('Forward packets destined to ports in the first range to the LAN machine with the specified IP address. You may optionally specify a second range (the ranges may not overlap and must be the same size).', LEFT);"
	onMouseOut="return nd();">
	<input type="hidden" name="forward_port" value="10">
	Port Forwards:&nbsp;&nbsp;
    </th>
    <td>&nbsp;&nbsp;</td>
    <td class="label">Protocol</td>
    <td>&nbsp;</td>
    <td class="label">WAN Port<br>Start</td>
    <td></td>
    <td class="label">WAN Port<br>End</td>
    <td>&nbsp;</td>
    <td class="label">LAN IP Address</td>
    <td>&nbsp;</td>
    <td class="label">LAN Port<br>Start</td>
    <td></td>
    <td class="label">LAN Port<br>End</td>
    <td>&nbsp;</td>
    <td class="label">Enabled</td>
  </tr>
  <% forward_port(0, 9); %>
</table>

<p>
<table border="0" cellpadding="0" cellspacing="0">
  <tr>
    <th width="310" valign="top" rowspan="11"
	onMouseOver="return overlib('Automatically forward connections.', LEFT);"
	onMouseOut="return nd();">
	<input type="hidden" name="autofw_port" value="5">
	Application Specific Port Forwards:&nbsp;&nbsp;
    </th>
    <td>&nbsp;&nbsp;</td>
    <td class="label">Outbound<br>Protocol</td>
    <td>&nbsp;</td>
    <td class="label">Outbound<br>Port Start</td>
    <td>&nbsp;</td>
    <td class="label">Outbound<br>Port End</td>
    <td>&nbsp;</td>
    <td class="label">Inbound<br>Protocol</td>
    <td>&nbsp;</td>
    <td class="label">Inbound<br>Port Start</td>
    <td></td>
    <td class="label">Inbound<br>Port End</td>
    <td>&nbsp;</td>
    <td class="label">To<br>Port Start</td>
    <td></td>
    <td class="label">To<br>Port End</td>
    <td>&nbsp;</td>
    <td class="label">Enabled</td>
  </tr>
  <% autofw_port(0, 9); %>
</table>

<p>
<table border="0" cellpadding="0" cellspacing="0">
  <tr>
    <th width="310"
	onMouseOver="return overlib('Symmetric NAT is more secured, Cone NAT supports NAT traversal technology like Teredo .', LEFT);"
	onMouseOut="return nd();">
	NAT Type:&nbsp;&nbsp;
    </th>
    <td>&nbsp;&nbsp;</td>
    <td>
	<select name="nat_type">
	  <option value="cone" <% nvram_match("nat_type", "cone", "selected"); %>>Cone NAT</option>
	  <option value="sym" <% nvram_match("nat_type", "sym", "selected"); %>>Symmetric NAT</option>
	</select>
    </td>
  </tr>
</table>

<p>
<table border="0" cellpadding="0" cellspacing="0">
  <tr>
    <th width="310"
	onMouseOver="return overlib('Forward all other incoming WAN packets to the LAN machine with the specified IP address.', LEFT);"
	onMouseOut="return nd();">
	DMZ IP Address:&nbsp;&nbsp;
    </th>
    <td>&nbsp;&nbsp;</td>
    <td><input name="dmz_ipaddr" value="<% nvram_get("dmz_ipaddr"); %>" size="15" maxlength="15"></td>
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

<p class="label">&#169;2001-2012 Broadcom Corporation. All rights reserved.</p>

</body>
</html>
