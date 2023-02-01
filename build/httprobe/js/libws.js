"use strict";

let ws;
let token;
let msg;

function IsConnected() {

	if (!ws || ws.readyState != 1)
		return false;
	return true;
}


function ServerInfo(srv_msg) {
	ParseServerInfo(srv_msg);
}
function StateInfo(srv_msg) {
	ParseStateInfo(srv_msg);
}

function SendLogin(ulogin,passw) {
	ws.send('{"cmd":"login","login":"'+ulogin+'","passwd":"'+passw+'","unique_id":"'+Date.now().toString()+'"}');
}
// not used
function SendLogout() {
	ws.send('{"cmd":"logout","token":"'+token+'"}');
}

function SendDetail(channel,subchannel) {
      let st = '{"cmd":"detail","token":"'+token+'","channel":'+channel+',"num":'+subchannel+'}';
//console.log(st);
      ws.send(st);
}
function SendDetailStop() {
     let st ='{"cmd":"detailstop","token":"'+token+'"}';
//console.log(st);
     ws.send(st);
}



function ServerMsg(rcv_msg) {

	switch(rcv_msg.cmd){
		
		case "login": 
			if(rcv_msg.msg != undefined) {
				msg = "MSG: " + rcv_msg.msg;
				ws.close();
				ws = null;
				token="";
				ModalVisible(true);
			}
			else 
				token = rcv_msg.token;
				ModalVisible(false);
			break; 
		
		case "logout":
			msg = "MSG: connection closed from server";
			ws.close();
			ws = null;
			token="";
			ModalVisible(true);
			break; 

		case "serverinfo" : ServerInfo(rcv_msg); break;
		case "state"      : StateInfo(rcv_msg);  break;
		default :  console.log("srv: " + rcv_msg.cmd + "," + rcv_msg.msg); break; 
		}
}

function initws(login,passw) {

	if(!IsConnected()){
		
		ws = new WebSocket('ws://' + window.location.host + '/ws');
		ws.onopen = function(){
			SendLogin(login,passw);
			};
		ws.onclose = function(){
			ws = null;
			token="";
			clearcv(msg);
			ModalVisible(true);
			};
		ws.onerror = function(event){
			ws = null;
			token="";
			clearcv("MSG: connect error");
			ModalVisible(true);
			};
		ws.onmessage = function(rcv){
			ServerMsg(JSON.parse(rcv.data));
			};
	}
}