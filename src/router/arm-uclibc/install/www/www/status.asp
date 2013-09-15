<!--
$Copyright Open Broadcom Corporation$

$Id: status.asp,v 1.34 2011-01-11 18:43:43 willfeng Exp $
-->

<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN">
<html lang="en">
<head>
<title>Broadcom Home Gateway Reference Design: Status</title>
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
	<span class="title">STATUS</span><br>
	<span class="subtitle">This page displays miscellaneous status
	information.</span>
    </td>
  </tr>
</table>

<p>
<table border="0" cellpadding="0" cellspacing="0">
  <tr>
    <th width="310"
	onMouseOver="return overlib('Shows the system up time since the router was booted.', LEFT);"
	onMouseOut="return nd();">
	System Up Time:&nbsp;&nbsp;
    </th>
    <td>&nbsp;&nbsp;</td>
    <td><% sysuptime(); %></td>
  </tr>
</table>

<p>
<table border="0" cellpadding="0" cellspacing="0">
  <tr>
    <th width="310" valign="top"
	onMouseOver="return overlib('Shows a log of recent connection attempts.', LEFT);"
	onMouseOut="return nd();">
	Connection Log:&nbsp;&nbsp;
    </th>
    <td>&nbsp;&nbsp;</td>
    <td><% dumplog(); %></td>
  </tr>
</table>

<p class="label">&#169;2001-2012 Broadcom Corporation. All rights reserved.</p>

</body>
</html>
