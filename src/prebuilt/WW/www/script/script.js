
    function buttonFilter()
    {
        var buttonElements,button;
        var name;


        buttonElements=document.getElementsByTagName('button');
        var i;
        for(i=0;i<buttonElements.length;i++)
        {
            if(buttonElements[i].type=='submit' || buttonElements[i].type=='button'|| buttonElements[i].type=='reset')
            {
                if((buttonElements[i].name!=document.forms[0].buttonHit.value))
                    buttonElements[i].disabled=1;
                else
                {
//                    buttonElements[i].value=document.forms[0].buttonValue.value;
                      name=buttonElements[i].name;
                      buttonElements[i].name='NoUse';
                      buttonElements[i].disabled=1;
                }
            }
        }

        button=document.getElementsByName('buttonValue');
        button[0].name=name;
        button=document.getElementsByName('buttonHit');
        button[0].disabled=1;

    }

    function buttonClick(btn,value)
    {

        var button;
        
        button=document.getElementsByName('buttonHit');
        button[0].value=btn.name;

        button=document.getElementsByName('buttonValue');
        button[0].value=value;
        return true;
    }

  function clickButton(message)
  {
      alert(message);
  }
  
  function mainOnload()
  {
  	
  }



  function changeCursorPointer()
  {
  	document.body.style.cursor='pointer';
  }

  function changeCursorDefault()
  {
  	document.body.style.cursor='default';
  }


  function iframeResize(iframe){
alert("Enter iframeResize "+iframe);
              if(iframe && !window.opera){
              	
      	          if(iframe.contentDocument && iframe.contentDocument.body.offsetHeight){      	          		
alert('before '+iframe.height+" document "+iframe.Document.body.offsetHeight);
                      iframe.height=iframe.contentDocument.body.offsetHeight+80;  
alert('after '+iframe.height);
                  }
        	        else if(iframe.Document && iframe.Document.body.scrollHeight){
alert('before '+iframe.style.height+" document "+iframe.Document.body.scrollHeight);

      	              iframe.style.height=iframe.Document.body.scrollHeight;
alert('after '+iframe.style.height);


                  }
      	      }
alert("Exit iframeResize ");
      
  }

function getElementsByName_iefix(tag, name) {
     var elem = document.getElementsByTagName(tag);
     var arr = new Array();
     for(i = 0,iarr = 0; i < elem.length; i++) {
          att = elem[i].getAttribute("name");
          if(att == name) {
               arr[iarr] = elem[i];
               iarr++;
          }
     }
     return arr;
}  
  
function grayout_button(ButtonName)
{
    var selectedButton=$('#'+ButtonName);
    var selectedButtonChildLeft = selectedButton.children(".roundleft_button");
    var selectedButtonChildRight = selectedButton.children(".roundright_button");
    
    selectedButton.attr('class', 'button-rule');
    selectedButtonChildLeft.attr('class', 'roundleft_grey');
    selectedButtonChildRight.attr('class', 'roundright_grey');
    
    selectedButton[0].disabled = true;
}

function ungrayout_button(ButtonName)
{
    var selectedButton=$('#'+ButtonName);
    var selectedButtonChildLeft = selectedButton.children(".roundleft_grey");
    var selectedButtonChildRight = selectedButton.children(".roundright_grey");
    
    selectedButton.attr('class', 'button-grey');
    selectedButtonChildLeft.attr('class', 'roundleft_button');
    selectedButtonChildRight.attr('class', 'roundright_button');
    
    selectedButton[0].disabled = false;
}  

function Security5G_disabled()
{
	$("input[name=security_type_an][@type=radio]").attr("disabled", "true");	
}
function Security5G_enabled()
{
	$("input[name=security_type_an][@type=radio]").attr("disabled", "");
}
function WPS_wizard_grayout()
{
    if(parent.$('#wps-wizard').hasClass('noSubLarge')){
        parent.$('#wps-wizard').addClass('noSubGrayLarge');
        parent.$('#wps-wizard').removeClass('noSub');
        parent.$('#wps-wizard').removeClass('noSubLarge');
    } else if(parent.$('#wps-wizard').hasClass('noSub')){
        parent.$('#wps-wizard').addClass('noSubGray');
        parent.$('#wps-wizard').removeClass('noSub');
        parent.$('#wps-wizard').removeClass('noSubLarge');
    }
}

function WPS_wizard_ungrayout()
{
    if(parent.$('#wps-wizard').hasClass('noSubGrayLarge')){
        parent.$('#wps-wizard').addClass('noSubLarge');
        parent.$('#wps-wizard').removeClass('noSubGrayLarge');
    } else if(parent.$('#wps-wizard').hasClass('noSubGray')){
        parent.$('#wps-wizard').addClass('noSub');
        parent.$('#wps-wizard').removeClass('noSubGray');
    }
}

function WDS_wizard_grayout()
{
    if(parent.$("#basic-int").hasClass("basic-menu-div")){
        parent.$("#basic-int").addClass('basic-menu-div-gray');
        parent.$("#basic-int").removeClass('basic-menu-div');
    } 

    if(parent.$('#wizard').hasClass('noSubLarge')){
        parent.$('#wizard').addClass('noSubGrayLarge');
        parent.$('#wizard').removeClass('noSub');
        parent.$('#wizard').removeClass('noSubLarge');
    } else if(parent.$('#wizard').hasClass('noSub')){
        parent.$('#wizard').addClass('noSubGray');
        parent.$('#wizard').removeClass('noSub');
        parent.$('#wizard').removeClass('noSubLarge');
    } 

    parent.$('.SubMenuWDS').each(function(){  
        if($(this).hasClass('SubMenuLarge')){
            $(this).addClass('SubMenuLargeDisable');
            $(this).removeClass('SubMenu');
            $(this).removeClass('Large');
            $(this).removeClass('SubMenuLarge');
        }                
        else if($(this).hasClass('SubMenu')){ 
            $(this).addClass('SubMenuDisable');
            $(this).removeClass('SubMenu');
        }
    }); 
}

function WDS_wizard_ungrayout()
{
    if(parent.$("#basic-int").hasClass("basic-menu-div-gray")){
        parent.$("#basic-int").addClass('basic-menu-div');
        parent.$("#basic-int").removeClass('basic-menu-div-gray');
    } 

    if(parent.$('#wizard').hasClass('noSubGrayLarge')){
        parent.$('#wizard').addClass('noSubLarge');
        parent.$('#wizard').removeClass('noSubGrayLarge');
    } else if(parent.$('#wizard').hasClass('noSubGray')){
        parent.$('#wizard').addClass('noSub');
        parent.$('#wizard').removeClass('noSubGray');
    } 

    parent.$('.SubMenuWDS').each(function(){
        if($(this).hasClass('SubMenuLargeDisable')){
            $(this).addClass('SubMenuLarge');
            $(this).addClass('SubMenu');
            $(this).addClass('Large');
            $(this).removeClass('SubMenuLargeDisable');
        }
        else if($(this).hasClass('SubMenuDisable')){
            $(this).addClass('SubMenu');
            $(this).removeClass('SubMenuDisable');
        }
    });
}

function sta_mode_grayout()
{
    if(parent.$("#basic-int").hasClass("basic-menu-div")){
        parent.$("#basic-int").addClass('basic-menu-div-gray');
        parent.$("#basic-int").removeClass('basic-menu-div');
    }
	if(parent.$("#basic-wls").hasClass("basic-menu-div")){
		parent.$("#basic-wls").addClass('basic-menu-div-gray');
        parent.$("#basic-wls").removeClass('basic-menu-div');
	}
	if(parent.$("#basic-par").hasClass("basic-menu-div")){
	    parent.$("#basic-par").addClass('basic-menu-div-gray');
        parent.$("#basic-par").removeClass('basic-menu-div');
	}
	if($("#basic-gst").hasClass("basic-menu-div")){
	    parent.$("#basic-gst").addClass('basic-menu-div-gray');
        parent.$("#basic-gst").removeClass('basic-menu-div');
	}

    if(parent.$('#wizard').hasClass('noSubLarge')){
        parent.$('#wizard').addClass('noSubGrayLarge');
        parent.$('#wizard').removeClass('noSub');
        parent.$('#wizard').removeClass('noSubLarge');
    } else if(parent.$('#wizard').hasClass('noSub')){
        parent.$('#wizard').addClass('noSubGray');
        parent.$('#wizard').removeClass('noSub');
        parent.$('#wizard').removeClass('noSubLarge');
    } 
	if(parent.$('#wps-wizard').hasClass('noSubLarge')){
        parent.$('#wps-wizard').addClass('noSubGrayLarge');
        parent.$('#wps-wizard').removeClass('noSub');
        parent.$('#wps-wizard').removeClass('noSubLarge');
    } else if(parent.$('#wps-wizard').hasClass('noSub')){
        parent.$('#wps-wizard').addClass('noSubGray');
        parent.$('#wps-wizard').removeClass('noSub');
        parent.$('#wps-wizard').removeClass('noSubLarge');
    }
	if(parent.$('#wireless_set').hasClass('SubMenu')){
        parent.$('#wireless_set').addClass('SubMenuDisable');
        parent.$('#wireless_set').removeClass('SubMenu');
    }
	if(parent.$('#guest_set').hasClass('SubMenu')){
        parent.$('#guest_set').addClass('SubMenuDisable');
        parent.$('#guest_set').removeClass('SubMenu');
    }
	if(parent.$('#wds_head').hasClass('SubMenu')){
        parent.$('#wds_head').addClass('SubMenuDisable');
        parent.$('#wds_head').removeClass('SubMenu');
    }
	if(parent.$('#Qos_set').hasClass('SubMenu')){
        parent.$('#Qos_set').addClass('SubMenuDisable');
        parent.$('#Qos_set').removeClass('SubMenu');
    }

    parent.$('.SubMenuWDS').each(function(){                
        if($(this).hasClass('SubMenuAP')){
            $(this).addClass('SubMenuDisable');
            $(this).removeClass('SubMenuAP');
        }
        else if($(this).hasClass('SubMenuLarge')){
            $(this).addClass('SubMenuLargeDisable');
            $(this).removeClass('SubMenu');
            $(this).removeClass('Large');
            $(this).removeClass('SubMenuLarge');
        }                
        else if($(this).hasClass('SubMenu')){ 
            $(this).addClass('SubMenuDisable');
            $(this).removeClass('SubMenu');
        }
    }); 
}

function ap_mode_grayout()
{
    if(parent.$("#basic-int").hasClass("basic-menu-div")){
        parent.$("#basic-int").addClass('basic-menu-div-gray');
        parent.$("#basic-int").removeClass('basic-menu-div');
    }      

    if(parent.$('#wizard').hasClass('noSubLarge')){
        parent.$('#wizard').addClass('noSubGrayLarge');
        parent.$('#wizard').removeClass('noSub');
        parent.$('#wizard').removeClass('noSubLarge');
    } else if(parent.$('#wizard').hasClass('noSub')){
        parent.$('#wizard').addClass('noSubGray');
        parent.$('#wizard').removeClass('noSub');
        parent.$('#wizard').removeClass('noSubLarge');
    } 

    parent.$('.SubMenuWDS').each(function(){                
        if($(this).hasClass('SubMenuAP')){
            /* do nothing */
        }
        else if($(this).hasClass('SubMenuLarge')){
            $(this).addClass('SubMenuLargeDisable');
            $(this).removeClass('SubMenu');
            $(this).removeClass('Large');
            $(this).removeClass('SubMenuLarge');
        }                
        else if($(this).hasClass('SubMenu')){ 
            $(this).addClass('SubMenuDisable');
            $(this).removeClass('SubMenu');
        }
    }); 
}

function ap_mode_ungrayout()
{
    if(parent.$("#basic-int").hasClass("basic-menu-div-gray")){
        parent.$("#basic-int").addClass('basic-menu-div');
        parent.$("#basic-int").removeClass('basic-menu-div-gray');
    } 

    if(parent.$('#wizard').hasClass('noSubGrayLarge')){
        parent.$('#wizard').addClass('noSubLarge');
        parent.$('#wizard').removeClass('noSubGrayLarge');
    } else if(parent.$('#wizard').hasClass('noSubGray')){
        parent.$('#wizard').addClass('noSub');
        parent.$('#wizard').removeClass('noSubGray');
    } 
    
    parent.$('.SubMenuWDS').each(function(){
        if($(this).hasClass('SubMenuLargeDisable')){
            $(this).addClass('SubMenuLarge');
            $(this).addClass('SubMenu');
            $(this).addClass('Large');
            $(this).removeClass('SubMenuLargeDisable');
        }
        else if($(this).hasClass('SubMenuDisable')){
            $(this).addClass('SubMenu');
            $(this).removeClass('SubMenuDisable');
        }
    });
}

function highLightMenu(title, subtitle)
{
    if(parent.load_page == 1 || !parent.load_page)
        return false;
        
    var checkElement = parent.$('#'+title).next();  
    parent.$("#first").hide();

    if((checkElement.is('li')) && (!checkElement.is(':visible'))) 
    { 
        parent.$('.subHeader:visible').slideUp('fast');
        checkElement.slideDown('normal');      
        parent.$('.SubActive').addClass('Sub');
        parent.$('.SubActive').removeClass('SubActive');
        parent.$('#'+title).addClass('SubActive');
        parent.$('#'+title).removeClass('Sub');
    }
       
    if( !parent.$('#'+subtitle).hasClass('SubMenuDisable') && !parent.$('#'+subtitle).hasClass('SubMenuLargeDisable'))
    {  
        parent.$('.noSubActive').removeClass('noSubActive');
        parent.$('.noSubActiveLarge').removeClass('noSubActiveLarge');
        parent.$('.SubMenuActive').addClass('SubMenu');
        parent.$('.SubMenuActive').removeClass('SubMenuActive');
        parent.$('.SubMenuActiveLarge').addClass('SubMenuLarge');
        parent.$('.SubMenuActiveLarge').addClass('SubMenu');
        parent.$('.SubMenuActiveLarge').addClass('Large');
        parent.$('.SubMenuActiveLarge').removeClass('SubMenuActiveLarge');
        if(parent.$('#'+subtitle).hasClass('Large'))
           parent.$('#'+subtitle).addClass('SubMenuActiveLarge');
        else
           parent.$('#'+subtitle).addClass('SubMenuActive');
        parent.$('#'+subtitle).removeClass('SubMenu');
        parent.$('#'+subtitle).removeClass('SubMenuLarge');

     }
     else
     {     
        return false;
     }                           
} 

function change_size()
{
    var sep_border_num = $(".table-seperate-border").length;
    var min_height = $('.scroll-pane').css("height");
    min_height = min_height.replace("px", "");
         

    var width = document.documentElement.clientWidth > 620 ? document.documentElement.clientWidth : 620;
    var height = document.documentElement.clientHeight > min_height ? document.documentElement.clientHeight : min_height;

    $('.subtop-image').css("width", width);
    $('.subtop-image').css("height", "32px");
    
    $('.body-image').css("width", width);
    $('.body-image').css("height", height-30);
    $('.body-image').css("position", "absolute");
    $('.body-image').css("top", 5);
    
    $('.subfooter-image').css("width", width);
    $('.subfooter-image').css("height", "24px");   
    $('.subfooter-image').css("position", "relative");
    $('.subfooter-image').css("top", -3);    
    
    $('.subhead2-table').css("position", "relative");
    $('.subhead2-table').css("top", -3);
    
    if(get_browser()=="Netscape") 
        $('.subhead2-table').css("left", 1);
    $('.subhead2-table').css("width", width-27);
    $('.scroll-pane').css("width", width-27);
    
    if(document.getElementById("topframe")) {    
        $('.subhead2-table').css("height", height-226);
        $('.scroll-pane').css("height", height-226);   
    } else if(sep_border_num==2) {
        $('.subhead2-table').css("height", height-120);
        $('.scroll-pane').css("height", height-120);    
    } else {
        $('.subhead2-table').css("height", height-75);
        $('.scroll-pane').css("height", height-75);        
    }
    
    $('.subhead2-bottom').css("width", width);
    
    $('.button-help-arrow').css("position", "absolute");
    $('.button-help-arrow').css("left", width/2);
        
    
    $('.bas-help-frame-div').css("width", width-50); 
    $('.help-frame-div').css("width", width-50); 
    if(get_browser()=="Firefox"|| get_browser()=="Netscape") { 
        $('.bas-help-frame-div').css("top", height-225);
        $('.help-frame-div').css("top", height-225);
    }    
    else {
        $('.bas-help-frame-div').css("top", height-230);
        $('.help-frame-div').css("top", height-230);
    }
        
    $('#helpframe').css("width", width-50);


    
    
    if(document.getElementById('helpframe')) {
        try{
            document.getElementById('helpframe').contentWindow.change_size_helpPage(width);
        }catch(e){}
    }
    
    
    $('.cover-image').css("display", "none");    
        
}

function change_size_helpPage(width)
{
    $('#content').css("width", width -70+ "px");

    if(get_ie_ver() == 9){
       ;
    }else{
       $('#content').jScrollPane('scrollbarMargin:5px');
    }
    var isResizing;
	// IE triggers the onResize event internally when you do the stuff in this function
	// so make sure we don't enter an infinite loop and crash the browser
	if (!isResizing) { 
		isResizing = true;
		$w = $(window);
		$c = $('#container');
		var p = (parseInt($c.css('paddingLeft')) || 0) + (parseInt($c.css('paddingRight')) || 0);
		$('body>.jScrollPaneContainer').css({'width': $w.width() + 'px'});
		$c.css({'width': ($w.width() - p) + 'px', 'overflow':'auto'});
		$c.jScrollPane();
		isResizing = false;	
	} 
}   
function change_size_ADVPage()
{
    var isResizing;
	// IE triggers the onResize event internally when you do the stuff in this function
	// so make sure we don't enter an infinite loop and crash the browser
    if (!isResizing) {
        isResizing = true;
        $w = $(window);
        $c = $('#scroll-pane');
        var p = (parseInt($c.css('paddingLeft')) || 0) + (parseInt($c.css('paddingRight')) || 0);
        $('body>.jScrollPaneContainer').css({'height': $w.height() + 'px'});
        if(get_browser()=="Chrome")
            $c.css({'height': ($w.height()-p) + 'px', 'overflow':'no'});
        else
            $c.css({'height': ($w.height()-p) + 'px', 'overflow':'auto'});
        $c.jScrollPane();
        isResizing = false;
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

function get_ie_ver()
{
    var version = 999; // we assume a sane browser
    if (navigator.appVersion.indexOf("MSIE") != -1)
		// bah, IE again, lets downgrade version number
		version = parseFloat(navigator.appVersion.split("MSIE")[1]);
    return version;
}

function setLanglistPosition()
{
    if(get_browser()=="Firefox")
    {
        if($('.tabs')[0].scrollHeight>40)
        {
            $('#lang_menu_li').css("position", "relative");
            $('#lang_menu_li').css("top", "-35px");
            $('#firmware-update').css("position", "relative");
            $('#firmware-update').css("left", "-50px");
        }
        else
        {
            $('#lang_menu_li').css("position", "relative");
            $('#lang_menu_li').css("top", "0px"); 
            $('#firmware-update').css("position", "relative");
            $('#firmware-update').css("left", "0px");            
        }
    }
    else if(get_browser()=="IE")
    {
        if($('.tabs')[0].scrollHeight>40)
        {
            $('#lang_menu_li').css("position", "relative");
            $('#lang_menu_li').css("top", "-43px");
            $('#firmware-update').css("position", "relative");
            $('#firmware-update').css("left", "0px");
        }
        else
        {
            
            $('#lang_menu_li').css("position", "relative");
            $('#lang_menu_li').css("top", "0px"); 
            $('#firmware-update').css("position", "relative");
            $('#firmware-update').css("left", "0px");            
        }  
    }
}


function setFooterClass()
{
	var footer_div = top.document.getElementById("footer");
	var content = footer_div.innerHTML.replace(/<\/?.+?>/g,"").replace(/[\r\n]/g, "").replace(/\s+/g, "");
	var content_len = content.length;
	var width = document.body.clientWidth;


	if( width > 967 ){
		footer_div.className = "footer";
	}
	else{

		if(content_len > 75)
        {
			footer_div.className = "footer_double";
            $('#container').css("top", "653px");
		}
        else
        {
            footer_div.className = "footer";
            $('#container').css("top", "610px");
        }
	}
	
	var go_btn = top.document.getElementById("search_button");
	content_len = go_btn.value.length;
	
	if(content_len >= 7)
		go_btn.className = "search_button_long";
	else if(content_len >= 4)
		go_btn.className = "search_button_middle";
	else 
		go_btn.className = "search_button";
	
	var width = document.getElementById("support").clientWidth + document.getElementById("search").clientWidth;
	var screen_width = document.body.clientWidth;

	if( width < screen_width - 60 )
    {
		footer_div.className = "footer";
        $('#container').css("top", "710px");
	}
    else
    {
		footer_div.className = "footer_double";
        $('#container').css("top", "753px");
	}
}

function subpage_resize()
{
   
    var page_contain_width = document.body.clientWidth-220 > 735 ? document.body.clientWidth-220 : 735;
    var page_contain_height = document.documentElement.clientHeight-160 > 470+adjustPageHeight ? document.documentElement.clientHeight-160 : 470+adjustPageHeight;
	var footer_div = document.getElementById("footer");
	var is_double = ( footer_div.className == "footer_double") ;

    if(is_double)
    {
        page_contain_width = page_contain_width - 50 > 735 ? page_contain_width - 50 : 735;
        page_contain_height = page_contain_height - 50> 470+adjustPageHeight ? page_contain_height - 50 : 470+adjustPageHeight;
    }
     
    $('.basic-menu').css("height", page_contain_height);
         

    var page_width = page_contain_width - 50;
    var page_height = page_contain_height - 0;         
    
    if($("#BasicTab").hasClass('current')){
        $('.basic-menu').css("height", page_contain_height);
        $('#page_contain').width(parseInt(page_contain_width));  
        $('#page_contain').height(parseInt(page_contain_height));  
        $('#page_contain').css("margin-top","5");            
        $('#page').attr("scrolling","no");
        $('#page').width(parseInt(page_width));
        $('#page').height(parseInt(page_height));
        $('#page').css("margin-top","0");
        $('#page').css("margin-left","30");   
    } else {
        $('.advance-menu').css("height", page_contain_height);
        $('#page_contain2').width(parseInt(page_contain_width));  
        $('#page_contain2').height(parseInt(page_contain_height));  
        $('#page_contain2').css("margin-top","5");            
        $('#page2').attr("scrolling","no");
        $('#page2').width(parseInt(page_width));
        $('#page2').height(parseInt(page_height));
        $('#page2').css("margin-top","0");
        $('#page2').css("margin-left","30");      
    }               
         

	var footer_div = document.getElementById("footer");
	var is_double = ( footer_div.className == "footer_double") ;    
    
    if(!is_double) {
        $('.container_center').css("width", document.body.clientWidth-40 > 925 ? document.body.clientWidth-40 : 925);
		if(get_browser()=="Netscape" || get_browser()=="Firefox")
			$('.container_center').css("top", document.body.scrollHeight >  630+adjustPageHeight ? document.body.scrollHeight :  630+adjustPageHeight); 
		else if(get_browser()=="IE")
			$('.container_center').css("top", document.documentElement.clientHeight >  630+adjustPageHeight ? document.documentElement.clientHeight :  630+adjustPageHeight); 
		else
			$('.container_center').css("top", document.documentElement.scrollHeight >  630+adjustPageHeight ? document.documentElement.scrollHeight :  630+adjustPageHeight); 
    } else{
        $('.container_center').css("width", document.body.clientWidth-40 > 925 ? document.body.clientWidth-40 : 925);
		if(get_browser()=="Netscape" || get_browser()=="Firefox")
			$('.container_center').css("top", document.body.scrollHeight >  675+adjustPageHeight ? document.body.scrollHeight :  675+adjustPageHeight);
		else if(get_browser()=="IE")
			$('.container_center').css("top", document.documentElement.clientHeight >  675+adjustPageHeight ? document.documentElement.clientHeight :  675+adjustPageHeight);   
		else
			$('.container_center').css("top", document.documentElement.scrollHeight >  675+adjustPageHeight ? document.documentElement.scrollHeight :  675+adjustPageHeight);    
    }         
         
    //document.getElementById('page').contentWindow.change_size(page_width, page_height);
    
}
function do_search()
{
	var key = document.getElementById("search_text").value.replace(/ /g,"%20");
	var winoptions = "width=960,height=800,menubar=yes,scrollbars=yes,toolbar=yes,status=yes,location=yes,resizable=yes";
	var url="http://kb.netgear.com/app/answers/list/kw/"+key;

	window.open(url,null,winoptions);
}
function detectEnter(type, e)
{
     var keycode, event;
	 if (window.event)
        {
                event = window.event;
                keycode = window.event.keyCode;
        }
        else if (e)
        {
                event = e;
                keycode = e.which;
        }
        else 
			return true;
			
		if(type == "num")
		{
	  if(keycode==13)
			do_search();
		}
		else
		return false;
}
