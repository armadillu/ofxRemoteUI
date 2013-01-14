//
//  ofxRemoteUI.cpp
//  emptyExample
//
//  Created by Oriol Ferrer Mesià on 09/01/13.
//
//

#include "ofxRemoteUI.h"
#include <iostream>

ofxRemoteUIServer* ofxRemoteUIServer::singleton = NULL;


ofxRemoteUIServer* ofxRemoteUIServer::instance(){
	if (!singleton){   // Only allow one instance of class to be generated.
		singleton = new ofxRemoteUIServer();
	}
	return singleton;
}


ofxRemoteUIServer::ofxRemoteUIServer(){
	readyToSend = false;
	timeSinceLastReply = 0;
	avgTimeSinceLastReply = 0;
	waitingForReply = false;
}



ofxRemoteUIClient::ofxRemoteUIClient(){
	readyToSend = false;
	timeSinceLastReply = 0;
	avgTimeSinceLastReply = 0;
	waitingForReply = false;
}


bool ofxRemoteUI::ready(){
	return readyToSend;
}

void ofxRemoteUIServer::close(){
	if(readyToSend)
		sendCIAO();
}

float ofxRemoteUI::connectionLag(){
	return avgTimeSinceLastReply;
}


void ofxRemoteUIServer::setup(int port_, float updateInterval_){
	params.clear();
	updateInterval = updateInterval_;
	avgTimeSinceLastReply = timeSinceLastReply = time = 0.0f;
	port = port_;
	receiver.setup(port);
}


void ofxRemoteUIClient::setup(string address, int port_){

	params.clear();
	port = port_;
	avgTimeSinceLastReply = timeSinceLastReply = time = 0.0f;
	host = address;
	cout << "ofxRemoteUIClient listening at port " << port + 1 << " ... " << endl;
	receiver.setup(port + 1);

	cout << "ofxRemoteUIClient connecting to " << address << endl;
	sender.setup(address, port);
}


void ofxRemoteUIServer::update(float dt){

	time += dt;
	timeSinceLastReply  += dt;
	if(readyToSend){
		if (time > updateInterval){
			time = 0.0f;
			vector<string> changes = scanForUpdatedParamsAndSync(); //sends changed params to client
			//cout << "ofxRemoteUIServer: sent " << ofToString(changes.size()) << " updates to client" << endl;
			//sendUpdateForParamsInList(changes);
		}
	}

	while( receiver.hasWaitingMessages() ){// check for waiting messages from client

		ofxOscMessage m;
		receiver.getNextMessage(&m);

		if (!readyToSend){ // if not connected, connect to our friend so we can talk back
			connect(m.getRemoteIp(), port + 1);
		}

		DecodedMessage dm = decode(m);

		switch (dm.action) {

			case HELO_ACTION: //if client says hi, say hi back
				sendHELLO();
				cout << "ofxRemoteUIServer: " << m.getRemoteIp() << " says HELLO!"  << endl;
				break;

			case REQUEST_ACTION:{ //send all params to client
				cout << "ofxRemoteUIServer: " << m.getRemoteIp() << " sends REQU!"  << endl;
				vector<string>paramsList = getAllParamNamesList();
				sendUpdateForParamsInList(paramsList);
			}
				break;

			case SEND_ACTION:{ //client is sending us an updated val
				cout << "ofxRemoteUIServer: " << m.getRemoteIp() << " sends SEND!"  << endl;
				updateParamFromDecodedMessage(m, dm);
			}
				break;

			case CIAO_ACTION:
				cout << "ofxRemoteUIServer: " << m.getRemoteIp() << " says CIAO!" << endl;
				sendCIAO();
				readyToSend = false;
				break;

			case TEST_ACTION: // we got a request from client, lets bounce back asap.
				sendTEST();
				//cout << "ofxRemoteUIServer: " << m.getRemoteIp() << " says TEST!" << endl;
				break;

			default: cout << "ofxRemoteUIServer::update >> ERR!" <<endl; break;
		}
	}
}


void ofxRemoteUIClient::update(float dt){

	if (!readyToSend){ // if not connected, connect

		cout << "ofxRemoteUIClient: sending HELLO!" << endl;
		sendHELLO();	//on first connect, send HI!
		sendTEST();		//and a lag test
		readyToSend = true;

	}else{

		time += dt;
		timeSinceLastReply += dt;

		if (time > LATENCY_TEST_RATE){
			if (!waitingForReply){
				time = 0.0f;
				sendTEST();
			}else{
				if (time > CONNECTION_TIMEOUT){
					avgTimeSinceLastReply = -1;
				}
			}
		}
	}

	while( receiver.hasWaitingMessages() ){// check for waiting messages from client

		ofxOscMessage m;
		receiver.getNextMessage(&m);

		DecodedMessage dm = decode(m);

		switch (dm.action) {

			case HELO_ACTION: //server says hi back, we ask for a big update
				cout << "ofxRemoteUIClient: " << m.getRemoteIp() << " answered HELLO!" << endl;
				requestCompleteUpdate();
				break;

			case REQUEST_ACTION: //should not happen, server doesnt request
				cout << "ofxRemoteUIClient: " << m.getRemoteIp() << " send REQU??? WTF!!!"  << endl;
				break;

			case SEND_ACTION:{ //server is sending us an updated val
					cout << "ofxRemoteUIClient: " << m.getRemoteIp() << " sends us a param update SEND!"  << endl;
					updateParamFromDecodedMessage(m, dm);
				}
				break;

			case CIAO_ACTION:
				cout << "ofxRemoteUIClient: " << m.getRemoteIp() << " says CIAO!" << endl;
				sendCIAO();
				readyToSend = false;
				break;

			case TEST_ACTION: // we got a reply from the server, lets measure how long it took;
				waitingForReply = false;
				if (avgTimeSinceLastReply > 0.0f){
					avgTimeSinceLastReply = 0.8 * (avgTimeSinceLastReply) + 0.2 * (timeSinceLastReply);
				}else{
					avgTimeSinceLastReply = timeSinceLastReply ;
				}
				timeSinceLastReply = 0.0f;
				//cout << "ofxRemoteUIClient: " << m.getRemoteIp() << " replied TEST!" << " and took " << avgTimeSinceLastReply << endl;
				break;

			default: cout << "ofxRemoteUIClient::update >> ERR!" <<endl; break;
		}
	}
}


void ofxRemoteUIServer::shareParam(string paramName, float* param, float min, float max){
	RemoteUIParam p;
	p.type = REMOTEUI_PARAM_FLOAT;
	p.floatValAddr = param;
	p.floatVal = *param;
	p.maxFloat = max;
	p.minFloat = min;
	addParamToDB(p, paramName);
}


void ofxRemoteUIServer::shareParam(string paramName, bool* param, int nothing , int nothing2  ){
	RemoteUIParam p;
	p.type = REMOTEUI_PARAM_BOOL;
	p.boolValAddr = param;
	p.boolVal = *param;
	addParamToDB(p, paramName);
}


void ofxRemoteUIServer::shareParam(string paramName, int* param, int min, int max){
	RemoteUIParam p;
	p.type = REMOTEUI_PARAM_INT;
	p.intValAddr = param;
	p.intVal = *param;
	p.maxInt = max;
	p.minInt = min;
	addParamToDB(p, paramName);
}


void ofxRemoteUIServer::shareParam(string paramName, string* param, int nothing, int nothing2 ){
	RemoteUIParam p;
	p.type = REMOTEUI_PARAM_STRING;
	p.stringValAddr = param;
	p.stringVal = *param;
	addParamToDB(p, paramName);
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
	
	//params[paramName] = p;
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
	//params[paramName] = p;
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
	//params[paramName] = p;
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
	//params[paramName] = p;
	addParamToDB(p, paramName);
}

void ofxRemoteUI::addParamToDB(RemoteUIParam p, string paramName){

	//see if we already had it, if we didnt, set its add order #
	map<string,RemoteUIParam>::iterator it = params.find(paramName);
	if ( it == params.end() ){	//not found!
		cout << "adding key: " << paramName <<endl;
		params[paramName] = p;
		keyOrder[ (int)keyOrder.size() ] = paramName;
	}else{
		params[paramName] = p;
		cout << "already have a Param with that name on the DB : " << paramName <<"!!" << endl;
	}
}

void ofxRemoteUIServer::connect(string ipAddress, int port){
	avgTimeSinceLastReply = timeSinceLastReply = time = 0.0f;
	//params.clear();
	sender.setup(ipAddress, port);
	readyToSend = true;
}


vector<string> ofxRemoteUI::getChangedParamsList(){
	std::vector<string> result (paramsChangedSinceLastCheck.begin(), paramsChangedSinceLastCheck.end());
	paramsChangedSinceLastCheck.clear();
	return result;
}

DecodedMessage ofxRemoteUI::decode(ofxOscMessage m){

	string msgAddress = m.getAddress();
	string action = msgAddress.substr(0, 4);

	//cout <<"Decode: "<< msgAddress << " action >> " << action << endl;

	DecodedMessage dm;

	if (msgAddress.length() >= 3) {
		if (action == "HELO") dm.action = HELO_ACTION;
		else
			if (action == "REQU") dm.action = REQUEST_ACTION;
			else
				if (action == "SEND") dm.action = SEND_ACTION;
				else
					if (action == "CIAO") dm.action = CIAO_ACTION;
					else
						if (action == "TEST") dm.action = TEST_ACTION;
	}

	if (msgAddress.length() >= 8) {
		string arg1 = msgAddress.substr(5, 3);
		//cout << "Decode"  << msgAddress << " arg1 >> " << arg1 << endl;
		dm.argument = NULL_ARG;

		if (arg1 == "FLT") dm.argument = FLT_ARG;
		else
			if (arg1 == "INT") dm.argument = INT_ARG;
			else
				if (arg1 == "BOL") dm.argument = BOL_ARG;
				else
					if (arg1 == "STR") dm.argument = STR_ARG;
	}

	if (msgAddress.length() >= 9) {
		string paramName = msgAddress.substr(9, msgAddress.length() - 9);
		//cout << msgAddress << " >> paramName: >" << paramName << "<" << endl;
		dm.paramName = paramName;
	}

	return dm;
}


void ofxRemoteUI::updateParamFromDecodedMessage(ofxOscMessage m, DecodedMessage dm){

	string paramName = dm.paramName;
	RemoteUIParam original;
	bool newParam = true;
	map<string,RemoteUIParam>::iterator it = params.find(paramName);
	if ( it != params.end() ){	//not found!
		original = params[paramName];
		newParam = false;
	}

	RemoteUIParam p = original;

	switch (dm.argument) {
		case FLT_ARG:
			p.type = REMOTEUI_PARAM_FLOAT;
 			p.floatVal = m.getArgAsFloat(0);
			p.minFloat = m.getArgAsFloat(1);
			p.maxFloat = m.getArgAsFloat(2);
			//cout << "updated " << dm.paramName << "to a new value: " << p.floatVal << endl;
			if (p.floatValAddr){
				*p.floatValAddr = p.floatVal;
			}break;

		case INT_ARG:
			p.type = REMOTEUI_PARAM_INT;
			p.intVal = m.getArgAsInt32(0);
			p.minInt = m.getArgAsInt32(1);
			p.maxInt = m.getArgAsInt32(2);
			//cout << "updated " << dm.paramName << "to a new value: " << p.intVal << endl;
			if (p.intValAddr){
				*p.intValAddr = p.intVal;
			}break;

		case BOL_ARG:
			p.type = REMOTEUI_PARAM_BOOL;
			p.boolVal = m.getArgAsInt32(0) == 0 ? false : true;
			//cout << "updated " << dm.paramName << "to a new value: " << p.boolVal << endl;
			if (p.boolValAddr){
				*p.boolValAddr = p.boolVal;
			}break;

		case STR_ARG:
			p.type = REMOTEUI_PARAM_STRING;
			p.stringVal = m.getArgAsString(0);
			//cout << "updated " << dm.paramName << "to a new value: " << (p.stringVal) << endl;
			if (p.stringValAddr){
				*p.stringValAddr = p.stringVal;
			}break;

		case NULL_ARG: cout << "updateParamFromDecodedMessage NULL type!" << endl; break;
		default: cout << "updateParamFromDecodedMessage unknown type!" << endl; break;
	}

	//keep track of the change
	//if(std::find(paramsChangedSinceLastCheck.begin(), paramsChangedSinceLastCheck.end(), paramName) == paramsChangedSinceLastCheck.end()){ //not found
	//	paramsChangedSinceLastCheck.push_back(paramName);
	//}
	if ( !p.isEqualTo(original)  || newParam ){ // if the udpdate changed the param, keep track of it
		paramsChangedSinceLastCheck.insert(paramName);
	}

	//here we update our param db
	//params[paramName] = p;
	if(newParam)
		addParamToDB(p, paramName);
	else
		params[paramName] = p;
}



vector<string> ofxRemoteUI::getAllParamNamesList(){

	vector<string>paramsList;

	//get list of params in add order
	cout << "getAllParamNamesList(): ";
	for( map<int,string>::iterator ii = keyOrder.begin(); ii != keyOrder.end(); ++ii ){
		string paramName = (*ii).second;
		paramsList.push_back(paramName);
		cout << paramName << ", ";
	}
	cout << endl;
	return paramsList;
}


vector<string> ofxRemoteUI::scanForUpdatedParamsAndSync(){

	vector<string>paramsPendingUpdate;

	for( map<string,RemoteUIParam>::iterator ii = params.begin(); ii != params.end(); ++ii ){

		RemoteUIParam p = (*ii).second;
		if ( hasParamChanged(p) ){
			//cout << "scanForUpdatedParamsAndSync: found '" << (*ii).first << + "'" << endl;
			paramsPendingUpdate.push_back( (*ii).first );
			syncParamToPointer((*ii).first);
		}
	}
	return paramsPendingUpdate;
}


void ofxRemoteUI::sendUpdateForParamsInList(vector<string>paramsPendingUpdate){

	for(int i = 0; i < paramsPendingUpdate.size(); i++){
		//cout << "ofxRemoteUIServer: sending updated param " + paramsPendingUpdate[i] << endl;
		RemoteUIParam p = params[paramsPendingUpdate[i]];
		sendParam(paramsPendingUpdate[i], p);
	}
}


void ofxRemoteUIClient::sendParamUpdate(RemoteUIParam p, string paramName){

	//p.print();
	params[paramName] = p; //TODO error check!
	vector<string>list;
	list.push_back(paramName);
	sendUpdateForParamsInList(list);
}


void ofxRemoteUI::syncParamToPointer(string paramName){

	RemoteUIParam p = params[paramName];

	switch (p.type) {
		case REMOTEUI_PARAM_FLOAT:
			if (p.floatValAddr){
				p.floatVal = *p.floatValAddr;
			}break;

		case REMOTEUI_PARAM_INT:
			if (p.intValAddr){
				p.intVal = *p.intValAddr;
			}break;

		case REMOTEUI_PARAM_BOOL:
			if (p.boolValAddr){
				p.boolVal = *p.boolValAddr;
			}break;

		case REMOTEUI_PARAM_STRING:
			if (p.stringValAddr){
				p.stringVal = *p.stringValAddr;
			}break;
	}

	params[paramName] = p;
}


bool ofxRemoteUI::hasParamChanged(RemoteUIParam p){

	switch (p.type) {
		case REMOTEUI_PARAM_FLOAT:
			if (p.floatValAddr){
				if (*p.floatValAddr != p.floatVal) return true; else return false;
			}
			//cout << "RemoteUIParam REMOTEUI_PARAM_FLOAT missing reference" << endl;
			return false;

		case REMOTEUI_PARAM_INT:
			if (p.intValAddr){
				if (*p.intValAddr != p.intVal) return true; else return false;
			}
			//cout << "RemoteUIParam REMOTEUI_PARAM_INT missing reference" << endl;
			return false;

		case REMOTEUI_PARAM_BOOL:
			if (p.boolValAddr){
				if (*p.boolValAddr != p.boolVal) return true; else return false;
			}
			//cout << "RemoteUIParam REMOTEUI_PARAM_BOOL missing reference" << endl;
			return false;

		case REMOTEUI_PARAM_STRING:
			if (p.stringValAddr){
				if (*p.stringValAddr != p.stringVal) return true; else return false;
			}
			//cout << "RemoteUIParam REMOTEUI_PARAM_STRING missing reference" << endl;
			return false;

	}
	cout << "ofxRemoteUIServer::hasParamChanged >> something went wrong, unknown param type" << endl;
	return false;
}


string ofxRemoteUI::stringForParamType(RemoteUIParamType t){
	switch (t) {
		case REMOTEUI_PARAM_FLOAT: return "FLT";
		case REMOTEUI_PARAM_INT: return "INT";
		case REMOTEUI_PARAM_BOOL: return "BOL";
		case REMOTEUI_PARAM_STRING: return "STR";
	}
	cout << "ofxRemoteUI::stringForParamType >> UNKNOWN TYPE!" << endl;
	return "ERR";
}


RemoteUIParam ofxRemoteUI::getParamForName(string paramName){

	RemoteUIParam p;
	map<string,RemoteUIParam>::iterator it = params.find(paramName);
	if ( it != params.end() ){	// found!
		p = params[paramName];
	}else{
		cout << "ofxRemoteUIClient::getParamForName >> param " + paramName + " not found!" << endl;
	}

	return p;
}


void ofxRemoteUIClient::requestCompleteUpdate(){
	//cout << "ofxRemoteUIClient: requestCompleteUpdate()" << endl;
	if(readyToSend){
		sendREQUEST();
	}
}


void ofxRemoteUIClient::sendUpdatedParam(string paramName){

	map<string,RemoteUIParam>::iterator it = params.find(paramName);
	if ( it != params.end() ){
		syncParamToPointer(paramName);
		sendParam(paramName, params[paramName]);
	}else{
		cout << "ofxRemoteUIClient::sendUpdatedParam >> param '" + paramName + "' not found!" << endl;
	}
}


void ofxRemoteUI::sendParam(string paramName, RemoteUIParam p){
	ofxOscMessage m;
	//printf("sending >> %s ", paramName.c_str());
	//p.print();
	m.setAddress("SEND " + stringForParamType(p.type) + " " + paramName);
	switch (p.type) {
		case REMOTEUI_PARAM_FLOAT: m.addFloatArg(p.floatVal); m.addFloatArg(p.minFloat); m.addFloatArg(p.maxFloat); break;
		case REMOTEUI_PARAM_INT: m.addIntArg(p.intVal); m.addIntArg(p.minInt); m.addIntArg(p.maxInt); break;
		case REMOTEUI_PARAM_BOOL: m.addIntArg(p.boolVal ? 1 : 0); /*cout << "sending bool" << endl; */ break;
		case REMOTEUI_PARAM_STRING: m.addStringArg(p.stringVal); /*cout << "sending string" << endl; */ break;
	}
	if(timeSinceLastReply == 0.0f) timeSinceLastReply = 0.0;
	sender.sendMessage(m);
}

void ofxRemoteUI::sendTEST(){
	//cout << "sendTEST()" << endl;
	waitingForReply = true;
	timeSinceLastReply = 0.0f;
	ofxOscMessage m;
	m.setAddress("TEST");
	sender.sendMessage(m);
}

void ofxRemoteUIClient::sendREQUEST(){
	ofxOscMessage m;
	m.setAddress("REQU");
	sender.sendMessage(m);
}


void ofxRemoteUI::sendHELLO(){
	ofxOscMessage m;
	m.setAddress("HELO");
	sender.sendMessage(m);
}


void ofxRemoteUI::sendCIAO(){
	ofxOscMessage m;
	m.setAddress("CIAO");
	sender.sendMessage(m);
}

