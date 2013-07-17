//
//  ofxRemoteUI.cpp
//  emptyExample
//
//  Created by Oriol Ferrer Mesià on 09/01/13.
//
//

#include "ofxRemoteUI.h"
#include <iostream>
#include <stdlib.h>
#include "uriencode.h"
#include <sstream>

bool ofxRemoteUI::ready(){
	return readyToSend;
}

void ofxRemoteUI::setVerbose(bool b){
	verbose = b;
}


float ofxRemoteUI::connectionLag(){
	return avgTimeSinceLastReply;
}


vector<string> ofxRemoteUI::getPresetsList(){
	return presetNames;
}

void ofxRemoteUI::addParamToDB(RemoteUIParam p, string paramName){

	//see if we already had it, if we didnt, set its add order #
	map<string,RemoteUIParam>::iterator it = params.find(paramName);
	if ( it == params.end() ){	//not found!
		//cout << "adding key: " << paramName <<endl;
		params[paramName] = p;
		orderedKeys[ (int)orderedKeys.size() ] = paramName;
		paramsFromCode[paramName] = p; //cos this didnt exist before, we store it as "from code"
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
				if (action == "SEND") dm.action = SEND_PARAM_ACTION;
				else
					if (action == "CIAO") dm.action = CIAO_ACTION;
					else
						if (action == "TEST") dm.action = TEST_ACTION;
						else
							if (action == "DELP") dm.action = DELETE_PRESET_ACTION;
							else
								if (action == "PREL") dm.action = PRESET_LIST_ACTION;
								else
									if (action == "SAVP") dm.action = SAVE_PRESET_ACTION;
									else
										if (action == "SETP") dm.action = SET_PRESET_ACTION;
										else
											if (action == "RESX") dm.action = RESET_TO_XML_ACTION;
											else
												if (action == "RESD") dm.action = RESET_TO_DEFAULTS_ACTION;
												else
													if (action == "SAVE") dm.action = SAVE_CURRENT_STATE_ACTION;

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
					else
						if (arg1 == "ENU") dm.argument = ENUM_ARG;
						else
							if (arg1 == "COL") dm.argument = COLOR_ARG;


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
	int arg = 0;

	p.group = dm.paramGroup;
	switch (dm.argument) {
		case FLT_ARG:
			p.type = REMOTEUI_PARAM_FLOAT;
 			p.floatVal = m.getArgAsFloat(arg); arg++;
			p.minFloat = m.getArgAsFloat(arg); arg++;
			p.maxFloat = m.getArgAsFloat(arg); arg++;
			//if(verbose)cout << "updateParamFromDecodedMessage: " << dm.paramName << " to a new value: " << p.floatVal << endl;
			if (p.floatValAddr){
				*p.floatValAddr = p.floatVal;
			}break;

		case INT_ARG:
			p.type = REMOTEUI_PARAM_INT;
			p.intVal = m.getArgAsInt32(arg); arg++;
			p.minInt = m.getArgAsInt32(arg); arg++;
			p.maxInt = m.getArgAsInt32(arg); arg++;
			//if(verbose)cout << "updateParamFromDecodedMessage: " << dm.paramName << " to a new value: " << p.intVal << endl;
			if (p.intValAddr){
				*p.intValAddr = p.intVal;
			}break;

		case COLOR_ARG:
			p.type = REMOTEUI_PARAM_COLOR;
			p.redVal = (int)m.getArgAsInt32(arg); arg++;
			p.greenVal = (int)m.getArgAsInt32(arg); arg++;
			p.blueVal = (int)m.getArgAsInt32(arg); arg++;
			p.alphaVal = (int)m.getArgAsInt32(arg); arg++;
			//if(verbose)cout << "updateParamFromDecodedMessage: " << dm.paramName << " to a new value: " <<(int)p.redVal<<" "<<(int)p.greenVal<<" "<<(int)p.blueVal<<" "<<(int)p.alphaVal << endl;
			if (p.redValAddr){
				*p.redValAddr = p.redVal;
				*(p.redValAddr+1) = p.greenVal;
				*(p.redValAddr+2) = p.blueVal;
				*(p.redValAddr+3) = p.alphaVal;
			}break;

		case ENUM_ARG:{
			p.type = REMOTEUI_PARAM_ENUM;
			p.intVal = m.getArgAsInt32(arg); arg++;
			p.minInt = m.getArgAsInt32(arg); arg++;
			p.maxInt = m.getArgAsInt32(arg); arg++;
			//if(verbose)cout << "updateParamFromDecodedMessage: " << dm.paramName << " to a new value: " << p.intVal << endl;
			if (p.intValAddr){
				*p.intValAddr = p.intVal;
			}
			int n = m.getNumArgs() - 5 - 3; // 3 >> the int vals, 5 >> 4 colors + 1 group
			int i = 0;
			for (i = 0; i < n; i++) {
				p.enumList.push_back( m.getArgAsString(arg + i) );
			}
			arg = arg + i;
			}break;

		case BOL_ARG:
			p.type = REMOTEUI_PARAM_BOOL;
			p.boolVal = m.getArgAsInt32(arg) == 0 ? false : true; arg++;
			//if(verbose)cout << "updateParamFromDecodedMessage: " << dm.paramName << " to a new value: " << p.boolVal << endl;
			if (p.boolValAddr){
				*p.boolValAddr = p.boolVal;
			}break;

		case STR_ARG:
			p.type = REMOTEUI_PARAM_STRING;
			p.stringVal = m.getArgAsString(arg); arg++;
			//if(verbose)cout << "updateParamFromDecodedMessage: " << dm.paramName << " to a new value: " << (p.stringVal) << endl;
			if (p.stringValAddr){
				*p.stringValAddr = p.stringVal;
			}break;

		case NULL_ARG: cout << "updateParamFromDecodedMessage NULL type!" << endl; break;
		default: cout << "updateParamFromDecodedMessage unknown type!" << endl; break;
	}

	p.r = m.getArgAsInt32(arg); arg++;
	p.g = m.getArgAsInt32(arg); arg++;
	p.b = m.getArgAsInt32(arg); arg++;
	p.a = m.getArgAsInt32(arg); arg++;
	p.group = m.getArgAsString(arg); arg++;

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
	for( map<int,string>::iterator ii = orderedKeys.begin(); ii != orderedKeys.end(); ++ii ){
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


void ofxRemoteUI::sendUpdateForParamsInList(vector<string>list){

	for(int i = 0; i < list.size(); i++){
		RemoteUIParam p = params[list[i]];
		//cout << "ofxRemoteUIServer: sending updated param " + list[i]; p.print();
		sendParam(list[i], p);
	}
}

void ofxRemoteUI::syncAllParamsToPointers(){
	for( map<string,RemoteUIParam>::iterator ii = params.begin(); ii != params.end(); ++ii ){
		syncParamToPointer( (*ii).first );
	}
}

void ofxRemoteUI::syncAllPointersToParams(){
	for( map<string,RemoteUIParam>::iterator ii = params.begin(); ii != params.end(); ++ii ){
		syncPointerToParam( (*ii).first );
	}
}

void ofxRemoteUI::syncPointerToParam(string paramName){

	RemoteUIParam p = params[paramName];

	switch (p.type) {
		case REMOTEUI_PARAM_FLOAT:
			if (p.floatValAddr){
				*p.floatValAddr = p.floatVal;
			}break;

		case REMOTEUI_PARAM_ENUM:
		case REMOTEUI_PARAM_INT:
			if (p.intValAddr){
				*p.intValAddr = p.intVal;
			}break;

		case REMOTEUI_PARAM_COLOR:
			if (p.redValAddr){
				*p.redValAddr = p.redVal ;
				*(p.redValAddr+1) = p.greenVal;
				*(p.redValAddr+2) = p.blueVal;
				*(p.redValAddr+3) = p.alphaVal;
			}break;

		case REMOTEUI_PARAM_BOOL:
			if (p.boolValAddr){
				*p.boolValAddr = p.boolVal;
			}break;

		case REMOTEUI_PARAM_STRING:
			if (p.stringValAddr){
				*p.stringValAddr = p.stringVal;
			}break;
	}

	params[paramName] = p;
}


void ofxRemoteUI::syncParamToPointer(string paramName){

	RemoteUIParam p = params[paramName];

	switch (p.type) {
		case REMOTEUI_PARAM_FLOAT:
			if (p.floatValAddr){
				p.floatVal = *p.floatValAddr;
			}break;

		case REMOTEUI_PARAM_ENUM:
		case REMOTEUI_PARAM_INT:
			if (p.intValAddr){
				p.intVal = *p.intValAddr;
			}break;

		case REMOTEUI_PARAM_COLOR:
			if (p.redValAddr){
				p.redVal = *p.redValAddr;
				p.greenVal = *(p.redValAddr+1);
				p.blueVal = *(p.redValAddr+2);
				p.alphaVal = *(p.redValAddr+3);
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
			return false;

		case REMOTEUI_PARAM_ENUM:
		case REMOTEUI_PARAM_INT:
			if (p.intValAddr){
				if (*p.intValAddr != p.intVal) return true; else return false;
			}
			return false;

		case REMOTEUI_PARAM_COLOR:
			if (p.redValAddr){
				if (*p.redValAddr != p.redVal || *(p.redValAddr+1) != p.greenVal || *(p.redValAddr+2) != p.blueVal || *(p.redValAddr+3) != p.alphaVal ) return true;
				else return false;
			}
			return false;

		case REMOTEUI_PARAM_BOOL:
			if (p.boolValAddr){
				if (*p.boolValAddr != p.boolVal) return true; else return false;
			}
			return false;

		case REMOTEUI_PARAM_STRING:
			if (p.stringValAddr){
				if (*p.stringValAddr != p.stringVal) return true; else return false;
			}
			return false;

	}
	cout << "ofxRemoteUIServer::hasParamChanged >> something went wrong, unknown param type" << endl;
	return false;
}


string ofxRemoteUI::stringForParamType(RemoteUIParamType t){
	switch (t) {
		case REMOTEUI_PARAM_FLOAT: return "FLT";
		case REMOTEUI_PARAM_INT: return "INT";
		case REMOTEUI_PARAM_COLOR: return "COL";
		case REMOTEUI_PARAM_ENUM: return "ENU";
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


string ofxRemoteUI::getValuesAsString(){
	stringstream out;
	map<int,string>::iterator it = orderedKeys.begin();
	while( it != orderedKeys.end() ){
		RemoteUIParam param = params[it->second];
		out << UriEncode(it->second) << "=";

		switch (param.type) {
			case REMOTEUI_PARAM_FLOAT: out << param.floatVal << endl; break;
			case REMOTEUI_PARAM_INT: out << param.intVal << endl; break;
			case REMOTEUI_PARAM_COLOR: out << (int)param.redVal << " " << (int)param.greenVal << " " << (int)param.blueVal << " " << (int)param.alphaVal << " " << endl; break;
			case REMOTEUI_PARAM_ENUM: out << param.intVal << endl; break;
			case REMOTEUI_PARAM_BOOL: out << (param.boolVal?"1":"0") << endl; break;
			case REMOTEUI_PARAM_STRING: out << UriEncode(param.stringVal) << endl; break;
		}
		++it;
	}
	return out.str();
}


void ofxRemoteUI::setValuesFromString( string values ){

	stringstream in(values);
	string name, value;
	vector<string>changedParam;

	while( !in.eof() ){
		getline( in, name, '=' );
		getline( in, value, '\n' );

		if( params.find( name ) != params.end() ){
			RemoteUIParam param = params[name];
			RemoteUIParam original = params[name];
			changedParam.push_back(name);
			stringstream valstr( UriDecode(value) );

			switch (param.type) {
				case REMOTEUI_PARAM_FLOAT: valstr >> param.floatVal; break;
				case REMOTEUI_PARAM_INT: valstr >> param.intVal; break;
				case REMOTEUI_PARAM_COLOR: {
					std::istringstream ss(UriDecode(value));
					std::string token;
					std::getline(ss, token, ' '); param.redVal = atoi(token.c_str());
					std::getline(ss, token, ' '); param.greenVal = atoi(token.c_str());
					std::getline(ss, token, ' '); param.blueVal = atoi(token.c_str());
					std::getline(ss, token, ' '); param.alphaVal = atoi(token.c_str());
				}
				case REMOTEUI_PARAM_ENUM: valstr >> param.intVal; break;
				case REMOTEUI_PARAM_BOOL: valstr >> param.boolVal; break;
				case REMOTEUI_PARAM_STRING: param.stringVal = valstr.str(); break;
			}

			if ( !param.isEqualTo(original) ){ // if the udpdate changed the param, keep track of it
				params[name] = param;
				paramsChangedSinceLastCheck.insert(name);
			}
		}
	}

	vector<string>::iterator it = changedParam.begin();
	while( it != changedParam.end() ){
		if ( params.find( *it ) != params.end()){
			RemoteUIParam param = params[*it];
			sendUntrackedParamUpdate(param, *it);
			cout << "sending update for " << *it << endl;
		}
		it++;
	}
}


void ofxRemoteUI::sendParam(string paramName, RemoteUIParam p){
	ofxOscMessage m;
	if(verbose){ printf("sending >> %s ", paramName.c_str()); p.print(); }
	m.setAddress("SEND " + stringForParamType(p.type) + " " + paramName);
	switch (p.type) {
		case REMOTEUI_PARAM_FLOAT: m.addFloatArg(p.floatVal); m.addFloatArg(p.minFloat); m.addFloatArg(p.maxFloat); break;
		case REMOTEUI_PARAM_INT: m.addIntArg(p.intVal); m.addIntArg(p.minInt); m.addIntArg(p.maxInt); break;
		case REMOTEUI_PARAM_COLOR: m.addIntArg(p.redVal); m.addIntArg(p.greenVal); m.addIntArg(p.blueVal); m.addIntArg(p.alphaVal); break;
		case REMOTEUI_PARAM_BOOL: m.addIntArg(p.boolVal ? 1 : 0); break;
		case REMOTEUI_PARAM_STRING: m.addStringArg(p.stringVal); break;
		case REMOTEUI_PARAM_ENUM:{
			m.addIntArg(p.intVal); m.addIntArg(p.minInt); m.addIntArg(p.maxInt);
			for (int i = 0; i < p.enumList.size(); i++) {
				m.addStringArg(p.enumList[i]);
			}
			}break;
	}
	m.addIntArg(p.r); m.addIntArg(p.g); m.addIntArg(p.b); m.addIntArg(p.a); // set bg color!
	m.addStringArg(p.group);
	if(timeSinceLastReply == 0.0f) timeSinceLastReply = 0.0;
	oscSender.sendMessage(m);
}

//if used by server, confirmation == YES
//else, NO
void ofxRemoteUI::sendREQU(bool confirmation){
	if(verbose) cout << "sendREQU()" << endl;
	ofxOscMessage m;
	m.setAddress("REQU");
	if (confirmation) m.addStringArg("OK");
	oscSender.sendMessage(m);
}


void ofxRemoteUI::sendRESX(bool confirm){
	if(verbose) cout << "sendRESX()" << endl;
	ofxOscMessage m;
	m.setAddress("RESX");
	if (confirm) m.addStringArg("OK");
	oscSender.sendMessage(m);
}

void ofxRemoteUI::sendRESD(bool confirm){
	if(verbose) cout << "sendRESD()" << endl;
	ofxOscMessage m;
	m.setAddress("RESD");
	if (confirm) m.addStringArg("OK");
	oscSender.sendMessage(m);
}

void ofxRemoteUI::sendSAVE(bool confirm){
	if(verbose) cout << "sendSAVE()" << endl;
	ofxOscMessage m;
	m.setAddress("SAVE");
	if (confirm) m.addStringArg("OK");
	oscSender.sendMessage(m);
}


void ofxRemoteUI::sendTEST(){
	if(verbose) cout << "sendTEST()" << endl;
	waitingForReply = true;
	timeSinceLastReply = 0.0f;
	ofxOscMessage m;
	m.setAddress("TEST");
	oscSender.sendMessage(m);
}

//on client call, presetNames should be empty vector (request ing the list)
//on server call, presetNames should have all the presetNames
void ofxRemoteUI::sendPREL( vector<string> presetNames_ ){
	if(verbose) cout << "sendPRES()" << endl;
	ofxOscMessage m;
	m.setAddress("PREL");
	if (presetNames_.size() == 0){ // if we are the client requesting a preset list, delete our current list
		presetNames.clear();
		m.addStringArg("OK");
	}
	for(int i = 0; i < presetNames_.size(); i++){
		m.addStringArg(presetNames_[i]);
	}
	oscSender.sendMessage(m);
}

//on client call, presetName should be the new preset name
//on server call, presetName should be empty string (so it will send "OK"
void ofxRemoteUI::sendSAVP(string presetName){
	if(verbose) cout << "sendSAVP()" << endl;
	ofxOscMessage m;
	m.setAddress("SAVP");
	if (presetName.size() > 0){
		m.addStringArg(presetName);
	}else{
		m.addStringArg("OK");
	}
	oscSender.sendMessage(m);
}

//on client call, presetName should be the new preset name
//on server call, presetName should be empty string (so it will send "OK"
void ofxRemoteUI::sendSETP(string presetName){
	if(verbose) cout << "sendSETP()" << endl;
	ofxOscMessage m;
	m.setAddress("SETP");
	if (presetName.size() > 0){
		m.addStringArg(presetName);
	}else{
		m.addStringArg("OK");
	}
	oscSender.sendMessage(m);
}

void ofxRemoteUI::sendDELP(string presetName){
	if(verbose) cout << "sendDELP()" << endl;
	ofxOscMessage m;
	m.setAddress("DELP");
	if (presetName.size() > 0){
		m.addStringArg(presetName);
	}else{
		m.addStringArg("OK");
	}
	oscSender.sendMessage(m);
}

void ofxRemoteUI::sendHELLO(){
	ofxOscMessage m;
	m.setAddress("HELO");
	oscSender.sendMessage(m);
}


void ofxRemoteUI::sendCIAO(){
	ofxOscMessage m;
	m.setAddress("CIAO");
	oscSender.sendMessage(m);
}

