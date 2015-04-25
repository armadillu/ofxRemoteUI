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
	broadcastReceiver.setup(OFXREMOTEUI_BROADCAST_PORT);
	OSCsetup = false;
	disconnectStrikes = OFXREMOTEUI_DISCONNECTION_STRIKES;
}

void ofxRemoteUIClient::setCallback( void (*callb)(RemoteUIClientCallBackArg) ){
	callBack = callb;
}

bool ofxRemoteUIClient::setup(string address, int port_){

	params.clear();
	orderedKeys.clear();
	presetNames.clear();

	if (port_ < 1000){
		RUI_LOG_ERROR << "cant use such low port";
		OSCsetup = false;
		return OSCsetup;
	}

	port = port_;
	avgTimeSinceLastReply = timeSinceLastReply = timeCounter = 0.0f;
	waitingForReply = false;
	host = address;
	RUI_LOG_NOTICE << "listening at port " << port + 1 << " ... " ;
	clearOscReceiverMsgQueue();
	oscReceiver.setup(port + 1);

	if(verbose_) RUI_LOG_NOTICE << "connecting to " << address;
	try{
		oscSender.setup(address, port);
		OSCsetup = true;
	}catch(exception e){
		RUI_LOG_ERROR << "exception setting up oscSender" << e.what();
		OSCsetup = false;
	}
	return OSCsetup;
}


vector<Neighbor> ofxRemoteUIClient::getNeighbors(){
	return closebyServers.getNeighbors();
}


void ofxRemoteUIClient::disconnect(){

	OSC_CHECK;
	if (readyToSend){
		if(verbose_) RUI_LOG_NOTICE << "disconnect()" ;
		sendCIAO();
		readyToSend = false;
		presetNames.clear();
	}else{
		if(verbose_) RUI_LOG_NOTICE << "can't disconnect(); we arent connected!" ;
	}
}


void ofxRemoteUIClient::connect(){
	OSC_CHECK;
	if(!readyToSend){
		if(verbose_) RUI_LOG_NOTICE << "connect()" ;
		sendHELLO();	//on first connect, send HI!
		sendTEST();		//and a lag test
		readyToSend = true;
		disconnectStrikes = OFXREMOTEUI_DISCONNECTION_STRIKES;
	}else{
		if(verbose_) RUI_LOG_NOTICE << "can't connect() now, we are already connected!" ;
	}
}


void ofxRemoteUIClient::saveCurrentStateToDefaultXML(){
	OSC_CHECK;
	sendSAVE();
}


void ofxRemoteUIClient::restoreAllParamsToInitialXML(){
	OSC_CHECK;
	sendRESX();
}


void ofxRemoteUIClient::restoreAllParamsToDefaultValues(){
	OSC_CHECK;
	sendRESD();
}


void ofxRemoteUIClient::updateAutoDiscovery(float dt){

	bool neigbhorChange = false;
	bool neighborJustLaunched = false;
	//listen for broadcast from all servers in the broadcast channel OFXREMOTEUI_BROADCAST_PORT
	while( broadcastReceiver.hasWaitingMessages() ){// check for waiting messages from client
		ofxOscMessage m;
		broadcastReceiver.getNextMessage(&m);
		neigbhorChange |= closebyServers.gotPing( m.getRemoteIp(), m.getArgAsInt32(0)/*port*/, m.getArgAsString(1), m.getArgAsString(2));
		//read the broadcast sequence number
		int broadcastSequenceNumber = m.getArgAsInt32(3);
		neighborJustLaunched = (broadcastSequenceNumber == 0); //keep track of just launched apps
		if(neighborJustLaunched){
			if(callBack != NULL){
				RemoteUIClientCallBackArg cbArg; // to notify our "delegate"
				cbArg.action = NEIGHBOR_JUST_LAUNCHED_SERVER;
				cbArg.host = m.getRemoteIp();
				cbArg.port = m.getArgAsInt32(0);
				callBack(cbArg);
			}
		}
		//cout << "got broadcast message from " << m.getRemoteIp() << ":" << m.getArgAsInt32(0) ;
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
}


void ofxRemoteUIClient::update(float dt){

	//cout << "ofxRemoteUIClient::update readyToSend = " << readyToSend ;


	if (readyToSend){ // if connected

		OSC_CHECK;

		timeCounter += dt;
		timeSinceLastReply += dt;

		if (timeCounter > OFXREMOTEUI_LATENCY_TEST_RATE){

			if(waitingForReply){ //we never heard back from the client, keep count of how many we missed
				RUI_LOG_NOTICE << "missed one TEST Packet... (" << disconnectStrikes << " left)" ;
				disconnectStrikes--;
			}else{
				disconnectStrikes = OFXREMOTEUI_DISCONNECTION_STRIKES; //reset the strike count, we heard back from server
			}

			if(disconnectStrikes >= 0){ //we got a reply, time to send another test pkt

				timeCounter = 0.0f; //reset timer
				sendTEST();
			}else{ //we tried for too long, out of strikes! assume server is gone
				RUI_LOG_WARNING << "disconnecting bc server connection timed out!" ;
				avgTimeSinceLastReply = -1;
				disconnect(); // testing here
				params.clear();
				orderedKeys.clear();
			}
		}

		while( oscReceiver.hasWaitingMessages() ){// check for waiting messages from server

			ofxOscMessage m;
			oscReceiver.getNextMessage(&m);

			DecodedMessage dm = decode(m);
			RemoteUIClientCallBackArg cbArg; // to notify our "delegate"
			cbArg.host = m.getRemoteIp();

			switch (dm.action) {

				case SEND_LOG_LINE_ACTION:{
					cbArg.msg = m.getArgAsString(0); //only one arg for the log msg
					if(callBack != NULL){
						cbArg.action = SERVER_SENT_LOG_LINE;
						callBack(cbArg);
					}else{
						RUI_LOG_NOTICE << cbArg.msg ;
					}
				}break;

				case HELO_ACTION:{ //server says hi back, we ask for a big update
					requestCompleteUpdate();
					if(callBack != NULL){
						cbArg.action = SERVER_CONNECTED;
						callBack(cbArg);
					}
				}break;

				case REQUEST_ACTION: //server closed the REQU, so we should have all the params
					if(verbose_) RUI_LOG_VERBOSE << m.getRemoteIp() << " says REQUEST_ACTION!" ;
					if(callBack != NULL){
						cbArg.action = SERVER_SENT_FULL_PARAMS_UPDATE;
						callBack(cbArg);
					}
					break;

				case SEND_PARAM_ACTION:{ //server is sending us an updated val
					if(verbose_) RUI_LOG_VERBOSE << m.getRemoteIp() << " says SEND_PARAM_ACTION!" ;
					updateParamFromDecodedMessage(m, dm);
					gotNewInfo = true;
				}
					break;

				case CIAO_ACTION:{
					if(verbose_) RUI_LOG_VERBOSE << m.getRemoteIp() << " says CIAO!" ;
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
					if(verbose_) RUI_LOG_VERBOSE << m.getRemoteIp() << " says PRESET_LIST_ACTION!" ;
					fillPresetListFromMessage(m);
					if(callBack != NULL){
						cbArg.action = SERVER_PRESETS_LIST_UPDATED;
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

				case SET_PRESET_ACTION: // server confirms that it has set the preset, request a full update
					if(verbose_) RUI_LOG_VERBOSE << m.getRemoteIp() << " says SET_PRESET_ACTION!" ;
					requestCompleteUpdate();
					if(callBack != NULL){
						cbArg.action = SERVER_DID_SET_PRESET;
						cbArg.msg = m.getArgAsString(0);
						callBack(cbArg);
					}
					break;

				case SAVE_PRESET_ACTION:{ // server confirms that it has save the preset,
					if(verbose_) RUI_LOG_VERBOSE << m.getRemoteIp() << " says SAVE_PRESET_ACTION!" ;
					vector<string> a;
					sendPREL(a); //request preset list to server, send empty vector
					cbArg.action = SERVER_SAVED_PRESET;
					cbArg.msg = m.getArgAsString(0);
					if(callBack != NULL) callBack(cbArg);
				}break;

				case DELETE_PRESET_ACTION:{ // server confirms that it has deleted preset
					if(verbose_) RUI_LOG_VERBOSE << m.getRemoteIp() << " says DELETE_PRESET_ACTION!" ;
					vector<string> a;
					sendPREL(a); //request preset list to server, send empty vector
					if(callBack != NULL){
						cbArg.action = SERVER_DELETED_PRESET;
						cbArg.msg = m.getArgAsString(0);
						callBack(cbArg);
					}
				}break;

				case RESET_TO_XML_ACTION:{ // server confrims
					if(verbose_) RUI_LOG_VERBOSE << m.getRemoteIp() << " says RESET_TO_XML_ACTION!" ;
					requestCompleteUpdate();
					if(callBack != NULL){
						cbArg.action = SERVER_DID_RESET_TO_XML;
						callBack(cbArg);
					}
				}break;

				case RESET_TO_DEFAULTS_ACTION:{ // server confrims
					if(verbose_) RUI_LOG_VERBOSE  << m.getRemoteIp() << " says RESET_TO_DEFAULTS_ACTION!" ;
					requestCompleteUpdate();
					if(callBack != NULL){
						cbArg.action = SERVER_DID_RESET_TO_DEFAULTS;
						callBack(cbArg);
					}
				}break;

				case SAVE_CURRENT_STATE_ACTION: // server confirms that it has saved
					if(verbose_) RUI_LOG_VERBOSE << m.getRemoteIp() << " says SAVE_CURRENT_STATE_ACTION!" ;
					if(callBack != NULL){
						cbArg.action = SERVER_CONFIRMED_SAVE;
						callBack(cbArg);
					}
					break;

				case SET_GROUP_PRESET_ACTION: // server confirms that it has set the group preset, request a full update
					if(verbose_) RUI_LOG_VERBOSE << m.getRemoteIp() << " says SET_GROUP_PRESET_ACTION!" ;
					requestCompleteUpdate();
					if(callBack != NULL){
						cbArg.action = SERVER_DID_SET_GROUP_PRESET;
						cbArg.msg = m.getArgAsString(0);
						cbArg.group = m.getArgAsString(1);
						callBack(cbArg);
					}
					break;

				case SAVE_GROUP_PRESET_ACTION:{ // server confirms that it has saved the group preset,
					if(verbose_) RUI_LOG_VERBOSE << m.getRemoteIp() << " says SAVE_GROUP_PRESET_ACTION!" ;
					vector<string> a;
					sendPREL(a); //request preset list to server, send empty vector
					cbArg.action = SERVER_SAVED_GROUP_PRESET;
					cbArg.msg = m.getArgAsString(0);
					cbArg.group = m.getArgAsString(1);
					if(callBack != NULL) callBack(cbArg);
				}break;

				case DELETE_GROUP_PRESET_ACTION:{ // server confirms that it has deleted the group preset
					if(verbose_) RUI_LOG_VERBOSE << m.getRemoteIp() << " says DELETE_GROUP_PRESET_ACTION!" ;
					vector<string> a;
					sendPREL(a); //request preset list to server, send empty vector
					if(callBack != NULL){
						cbArg.action = SERVER_DELETED_GROUP_PRESET;
						cbArg.msg = m.getArgAsString(0);
						cbArg.group = m.getArgAsString(1);
						callBack(cbArg);
					}
				}break;

				default: RUI_LOG_ERROR << "update >> UNKNOWN ACTION!!" <<endl; break;
			}
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

void ofxRemoteUIClient::setGroupPreset(string preset, string group){
	sendSETp(preset, group);
}

void ofxRemoteUIClient::saveGroupPresetWithName(string presetName, string group){
	sendSAVp(presetName, group);
}

void ofxRemoteUIClient::deleteGroupPreset(string presetName, string group){
	sendDELp(presetName, group);
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
	unordered_map<string, RemoteUIParam>::iterator it = params.find(paramName);
	if ( it == params.end() ){	//not found! we add it
		p.type = REMOTEUI_PARAM_FLOAT;
	}else{
		p = params[paramName];
		if (p.type != REMOTEUI_PARAM_FLOAT ){
			RUI_LOG_ERROR << "wtf called trackParam(float) on a param that's not a float!" ;
		}
	}
	p.floatValAddr = param;
	addParamToDB(p, paramName);
}


void ofxRemoteUIClient::trackParam(string paramName, int* param){
	RemoteUIParam p;
	unordered_map<string, RemoteUIParam>::iterator it = params.find(paramName);
	if ( it == params.end() ){	//not found! we add it
		p.type = REMOTEUI_PARAM_INT;
	}else{
		p = params[paramName];
		if (p.type != REMOTEUI_PARAM_INT ){
			RUI_LOG_ERROR << "wtf called trackParam(int) on a param that's not a int!" ;
		}
	}
	p.intValAddr = param;
	addParamToDB(p, paramName);
}


void ofxRemoteUIClient::trackParam(string paramName, int* param, vector<string> list){ //TODO!
	RemoteUIParam p;
	unordered_map<string, RemoteUIParam>::iterator it = params.find(paramName);
	if ( it == params.end() ){	//not found! we add it
		p.type = REMOTEUI_PARAM_ENUM;
	}else{
		p = params[paramName];
		if (p.type != REMOTEUI_PARAM_ENUM ){
			RUI_LOG_ERROR << "wtf called trackParam(int) on a param that's not a int!" ;
		}
	}
	p.intValAddr = param;
	addParamToDB(p, paramName);
}


void ofxRemoteUIClient::trackParam(string paramName, string* param){
	RemoteUIParam p;
	unordered_map<string, RemoteUIParam>::iterator it = params.find(paramName);
	if ( it == params.end() ){	//not found! we add it
		p.type = REMOTEUI_PARAM_STRING;
	}else{
		p = params[paramName];
		if (p.type != REMOTEUI_PARAM_STRING ){
			RUI_LOG_ERROR << "wtf called trackParam(string) on a param that's not a string!" ;
		}
	}
	p.stringValAddr = param;
	addParamToDB(p, paramName);
}


void ofxRemoteUIClient::trackParam(string paramName, bool* param){
	RemoteUIParam p;
	unordered_map<string, RemoteUIParam>::iterator it = params.find(paramName);
	if ( it == params.end() ){	//not found! we add it
		p.type = REMOTEUI_PARAM_BOOL;
	}else{
		p = params[paramName];
		if (p.type != REMOTEUI_PARAM_BOOL ){
			RUI_LOG_ERROR << "wtf called trackParam(bool) on a param that's not a bool!" ;
		}
	}
	p.boolValAddr = param;
	addParamToDB(p, paramName);
}


void ofxRemoteUIClient::trackParam(string paramName, unsigned char* param){
	RemoteUIParam p;
	unordered_map<string, RemoteUIParam>::iterator it = params.find(paramName);
	if ( it == params.end() ){	//not found! we add it
		p.type = REMOTEUI_PARAM_COLOR;
	}else{
		p = params[paramName];
		if (p.type != REMOTEUI_PARAM_COLOR ){
			RUI_LOG_ERROR << "wtf called trackParam(color) on a param that's not a color!" ;
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

	unordered_map<string, RemoteUIParam>::iterator it = params.find(paramName);
	if ( it != params.end() ){
		syncParamToPointer(paramName);
		sendParam(paramName, params[paramName]);
	}else{
		RUI_LOG_ERROR << "sendTrackedParamUpdate >> param '" + paramName + "' not found!" ;
	}
}


void ofxRemoteUIClient::requestCompleteUpdate(){
	//cout << "ofxRemoteUIClient: requestCompleteUpdate()" ;
	if(readyToSend){
		sendREQU();
		vector<string> a;
		sendPREL(a);
	}
}


bool ofxRemoteUIClient::isReadyToSend(){
	return readyToSend;
}

bool ofxRemoteUIClient::isSetup(){
	return OSCsetup;
}




