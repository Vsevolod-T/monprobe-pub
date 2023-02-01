"use strict";


function IsModalVisible() {
	
	if (document.getElementById('id01').style.display == 'block')
		return true;
	return false;
}

function ModalVisible(mode) {

	if (mode==true)
		document.getElementById('id01').style.display='block';
	else
		document.getElementById('id01').style.display='none';
}

function button_enter() {
	
	let login = document.getElementsByName('login')[0].value;
	let passw = document.getElementsByName('passw')[0].value;

	initws(login,passw);
	initcv();
}

function ready() {
    
	ModalVisible(true); // DOM ready
	initcv();
}

document.addEventListener("DOMContentLoaded", ready);



