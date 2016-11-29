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

//Many functions from startupScreen.html
//Sets channel_list to a string with the list of channels from file
function getChannelList() {
	var xmlHttp = new XMLHttpRequest();
	xmlHttp.open('GET', "http://localhost:50050/trmdiag/channel_list.txt", true);
	try
	{
		xmlHttp.send(null);
		channel_list = xmlHttp.responseText;
	}
	catch(err)
	{
		console.log("getChannelList::could not load! - " + err.message);
		alert("Failed to load channel_list: " + err.message);
		channel_list = "";
	}	
	
}
/* function getDeviceData() {
	var xmlHttp = new XMLHttpRequest();
	xmlHttp.open('POST', "http://localhost:50050/trmdiag/box_info.sh", false);
	try
	{
		xmlHttp.send(null);
		alert(xmlHttp.responseText);
	}
	catch(err)
	{
		console.log("getDeviceData::could not load! - " + err.message);
		channel_list = "";
		alert("Failed to load box_info.sh: " + err.message);
	}	
	
} */
function load()
{
	getChannelList();
	get_local_tuner_states();
	WSconnect("127.0.0.1");
   document.addEventListener('keydown', function(event) {
      onKeyPress(event.keyCode);
   });
}
//Note: currentPage is 0 when on the top page. It increases when going down pages.
var currentPage = 0;
function onKeyPress(keyCode) {
	switch(keyCode) {
		case 38: //up
			selectCell(1);
			break;
		case 40: //down
			selectCell(-1);
			break;
		case 37: //left
			selectCell(-2);
			break;
		case 39: //right
			selectCell(2);
			break;
		// case 33: //page up
			// if(display_device_table(currentPage - 1)){
				// --currentPage;
			// }
			// break;
		// case 34: //page down
			// if(display_device_table(currentPage + 1)){
				// ++currentPage;
			// }
			// break;
		case 8: //Last
			//Go to next page or cycle
			if(display_device_table(currentPage + 1)) ++currentPage;
			else {
				display_device_table(0);
				currentPage = 0;
			}
	}
}
