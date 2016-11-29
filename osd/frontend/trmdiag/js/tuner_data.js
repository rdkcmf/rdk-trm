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

var open_ws = null;
function WSconnect(new_ip) {
	try {
		var port = 9989;
		var tunerSocket = new WebSocket("ws://" + new_ip + ":" + port + "/");
		tunerSocket.onmessage = function(event) {
			console.log(event.data);
			var msg = JSON.parse(event.data);
			console.log(msg);
			if(msg.getAllTunerStatesResponse) {
				var tuners = msg.getAllTunerStatesResponse.detailedStates;
				display_tuners(tuners);
				if(!displaying_table())
					display_device_table(0);
			}
			else if(msg.notifyTunerStatesUpdate){
				var tuners = msg.notifyTunerStatesUpdate.detailedStates;
				display_tuners(tuners);
				if(!displaying_table())
					display_device_table(0);
			}

		};
		tunerSocket.onerror = function (event) {
			console.log("Error: ", event);
			alert("Error, unable to connect to ip address " + new_ip + ":" + port);
		};
		//Sending a message to the stb is required to receive messages
		tunerSocket.onopen = function (event) {
			var msg = { "getAllTunerStates" : { "requestId" : "Initial_States"} };
			tunerSocket.send(JSON.stringify(msg)); 
			open_ws = tunerSocket;
		};
		tunerSocket.onclose = function(){} //Do nothing
	}
	catch(exception) {
		console.log(exception);
		alert(exception);
	}
	
}

function get_local_tuner_states() {
	try {
		var ip = "127.0.0.1";
		var port = 9988;
		var tunerSocket = new WebSocket("ws://" + ip + ":" + port + "/");
		tunerSocket.onmessage = function(event) {
			console.log(event.data);
			var msg = JSON.parse(event.data);
			if(msg.getAllTunerStatesResponse) {
				var tuners = msg.getAllTunerStatesResponse.detailedStates;
				display_tuners(tuners);
				if(!displaying_table())
					display_device_table(0);
				tunerSocket.close();
			}

		};
		tunerSocket.onerror = function (event) {
			console.log("Error: ", event);
			alert("Error, unable to connect to ip address " + ip + ":" + port);
		};
		//Sending a message to the stb is required to receive messages
		tunerSocket.onopen = function (event) {
			var msg = { "getAllTunerStates" : { "requestId" : "Initial_States"} };
			tunerSocket.send(JSON.stringify(msg)); 
		};
		tunerSocket.onclose = function(){} //Do nothing
	}
	catch(exception) {
		console.log(exception);
		alert(exception);
	}
}

window.onbeforeunload = function() {
	if(open_ws) {
		open_ws.close();
	}
}

