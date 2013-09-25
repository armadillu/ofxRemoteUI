//
//  ofxRemoteUI.cpp
//  emptyExample
//
//  Created by Oriol Ferrer Mesià on 09/01/13.
//
//

#include "ofxRemoteUI.h"
#include <iostream>

ofxRemoteUIServer::ofxRemoteUIServer(){
	readyToSend = false;
}

ofxRemoteUIClient::ofxRemoteUIClient(){
	readyToSend = false;
}

void ofxRemoteUIServer::setup(float updateInterval_){
	updateInterval = updateInterval_;
	receiver.setup(OFXREMOTEUI_SERVER_PORT);
}

void ofxRemoteUIClient::setup(string address, float updateInterval_){

	updateInterval = updateInterval_;
	host = address;
	cout << "ofxRemoteUIClient listening... " << endl;
	receiver.setup(OFXREMOTEUI_CLIENT_PORT);

	cout << "ofxRemoteUIClient connecting to " << address << endl;
	sender.setup(address, OFXREMOTEUI_SERVER_PORT);
}


void ofxRemoteUIServer::update(float dt){

	time += dt;
	if (time > updateInterval){
		time = 0.0f;
		vector<string> changes = scanForUpdatedParams(); //sends changed params to client
		//cout << "ofxRemoteUIServer: sent " << ofToString(changes.size()) << " updates to client" << endl;
		updateParamsInList(changes);
	}

	while( receiver.hasWaitingMessages() ){// check for waiting messages from client

		ofxOscMessage m;
		receiver.getNextMessage(&m);

		if (!readyToSend){ // if not connected, connect to our friend so we can talk back
			connect(m.getRemoteIp(), OFXREMOTEUI_CLIENT_PORT);
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
				updateParamsInList(paramsList);
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

			default: cout << "ofxRemoteUIServer::update >> ERR!" <<endl; break;
		}
	}
}


void ofxRemoteUIClient::update(float dt){

	if (!readyToSend){ // if not connected, connect
		sendHELLO(); //on first connect, send HI!
		readyToSend = true;
	}

	time += dt;
	if (time > updateInterval){
		time = 0.0f;
		//vector<string> changes = scanForUpdatedParams(); //sends changed params to client
	}

	while( receiver.hasWaitingMessages() ){// check for waiting messages from client

		ofxOscMessage m;
		receiver.getNextMessage(&m);

		DecodedMessage dm = decode(m);

		switch (dm.action) {

			case HELO_ACTION: //server says hi back, we ask for a big update
				cout << "ofxRemoteUIClient: " << m.getRemoteIp() << " answered HELLO!" << endl;
				sendREQUEST();
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
				cout << "ofxRemoteUIClient: " << m.getRemoteIp() << " says CIAO!";
				sendCIAO();
				readyToSend = false;
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


void ofxRemoteUIServer::shareParam(string paramName, bool* param ){
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


void ofxRemoteUIServer::shareParam(string paramName, string* param ){
	RemoteUIParam p;
	p.type = REMOTEUI_PARAM_STRING;
	p.stringValAddr = param;
	p.stringVal = *param;
	addParamToDB(p, paramName);
}


void ofxRemoteUIServer::addParamToDB(RemoteUIParam p, string paramName){

	map<string,RemoteUIParam>::iterator it = params.find(paramName);
	if ( it == params.end() ){	//not found!
		params[paramName] = p;
	}else{
		cout << "already sharing a Param with that name " << paramName <<"!!" << endl;
	}
}

void ofxRemoteUI::connect(string ipAddress, int port){
	sender.setup(ipAddress, port);
	readyToSend = true;
}


DecodedMessage ofxRemoteUI::decode(ofxOscMessage m){

	string msgAddress = m.getAddress();
	string action = msgAddress.substr(0, 4);

	//cout << msgAddress << " action >> " << action << endl;

	DecodedMessage dm;

	if (msgAddress.length() >= 3) {
		if (action == "HELO") dm.action = HELO_ACTION;
		else
			if (action == "REQU") dm.action = REQUEST_ACTION;
		else
			if (action == "SEND") dm.action = SEND_ACTION;
		else
			if (action == "CIAO") dm.action = CIAO_ACTION;
	}

	if (msgAddress.length() >= 8) {
		string arg1 = msgAddress.substr(5, 3);
		//cout << msgAddress << " arg1 >> " << arg1 << endl;
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

	RemoteUIParam p = params[dm.paramName];

	switch (dm.argument) {
		case FLT_ARG:
 			p.floatVal = m.getArgAsFloat(0);
			p.minFloat = m.getArgAsFloat(1);
			p.maxFloat = m.getArgAsFloat(2);
			//cout << "updated " << dm.paramName << "to a new value: " << ofToString(p.floatVal) << endl;
			if (p.floatValAddr){
				*p.floatValAddr = p.floatVal;
			}break;

		case INT_ARG:
			p.intVal = m.getArgAsInt32(0);
			p.minInt = m.getArgAsInt32(1);
			p.maxInt = m.getArgAsInt32(2);
			//cout << "updated " << dm.paramName << "to a new value: " << ofToString(p.intVal) << endl;
			if (p.intValAddr){
				*p.intValAddr = p.intVal;
			}break;

		case BOL_ARG:
			p.boolVal = m.getArgAsInt32(0) == 0 ? false : true;
			//cout << "updated " << dm.paramName << "to a new value: " << ofToString(p.boolVal) << endl;
			if (p.boolValAddr){
				*p.boolValAddr = p.boolVal;
			}break;

		case STR_ARG:
			p.stringVal = m.getArgAsString(0);
			//cout << "updated " << dm.paramName << "to a new value: " << ofToString(p.stringVal) << endl;
			if (p.stringValAddr){
				*p.stringValAddr = p.stringVal;
			}break;

		case NULL_ARG: cout << "updateParamFromDecodedMessage NULL type!" << endl; break;
		default: cout << "updateParamFromDecodedMessage unknown type!" << endl; break;
	}
	params[dm.paramName] = p;
}



vector<string> ofxRemoteUI::getAllParamNamesList(){

	vector<string>paramsList;
	for( map<string,RemoteUIParam>::iterator ii = params.begin(); ii != params.end(); ++ii ){
		string name = (*ii).first;
		paramsList.push_back(name);
	}
	return paramsList;
}


vector<string> ofxRemoteUI::scanForUpdatedParams(){

	vector<string>paramsPendingUpdate;

	for( map<string,RemoteUIParam>::iterator ii = params.begin(); ii != params.end(); ++ii ){

		RemoteUIParam p = (*ii).second;
		if ( hasParamChanged(p) ){
			//cout << "scanForUpdatedParams: found '" << (*ii).first << + "'" << endl;
			paramsPendingUpdate.push_back( (*ii).first );
			syncParamToPinter((*ii).first);
		}
	}
	return paramsPendingUpdate;
}

void ofxRemoteUI::updateParamsInList(vector<string>paramsPendingUpdate){

	for(int i = 0; i < paramsPendingUpdate.size(); i++){
		//cout << "ofxRemoteUIServer: sending updated param " + paramsPendingUpdate[i] << endl;
		RemoteUIParam p = params[paramsPendingUpdate[i]];
		sendParam(paramsPendingUpdate[i], p);
	}
}

void ofxRemoteUI::syncParamToPinter(string paramName){

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


void ofxRemoteUI::sendParam(string paramName, RemoteUIParam p){
	ofxOscMessage m;
	m.setAddress("SEND " + stringForParamType(p.type) + " " + paramName);
	switch (p.type) {
		case REMOTEUI_PARAM_FLOAT: m.addFloatArg(p.floatVal); m.addFloatArg(p.minFloat); m.addFloatArg(p.maxFloat); break;
		case REMOTEUI_PARAM_INT: m.addIntArg(p.intVal); m.addIntArg(p.minInt); m.addIntArg(p.maxInt); break;
		case REMOTEUI_PARAM_BOOL: m.addIntArg(p.boolVal ? 1 : 0); /*cout << "sending bool" << endl; */ break;
		case REMOTEUI_PARAM_STRING: m.addStringArg(p.stringVal); /*cout << "sending string" << endl; */ break;
	}
	sender.sendMessage(m);
}


RemoteUIParam ofxRemoteUI::getParamForName(string paramName){

	RemoteUIParam p;
	map<string,RemoteUIParam>::iterator it = params.find(paramName);
	if ( it != params.end() ){	//not found!
		p = params[paramName];
	}else{
		cout << "ofxRemoteUIClient::getParamForName >> param " + paramName + " not found!" << endl;
	}

	return p;
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

