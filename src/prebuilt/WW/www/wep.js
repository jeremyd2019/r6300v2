// ---------------------------- Start WEP functions
var lastkeypressed;
var keyTooBig = false;
var keyTooBig_an = false;
var mustbeHEX = false;
var keysize;
var keysize_an;
var lastObj;
if (document.layers) document.captureEvents(Event.KEYPRESS);
    document.onkeypress = checkKey;
    
function checkKey(evt)
{
    evt = (evt) ? evt : ((window.event) ? window.event : null);
    if (evt)
        lastkeypressed = (evt.keyCode) ? evt.keyCode : (evt.which ) ? evt.which : null;
    if ((lastkeypressed != 13) && (lastkeypressed != 8) && ( keyTooBig ))
    {
        keyTooBig = false;
        alert("<%1373%> " + keysize + " <%1374%>");
        chkSize(lastObj); // for NS 6/7
        return false;
    }
    else if ((lastkeypressed != 13) && (lastkeypressed != 8) && ( keyTooBig_an ))
    {
        keyTooBig_an = false;
        alert("<%1373%> " + keysize_an + " <%1374%>");
        chkSize_an(lastObj); // for NS 6/7
        return false;
    }
    else if (mustbeHEX) // not used here, don't know which input is being used
    {
        mustbeHEX = false;
        if ( ((lastkeypressed > 47) && (lastkeypressed < 58 ))
            || ((lastkeypressed > 96) && (lastkeypressed < 103))
            || ((lastkeypressed > 64) && (lastkeypressed < 71 ))
            || (lastkeypressed == 8)
            || (lastkeypressed == 13) )
            return true; // OK
        else
            return false;
    }
    return true;
}
function chkSize(fobj)
{
    if(fobj.value.length > keysize)
        fobj.value = fobj.value.substr(0,keysize);
}
function chkSize_an(fobj)
{
    if(fobj.value.length > keysize_an)
        fobj.value = fobj.value.substr(0,keysize_an);
}
function keyCheck(fobj)
{
    lastObj = fobj;
    keyTooBig = (fobj.value.length >= keysize ) ? true : false;
}
function keyCheck_an(fobj)
{
    lastObj = fobj;
    keyTooBig_an = (fobj.value.length >= keysize_an) ? true : false;
}
function calcPassphrase(F)
{
    if(F.passphraseStr.value.length == 0)
        return;
    switch(F.wepenc.selectedIndex)
    {
        case 0:
            PassPhrase40(F);
            break;
        case 1:
            PassPhrase104(F);
            break;
        default:
            break;
    }
}
function PassPhrase40(F)
{
    var seed = 0;
    var pseed = new Array(0, 0, 0, 0);
    var pkey = new Array(4);
    var asciiObj = new Array(4);
    Length = F.passphraseStr.value.length;
    if(Length != 0)
    {
        for (i=0; i<Length; i++ )
            pseed[i%4] ^= F.passphraseStr.value.charCodeAt(i);
        seed = pseed[0];
        seed += pseed[1] << 8;
        seed += pseed[2] << 16;
        seed += pseed[3] << 24;
    }
    F.KEY1.value = F.KEY2.value = "";
    F.KEY3.value = F.KEY4.value = "";
    pkey[0] = F.KEY1;
    pkey[1] = F.KEY2;
    pkey[2] = F.KEY3;
    pkey[3] = F.KEY4;
    for(j=0; j<4; j++)
    {
        for (i=0; i<5 ;i++ )
        {
            seed = (214013 * seed) & 0xffffffff;
            if(seed & 0x80000000)
                seed = (seed & 0x7fffffff) + 0x80000000 + 0x269ec3;
            else
                seed = (seed & 0x7fffffff) + 0x269ec3;
            temp = ((seed >> 16) & 0xff);
            if(temp < 0x10)
                pkey[j].value += "0" + temp.toString(16).toUpperCase();
            else
                pkey[j].value += temp.toString(16).toUpperCase();
        }
    }
    F.wep_key_no[0].checked = true;
}
function PassPhrase104(F)
{
    var pseed2 = "";
    Length2 = F.passphraseStr.value.length;
    var asciiObj = "";
    for(p=0; p<64; p++)
    {
        tempCount = p % Length2;
        pseed2 += F.passphraseStr.value.substring(tempCount, tempCount+1);
    }
    md5Str = calcMD5(pseed2);
    F.KEY1.value = md5Str.substring(0, 26).toUpperCase();
    F.KEY2.value = F.KEY1.value;
    F.KEY3.value = F.KEY1.value;
    F.KEY4.value = F.KEY1.value;
    F.wep_key_no[0].checked = true;
}
function calcPassphrase_an(F)
{
    if(F.passphraseStr_an.value.length == 0)
        return;
    switch(F.wepenc_an.selectedIndex)
    {
        case 0:
            PassPhrase40_an(F);
            break;
        case 1:
            PassPhrase104_an(F);
            break;
        default:
            break;
    }
}
function PassPhrase40_an(F)
{
    var seed = 0;
    var pseed = new Array(0, 0, 0, 0);
    var pkey = new Array(4);
    var asciiObj = new Array(4);
    Length = F.passphraseStr_an.value.length;
    if(Length != 0)
    {
        for (i=0; i<Length; i++ )
            pseed[i%4] ^= F.passphraseStr_an.value.charCodeAt(i);
        seed = pseed[0];
        seed += pseed[1] << 8;
        seed += pseed[2] << 16;
        seed += pseed[3] << 24;
    }
    F.KEY1_an.value = F.KEY2_an.value = "";
    F.KEY3_an.value = F.KEY4_an.value = "";
    pkey[0] = F.KEY1_an;
    pkey[1] = F.KEY2_an;
    pkey[2] = F.KEY3_an;
    pkey[3] = F.KEY4_an;
    for(j=0; j<4; j++)
    {
        for (i=0; i<5 ;i++ )
        {
            seed = (214013 * seed) & 0xffffffff;
            if(seed & 0x80000000)
                seed = (seed & 0x7fffffff) + 0x80000000 + 0x269ec3;
            else
                seed = (seed & 0x7fffffff) + 0x269ec3;
            temp = ((seed >> 16) & 0xff);
            if(temp < 0x10)
                pkey[j].value += "0" + temp.toString(16).toUpperCase();
            else
                pkey[j].value += temp.toString(16).toUpperCase();
        }
    }
    F.wep_key_no_an[0].checked = true;
}
function PassPhrase104_an(F)
{
    var pseed2 = "";
    Length2 = F.passphraseStr_an.value.length;
    var asciiObj = "";
    for(p=0; p<64; p++)
    {
        tempCount = p % Length2;
        pseed2 += F.passphraseStr_an.value.substring(tempCount, tempCount+1);
    }
    md5Str = calcMD5(pseed2);
    F.KEY1_an.value = md5Str.substring(0, 26).toUpperCase();
    F.KEY2_an.value = F.KEY1_an.value;
    F.KEY3_an.value = F.KEY1_an.value;
    F.KEY4_an.value = F.KEY1_an.value;
    F.wep_key_no_an[0].checked = true;
}
