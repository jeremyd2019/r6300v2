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
	
var counter = "<% nvram_get("wfi_count"); %>";
var pinmode = "<% nvram_match("wfi_pinmode", "1", "1"); %>";
var manual_pin = "<% nvram_match("wfi_manual_pin", "1", "1"); %>";

function setTimer()
{
	if(!pinmode || manual_pin ){
  		setTimeout ( "checkTimer()", 5000 );
  	}
}

function checkTimer()
{
	document.forms[0].action.value = "Check";
	document.forms[0].count.value = (parseInt(counter) - 5);
	document.forms[0].submit();
}
function inputPin()
{
	if(pinmode == 1 && manual_pin == 0){
		document.writeln("Please enter the PIN, then click the \"<b>Enter</b>\" button.</p><p>");
		document.writeln("<table border=\"0\" cellpadding=\"0\" cellspacing=\"0\">");
		document.writeln("<tr>");
		document.writeln("<th width=\"310\" onMouseOver=\"return overlib(\'Wifi-Invite PIN Mode.\', LEFT);\" onMouseOut=\"return nd();\">");
		document.writeln("Wifi-Invite PIN Mode:&nbsp;&nbsp;");
		document.writeln("</th>");
		document.writeln("<td>&nbsp;&nbsp;</td>");
		document.writeln("<td> Manual </td>");
		document.writeln("</tr>");
		document.writeln("<tr>");
		document.writeln("<th width=\"310\">PIN Number:&nbsp;&nbsp;</th>");
		document.writeln("<td>&nbsp;&nbsp;</td>");
		document.writeln("<td>");
		document.writeln("<input name=\"wps_sta_pin\" value=\"\" size=\"8\" maxlength=\"8\">");
		document.writeln("</td>");
		document.writeln("</tr>");
		document.writeln("<tr>");
		document.writeln("<th width=\"310\">&nbsp;");
		document.writeln("</th>");
		document.writeln("<td>&nbsp;&nbsp;</td>");
		document.writeln("<td>");
		document.writeln("<input type=\"button\" name=\"button2\" value=\"Enter\" onclick=\"button2Clicked();\" style=\"height: 25px; width: 100px\">");
		document.writeln("</td>");
		document.writeln("</tr>");
		document.writeln("</table>");
 	}
 	else {
 		document.writeln("Waiting for inviting status...");
 	}
	return;
}

function button2Clicked()
{
	
	if (document.forms[0].wps_sta_pin.value.length < 8)
	{
		alert("Please Enter 8 digits PIN number!");
 		return false;
	}
	document.forms[0].action.value = "Enter";
	document.forms[0].submit();
}

//-->

</script>
</head>

<body onLoad="setTimer();">
<div id="overDiv" style="position:absolute; visibility:hidden; z-index:1000;"></div>

<form method="post" action="apply.cgi">
<input type="hidden" name="page" value="cancelinvite.asp">
<input type="hidden" name="count" value=0>
<input type="hidden" name="action" value="Cancel">

<p>Inviting the STA...</p>
<p>
	  <script language="JavaScript" type="text/javascript">
		inputPin();
	  </script>
</p>

<p>If you want to cancel the invitation, click the "<b>Cancel</b>" button.</p>
<input type="submit" name="button1" value="Cancel" style="height: 25px; width: 100px">
</form>

<p class="label">&#169;2001-2012 Broadcom Corporation. All rights
reserved. 54g and XPress are trademarks of Broadcom Corporation.</p>

</body>
</html>
