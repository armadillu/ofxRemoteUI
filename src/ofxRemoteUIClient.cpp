//
//  ofxRemoteUI.cpp
//  emptyExample
//
//  Created by Oriol Ferrer Mesià on 09/01/13.
//
//

#include "ofxRemoteUIClient.h"
#include <iostream>

using namespace std;

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
		RLOG_ERROR << "cant use such low port";
		OSCsetup = false;
		return OSCsetup;
	}

	port = port_;
	avgTimeSinceLastReply = timeSinceLastReply = timeCounter = 0.0f;
	waitingForReply = false;
	host = address;
	RLOG_NOTICE << "listening at port " << port + 1 << " ... " ;
	clearOscReceiverMsgQueue();
	try{
		oscReceiver.setup(port + 1);
	}catch(...){
		RLOG_ERROR << "exception setting up OSC RX";
	}

	if(verbose_) RLOG_NOTICE << "connecting to " << address;
	try{
		oscSender.setup(address, port);
		OSCsetup = true;
	}catch(exception e){
		RLOG_ERROR << "exception setting up oscSender" << e.what();
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
		if(verbose_) RLOG_NOTICE << "disconnect()" ;
		sendCIAO();
		readyToSend = false;
		presetNames.clear();
	}else{
		if(verbose_) RLOG_NOTICE << "can't disconnect(); we arent connected!" ;
	}
}


void ofxRemoteUIClient::connect(){
	OSC_CHECK;
	if(!readyToSend){
		if(verbose_) RLOG_NOTICE << "connect()" ;
		sendHELLO();	//on first connect, send HI!
		sendTEST();		//and a lag test
		readyToSend = true;
		disconnectStrikes = OFXREMOTEUI_DISCONNECTION_STRIKES;
	}else{
		if(verbose_) RLOG_NOTICE << "can't connect() now, we are already connected!" ;
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
		broadcastReceiver.getNextMessage(m);
		string host = m.getRemoteIp();
		neigbhorChange |= closebyServers.gotPing( host, m.getArgAsInt32(0)/*port*/, m.getArgAsString(1)/*computerName*/, m.getArgAsString(2)/*binaryName*/);
		//read the broadcast sequence number
		int broadcastSequenceNumber = m.getArgAsInt32(3);
		neighborJustLaunched = (broadcastSequenceNumber == 0); //keep track of just launched apps
		if(neighborJustLaunched){
			if(callBack != NULL){
				RemoteUIClientCallBackArg cbArg; // to notify our "delegate"
				cbArg.action = NEIGHBOR_JUST_LAUNCHED_SERVER;
				cbArg.host = host;
				cbArg.port = m.getArgAsInt32(0);
				callBack(cbArg);
			}
		}
		//cout << "got broadcast message from " << host << ":" << m.getArgAsInt32(0) ;
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
				RLOG_NOTICE << "missed one TEST Packet... (" << disconnectStrikes << " left)" ;
				disconnectStrikes--;
			}else{
				disconnectStrikes = OFXREMOTEUI_DISCONNECTION_STRIKES; //reset the strike count, we heard back from server
			}

			if(disconnectStrikes >= 0){ //we got a reply, time to send another test pkt

				timeCounter = 0.0f; //reset timer
				sendTEST();
			}else{ //we tried for too long, out of strikes! assume server is gone
				RLOG_WARNING << "disconnecting bc server connection timed out!" ;
				avgTimeSinceLastReply = -1;
				disconnect(); // testing here
				params.clear();
				orderedKeys.clear();
			}
		}

		while( oscReceiver.hasWaitingMessages() ){// check for waiting messages from server

			ofxOscMessage m;
			oscReceiver.getNextMessage(m);

			DecodedMessage dm = decode(m);
			RemoteUIClientCallBackArg cbArg; // to notify our "delegate"
			string host = m.getRemoteIp();
			cbArg.host = host;

			switch (dm.action) {

				case SEND_LOG_LINE_ACTION:{
					cbArg.msg = m.getArgAsString(0); //only one arg for the log msg
					if(callBack != NULL){
						cbArg.action = SERVER_SENT_LOG_LINE;
						callBack(cbArg);
					}else{
						RLOG_NOTICE << cbArg.msg ;
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
					if(verbose_) RLOG_VERBOSE << host << " says REQUEST_ACTION!" ;
					if(callBack != NULL){
						cbArg.action = SERVER_SENT_FULL_PARAMS_UPDATE;
						callBack(cbArg);
					}
					break;

				case SEND_PARAM_ACTION:{ //server is sending us an updated val
					if(verbose_) RLOG_VERBOSE << host << " says SEND_PARAM_ACTION!" ;
					updateParamFromDecodedMessage(m, dm);
				}
					break;

				case CIAO_ACTION:{
					if(verbose_) RLOG_VERBOSE << host << " says CIAO!" ;
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
					if(verbose_) RLOG_VERBOSE << host << " says PRESET_LIST_ACTION!" ;
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
							cbArg.paramList.emplace_back(m.getArgAsString(i));
						}
						callBack(cbArg);
					}
					break;

				case SET_PRESET_ACTION: // server confirms that it has set the preset, request a full update
					if(verbose_) RLOG_VERBOSE << host << " says SET_PRESET_ACTION!" ;
					requestCompleteUpdate();
					if(callBack != NULL){
						cbArg.action = SERVER_DID_SET_PRESET;
						cbArg.msg = m.getArgAsString(0);
						callBack(cbArg);
					}
					break;

				case SAVE_PRESET_ACTION:{ // server confirms that it has save the preset,
					if(verbose_) RLOG_VERBOSE << host << " says SAVE_PRESET_ACTION!" ;
					vector<string> a;
					sendPREL(a); //request preset list to server, send empty vector
					cbArg.action = SERVER_SAVED_PRESET;
					cbArg.msg = m.getArgAsString(0);
					if(callBack != NULL) callBack(cbArg);
				}break;

				case DELETE_PRESET_ACTION:{ // server confirms that it has deleted preset
					if(verbose_) RLOG_VERBOSE << host << " says DELETE_PRESET_ACTION!" ;
					vector<string> a;
					sendPREL(a); //request preset list to server, send empty vector
					if(callBack != NULL){
						cbArg.action = SERVER_DELETED_PRESET;
						cbArg.msg = m.getArgAsString(0);
						callBack(cbArg);
					}
				}break;

				case RESET_TO_XML_ACTION:{ // server confrims
					if(verbose_) RLOG_VERBOSE << host << " says RESET_TO_XML_ACTION!" ;
					requestCompleteUpdate();
					if(callBack != NULL){
						cbArg.action = SERVER_DID_RESET_TO_XML;
						callBack(cbArg);
					}
				}break;

				case RESET_TO_DEFAULTS_ACTION:{ // server confrims
					if(verbose_) RLOG_VERBOSE  << host << " says RESET_TO_DEFAULTS_ACTION!" ;
					requestCompleteUpdate();
					if(callBack != NULL){
						cbArg.action = SERVER_DID_RESET_TO_DEFAULTS;
						callBack(cbArg);
					}
				}break;

				case SAVE_CURRENT_STATE_ACTION: // server confirms that it has saved
					if(verbose_) RLOG_VERBOSE << host << " says SAVE_CURRENT_STATE_ACTION!" ;
					if(callBack != NULL){
						cbArg.action = SERVER_CONFIRMED_SAVE;
						callBack(cbArg);
					}
					break;

				case SET_GROUP_PRESET_ACTION: // server confirms that it has set the group preset, request a full update
					if(verbose_) RLOG_VERBOSE << host << " says SET_GROUP_PRESET_ACTION!" ;
					requestCompleteUpdate();
					if(callBack != NULL){
						cbArg.action = SERVER_DID_SET_GROUP_PRESET;
						cbArg.msg = m.getArgAsString(0);
						cbArg.group = m.getArgAsString(1);
						callBack(cbArg);
					}
					break;

				case SAVE_GROUP_PRESET_ACTION:{ // server confirms that it has saved the group preset,
					if(verbose_) RLOG_VERBOSE << host << " says SAVE_GROUP_PRESET_ACTION!" ;
					vector<string> a;
					sendPREL(a); //request preset list to server, send empty vector
					cbArg.action = SERVER_SAVED_GROUP_PRESET;
					cbArg.msg = m.getArgAsString(0);
					cbArg.group = m.getArgAsString(1);
					if(callBack != NULL) callBack(cbArg);
				}break;

				case DELETE_GROUP_PRESET_ACTION:{ // server confirms that it has deleted the group preset
					if(verbose_) RLOG_VERBOSE << host << " says DELETE_GROUP_PRESET_ACTION!" ;
					vector<string> a;
					sendPREL(a); //request preset list to server, send empty vector
					if(callBack != NULL){
						cbArg.action = SERVER_DELETED_GROUP_PRESET;
						cbArg.msg = m.getArgAsString(0);
						cbArg.group = m.getArgAsString(1);
						callBack(cbArg);
					}
				}break;

				case REMOVE_PARAM:{
					if(verbose_) RLOG_NOTICE << "client confirms that it removed param named: " << dm.paramName;
					removeParamFromDB(dm.paramName);
					if(callBack != NULL){
						cbArg.action = SERVER_ASKED_TO_REMOVE_PARAM;
						cbArg.msg = dm.paramName;
						callBack(cbArg);
					}
				}

				default: RLOG_ERROR << "update >> UNKNOWN ACTION!!" <<endl; break;
			}
		}
	}
}

void ofxRemoteUIClient::removeParamFromDB(const string & paramName){

	auto it = params.find(paramName);

	if (it != params.end()){

		if(verbose_) RLOG_WARNING << "removing Param '" << paramName << "' from DB!" ;

		params.erase(it);

		it = paramsFromCode.find(paramName);
		if (it != paramsFromCode.end()){
			paramsFromCode.erase(paramsFromCode.find(paramName));
		}

		it = paramsFromXML.find(paramName);
		if (it != paramsFromXML.end()){
			paramsFromXML.erase(it);
		}

		//re-create orderedKeys
		vector<string> myOrderedKeys;
		for(auto iterator = orderedKeys.begin(); iterator != orderedKeys.end(); iterator++) {
			if (iterator->second != paramName){
				myOrderedKeys.emplace_back(iterator->second);
			}
		}
		orderedKeys.clear();
		for(int i = 0; i < myOrderedKeys.size(); i++){
			orderedKeys[i] = myOrderedKeys[i];
		}

	}else{
		RLOG_ERROR << "removeParamFromDB >> trying to delete an nonexistent param (" << paramName << ")" ;
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
		presetNames.emplace_back( m.getArgAsString(i));
	}
	std::sort(presetNames.begin(), presetNames.end());
}


void ofxRemoteUIClient::trackParam(string paramName, float* param){
	RemoteUIParam p;
	auto it = params.find(paramName);
	if ( it == params.end() ){	//not found! we add it
		p.type = REMOTEUI_PARAM_FLOAT;
	}else{
		p = params[paramName];
		if (p.type != REMOTEUI_PARAM_FLOAT ){
			RLOG_ERROR << "wtf called trackParam(float) on a param that's not a float!" ;
		}
	}
	p.floatValAddr = param;
	addParamToDB(p, paramName);
}


void ofxRemoteUIClient::trackParam(string paramName, int* param){
	RemoteUIParam p;
	auto it = params.find(paramName);
	if ( it == params.end() ){	//not found! we add it
		p.type = REMOTEUI_PARAM_INT;
	}else{
		p = params[paramName];
		if (p.type != REMOTEUI_PARAM_INT ){
			RLOG_ERROR << "wtf called trackParam(int) on a param that's not a int!" ;
		}
	}
	p.intValAddr = param;
	addParamToDB(p, paramName);
}


void ofxRemoteUIClient::trackParam(string paramName, int* param, vector<string> list){ //FIXME: trackParam
	RemoteUIParam p;
	auto it = params.find(paramName);
	if ( it == params.end() ){	//not found! we add it
		p.type = REMOTEUI_PARAM_ENUM;
	}else{
		p = params[paramName];
		if (p.type != REMOTEUI_PARAM_ENUM ){
			RLOG_ERROR << "wtf called trackParam(int) on a param that's not a int!" ;
		}
	}
	p.intValAddr = param;
	addParamToDB(p, paramName);
}


void ofxRemoteUIClient::trackParam(string paramName, string* param){
	RemoteUIParam p;
	auto it = params.find(paramName);
	if ( it == params.end() ){	//not found! we add it
		p.type = REMOTEUI_PARAM_STRING;
	}else{
		p = params[paramName];
		if (p.type != REMOTEUI_PARAM_STRING ){
			RLOG_ERROR << "wtf called trackParam(string) on a param that's not a string!" ;
		}
	}
	p.stringValAddr = param;
	addParamToDB(p, paramName);
}


void ofxRemoteUIClient::trackParam(string paramName, bool* param){
	RemoteUIParam p;
	auto it = params.find(paramName);
	if ( it == params.end() ){	//not found! we add it
		p.type = REMOTEUI_PARAM_BOOL;
	}else{
		p = params[paramName];
		if (p.type != REMOTEUI_PARAM_BOOL ){
			RLOG_ERROR << "wtf called trackParam(bool) on a param that's not a bool!" ;
		}
	}
	p.boolValAddr = param;
	addParamToDB(p, paramName);
}


void ofxRemoteUIClient::trackParam(string paramName, unsigned char* param){
	RemoteUIParam p;
	auto it = params.find(paramName);
	if ( it == params.end() ){	//not found! we add it
		p.type = REMOTEUI_PARAM_COLOR;
	}else{
		p = params[paramName];
		if (p.type != REMOTEUI_PARAM_COLOR ){
			RLOG_ERROR << "wtf called trackParam(color) on a param that's not a color!" ;
		}
	}
	p.redValAddr = param;
	addParamToDB(p, paramName);
}


void ofxRemoteUIClient::sendUntrackedParamUpdate(RemoteUIParam p, string paramName){
	//p.print();
	params[paramName] = p; //FIXME: error check!
	vector<string>list;
	list.emplace_back(paramName);
	sendUpdateForParamsInList(list);
}

void ofxRemoteUIClient::sendTrackedParamUpdate(string paramName){

	auto it = params.find(paramName);
	if ( it != params.end() ){
		syncParamToPointer(paramName);
		sendParam(paramName, params[paramName]);
	}else{
		RLOG_ERROR << "sendTrackedParamUpdate >> param '" + paramName + "' not found!" ;
	}
}

void ofxRemoteUIClient::sendMessage(ofxOscMessage m){
    oscSender.sendMessage(m);
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




