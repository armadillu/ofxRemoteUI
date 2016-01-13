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

#if !defined(__APPLE__) && !defined(TARGET_WIN32)
    #ifndef __linux__
        #define __linux__
    #endif
#endif

#if defined(__APPLE__) || defined(__linux__)
#include <arpa/inet.h>
#include <net/if.h>
#include <ifaddrs.h>
#include <errno.h>
#endif

#ifdef TARGET_WIN32
#include <windows.h>
#include <iphlpapi.h>
#include <WinSock2.h>
#pragma comment(lib, "iphlpapi.lib")
#endif

void split(vector<string> &tokens, const string &text, char separator) {
	std::size_t start = 0, end = 0;
	while ((end = text.find(separator, start)) != string::npos) {
		tokens.push_back(text.substr(start, end - start));
		start = end + 1;
	}
	tokens.push_back(text.substr(start));
}


bool ofxRemoteUI::ready(){
	return readyToSend;
}

void ofxRemoteUI::setVerbose(bool b){
	verbose_ = b;
}


float ofxRemoteUI::connectionLag(){
	return avgTimeSinceLastReply;
}


vector<string> ofxRemoteUI::getPresetsList(){
	return presetNames;
}


void ofxRemoteUI::printAllParamsDebug(){
	if(orderedKeys.size() == 0) return;
	cout << "#### FULL PARAM LIST ################################" << endl;
	for(int i = 0; i < orderedKeys.size(); i++){
		string key = orderedKeys[i];
		RemoteUIParam thisP = params[key];
		cout << "   index: " << i << "  list: " << key << " > "; thisP.print();
	}
	cout << "####################################################" << endl;
}

void ofxRemoteUI::addParamToDB(RemoteUIParam p, string thisParamName){

	//see if we already had it, if we didnt, set its add order #
	unordered_map<string, RemoteUIParam>::iterator it = params.find(thisParamName);
	if ( it == params.end() ){	//not found!

		params[thisParamName] = p;
		orderedKeys[ (int)orderedKeys.size() ] = thisParamName;
		paramsFromCode[thisParamName] = p; //cos this didnt exist before, we store it as "from code"

	}else{
		params[thisParamName] = p;
		RLOG_WARNING << "already have a Param with that name on the DB : '" << thisParamName << "'. Ignoring it!";
	}
}


void ofxRemoteUI::clearOscReceiverMsgQueue(){
	ofxOscMessage tempM;
	//int c = 0;
	//delete all pending messages
	while (oscReceiver.getNextMessage(&tempM)) {
		//cout << "clearOscReceiverMsgQueue " << c << endl;
		//c++;
	}
}


vector<string> ofxRemoteUI::getChangedParamsList(){
	std::vector<string> result (paramsChangedSinceLastCheck.begin(), paramsChangedSinceLastCheck.end());
	paramsChangedSinceLastCheck.clear();
	return result;
}


DecodedMessage ofxRemoteUI::decode(ofxOscMessage m){

	string msgAddress = m.getAddress();
	if(msgAddress.size() > 0){
		if(msgAddress[0] == '/'){ //if address starts with "/", drop it to match the fucked up remoteUI protocol
			msgAddress = msgAddress.substr(1, msgAddress.size() - 1);
		}
	}
	string action = msgAddress.substr(0, 4);

	//cout <<"Decode: "<< msgAddress << " action >> " << action << endl;
	DecodedMessage dm;

	//this is the lazynes maximus!
	if (msgAddress.length() >= 3) {
		if (action == "HELO") dm.action = HELO_ACTION;
		else if (action == "REQU") dm.action = REQUEST_ACTION;
		else if (action == "SEND") dm.action = SEND_PARAM_ACTION;
		else if (action == "CIAO") dm.action = CIAO_ACTION;
		else if (action == "TEST") dm.action = TEST_ACTION;
		else if (action == "PREL") dm.action = PRESET_LIST_ACTION;
		else if (action == "DELP") dm.action = DELETE_PRESET_ACTION;
		else if (action == "SAVP") dm.action = SAVE_PRESET_ACTION;
		else if (action == "SETP") dm.action = SET_PRESET_ACTION;
		else if (action == "RESX") dm.action = RESET_TO_XML_ACTION;
		else if (action == "RESD") dm.action = RESET_TO_DEFAULTS_ACTION;
		else if (action == "SAVE") dm.action = SAVE_CURRENT_STATE_ACTION;
		else if (action == "MISP") dm.action = GET_MISSING_PARAMS_IN_PRESET;
		//groups (note lower case p, l)
		else if (action == "DELp") dm.action = DELETE_GROUP_PRESET_ACTION;
		else if (action == "SAVp") dm.action = SAVE_GROUP_PRESET_ACTION;
		else if (action == "SETp") dm.action = SET_GROUP_PRESET_ACTION;
		//log
		else if (action == "LOG_") dm.action = SEND_LOG_LINE_ACTION;
	}

	if (msgAddress.length() >= 8) {
		string arg1 = msgAddress.substr(5, 3);
		//cout << "Decode"  << msgAddress << " arg1 >> " << arg1 << endl;
		dm.argument = NULL_ARG;

		if (arg1 == "FLT") dm.argument = FLT_ARG;
		else if (arg1 == "INT") dm.argument = INT_ARG;
		else if (arg1 == "BOL") dm.argument = BOL_ARG;
		else if (arg1 == "STR") dm.argument = STR_ARG;
		else if (arg1 == "ENU") dm.argument = ENUM_ARG;
		else if (arg1 == "COL") dm.argument = COLOR_ARG;
		else if (arg1 == "SPA") dm.argument = SPACER_ARG;
	}else{
		//must be a LOG_ action
	}

	if (msgAddress.length() >= 9) {
		string paramName = msgAddress.substr(9, msgAddress.length() - 9);
		dm.paramName = paramName;
	}else{
		//must be a LOG_ action
	}

	return dm;
}


string ofxRemoteUI::getMyIP(string userChosenInteface, string & subnetMask){

	//from https://github.com/jvcleave/LocalAddressGrabber/blob/master/src/LocalAddressGrabber.h
	//and http://stackoverflow.com/questions/17288908/get-network-interface-name-from-ipv4-address
	string output = RUI_LOCAL_IP_ADDRESS;

#if defined(__APPLE__) || defined(__linux__)
	struct ifaddrs *myaddrs;
	struct ifaddrs *ifa;
	struct sockaddr_in *s4;
	int status;
	char buf[64];

	status = getifaddrs(&myaddrs);
	if (status != 0){
		RLOG_ERROR << "getifaddrs failed! errno: " << errno;
	}else{
		for (ifa = myaddrs; ifa != NULL; ifa = ifa->ifa_next){
			if (ifa->ifa_addr == NULL) continue;
			if ((ifa->ifa_flags & IFF_UP) == 0) continue;

			if (ifa->ifa_addr->sa_family == AF_INET){
				s4 = (struct sockaddr_in *)(ifa->ifa_addr);
				if (inet_ntop(ifa->ifa_addr->sa_family, (void *)&(s4->sin_addr), buf, sizeof(buf)) == NULL){
					RLOG_ERROR <<ifa->ifa_name << ": inet_ntop failed!";
				}

				void * tmpAddrPtr = &((struct sockaddr_in *)ifa->ifa_netmask)->sin_addr;
				char SnAddressBuffer[INET_ADDRSTRLEN];
				if(inet_ntop(AF_INET, tmpAddrPtr, SnAddressBuffer, INET_ADDRSTRLEN) == NULL){
					RLOG_ERROR <<ifa->ifa_name << ": inet_ntop for subnet failed!";
				}

				if(inet_ntop(ifa->ifa_addr->sa_family, (void *)&(s4->sin_addr), buf, sizeof(buf)) == NULL){
					RLOG_ERROR <<ifa->ifa_name << ": inet_ntop for address failed!";
				}else{
					string interface = string(ifa->ifa_name);
					if(verbose_) RLOG_VERBOSE << "found interface: " << interface ;
					if( interface.length() > 2 || interface == userSuppliedNetInterface ){
						if (userSuppliedNetInterface.length() > 0){
							if (interface == userSuppliedNetInterface){
								output = string(buf);
								subnetMask = string(SnAddressBuffer);
								RLOG_VERBOSE << "using user chosen interface: " << interface;
								break;
							}
						}else{
							if ((interface[0] == 'e' && interface[1] == 'n') || (interface[0] == 'e' && interface[1] == 't')){
								if(strlen(buf) > 2){
									bool is169 = buf[0] == '1' && buf[1] == '6' && buf[2] == '9';
									if(!is169){ //avoid 169.x.x.x addresses
										output = string(buf);
										subnetMask = string(SnAddressBuffer);
										RLOG_NOTICE << "Using interface: " << interface << "       IP: " << output << "       SubnetMask: " << subnetMask;
										break;
									}
								}
							}
						}
					}
				}
			}
		}
		freeifaddrs(myaddrs);
	}

	if (userSuppliedNetInterface.length() > 0){
		if (output == RUI_LOCAL_IP_ADDRESS){
			RLOG_ERROR << "could not find the user supplied net interface: " << userSuppliedNetInterface;
			RLOG_ERROR << "automatic advertising will not work! ";
		}
	}

#endif

#ifdef TARGET_WIN32
	ULONG buflen = sizeof(IP_ADAPTER_INFO);
	IP_ADAPTER_INFO *pAdapterInfo = (IP_ADAPTER_INFO *)malloc(buflen);

	if (GetAdaptersInfo(pAdapterInfo, &buflen) == ERROR_BUFFER_OVERFLOW) {
		free(pAdapterInfo);
		pAdapterInfo = (IP_ADAPTER_INFO *)malloc(buflen);
	}

	if (GetAdaptersInfo(pAdapterInfo, &buflen) == NO_ERROR) {
		//this is crappy, we get the first non 0.0.0.0 interface (no idea which order they come in) TODO!
		for (IP_ADAPTER_INFO *pAdapter = pAdapterInfo; pAdapter; pAdapter = pAdapter->Next) {
			printf("%s (%s)\n", pAdapter->IpAddressList.IpAddress.String, pAdapter->Description);
			string ip = pAdapter->IpAddressList.IpAddress.String;
			if (ip != "0.0.0.0"){
				output = ip;
				break;
			}
		}
	}
	if (pAdapterInfo) free(pAdapterInfo);
#endif
	return output;
}

#ifdef TARGET_WIN32
void GetHostName(std::string& host_name){
    WSAData wsa_data;
    int ret_code;
    char buf[MAX_PATH];
    WSAStartup(MAKEWORD(1, 1), &wsa_data);
    ret_code = gethostname(buf, MAX_PATH);

    if (ret_code == SOCKET_ERROR)
    	host_name = RUI_LOCAL_IP_ADDRESS;
	else
		host_name = buf;
    WSACleanup();
}
#endif

void ofxRemoteUI::updateParamFromDecodedMessage(ofxOscMessage m, DecodedMessage dm){

	string paramName = dm.paramName;
	RemoteUIParam original;
	bool newParam = true;
	unordered_map<string, RemoteUIParam>::iterator it = params.find(paramName);
	if ( it != params.end() ){	//found the param, we already had it
		original = params[paramName];
		newParam = false;
	}

	RemoteUIParam p = original;
	int arg = 0;

	switch (dm.argument) {
		case FLT_ARG:
			p.type = REMOTEUI_PARAM_FLOAT;
 			p.floatVal = m.getArgAsFloat(arg); arg++;
			if(m.getNumArgs() > 1){
				p.minFloat = m.getArgAsFloat(arg); arg++;
				p.maxFloat = m.getArgAsFloat(arg); arg++;
			}
			if (p.floatValAddr){
				*p.floatValAddr = p.floatVal;
			}break;

		case INT_ARG:
			p.type = REMOTEUI_PARAM_INT;
			p.intVal = m.getArgAsInt32(arg); arg++;
			p.minInt = m.getArgAsInt32(arg); arg++;
			p.maxInt = m.getArgAsInt32(arg); arg++;
			if (p.intValAddr){
				*p.intValAddr = p.intVal;
			}break;

		case COLOR_ARG:
			p.type = REMOTEUI_PARAM_COLOR;
			p.redVal = (int)m.getArgAsInt32(arg); arg++;
			p.greenVal = (int)m.getArgAsInt32(arg); arg++;
			p.blueVal = (int)m.getArgAsInt32(arg); arg++;
			p.alphaVal = (int)m.getArgAsInt32(arg); arg++;
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
			if (p.intValAddr){
				*p.intValAddr = p.intVal;
			}
			int n = m.getNumArgs() - 5 - 3; // 3 >> the int vals, 5 >> 4 colors + 1 group
			int i = 0;
			p.enumList.clear();
			for (i = 0; i < n; i++) {
				p.enumList.push_back( m.getArgAsString(arg + i) );
			}
			arg = arg + i;
		}break;

		case BOL_ARG:
			p.type = REMOTEUI_PARAM_BOOL;
			p.boolVal = m.getArgAsInt32(arg) == 0 ? false : true; arg++;
			if (p.boolValAddr){
				*p.boolValAddr = p.boolVal;
			}break;

		case STR_ARG:
			p.type = REMOTEUI_PARAM_STRING;
			p.stringVal = m.getArgAsString(arg); arg++;
			if (p.stringValAddr){
				*p.stringValAddr = p.stringVal;
			}break;

		case SPACER_ARG:
			p.type = REMOTEUI_PARAM_SPACER;
			p.stringVal = m.getArgAsString(arg); arg++;
			break;

		case NULL_ARG: RLOG_ERROR << "updateParamFromDecodedMessage NULL type!"; break;
		default: RLOG_ERROR << "updateParamFromDecodedMessage unknown type!"; break;
	}

	if(m.getNumArgs() > 1){
		p.r = m.getArgAsInt32(arg); arg++;
		p.g = m.getArgAsInt32(arg); arg++;
		p.b = m.getArgAsInt32(arg); arg++;
		p.a = m.getArgAsInt32(arg); arg++;
		p.group = m.getArgAsString(arg); arg++;
	}

	if ( !p.isEqualTo(original) || newParam ){ // if the udpdate changed the param, keep track of it
		if(std::find(paramsChangedSinceLastCheck.begin(), paramsChangedSinceLastCheck.end(), paramName) == paramsChangedSinceLastCheck.end()){
			paramsChangedSinceLastCheck.push_back(paramName);
		}
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
	for( map<int,string>::iterator ii = orderedKeys.begin(); ii != orderedKeys.end(); ++ii ){
		string paramName = (*ii).second;
		paramsList.push_back(paramName);
	}
	return paramsList;
}


vector<string> ofxRemoteUI::scanForUpdatedParamsAndSync(){

	vector<string>paramsPendingUpdate;

	for( unordered_map<string, RemoteUIParam>::iterator ii = params.begin(); ii != params.end(); ++ii ){

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
		string name = list[i];
		unordered_map<string, RemoteUIParam>::const_iterator it = params.find(name);
		if(it!=params.end()){
			RemoteUIParam p = params[list[i]];
			//cout << "ofxRemoteUIServer: sending updated param " + list[i]; p.print();
			sendParam(list[i], p);
		}else{
			RLOG_ERROR << "param not found?!";
		}
	}
}

void ofxRemoteUI::syncAllParamsToPointers(){
	for( unordered_map<string, RemoteUIParam>::iterator ii = params.begin(); ii != params.end(); ++ii ){
		syncParamToPointer( (*ii).first );
	}
}

void ofxRemoteUI::syncAllPointersToParams(){
	for( unordered_map<string, RemoteUIParam>::iterator ii = params.begin(); ii != params.end(); ++ii ){
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

		case REMOTEUI_PARAM_SPACER:
		case REMOTEUI_PARAM_STRING:
			if (p.stringValAddr){
				*p.stringValAddr = p.stringVal;
			}break;
		default: break;
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

		case REMOTEUI_PARAM_SPACER:
		case REMOTEUI_PARAM_STRING:
			if (p.stringValAddr){
				p.stringVal = *p.stringValAddr;
			}break;
		default: break;
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
		case REMOTEUI_PARAM_SPACER: return false;
		default: break;
	}
	RLOG_ERROR << "hasParamChanged >> something went wrong, unknown param type";
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
		case REMOTEUI_PARAM_SPACER: return "SPA";
		default: break;
	}
	RLOG_ERROR << "stringForParamType >> UNKNOWN TYPE!";
	return "ERR";
}


bool ofxRemoteUI::paramExistsForName(string paramName){
	unordered_map<string, RemoteUIParam>::iterator it = params.find(paramName);
	return  it != params.end();
}

RemoteUIParam ofxRemoteUI::getParamForName(string paramName){

	RemoteUIParam p;
	unordered_map<string, RemoteUIParam>::iterator it = params.find(paramName);
	if ( it != params.end() ){	// found!
		p = params[paramName];
	}else{
		RLOG_ERROR << "getParamForName >> param " + paramName + " not found!";
	}
	return p;
}

RemoteUIParam& ofxRemoteUI::getParamRefForName(string paramName){
	unordered_map<string, RemoteUIParam>::iterator it = params.find(paramName);
	if ( it != params.end() ){	// found!
		return params[paramName];
	}else{
		RLOG_ERROR << "getParamForName >> param " + paramName + " not found!";
	}
	return nullParam;
}

string ofxRemoteUI::getValuesAsString(){
	stringstream out;
	map<int,string>::iterator it = orderedKeys.begin();
	while( it != orderedKeys.end() ){
		RemoteUIParam param = params[it->second];
		if(param.type != REMOTEUI_PARAM_SPACER){
			out << UriEncode(it->second) << "=";
		}
		switch (param.type) {
			case REMOTEUI_PARAM_FLOAT: out << param.floatVal << endl; break;
			case REMOTEUI_PARAM_INT: out << param.intVal << endl; break;
			case REMOTEUI_PARAM_COLOR: out << (int)param.redVal << " " << (int)param.greenVal << " " << (int)param.blueVal << " " << (int)param.alphaVal << " " << endl; break;
			case REMOTEUI_PARAM_ENUM: out << param.intVal << endl; break;
			case REMOTEUI_PARAM_BOOL: out << (param.boolVal?"1":"0") << endl; break;
			case REMOTEUI_PARAM_STRING: out << UriEncode(param.stringVal) << endl; break;
			case REMOTEUI_PARAM_SPACER: break;
			default: break;
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
				case REMOTEUI_PARAM_SPACER: break;
				default: break;
			}

			if ( !param.isEqualTo(original) ){ // if the udpdate changed the param, keep track of it
				params[name] = param;
				if(std::find(paramsChangedSinceLastCheck.begin(), paramsChangedSinceLastCheck.end(), name) == paramsChangedSinceLastCheck.end()){
					paramsChangedSinceLastCheck.push_back(name);
				}
			}
		}
	}

	vector<string>::iterator it = changedParam.begin();
	while( it != changedParam.end() ){
		if ( params.find( *it ) != params.end()){
			RemoteUIParam param = params[*it];
			sendUntrackedParamUpdate(param, *it);
			RLOG_VERBOSE << "sending update for " << *it ;
		}
		it++;
	}
}


void ofxRemoteUI::sendParam(string paramName, RemoteUIParam p){
	ofxOscMessage m;
	//if(verbose_){ ofLogVerbose("sending >> %s ", paramName.c_str()); p.print(); }
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
		case REMOTEUI_PARAM_SPACER: m.addStringArg(p.stringVal); break;
		default: break;
	}
	m.addIntArg(p.r); m.addIntArg(p.g); m.addIntArg(p.b); m.addIntArg(p.a); // set bg color!
	m.addStringArg(p.group);
	try{
		oscSender.sendMessage(m);
	}catch(exception e){
		RLOG_ERROR << "exception sendParam " << paramName;
	}
}

//if used by server, confirmation == YES
//else, NO
void ofxRemoteUI::sendREQU(bool confirmation){
	if(verbose_) RLOG_VERBOSE << "sendREQU()";
	ofxOscMessage m;
	m.setAddress("REQU");
	if (confirmation) m.addStringArg("OK");
	oscSender.sendMessage(m);
}


void ofxRemoteUI::sendRESX(bool confirm){
	if(verbose_) RLOG_VERBOSE << "sendRESX()";
	ofxOscMessage m;
	m.setAddress("RESX");
	if (confirm) m.addStringArg("OK");
	oscSender.sendMessage(m);
}

void ofxRemoteUI::sendRESD(bool confirm){
	if(verbose_) RLOG_VERBOSE << "sendRESD()";
	ofxOscMessage m;
	m.setAddress("RESD");
	if (confirm) m.addStringArg("OK");
	oscSender.sendMessage(m);
}

void ofxRemoteUI::sendSAVE(bool confirm){
	if(verbose_) RLOG_VERBOSE << "sendSAVE()";
	ofxOscMessage m;
	m.setAddress("SAVE");
	if (confirm) m.addStringArg("OK");
	oscSender.sendMessage(m);
}


void ofxRemoteUI::sendTEST(){
	if(verbose_) RLOG_VERBOSE << "sendTEST()";
	waitingForReply = true;
	timeSinceLastReply = 0.0f;
	ofxOscMessage m;
	m.setAddress("TEST");
	oscSender.sendMessage(m);
}

//on client call, presetNames should be empty vector (request ing the list)
//on server call, presetNames should have all the presetNames
void ofxRemoteUI::sendPREL( vector<string> presetNames_ ){
	if(verbose_) RLOG_VERBOSE << "sendPRES()";
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
void ofxRemoteUI::sendSAVP(string presetName, bool confirm){
	if(verbose_) RLOG_VERBOSE << "sendSAVP()";
	ofxOscMessage m;
	m.setAddress("SAVP");
	m.addStringArg(presetName);
	if (confirm){
		m.addStringArg("OK");
	}
	oscSender.sendMessage(m);
}

//on client call, presetName should be the new preset name
//on server call, presetName should be empty string (so it will send "OK"
void ofxRemoteUI::sendSETP(string presetName, bool confirm){
	if(verbose_) RLOG_VERBOSE << "sendSETP()";
	ofxOscMessage m;
	m.setAddress("SETP");
	m.addStringArg(presetName);
	if (confirm){
		m.addStringArg("OK");
	}
	oscSender.sendMessage(m);
}

void ofxRemoteUI::sendMISP(vector<string> missingParamsInPreset){
	if (missingParamsInPreset.size() == 0) return; //do nothing if no params are missing
	if(verbose_) RLOG_VERBOSE << "sendMISP()";
	ofxOscMessage m;
	m.setAddress("MISP");
	for(int i = 0; i < missingParamsInPreset.size(); i++){
		m.addStringArg(missingParamsInPreset[i]);
	}
	oscSender.sendMessage(m);
}

void ofxRemoteUI::sendDELP(string presetName, bool confirm){
	if(verbose_) RLOG_VERBOSE << "sendDELP()";
	ofxOscMessage m;
	m.setAddress("DELP");
	m.addStringArg(presetName);
	if (confirm){
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

//on client call, presetName should be the new preset name
//on server call, presetName should be empty string (so it will send "OK"
void ofxRemoteUI::sendSETp(string presetName, string group, bool confirm){
	if(verbose_) RLOG_VERBOSE << "sendSETp()";
	ofxOscMessage m;
	m.setAddress("SETp");
	m.addStringArg(presetName);
	m.addStringArg(group);
	if (confirm){
		m.addStringArg("OK");
	}
	oscSender.sendMessage(m);
}

//on client call, presetName should be the new preset name
//on server call, presetName should be empty string (so it will send "OK"
void ofxRemoteUI::sendSAVp(string presetName, string group, bool confirm){
	if(verbose_) RLOG_VERBOSE << "sendSAVp()";
	ofxOscMessage m;
	m.setAddress("SAVp");
	m.addStringArg(presetName);
	m.addStringArg(group);
	if (confirm){
		m.addStringArg("OK");
	}
	oscSender.sendMessage(m);
}

//on client call, presetName should be the new preset name
//on server call, presetName should be empty string (so it will send "OK"
void ofxRemoteUI::sendDELp(string presetName, string group, bool confirm){
	if(verbose_) RLOG_VERBOSE << "sendDELp()";
	ofxOscMessage m;
	m.setAddress("DELp");
	m.addStringArg(presetName);
	m.addStringArg(group);
	if (confirm){
		m.addStringArg("OK");
	}
	oscSender.sendMessage(m);
}


const char *get_filename_ext(const char *filename) {
    const char *dot = strrchr(filename, '.');
    if(!dot || dot == filename) return "";
    return dot + 1;
}
