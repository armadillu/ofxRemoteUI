//
//  ofxRemoteUI.cpp
//  emptyExample
//
//  Created by Oriol Ferrer Mesià on 09/01/13.
//
//

#include "ofxRemoteUI.h"
#include <iostream>

bool ofxRemoteUI::ready(){
	return readyToSend;
}

float ofxRemoteUI::connectionLag(){
	return avgTimeSinceLastReply;
}


void ofxRemoteUI::addParamToDB(RemoteUIParam p, string paramName){

	//see if we already had it, if we didnt, set its add order #
	map<string,RemoteUIParam>::iterator it = params.find(paramName);
	if ( it == params.end() ){	//not found!
		//cout << "adding key: " << paramName <<endl;
		params[paramName] = p;
		keyOrder[ (int)keyOrder.size() ] = paramName;
	}else{
		params[paramName] = p;
		cout << "already have a Param with that name on the DB : " << paramName <<"!!" << endl;
	}
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
	//cout << "getAllParamNamesList(): ";
	for( map<int,string>::iterator ii = keyOrder.begin(); ii != keyOrder.end(); ++ii ){
		string paramName = (*ii).second;
		paramsList.push_back(paramName);
		//cout << paramName << ", ";
	}
	//cout << endl;
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

