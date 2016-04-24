var Speed=50;

var timer_internet=0;
var timer_wireless=0;
var timer_attached=0;
var timer_parental, timer_readyshare, timer_guest;

var Internet_bottom, Wireless_bottom, AttachedDevices_bottom;
var ParentalControls_bottom, ReadyShare_bottom, GuestNetwork_bottom;

var internet1, internet2;
var wireless1, wireless2, wireless3, wireless4;
var attached1, attached2;
var parental1, parental2;
var readyshare1, readyshare2;
var guest1, guest2, guest3, guest4;

var Internet_status;
var Wireless_status1, Wireless_status2;
var AttachedDevices_status;
var ParentalControls_status;
var ReadyShare_status;
var Guest_status1, Guest_status2;

function run_marquee() {
    Marquee_init();
    Marquee_set_timeInterval();
    Marquee_set_event();
}

function Marquee_init()
{
    Speed=50;

    timer_internet=0;
    timer_wireless=0;
    timer_attached=0;
    timer_parental=0;
    timer_readyshare=0;
    timer_guest=0;

    Internet_bottom = document.getElementById("Internet_bottom");
    Wireless_bottom = document.getElementById("Wireless_bottom");
    AttachedDevices_bottom = document.getElementById("AttachedDevices-bottom");
    ParentalControls_bottom = document.getElementById("ParentalControls-bottom");
    ReadyShare_bottom = document.getElementById("ReadyShare-bottom");
    GuestNetwork_bottom = document.getElementById("GuestNetwork-bottom");

    internet1= document.getElementById("internet1");
    internet2= document.getElementById("internet2");
    wireless1= document.getElementById("wireless1");
    wireless2= document.getElementById("wireless2");
    wireless3= document.getElementById("wireless3");
    wireless4= document.getElementById("wireless4");
    attached1= document.getElementById("attached1");
    attached2= document.getElementById("attached2");
    parental1= document.getElementById("parental1");
    parental2= document.getElementById("parental2");
    readyshare1= document.getElementById("readyshare1");
    readyshare2= document.getElementById("readyshare2");
    guest1= document.getElementById("guest1");
    guest2= document.getElementById("guest2");
    guest3= document.getElementById("guest3");
    guest4= document.getElementById("guest4");

    Internet_status= document.getElementById("Internet-status");
    Wireless_status1= document.getElementById("Wireless-status1");
    Wireless_status2= document.getElementById("Wireless-status2");
    AttachedDevices_status= document.getElementById("AttachedDevices-status");
    ParentalControls_status= document.getElementById("ParentalControls-status");
    ReadyShare_status= document.getElementById("ReadyShare-status");
    Guest_status1= document.getElementById("Guest-status1");
    Guest_status2= document.getElementById("Guest-status2");    
}

function Marquee_set_timeInterval()
{
    if(need_to_run_marquee("Internet")) {

        internet2.innerHTML=internet1.innerHTML;

        timer_internet=setInterval(picMarquee_interent,Speed);
    } 
    
   
    if(need_to_run_marquee("Wireless")) {
            $("#wireless1").css("text-align", "left");
            $("#wireless2").css("text-align", "left");  
            $("#wireless3").css("text-align", "left");  
            $("#wireless4").css("text-align", "left");  
            wireless2.innerHTML=wireless1.innerHTML;
            wireless4.innerHTML=wireless3.innerHTML;
            timer_wireless=setInterval(picMarquee_wireless,Speed);
        }    
        
    if(need_to_run_marquee("Attached")) {  
        attached2.innerHTML=attached1.innerHTML;
        timer_attached=setInterval(picMarquee_attached,Speed);
    }
        

    if(need_to_run_marquee("Parental")) {   
        parental2.innerHTML=parental1.innerHTML;
        timer_parental=setInterval(picMarquee_parental,Speed);
    }
    
    //alert(readyshare1.clientWidth + " " + ReadyShare_bottom.clientWidth);
    //alert(document.getElementById("ReadyShare-status").scrollWidth);
    //alert(document.getElementById("ReadyShare-condition").scrollWidth);
    
    if(need_to_run_marquee("ReadySHARE")) {
        readyshare2.innerHTML=readyshare1.innerHTML;
        timer_readyshare=setInterval(picMarquee_readyshare,Speed);
    }
    if(need_to_run_marquee("Guest1")) {
            $("#guest1").css("text-align", "left");
            $("#guest2").css("text-align", "left");
            guest2.innerHTML=guest1.innerHTML;
            timer_guest=setInterval(picMarquee_guest,Speed);
        }    
    if(need_to_run_marquee("Guest2")) {
            $("#guest3").css("text-align", "left");
            $("#guest4").css("text-align", "left");
            guest4.innerHTML=guest3.innerHTML;
            if(timer_guest==0)
                timer_guest=setInterval(picMarquee_guest,Speed);
        }  

}

/* handle mouseover and mouseout event */
function Marquee_set_event()
{
    if(timer_internet) {
        Internet_bottom.onmouseover=function(){
            clearInterval(timer_internet);
        }	
        Internet_bottom.onmouseout=function(){
            timer_internet=setInterval(picMarquee_interent,Speed);
        }        
    }
    if(wireless2 && wireless4 && timer_wireless) {
        Wireless_bottom.onmouseover=function(){
            clearInterval(timer_wireless);
        }
        Wireless_bottom.onmouseout=function(){
            timer_wireless=setInterval(picMarquee_wireless,Speed);
        }         
    }
    
    if(timer_attached) {
        AttachedDevices_bottom.onmouseover=function(){
            clearInterval(timer_attached);
        }
        AttachedDevices_bottom.onmouseout=function(){
            timer_attached=setInterval(picMarquee_attached,Speed);
        }        
    }
    if(timer_parental) {
        ParentalControls_bottom.onmouseover=function(){
            clearInterval(timer_parental);
        }
        ParentalControls_bottom.onmouseout=function(){
            timer_parental=setInterval(picMarquee_parental,Speed);
        }    
    }
    if(timer_readyshare) {
        ReadyShare_bottom.onmouseover=function(){
            clearInterval(timer_readyshare);
        }
        ReadyShare_bottom.onmouseout=function(){
            timer_readyshare=setInterval(picMarquee_readyshare,Speed);
        }    
    }
    
    if((guest2 || guest4) && timer_guest) {
        GuestNetwork_bottom.onmouseover=function(){
            clearInterval(timer_guest);
        }
        GuestNetwork_bottom.onmouseout=function(){
            timer_guest=setInterval(picMarquee_guest,Speed);
        }        
    }
}

function need_to_run_marquee(item)
{
    if(get_browser()=="Netscape") {
        if(item=="Internet" && Internet_status) {
            if (Internet_status.scrollWidth + 12 >
                Internet_bottom.clientWidth)
                return true;    
        } else if (item=="Wireless" && Wireless_status1) {
            if ((Wireless_status1.scrollWidth + 12 >
                Wireless_bottom.clientWidth) ||
                (Wireless_status2.scrollWidth + 12 >
                Wireless_bottom.clientWidth))
                return true;    
        } else if (item=="Attached" && AttachedDevices_status) {
            if (AttachedDevices_status.scrollWidth + 12 >
                AttachedDevices_bottom.clientWidth)
                return true;    
        } else if (item=="Parental" && ParentalControls_status) {      
            if (ParentalControls_status.scrollWidth + 12 >
                ParentalControls_bottom.clientWidth)
                return true;    
        } else if (item=="ReadySHARE" && ReadyShare_status) {
            if (ReadyShare_status.scrollWidth + 12 >
                ReadyShare_bottom.clientWidth)
                return true;
        } else if (item=="Guest1" && Guest_status1) {
            
            if (Guest_status1.scrollWidth + 12 >
                GuestNetwork_bottom.clientWidth)
                return true;    
        } else if (item=="Guest2" && Guest_status2) {
            if (Guest_status2.scrollWidth + 12 >
                GuestNetwork_bottom.clientWidth)
                return true;    
        }
    }
    else{
        if(item=="Internet") {
            if (internet1.clientWidth - Internet_bottom.clientWidth > 0)
                return true;    
        } else if (item=="Wireless") {
            if(wireless1 && wireless3 && wireless2 && wireless4 && Wireless_bottom &&
                (wireless1.clientWidth - Wireless_bottom.clientWidth > 0 ||
                wireless3.clientWidth - Wireless_bottom.clientWidth > 0) 
            ) 
                return true;    
        } else if (item=="Attached") {
            if (attached1.clientWidth - AttachedDevices_bottom.clientWidth > 0)
                return true;    
        } else if (item=="Parental") {
            if (parental1.clientWidth - ParentalControls_bottom.clientWidth > 0)
                return true;    
        } else if (item=="ReadySHARE") {
            if (readyshare1.clientWidth - ReadyShare_bottom.clientWidth > 0)
                return true;
        } else if (item=="Guest1") {
            if(guest1 && guest2)  
                if(guest1.clientWidth - GuestNetwork_bottom.clientWidth > 0)
                    return true;    
        } else if (item=="Guest2") {
            if(guest3 && guest4)
                if(guest3.clientWidth - GuestNetwork_bottom.clientWidth > 0) 
                    return true;    
        }    
    }

    return false;
}

/* Internet */	
function picMarquee_interent(){
    if(internet2.offsetWidth - Internet_bottom.scrollLeft <= 0){
        Internet_bottom.scrollLeft = 0;
    }else{
        var temp = Internet_bottom.scrollLeft;
        Internet_bottom.scrollLeft++;
        if(temp==Internet_bottom.scrollLeft)
            Internet_bottom.scrollLeft=0;
    }
}

/* Wireless */    
function picMarquee_wireless(){
    if(wireless2.offsetWidth - Wireless_bottom.scrollLeft <= 0){
        Wireless_bottom.scrollLeft = 0;
    }else{
        var temp = Wireless_bottom.scrollLeft;
        Wireless_bottom.scrollLeft++;
        if(temp==Wireless_bottom.scrollLeft)
            Wireless_bottom.scrollLeft=0;
    }
}


/* Attached Devices */
function picMarquee_attached(){
    if(attached2.offsetWidth - AttachedDevices_bottom.scrollLeft <= 0){
        AttachedDevices_bottom.scrollLeft = 0;
    }else{
        var temp = AttachedDevices_bottom.scrollLeft;
        AttachedDevices_bottom.scrollLeft++;
        if(temp==AttachedDevices_bottom.scrollLeft)
            AttachedDevices_bottom.scrollLeft=0;
    }
}

/* Parental Controls */
function picMarquee_parental(){

    if(parental2.offsetWidth - ParentalControls_bottom.scrollLeft <= 0){
        ParentalControls_bottom.scrollLeft = 0;
    }else{
        var temp = ParentalControls_bottom.scrollLeft;
        ParentalControls_bottom.scrollLeft++;
        if(temp==ParentalControls_bottom.scrollLeft)
            ParentalControls_bottom.scrollLeft=0;
    }
}

/* ReadySHARE */
function picMarquee_readyshare(){
    if(readyshare2.offsetWidth - ReadyShare_bottom.scrollLeft <= 0){
        ReadyShare_bottom.scrollLeft = 0;
    }else{
        var temp = ReadyShare_bottom.scrollLeft;
        ReadyShare_bottom.scrollLeft++;
        if(temp==ReadyShare_bottom.scrollLeft)
            ReadyShare_bottom.scrollLeft=0;
    }
}

/* Guest Network */    
function picMarquee_guest(){
    if(guest2.offsetWidth - GuestNetwork_bottom.scrollLeft <= 0){
        GuestNetwork_bottom.scrollLeft = 0;
    }else{
        var temp = GuestNetwork_bottom.scrollLeft;
        GuestNetwork_bottom.scrollLeft++;
        if(temp==GuestNetwork_bottom.scrollLeft)
            GuestNetwork_bottom.scrollLeft=0;
    }
}

function get_browser()
{
	if(navigator.userAgent.indexOf("Navigator") != -1) 
        return "Netscape";
    else if(navigator.userAgent.indexOf("MSIE") != -1)
		return "IE";
	else if(navigator.userAgent.indexOf("Chrome") != -1 )
		return "Chrome";//bug 21975:spec1.9-p228,[usb] the real links are different for different browsers
	else if(navigator.userAgent.indexOf("Firefox") != -1)
		return "Firefox";
	else if(navigator.userAgent.indexOf("Safari") != -1 )
		return "Safari";
	else if(navigator.userAgent.indexOf("Camino") != -1) 
		return "Camino"; 
 	else if(navigator.userAgent.indexOf("Gecko/") != -1)
   		return "Gecko"; 
	else if(navigator.userAgent.indexOf("Opera") != -1)
		return "Opera";
	else
   		return "";		
} 

var msg_invalid_ip = "<%10%>\n";
function isIPaddr(addr) {
    var i;
    var a;
    if(addr.split) {	
        a = addr.split(".");
    }else {	
        a = cdisplit(addr,".");
    }
    if(a.length != 4) {
        return false;
    }
    for(i = 0; i<a.length; i++) {
        var x = a[i];
        if( x == null || x == "" || !_isNumeric(x) || x<0 || x>255 ) {
            return false;
        }
    }
    return true;
}

function isValidIPaddr(addr) {
    var i;
    var a;
    if(addr.split) {
        a = addr.split(".");
    }else {
        a = cdisplit(addr,".");
    }
    if(a.length != 4) {
        return false;
    }
    for(i = 0; i<a.length; i++) {
        var x = a[i];
        if( x == null || x == "" || !_isNumeric(x) || x<0 || x>255 ) {
            return false;
        }
    }
    
    var ip1 = new String("");
    var ip2 = new String("");
    var ip3 = new String("");
    var ip4 = new String("");
    ip1.value = a[0];
    ip2.value = a[1];
    ip3.value = a[2];
    ip4.value = a[3];
    
    if(checkIP(ip1,ip2,ip3,ip4,254)||(parseInt(ip4.value,10)==0))
        return false;
    
    return true;
}

function isNetmask(mask1,mask2,mask3,mask4)
{
    var netMask;
    var i;
    var bit;
    var isChanged = 0;

    netMask = (mask1.value << 24) | (mask2.value << 16) | (mask3.value << 8) | (mask4.value);
    
    /* Check most byte (must be 255) and least significant bit (must be 0) */
    bit = (netMask & 0xFF000000) >>> 24;
    if (bit != 255)
        return false;
    
    bit = netMask & 1;
    if (bit != 0)
        return false;

    /* Now make sure the bit pattern changes from 0 to 1 only once */
    for (i=1; i<31; i++)
    {
        netMask = netMask >>> 1;
        bit = netMask & 1;

        if (bit != 0)
        {
            if (isChanged == 0)
                isChanged = 1;
        }
        else
        {
            if (isChanged == 1)
                return false;
        }
    }

    return true;
}

function trim(vString)
{ 
	var tString = vString;

	//trim left spaces
	if (tString.charAt(0) == " ")
		tString = trim(tString.substring(1, tString.length));

	//trim right spaces
	if (tString.charAt(tString.length-1) == " ")
		tString = trim(tString.substring(0, tString.length-1))

	return(tString);
}
function isNumOnly(vString)
{
	var NUMBERS = "0123456789";
	var valid = true;
	for(var i=0;i<vString.length;i++)
		if(NUMBERS.indexOf(vString.charAt(i))<0)
			valid = false;
			
	return(valid);
}
function isNull(vField) {
	var ret = false
	vField.value = trim(vField.value)
	
	if (vField.value.length == 0)
		ret = true
	return(ret)
}

function _isNumeric(str) {
    var i;

    if(str.length == 0 || str == null || str == "" || !isDecimalNumber(str)) {
        return false;
    }

    for(i = 0; i<str.length; i++) {
        var c = str.substring(i, i+1);
        if("0" <= c && c <= "9") {
            continue;
        }
        return false;
    }
    return true;
}

function isHex(str) {
    var i;
    for(i = 0; i<str.length; i++) {
        var c = str.substring(i, i+1);
        if(("0" <= c && c <= "9") || ("a" <= c && c <= "f") || ("A" <= c && c <= "F")) {
            continue;
        }
        return false;
    }
    return true;
}

/* Just check if MAC address is all "F", or all "0" , with  ':' in it or not weal @ Aug 14*/ 
function MacStrallf(mac) {
	var temp=mac.value;

    for(i=0; i<mac.value.length;i++) {
        var c = mac.value.substring(i, i+1)
        if (c == "f" || c == "F" || c == "0" || c == ":" || c == "-")
            continue;
		else
			break;
	}
	if (i == mac.value.length)
		return true;
	else
		return false;
}
function checkMacStr(mac) {
    if((mac.value.indexOf(':') != -1)||(mac.value.indexOf('-') != -1))
    {
        mac.value = mac.value.replace(/:/g,"");
        mac.value = mac.value.replace(/-/g,"");
    }
    var temp=mac.value;
    if(mac.value.length != 12) {
        if (mac.focus)
            mac.focus();
        return true;
    }
    for(i=0; i<mac.value.length;i++) {
        var c = mac.value.substring(i, i+1);
        if(("0" <= c && c <= "9") || ("a" <= c && c <= "f") || ("A" <= c && c <= "F")) {
            continue;
        }
        if (mac.focus)
            mac.focus();
        return true;
    }

    if(checkMulticastMAC(mac) || MacStrallf(mac))
    {
        if (mac.focus)
            mac.focus();
        return true;
    }

    mac.value = temp.toUpperCase();
    return false;
}
/* Check Mac Address Format*/
function checkMacMain(mac) {
    if(mac.value.length == 0) {
        if (mac.focus)
            mac.focus();
        return true;
    }
    for(i=0; i<mac.value.length;i++) {
        var c = mac.value.substring(i, i+1)
        if(("0" <= c && c <= "9") || ("a" <= c && c <= "f") || ("A" <= c && c <= "F")) {
            continue;
        }
        if (mac.focus)
            mac.focus();
        return true;
    }
    if(mac.value.length == 1) {
        mac.value = "0"+mac.value;
    }
    mac.value = mac.value.toUpperCase();
    return false;
}
function checkMacAddress(mac1, mac2, mac3, mac4, mac5, mac6) {
    if(checkMacMain(mac1)) return true;
    if(checkMacMain(mac2)) return true;
    if(checkMacMain(mac3)) return true;
    if(checkMacMain(mac4)) return true;
    if(checkMacMain(mac5)) return true;
    if(checkMacMain(mac6)) return true;
    var mac_str = new String("");
    mac_str.value =  mac1.value + mac2.value + mac3.value
                 + mac4.value + mac5.value + mac6.value;
    if(checkMulticastMAC(mac_str) || MacStrallf(mac_str))
        return true;
    
    return false;
}

/* Check Multicast MAC */
function checkMulticastMAC(macaddr) {
    var mac_defined = macaddr.value;
    var macadr_first_byte = parseInt(mac_defined.substring(0,2),16);
    var Multicast_Flag = macadr_first_byte & 0x01;
    if( Multicast_Flag )
        return true;
    return false;
}

/* Check IP Address Format*/
function checkIPMain(ip,max) {
    if( isNumeric(ip, max) ) {
        if (ip.focus)
            ip.focus();
        return true;
    }
}

function checkIP(ip1, ip2, ip3, ip4,max) {
    if(checkIPMain(ip1,255)) return true; 
    if(checkIPMain(ip2,255)) return true;
    if(checkIPMain(ip3,255)) return true;
    if(checkIPMain(ip4,max)) return true;

    if((ip1.value.charAt(0)=="0" && ip1.value.length!=1) ||
       (ip2.value.charAt(0)=="0" && ip2.value.length!=1) ||
       (ip3.value.charAt(0)=="0" && ip3.value.length!=1) ||
       (ip4.value.charAt(0)=="0" && ip4.value.length!=1)) 
        return true; 

    if((parseInt(ip1.value)==0)||
       ((parseInt(ip1.value)==0)&&(parseInt(ip2.value)==0)&&
        (parseInt(ip3.value)==0)&&(parseInt(ip4.value)==0)))
        return true;

    var loopback_ip_start   = (127 << 24)  |   (0 << 16)   |   (0 << 8)   |   (0);
    var loopback_ip_end     = (127 << 24)  |  (255 << 16)  |  (255 << 8)  | (255);
    var groupcast_ip_start  = (224 << 24)  |   (0 << 16)   |   (0 << 8)   |   (0);
    var groupcast_ip_end    = (239 << 24)  |  (255 << 16)  |  (255 << 8)  | (255);
    var dhcpresv_ip_start   = (169 << 24)  |  (254 << 16)  |   (0 << 8)   |   (0);
    var dhcpresv_ip_end     = (169 << 24)  |  (254 << 16)  |  (255 << 8)  | (255);
    var ip_addr = (ip1.value << 24) | (ip2.value << 16) | (ip3.value << 8) | (ip4.value);
    if((ip_addr >= loopback_ip_start)&&(ip_addr <= loopback_ip_end))
        return true;
    if((ip_addr >= groupcast_ip_start)&&(ip_addr <= groupcast_ip_end ))
        return true;
    if((ip_addr >= dhcpresv_ip_start)&&(ip_addr <= dhcpresv_ip_end))
        return true;

    return false;
}

function checkIPMatchWithNetmask(mask1,mask2,mask3,mask4,ip1, ip2, ip3, ip4)
{
    var netMask;
	var ipAddr;

    netMask = (mask1.value << 24) | (mask2.value << 16) | (mask3.value << 8) | (mask4.value);
    ipAddr = (ip1.value << 24) | (ip2.value << 16) | (ip3.value << 8) | (ip4.value);

    if (((~netMask)&ipAddr) == 0)
		return true;
	if (((~netMask)&ipAddr) == (~netMask))
		return true;
	return false;
}

function isDecimalNumber(str)
{
    if ((str.charAt(0)=='0') && (str.length != 1))
         return false;
    for(var i=0;i<str.length;i++)
    {
         if(str.charAt(i)<'0'||str.charAt(i)>'9')
         return false;
    }
    return true;
}

/* Check Numeric*/
function isNumeric(str, max) {
    if(str.value.length == 0 || str.value == null || str.value == "") {
        if (str.focus)
            str.focus();
        return true;
    }
    
    var i = parseInt(str.value,10);
    if(i>max) {
        if (str.focus)
            str.focus();
        return true;
    }
    for(i=0; i<str.value.length; i++) {
        var c = str.value.substring(i, i+1);
        if("0" <= c && c <= "9") {
            continue;
        }
        if (str.focus)
            str.focus();
        return true;
    }
    return false;
}

/* Check Blank*/
function isBlank(str) {
    if(str.value == "") {
        if (str.focus)
            str.focus();
        return true;
    } else 
        return false;
}

/* Check Phone Number*/
function isPhonenum(str) {
    var i;
    if(str.value.length == 0) {
        if (str.focus)
            str.focus();
        return true;
    }
    for (i = 0; i<str.value.length; i++) {
        var c = str.value.substring(i, i+1);
        if (c>= "0" && c <= "9")
            continue;
        if ( c == '-' && i !=0 && i != (str.value.length-1) )
            continue;
        if ( c == ',' ) continue;
        if (c == ' ') continue;
        if (c>= 'A' && c <= 'Z') continue;
        if (c>= 'a' && c <= 'z') continue;
        if (str.focus)
            str.focus();
        return true;
    }
    return false;
}

/* 0:close 1:open*/
function openHelpWindow(filename) {
    helpWindow = window.open(filename,"thewindow","width=300,height=400,scrollbars=yes,resizable=yes,menubar=no");
}

function alertPassword(formObj) {
    if (formObj.focus)
        formObj.focus();
}
function isEqual(cp1,cp2)
{
	if(parseInt(cp1.value,10) == parseInt(cp2.value,10))
	{
		if (cp2.focus)
			cp2.focus();
		return true;
	}	
	else return false;
}
function setDisabled(OnOffFlag,formFields)
{
	for (var i = 1; i < setDisabled.arguments.length; i++)
		setDisabled.arguments[i].disabled = OnOffFlag;
}

function cp_ip(from1,from2,from3,from4,to1,to2,to3,to4)
//true invalid from and to ip;  false valid from and to ip;
{
    var total1 = 0;
    var total2 = 0;
    
    total1 += parseInt(from4.value,10);
    total1 += parseInt(from3.value,10)*256;
    total1 += parseInt(from2.value,10)*256*256;
    total1 += parseInt(from1.value,10)*256*256*256;
    
    total2 += parseInt(to4.value,10);
    total2 += parseInt(to3.value,10)*256;
    total2 += parseInt(to2.value,10)*256*256;
    total2 += parseInt(to1.value,10)*256*256*256;
    if(total1 > total2)
        return true;
    return false;
}
function pi(val)
{
    return parseInt(val,10);
}    
function alertR(str)    
{
    alert(str);
    return false;
}    
    