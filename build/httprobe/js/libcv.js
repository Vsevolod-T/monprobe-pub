"use strict";

let Interfaces = [];//все интерфейсы [name,ip,traffic]
let Channels = [];  // все каналы [ip,name,total,num,state]

let idxSelCh = null;  // idx selected channel in Channels
let Detail = [];      // es details table info

let Area = [];   // text area + place

// =======================================

function clearcv(msg) {
	idxSelCh = null;
	Interfaces = [];
	Channels = [];
	Detail = [];
	Area = "Place:";


	document.getElementById('id_interfaces').innerHTML = msg;

	document.getElementById('selChannel').innerHTML = "";

	let tbody = document.getElementById('detailTableBody');
	DeleteRows(tbody);

	let ch = document.getElementById("list_ch");
	while (ch.firstChild) {
	  ch.removeChild(ch.lastChild);
	}

	let ind = document.getElementById("list_status");
	while (ind.firstChild) {
	  ind.removeChild(ind.lastChild);
	}
}


function onclickChannel(event) {
	
	let idx_ch = event.currentTarget.attr;

	if (idxSelCh==idx_ch) {
		SendDetailStop();
		
		let cell = document.getElementById('ind_id'+idxSelCh);
		cell.style.boxShadow = '';
		
		cell = document.getElementById('list_id'+idxSelCh);
		cell.style.backgroundColor='black';

		idxSelCh=null;
	}
	else  {
		SendDetail(idx_ch,1);
		
		if (idxSelCh != null) {
			let cell = document.getElementById('ind_id'+idxSelCh);
			cell.style.boxShadow = '';
			
			cell = document.getElementById('list_id'+idxSelCh);
			cell.style.backgroundColor='black';
		}
		idxSelCh=idx_ch;
		// indicator 
		let cell = document.getElementById('ind_id'+idx_ch);
		cell.style.boxShadow = 'inset 0px 0px 0px  5px rgba(255,0,0,1.0)';
		// scroll list
		cell = document.getElementById('list_id'+idxSelCh);
		cell.style.backgroundColor='gray';
		//cell.parentNode.scrollTop = cell.offsetTop - cell.parentNode.offsetTop;
	}
	DrawDetails();
}

function onclickIndicator(event) {

	let idx_ch = event.currentTarget.attr;

	if (idxSelCh==idx_ch) {
		SendDetailStop();
		
		let cell = document.getElementById('ind_id'+idxSelCh);
		cell.style.boxShadow = '';
		
		cell = document.getElementById('list_id'+idxSelCh);
		cell.style.backgroundColor='black';
		idxSelCh=null;
	}
	else  {
		SendDetail(idx_ch,1);
		if (idxSelCh != null) {
			let cell = document.getElementById('ind_id'+idxSelCh);
			cell.style.boxShadow = '';
			
			cell = document.getElementById('list_id'+idxSelCh);
			cell.style.backgroundColor='black';
		}
		idxSelCh=idx_ch;
		// indicator 
		let cell = document.getElementById('ind_id'+idx_ch);
		cell.style.boxShadow = 'inset 0px 0px 0px  5px rgba(255,0,0,1.0)';
		// scroll list
		cell = document.getElementById('list_id'+idxSelCh);
		cell.style.backgroundColor='gray';
		cell.parentNode.scrollTop = cell.offsetTop - cell.parentNode.offsetTop;
	}
	DrawDetails();
}


function FillChannels() {

	let ch = document.getElementById("list_ch");
	while (ch.firstChild) {
	  ch.removeChild(ch.lastChild);
	}


	// list channels
	let fragment1 = new DocumentFragment();
	for (let i=0;i<Channels.length;i++) {
		let txt = Channels[i].name;
		let cell = document.createElement("div");
		let cellText = document.createTextNode(txt);
		cell.id = 'list_id'+i;
		cell.className = "s_ch";
		cell.title= Channels[i].ip;
		cell.attr = i;
		cell.onclick = onclickChannel;
      	cell.appendChild(cellText);
		fragment1.appendChild(cell);
	}
	document.getElementById('list_ch').appendChild(fragment1);
}
	
function FillIndicators() {


	let ind = document.getElementById("list_status");
	while (ind.firstChild) {
	  ind.removeChild(ind.lastChild);
	}

	let count = Channels.length;

	// full heigh = 90% 
	let x = 10
	let count_rows = count / x;
	if (count % x) count_rows++;
	let h = Math.floor(90 / count_rows);
	let calc_height = h+"%";  
	
	let fragment2 = new DocumentFragment();
	for (let i=0;i<count;i++) {
		let div = document.createElement("div");
		div.id = 'ind_id'+i;
		div.className = "indicator";
		div.attr = i;
		div.onclick = onclickIndicator;
		
		div.style.height = calc_height;
		//div.style.backgroundColor = "#ffffff";
		fragment2.appendChild(div);
	}
	document.getElementById('list_status').appendChild(fragment2);
}


function DrawChannels(idx_channel,status) {
	let color;

	switch(status) {
		case 0: color= 'rgb(90,90,90)';    break; //'gray';
		case 1: color= 'rgb(255,0,0)';     break;//'red';
		case 2: color= 'rgb(255,255,0)';   break;//'yellow';
		case 3: color= 'rgb(0,255,0)';     break;//'green'; 
		case 4: color= 'rgb(0,0,255)';     break;//'blue';
		case 5: color= 'rgb(156,156,80)';  break;//'brown';
		case 6: color= 'rgb(255,255,255)'; break;//'white';
		default:
				color= 'rgb(0,0,0)'; break;//'black';
	};
	document.getElementById('ind_id'+idx_channel).style.backgroundColor=color;
}

function DrawPlace() {
	let el = document.getElementById('id_place');
	el.innerHTML = Area;
}

function DrawTraffic() {
	let txt = "";
	for (let i=0;i<Interfaces.length;i++) {
		txt = txt + Interfaces[i].name + " : " + Interfaces[i].traffic + "MBit ";
	}
	document.getElementById('id_interfaces').innerHTML = txt;
}	

function AddRow(tbody,es_pid,es_typ,es_desc,es_traff,es_err,es_min)
{
	let newRow = tbody.insertRow(-1); // Insert a row at the end of the table
	newRow.insertCell(0).appendChild(document.createTextNode(es_pid));
	newRow.insertCell(1).appendChild(document.createTextNode(es_typ));
	newRow.insertCell(2).appendChild(document.createTextNode(es_desc));
	newRow.insertCell(3).appendChild(document.createTextNode(es_traff));
	newRow.insertCell(4).appendChild(document.createTextNode(es_err));
	newRow.insertCell(5).appendChild(document.createTextNode(es_min));
}

function DeleteRows(tbody)
{
	let countRow = tbody.rows.length;
	for (let i=countRow-1; i>-1; i--)
		tbody.deleteRow(i);
}


function DrawDetails() {

// caption
	let selCh = document.getElementById('selChannel');

	if (idxSelCh==null) {
		selCh.innerHTML = "";
	}
	else {
		selCh.innerHTML = Channels[idxSelCh].name + " (" + Channels[idxSelCh].ip + ")";
	}

// body
	let tbody = document.getElementById('detailTableBody');
	DeleteRows(tbody);

	if (idxSelCh==null)
		return;

	// rows Add
	if ((Detail.root)!== undefined) {
		let root = Detail.root;
		for (let i=0;i<root.length;i++) {
			let es_pid=root[i].es[0];
			let es_typ=root[i].type[0];
			let es_desc =root[i].type[2]+root[i].type[3];
			let es_traf =root[i].type[1];
			let es_err = root[i].es[2];
			
			if (es_err == "0")
				es_err = '';
			
			let es_min = '';
			if ((Detail.root[i].checked)!== undefined) {
				es_min = Detail.root[i].checked.traffic_min;
				if (es_min == "0")
					es_min = '';
				else 
					es_min = es_min + " kBit";
			}
			
			AddRow(tbody,es_pid,es_typ,es_desc,es_traf,es_err,es_min);
		}
	}
	//console.log(Detail.service[0].checked);
	if ((Detail.service)!== undefined) {
		let service = Detail.service;
		for (let i=0;i<service.length;i++) {
			let es_pid=service[i].es[0];
			let es_typ=service[i].type[0];
			let es_desc =service[i].type[2]+service[i].type[3];
			let es_traf =service[i].type[1];
			let es_err = service[i].es[2];
			
			if (es_err == "0")
				es_err = '';

			let es_min = '';
			if ((Detail.service[i].checked)!== undefined) {
				es_min = Detail.service[i].checked.traffic_min;
				if (es_min == "0")
					es_min = '';
				else 
					es_min = es_min + " kBit";
			}
			AddRow(tbody,es_pid,es_typ,es_desc,es_traf,es_err,es_min);
		}
	}
}

function ParseServerInfo(server_msg) {

	idxSelCh = null;
	Interfaces = [];
	Channels = [];
	Detail = [];

	Area = "Place:";

	if ((server_msg.area)!== undefined) {
		Area = server_msg.area;
		if ((server_msg.place)!== undefined) {
			Area = Area + " : " + server_msg.place;
		}
	}

	let items = server_msg.arr_if ;
	for (let i=0;i<items.length;i++) {
		let obj = {
			name    : items[i].name,
			ip      : items[i].ip,
			traffic : items[i].traffic
		};
		Interfaces.push(obj);
	}// for arr_if 

	items = server_msg.arr_ch ;
	for (let i=0;i<items.length;i++) {
		let obj = {
			name    : items[i].name,
			ip      : items[i].ip,
			state   : items[i].state
		};
		
		if (obj.name=="")
			obj.name="noname";
		
		Channels.push(obj);
	}// for arr_ch 

	FillChannels();
	FillIndicators();

	items = server_msg.arr_ch;
	for (let i=0;i<items.length;i++) {
		DrawChannels(i,Channels[i].state);
	}
	DrawPlace();
	DrawTraffic();

}

function ParseStateInfo(server_msg) {

	let items = server_msg.arr_if ;
	
	for (let i=0;i<items.length;i++) {
		Interfaces[i].traffic=items[i].traffic;
	}// for arr_if 

	if((server_msg.arr_stat)!== undefined) {

		items = server_msg.arr_stat;
		for (let i=0;i<items.length;i++) {

			let idx_channel = items[i][0];
			let status = items[i][1];
			Channels[idx_channel].state=status;
			DrawChannels(idx_channel,status);
		}// for arr_stat 
	}

	if((server_msg.detail)!== undefined) {
		Detail = server_msg.detail;
		DrawDetails();
	}
	
	DrawPlace();
	DrawTraffic();
}

function initcv() {
}
