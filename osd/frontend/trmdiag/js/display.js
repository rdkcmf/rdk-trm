/*
 * If not stated otherwise in this file or this component's Licenses.txt file the
 * following copyright and licenses apply:
 *
 * Copyright 2016 RDK Management
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
*/

var selectedRow = 0;
var selectedCol = 0;
var maxRow = 0;
var maxCol = 3;
var deviceIds = [];
var currentTableDisp = null;
//Returns true if it switches pages
function display_device_table(num) {
	if(num >=0 && num < deviceIds.length){
		if(currentTableDisp != null) {
			hide_table(currentTableDisp);
			unselectTd(selectedRow, selectedCol);
		}
		currentTableDisp = table_from_device_id(deviceIds[num]);
		show_table(currentTableDisp);
		selectedRow =0, selectedCol=0;
	    selectTd(0,0);
		maxRow = currentTableDisp.rows.length -1;
		set_page_num(num);
		return true;
	}
	return false;
}

//Note that if a message from a new device does not have a reservedDeviceId, the data is not added.
function display_tuners(tuner_details) {
	for(var tuner in tuner_details) {
		if(tuner_details.hasOwnProperty(tuner) && (device_id = tuner_details[tuner].reservedDeviceId) != null) {
			//device_id = tuner_details[tuner].reservedDeviceId;
			var table = table_from_device_id(device_id);
			if(table == null){
				table = new_device_table(device_id);
				console.log("Adding table:", table, "with: ", tuner_details);
				set_tuners_in_table(tuner_details, table, add_tuner);
				add_heading_above_table(table, device_id);
			}
			else {
				console.log("Updating table:", table, "with: ", tuner_details);
				set_tuners_in_table(tuner_details, table, update_tuner);
			}
			return; //Only 1 unique reservedDeviceId per stb
		}
	}
}
function table_from_device_id(device_id) {
	var ind = deviceIds.indexOf(device_id);
	if(ind >=0) return document.getElementById("tuner_state_table" + ind);
	else return null;
}
function new_device_table(device_id) {
	deviceIds.push(device_id);
	var newPage = '<div class="TRM-Diag-Page Other-Page"> <h1 class="TRM-Device"></h1> <table id="tuner_state_table' + (deviceIds.length-1).toString() + //Index of device_id in deviceIds
	'" class = "Tuner-Table-With-Info"> <tr class="headers"> <th>Device</th> <th>State</th> <th>Channel</th> <th>Tuner ID</th> </tr> </table> </div>';
	document.getElementById("Pages").innerHTML += newPage;
	return document.getElementById("tuner_state_table" + (deviceIds.length-1).toString());
}


//set_tuner should be either add_tuners or update_tuners
function set_tuners_in_table(tuner_details, table, set_tuner){
	var free_tuners = [];
	var recording_tuners = [];
	var row_num = 0;
	for (var tuner in tuner_details) {
		  if (tuner_details.hasOwnProperty(tuner)) {
			var state = tuner_details[tuner].state;
			 //Always show Live at top
			if(state == "Live" || state == "Hybrid") {
				set_tuner(table, row_num, tuner, tuner_details[tuner]);
				row_num++;

			}
			//Save recorded to load next
			else if(state == "Record") {
				recording_tuners.push([tuner, tuner_details[tuner]]);
					
			}
			else //Save Free for last
				free_tuners.push([tuner, tuner_details[tuner]]);
		}
	}
		for(var i = 0; i < recording_tuners.length; ++i) {
			set_tuner(table, row_num, recording_tuners[i][0], recording_tuners[i][1]);
			row_num++;
		}
		for(var i = 0; i < free_tuners.length; ++i) {
			set_tuner(table, row_num, free_tuners[i][0], free_tuners[i][1]);
			row_num++;
		}	
}
//Removes a set of tuners from the screen. This function will leave the header row but remove all subsequent rows.
function clear_tuners(table) {
	var row_num = table.rows.length;
	for (var i = row_num - 1; i > 0; i--) {
		table.deleteRow(i);
	}
	table.className = "Tuner-Table-No-Info";
}



function add_tuner(table, row_num, tuner_id, tuner_details) {
	var row = table.insertRow(row_num + 1);
	//Set row id for later reuse
	row.id = table.id + "row" + row_num;
	//Set up device element
	var deviceTd = row.insertCell(0);
	deviceTd.className = "Trm-Tuner-Device";
	state = tuner_details.state;
	if (state == "Free") {
		deviceTd.innerHTML = "None";
		var stateTd =  row.insertCell(1);
		stateTd.className = "Trm-Tuner-Usage";
		stateTd.innerHTML = state_images(state);
		// var channelNumTd = row.insertCell(2);
		// channelNumTd.className = "Trm-Tuner-Channel-Num";
		// channelNumTd.innerHTML = "None";
		var channelTd = row.insertCell(2);
		channelTd.className = "Trm-Tuner-Channel-Name";
		channelTd.innerHTML = "None";
	}
	else {
		if(state== "Hybrid")
			deviceTd.innerHTML = tuner_details.owners["Live"].device;
		else
			deviceTd.innerHTML = tuner_details.owners[state].device;
		var stateTd =  row.insertCell(1);
		stateTd.className = "Trm-Tuner-Usage";
		stateTd.innerHTML = state_images(state);
		//Set up Channel
		hexCode = hex_code(tuner_details.serviceLocator);
		// var channelNumTd = row.insertCell(2);
		// channelNumTd.className = "Trm-Tuner-Channel-Num";
		// channelNumTd.innerHTML =` channelNum(hexCode);
		var channelTd = row.insertCell(2);
		channelTd.className = "Trm-Tuner-Channel-Name";
		channelTd.innerHTML = channelName(hexCode);
	}

	//Set up tuner id
	var tId = row.insertCell(3);
	tId.className = "Trm-Tuner-Id";
	tId.innerHTML = tuner_id;
				
}
//extracts hex code from string
function hex_code(string_with_hex) {
	var hexRx = new RegExp("0x.*$");
	var channel_hex = hexRx.exec(string_with_hex);
	return channel_hex[0];
}


function update_tuner(table, row_num, tuner_id, tuner_details) {
	var tuner = document.getElementById(table.id + "row" + row_num);
	tuner.cells[1].innerHTML =  state_images(tuner_details.state);
	tuner.cells[3].innerHTML = tuner_id;
	if(tuner_details.state == "Free") {
		tuner.cells[0].innerHTML = "None";
		tuner.cells[2].innerHTML = "None";
		//tuner.cells[3].innerHTML = "None";
	}
	else {
		if(tuner_details.state== "Hybrid")
			tuner.cells[0].innerHTML = tuner_details.owners["Live"].device;
		else
			tuner.cells[0].innerHTML = tuner_details.owners[tuner_details.state].device;
		hexCode = hex_code(tuner_details.serviceLocator);
		//tuner.cells[2].innerHTML = channelNum(hexCode);
		tuner.cells[2].innerHTML = channelName(hexCode);
	}
}


function state_images(state) {
	if (state == "Free")  {
		//Two images, one for live and the other for recording
		return "<div class=state_container><div class='white-circle'></div><div class='white-circle'></div></div>";
	}
	else if (state == "Live") {
		return "<div class=state_container><div class='green-circle'></div><div class='white-circle'></div></div>";
	}
	else if (state == "Record") {
		return "<div class=state_container><div class='white-circle'></div><div class='red-circle'></div></div>";

	}
	else if (state == "Hybrid") {
		return "<div class=state_container><div class='green-circle'></div><div class='red-circle'></div></div>";
	}
	else return "";
}
//Searches channel list and returns channel name
//channel_hex should be in the format: "0x#### where the # are hex values. 	
function channelName(channel_hex) {
	//Comp test return "";
	var hex = channel_hex.substring(2);
	var image = channel_images[hex];
	if(image) {
		console.log("ADDED image for: ", hex);
		return "<img class='channel-image' src='images/logos/" + image + "'\>"; 
	}
	var lineRx = new RegExp(hex + "-Name\\[(.*)\\]-State-");
	channel_results = lineRx.exec(channel_list);
	try {
		return channel_results[1];
	}
	catch(err) {
		console.log("Failed to load channel name; error:", err);
		return channel_hex;
	}
}

function channelNum(channel_hex) {
	//Comp test return 0;
	hex = channel_hex.substring(2)
	var lineRx = new RegExp("VCN#(\\d{6})-.{3}ID#\\d*" + hex);
	channel_results = lineRx.exec(channel_list);
	try {
		return parseInt(channel_results[1]);
	}
	catch(err) {
		alert("Error loading channelNum: ", err);
		return -1;
	}
}

//1 for up, -1 for down, 2 for right, -2 for left
function selectCell(movCode) {
	unselectTd(selectedRow, selectedCol);
	switch(movCode) {
		case 1: //up
			if(selectedRow == 0) {//wrap
				selectTd(maxRow, selectedCol);
				selectedRow = maxRow;
			}
			else {
				selectTd(--selectedRow, selectedCol);
			}
			break;
		case -1: //down
			if(selectedRow == maxRow) {//wrap
				selectTd(0, selectedCol);
				selectedRow = 0;
			}
			else {
				selectTd(++selectedRow, selectedCol);
			}
			break;
		case 2: //right
			if(selectedCol == maxCol) {//wrap
				selectTd(selectedRow, 0);
				selectedCol = 0;
			}
			else {
				selectTd(selectedRow, ++selectedCol);
			}
			break;
		case -2: //left
			if(selectedCol == 0) {//wrap
				selectTd(selectedRow, maxCol);
				selectedCol = maxCol;
			}
			else {
				selectTd(selectedRow, --selectedCol);
			}
			break;
	}
}

function unselectTd(row, col)  {
	var table = currentTableDisp;
	var tdElm = table.rows[row].cells[col];
	var classes = tdElm.className.split(' ');
	tdElm.className = classes[0] + ' ' + "unselected"
}
function selectTd(row, col) {
	//console.log("row: ", row, "col: ", col);
	var table = currentTableDisp;
	var tdElm = table.rows[row].cells[col];
	var classes = tdElm.className.split(' ');
	tdElm.className = classes[0] + ' ' + "selected"
}

function hide_table(table) {
	var classes = table.parentNode.className.split(' ');
	table.parentNode.className = classes[0] + ' ' + "Other-Page";
}

function show_table(table) {
	var classes = table.parentNode.className.split(' ');
	table.parentNode.className = classes[0] + ' ' + "Current-Page";
}
function add_heading_above_table(table, text) {
	var header = table.parentNode.children[0].innerHTML = text;
}

//Takes in the page number visible onscreen
function set_page_num(pnum) {
	pagenum_div = document.getElementById("pagenum");
	pagenum_div.innerHTML = (pnum + 1).toString() + '/' + deviceIds.length;
}
function displaying_table() {
	console.log(currentTableDisp);
	return currentTableDisp != null;
}

var channel_images = {
	//SRCID#:image name
	"3653": "448114160.png",//Food
	"105a": "448114160.png",//Food
	"1053":"311185280.png", //ABC
	"2a4f":"311185280.png", //ABC
	"4134":"311185280.png", //ABC
	"249":"311185280.png", //ABC
	"24c":"311185280.png", //ABC
	"250":"311185280.png", //ABC
	"3211":"311463069.png", //TBS
	"125e":"312868042.png", //NBC
	"100d":"312868042.png", //NBC
	"14a6":"312868042.png", //NBC
	"2a50":"312868042.png", //NBC
	"3fcc":"312868042.png", //NBC
	"1036":"308143845.png", //MSNBC
	"4774":"308143845.png", //MSNBC
	"248":"290840530.png",//CNN
	"3f0d":"290840530.png", //CNN
	"24d":"290840530.png", //CNN
	"100e":"290840530.png", //CNN
	"1dd9":"290840530.png", //CNN
	"252":"290840530.png", //CNN
}

