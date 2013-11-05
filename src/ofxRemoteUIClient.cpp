//
//  ofxRemoteUI.cpp
//  emptyExample
//
//  Created by Oriol Ferrer Mesià on 09/01/13.
//
//

#include "ofxRemoteUIClient.h"
#include <iostream>



ofxRemoteUIClient::ofxRemoteUIClient(){
	readyToSend = false;
	timeSinceLastReply = 0;
	avgTimeSinceLastReply = 0;
	waitingForReply = false;
	callBack = NULL;
	verbose_ = false;
}

void ofxRemoteUIClient::setCallback( void (*callb)(RemoteUIClientCallBackArg) ){
	callBack = callb;
}

void ofxRemoteUIClient::setup(string address, int port_){

	params.clear();
	orderedKeys.clear();
	presetNames.clear();

	port = port_;
	avgTimeSinceLastReply = timeSinceLastReply = timeCounter = 0.0f;
	waitingForReply = false;
	host = address;
	cout << "ofxRemoteUIClient listening at port " << port + 1 << " ... " << endl;
	clearOscReceiverMsgQueue();
	oscReceiver.setup(port + 1);

	if(verbose_) cout << "ofxRemoteUIClient connecting to " << address << endl;
	oscSender.setup(address, port);
	broadcastReceiver.setup(OFXREMOTEUI_BROADCAST_PORT);
}

vector<Neighbor> ofxRemoteUIClient::getNeighbors(){
	return closebyServers.getNeighbors();
}

void ofxRemoteUIClient::disconnect(){
	sendCIAO();
	readyToSend = false;
}

void ofxRemoteUIClient::saveCurrentStateToDefaultXML(){
	sendSAVE();
}

void ofxRemoteUIClient::restoreAllParamsToInitialXML(){
	sendRESX();
}

void ofxRemoteUIClient::restoreAllParamsToDefaultValues(){
	sendRESD();
}

void ofxRemoteUIClient::update(float dt){

	bool neigbhorChange = false;
	//listen for broadcast from all servers in the broadcast channel OFXREMOTEUI_BROADCAST_PORT
	while( broadcastReceiver.hasWaitingMessages() ){// check for waiting messages from client
		ofxOscMessage m;
		broadcastReceiver.getNextMessage(&m);
		neigbhorChange |= closebyServers.gotPing(m.getRemoteIp(), m.getArgAsInt32(0), m.getArgAsString(1), m.getArgAsString(2));
		//cout << "got broadcast message from " << m.getRemoteIp() << ":" << m.getArgAsInt32(0) << endl;
		//closebyServers.print();
	}

	neigbhorChange |= closebyServers.update(dt);

	if(neigbhorChange){
		if(callBack != NULL){
			RemoteUIClientCallBackArg cbArg; // to notify our "delegate"
			cbArg.action = NEIGHBORS_UPDATED;
			callBack(cbArg);
		}
	}

	if (!readyToSend){ // if not connected, connect

		if(verbose_) cout << "ofxRemoteUIClient: sending HELLO!" << endl;
		sendHELLO();	//on first connect, send HI!
		sendTEST();		//and a lag test
		readyToSend = true;
		
	}else{

		timeCounter += dt;
		timeSinceLastReply += dt;
		//printf("waiting for reply: %d\n", waitingForReply);
		if (timeCounter > OFXREMOTEUI_LATENCY_TEST_RATE){
			if (!waitingForReply){
				timeCounter = 0.0f;
				sendTEST();
			}else{
				if (timeCounter > OFXREMOTEUI_CONNECTION_TIMEOUT){
					avgTimeSinceLastReply = -1;
					readyToSend = false; // testing here
					params.clear();
					orderedKeys.clear();
				}
			}
		}
	}

	while( oscReceiver.hasWaitingMessages() ){// check for waiting messages from server

		ofxOscMessage m;
		oscReceiver.getNextMessage(&m);

		DecodedMessage dm = decode(m);
		RemoteUIClientCallBackArg cbArg; // to notify our "delegate"
		cbArg.host = m.getRemoteIp();
		switch (dm.action) {

			case HELO_ACTION:{ //server says hi back, we ask for a big update
				requestCompleteUpdate();
				if(callBack != NULL){
					cbArg.action = SERVER_CONNECTED;
					callBack(cbArg);
				}
				}break;

			case REQUEST_ACTION: //should not happen, server doesnt request << IS THIS BS?
				if(verbose_) cout << "ofxRemoteUIClient: " << m.getRemoteIp() << " says REQUEST_ACTION!" << endl;
				if(callBack != NULL){
					cbArg.action = SERVER_REQUESTED_ALL_PARAMS_UPDATE;
					callBack(cbArg);
				}
				break;

			case SEND_PARAM_ACTION:{ //server is sending us an updated val
				if(verbose_) cout << "ofxRemoteUIClient: " << m.getRemoteIp() << " says SEND_PARAM_ACTION!" << endl;
					updateParamFromDecodedMessage(m, dm);
					gotNewInfo = true;
				}
				break;

			case CIAO_ACTION:{
				if(verbose_) cout << "ofxRemoteUIClient: " << m.getRemoteIp() << " says CIAO!" << endl;
				if(callBack != NULL){
					cbArg.action = SERVER_DISCONNECTED;
					callBack(cbArg);
				}
				sendCIAO();
				params.clear();
				orderedKeys.clear();
				clearOscReceiverMsgQueue();
				readyToSend = false;
				}break;

			case TEST_ACTION: // we got a reply from the server, lets measure how long it took;
				waitingForReply = false;
				if (avgTimeSinceLastReply > 0.0f){
					avgTimeSinceLastReply = 0.8 * (avgTimeSinceLastReply) + 0.2 * (timeSinceLastReply);
				}else{
					avgTimeSinceLastReply = timeSinceLastReply ;
				}
				timeSinceLastReply = 0.0f;
				break;

			case PRESET_LIST_ACTION: //server sends us the list of current presets
				if(verbose_) cout << "ofxRemoteUIClient: " << m.getRemoteIp() << " says PRESET_LIST_ACTION!" << endl;
				fillPresetListFromMessage(m);
				if(callBack != NULL){
					cbArg.action = SERVER_PRESETS_LIST_UPDATED;
					callBack(cbArg);
				}
				break;

			case SET_PRESET_ACTION: // server confirms that it has set the preset, request a full update
				if(verbose_) cout << "ofxRemoteUIClient: " << m.getRemoteIp() << " says SET_PRESET_ACTION!" << endl;
				requestCompleteUpdate();
				if(callBack != NULL){
					cbArg.action = SERVER_DID_SET_PRESET;
					cbArg.msg = m.getArgAsString(0);
					callBack(cbArg);
				}
				break;

			case GET_MISSING_PARAMS_IN_PRESET:
				if(callBack != NULL){
					cbArg.action = SERVER_REPORTS_MISSING_PARAMS_IN_PRESET;
					for (int i = 0; i < m.getNumArgs(); i++){
						cbArg.paramList.push_back(m.getArgAsString(i));
					}
					callBack(cbArg);
				}

				break;

			case SAVE_PRESET_ACTION:{ // server confirms that it has save the preset,
				if(verbose_) cout << "ofxRemoteUIClient: " << m.getRemoteIp() << " says SAVE_PRESET_ACTION!" << endl;
				vector<string> a;
				sendPREL(a); //request preset list to server, send empty vector
				cbArg.action = SERVER_SAVED_PRESET;
				cbArg.msg = m.getArgAsString(0);
				if(callBack != NULL) callBack(cbArg);
				}break;

			case DELETE_PRESET_ACTION:{ // server confirms that it has deleted preset
				if(verbose_) cout << "ofxRemoteUIClient: " << m.getRemoteIp() << " says DELETE_PRESET_ACTION!" << endl;
				vector<string> a;
				sendPREL(a); //request preset list to server, send empty vector
				if(callBack != NULL){
					cbArg.action = SERVER_DELETED_PRESET;
					cbArg.msg = m.getArgAsString(0);
					callBack(cbArg);
				}
				}break;

			case RESET_TO_XML_ACTION:{ // server confrims
				if(verbose_) cout << "ofxRemoteUIClient: " << m.getRemoteIp() << " says RESET_TO_XML_ACTION!" << endl;
				requestCompleteUpdate();
				if(callBack != NULL){
					cbArg.action = SERVER_DID_RESET_TO_XML;
					callBack(cbArg);
				}
			}break;

			case RESET_TO_DEFAULTS_ACTION:{ // server confrims
				if(verbose_) cout << "ofxRemoteUIClient: " << m.getRemoteIp() << " says RESET_TO_DEFAULTS_ACTION!" << endl;
				requestCompleteUpdate();
				if(callBack != NULL){
					cbArg.action = SERVER_DID_RESET_TO_DEFAULTS;
					callBack(cbArg);
				}
			}break;

			case SAVE_CURRENT_STATE_ACTION: // server confirms that it has saved
				if(verbose_) cout << "ofxRemoteUIClient: " << m.getRemoteIp() << " says SAVE_CURRENT_STATE_ACTION!" << endl;
				if(callBack != NULL){
					cbArg.action = SERVER_CONFIRMED_SAVE;
					callBack(cbArg);
				}
				break;

			default: cout << "ofxRemoteUIClient::update >> UNKNOWN ACTION!!" <<endl; break;
		}
	}
}


void ofxRemoteUIClient::setPreset(string preset){
	sendSETP(preset);
}


void ofxRemoteUIClient::savePresetWithName(string presetName){
	sendSAVP(presetName);
}


void ofxRemoteUIClient::deletePreset(string presetName){
	sendDELP(presetName);
}



void ofxRemoteUIClient::fillPresetListFromMessage(ofxOscMessage m){

	int n = m.getNumArgs();

	//check if server has no presets at all
	if (m.getNumArgs() == 1){
		if(m.getArgAsString(0) == OFXREMOTEUI_NO_PRESETS){ //if no prests, server sends only one with this value
			//we know there's no presets
		}
	}
	presetNames.clear();
	for(int i = 0; i < n; i++){
		presetNames.push_back( m.getArgAsString(i));
	}
}


void ofxRemoteUIClient::trackParam(string paramName, float* param){
	RemoteUIParam p;
	map<string,RemoteUIParam>::iterator it = params.find(paramName);
	if ( it == params.end() ){	//not found! we add it
		p.type = REMOTEUI_PARAM_FLOAT;
	}else{
		p = params[paramName];
		if (p.type != REMOTEUI_PARAM_FLOAT ){
			cout << "wtf called trackParam(float) on a param that's not a float!" << endl;
		}
	}
	p.floatValAddr = param;
	addParamToDB(p, paramName);
}


void ofxRemoteUIClient::trackParam(string paramName, int* param){
	RemoteUIParam p;
	map<string,RemoteUIParam>::iterator it = params.find(paramName);
	if ( it == params.end() ){	//not found! we add it
		p.type = REMOTEUI_PARAM_INT;
	}else{
		p = params[paramName];
		if (p.type != REMOTEUI_PARAM_INT ){
			cout << "wtf called trackParam(int) on a param that's not a int!" << endl;
		}
	}
	p.intValAddr = param;
	addParamToDB(p, paramName);
}


void ofxRemoteUIClient::trackParam(string paramName, int* param, vector<string> list){ //TODO!
	RemoteUIParam p;
	map<string,RemoteUIParam>::iterator it = params.find(paramName);
	if ( it == params.end() ){	//not found! we add it
		p.type = REMOTEUI_PARAM_ENUM;
	}else{
		p = params[paramName];
		if (p.type != REMOTEUI_PARAM_ENUM ){
			cout << "wtf called trackParam(int) on a param that's not a int!" << endl;
		}
	}
	p.intValAddr = param;
	addParamToDB(p, paramName);
}


void ofxRemoteUIClient::trackParam(string paramName, string* param){
	RemoteUIParam p;
	map<string,RemoteUIParam>::iterator it = params.find(paramName);
	if ( it == params.end() ){	//not found! we add it
		p.type = REMOTEUI_PARAM_STRING;
	}else{
		p = params[paramName];
		if (p.type != REMOTEUI_PARAM_STRING ){
			cout << "wtf called trackParam(string) on a param that's not a string!" << endl;
		}
	}
	p.stringValAddr = param;
	addParamToDB(p, paramName);
}


void ofxRemoteUIClient::trackParam(string paramName, bool* param){
	RemoteUIParam p;
	map<string,RemoteUIParam>::iterator it = params.find(paramName);
	if ( it == params.end() ){	//not found! we add it
		p.type = REMOTEUI_PARAM_BOOL;
	}else{
		p = params[paramName];
		if (p.type != REMOTEUI_PARAM_BOOL ){
			cout << "wtf called trackParam(bool) on a param that's not a bool!" << endl;
		}
	}
	p.boolValAddr = param;
	addParamToDB(p, paramName);
}


void ofxRemoteUIClient::trackParam(string paramName, unsigned char* param){
	RemoteUIParam p;
	map<string,RemoteUIParam>::iterator it = params.find(paramName);
	if ( it == params.end() ){	//not found! we add it
		p.type = REMOTEUI_PARAM_COLOR;
	}else{
		p = params[paramName];
		if (p.type != REMOTEUI_PARAM_COLOR ){
			cout << "wtf called trackParam(bool) on a param that's not a color!" << endl;
		}
	}
	p.redValAddr = param;
	addParamToDB(p, paramName);
}


void ofxRemoteUIClient::sendUntrackedParamUpdate(RemoteUIParam p, string paramName){
	//p.print();
	params[paramName] = p; //TODO error check!
	vector<string>list;
	list.push_back(paramName);
	sendUpdateForParamsInList(list);
}

void ofxRemoteUIClient::sendTrackedParamUpdate(string paramName){

	map<string,RemoteUIParam>::iterator it = params.find(paramName);
	if ( it != params.end() ){
		syncParamToPointer(paramName);
		sendParam(paramName, params[paramName]);
	}else{
		cout << "ofxRemoteUIClient::sendTrackedParamUpdate >> param '" + paramName + "' not found!" << endl;
	}
}


void ofxRemoteUIClient::requestCompleteUpdate(){
	//cout << "ofxRemoteUIClient: requestCompleteUpdate()" << endl;
	if(readyToSend){
		sendREQU();
		vector<string> a;
		sendPREL(a);
	}
}


bool ofxRemoteUIClient::isReadyToSend(){
	return readyToSend;
}




