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

#ifdef _WIN32
#include <windows.h>
#include <iphlpapi.h>
#include <WinSock2.h>
#pragma comment(lib, "iphlpapi.lib")
#endif

using namespace std;

void split(vector<std::string> &tokens, const std::string &text, char separator) {
	std::size_t start = 0, end = 0;
	while ((end = text.find(separator, start)) != std::string::npos) {
		tokens.emplace_back(text.substr(start, end - start));
		start = end + 1;
	}
	tokens.emplace_back(text.substr(start));
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


vector<std::string> ofxRemoteUI::getPresetsList(){
	return presetNames;
}


void ofxRemoteUI::printAllParamsDebug(){
	if(orderedKeys.size() == 0) return;
	dataMutex.lock();
	cout << "#### FULL PARAM LIST ################################" << endl;
	for(size_t i = 0; i < orderedKeys.size(); i++){
		string key = orderedKeys[i];
		RemoteUIParam thisP = params[key];
		cout << "   index: " << i << "  list: " << key << " > "; thisP.print();
	}
	dataMutex.unlock();
	cout << "####################################################" << endl;
}

bool ofxRemoteUI::addParamToDB(const RemoteUIParam & p, const std::string & thisParamName){

	//see if we already had it, if we didnt, set its add order #
	dataMutex.lock();
	bool ok;
	auto it = params.find(thisParamName);
	if ( it == params.end() ){	//not found!

		params[thisParamName] = p;
		orderedKeys[ (int)orderedKeys.size() ] = thisParamName;
		paramsFromCode[thisParamName] = p; //cos this didnt exist before, we store it as "from code"
		ok = true;
	}else{
		RLOG_ERROR << "already have a Param with that name on the DB : '" << thisParamName << "'. Ignoring it!";
		ok = false;
	}
	dataMutex.unlock();
	return ok;
}

void ofxRemoteUI::setParamDescription(const std::string & paramName, const std::string & description){
	dataMutex.lock();
	auto it = params.find(paramName);
	if ( it != params.end() ){	//not found!
		it->second.description = description;
	}
	dataMutex.unlock();
}


void ofxRemoteUI::clearOscReceiverMsgQueue(){
	ofxOscMessage tempM;
	//int c = 0;
	//delete all pending messages

	while (oscReceiver.getNextMessage(tempM)) {
		//cout << "clearOscReceiverMsgQueue " << c << endl;
		//c++;
	}
}


vector<std::string> ofxRemoteUI::getChangedParamsList(){
	std::vector<std::string> result (paramsChangedSinceLastCheck.begin(), paramsChangedSinceLastCheck.end());
	paramsChangedSinceLastCheck.clear();
	return result;
}


DecodedMessage ofxRemoteUI::decode(const ofxOscMessage & m){

	std::string msgAddress = m.getAddress();
	if(msgAddress.size() > 0){

		if(msgAddress[0] == '/'){ //if address starts with "/", drop it to match the fucked up remoteUI protocol
			msgAddress = msgAddress.substr(1, msgAddress.size() - 1);
		}
		//allow address to use the standard style /SEND/FLT/paramName instead of the legacy "SEND FLT paramName"
		//allow /bbb/jjj/ syntax, but convert to space-based to avoid changing all the internal logic
		for(int i = 0; i < msgAddress.size(); i++){
			if(msgAddress[i] == '/') msgAddress[i] = ' ';
		}
	}
	std::string action = msgAddress.substr(0, 4);

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
		//remove params
		else if (action == "REMp") {
			dm.action = REMOVE_PARAM;
			dm.paramName = m.getArgAsString(0);
		}
	}

	if (msgAddress.length() >= 8) {
		std::string arg1 = msgAddress.substr(5, 3);
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
		std::string paramName = msgAddress.substr(9, msgAddress.length() - 9);
		dm.paramName = paramName;
	}else{
		//must be a LOG_ action
	}

	return dm;
}


std::string ofxRemoteUI::getMyIP(std::string userChosenInteface, std::string & subnetMask){

	//from https://github.com/jvcleave/LocalAddressGrabber/blob/master/src/LocalAddressGrabber.h
	//and http://stackoverflow.com/questions/17288908/get-network-interface-name-from-ipv4-address
	std::string output = RUI_LOCAL_IP_ADDRESS;

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
					std::string interface = std::string(ifa->ifa_name);
					if(verbose_) RLOG_VERBOSE << "found interface: " << interface ;
					if( interface.length() > 2 || interface == userSuppliedNetInterface ){
						if (userSuppliedNetInterface.length() > 0){
							if (interface == userSuppliedNetInterface){
								output = std::string(buf);
								subnetMask = std::string(SnAddressBuffer);
								RLOG_VERBOSE << "using user chosen interface: " << interface;
								break;
							}
						}else{
							if ((interface[0] == 'e' && interface[1] == 'n') || (interface[0] == 'e' && interface[1] == 't')){
								if(strlen(buf) > 2){
									bool is169 = buf[0] == '1' && buf[1] == '6' && buf[2] == '9';
									if(!is169){ //avoid 169.x.x.x addresses
										output = std::string(buf);
										subnetMask = std::string(SnAddressBuffer);
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

#ifdef _WIN32
	ULONG buflen = sizeof(IP_ADAPTER_INFO);
	IP_ADAPTER_INFO *pAdapterInfo = (IP_ADAPTER_INFO *)malloc(buflen);

	if (GetAdaptersInfo(pAdapterInfo, &buflen) == ERROR_BUFFER_OVERFLOW) {
		free(pAdapterInfo);
		pAdapterInfo = (IP_ADAPTER_INFO *)malloc(buflen);
	}

	if (GetAdaptersInfo(pAdapterInfo, &buflen) == NO_ERROR) {
		//FIXME: this is crappy, we get the first non 0.0.0.0 interface (no idea which order they come in)
		for (IP_ADAPTER_INFO *pAdapter = pAdapterInfo; pAdapter; pAdapter = pAdapter->Next) {
			//printf("%s (%s)\n", pAdapter->IpAddressList.IpAddress.String, pAdapter->Description);
			std::string ip = pAdapter->IpAddressList.IpAddress.String;
			if (ip != "0.0.0.0"){
				output = ip;
				subnetMask = pAdapter->IpAddressList.IpMask.String;
				break;
			}
		}
	}
	if (pAdapterInfo) free(pAdapterInfo);
#endif
	return output;
}

#ifdef _WIN32
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

void ofxRemoteUI::updateParamFromDecodedMessage(const ofxOscMessage & m, DecodedMessage dm){

	std::string paramName = dm.paramName;
	RemoteUIParam original;
	bool newParam = true;
	auto it = params.find(paramName);
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
			if(m.getNumArgs() > 1){
				p.minInt = m.getArgAsInt32(arg); arg++;
				p.maxInt = m.getArgAsInt32(arg); arg++;
			}
			if (p.intValAddr){
				*p.intValAddr = p.intVal;
			}break;

		case COLOR_ARG:
			p.type = REMOTEUI_PARAM_COLOR;
			if(m.getNumArgs() == 1){ //hex rgba encoded in one single int - vezér style
				#ifdef OF_AVAILABLE
				std::uint32_t rgba = m.getArgAsRgbaColor(arg); arg++;
				#else
				std::uint32_t rgba = m.getArgAsInt32(arg); arg++;
				#endif
				p.redVal = (rgba & 0xFF000000) >> 24;
				p.greenVal = (rgba & 0x00FF0000) >> 16;
				p.blueVal = (rgba & 0x0000FF00) >> 8;
				p.alphaVal = (rgba & 0x000000FF);
			}else{ //legacy ofxRemoteUI
				p.redVal = (int)m.getArgAsInt32(arg); arg++;
				p.greenVal = (int)m.getArgAsInt32(arg); arg++;
				p.blueVal = (int)m.getArgAsInt32(arg); arg++;
				p.alphaVal = (int)m.getArgAsInt32(arg); arg++;
			}
			if (p.redValAddr){
				*p.redValAddr = p.redVal;
				*(p.redValAddr+1) = p.greenVal;
				*(p.redValAddr+2) = p.blueVal;
				*(p.redValAddr+3) = p.alphaVal;
			}break;

		case ENUM_ARG:{
			p.type = REMOTEUI_PARAM_ENUM;
			p.intVal = m.getArgAsInt32(arg); arg++;
			if (p.intValAddr){
				*p.intValAddr = p.intVal;
			}
			if(m.getNumArgs() > 1){ //for standard RUI client
				p.minInt = m.getArgAsInt32(arg); arg++;
				p.maxInt = m.getArgAsInt32(arg); arg++;
				int n = p.maxInt - p.minInt + 1; 
				int i = 0;
				p.enumList.clear();
				for (i = 0; i < n; i++) {
					p.enumList.emplace_back( m.getArgAsString(arg + i) );
				}
				arg = arg + i;
			}else{ //for basic enum suppport, where only the enum int value is sent (ie vezer)
				arg ++; //only one param was used
			}
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

	if(m.getNumArgs() > arg){ //if msg contains bg color, parse it
		p.r = m.getArgAsInt32(arg); arg++;
		p.g = m.getArgAsInt32(arg); arg++;
		p.b = m.getArgAsInt32(arg); arg++;
		p.a = m.getArgAsInt32(arg); arg++;
		p.group = m.getArgAsString(arg); arg++;
		if(m.getNumArgs() > arg){ //if it provides a description, read it
			p.description = m.getArgAsString(arg); arg++;
		}
	}

	if ( !p.isEqualTo(original) || newParam ){ // if the udpdate changed the param, keep track of it
		if(std::find(paramsChangedSinceLastCheck.begin(), paramsChangedSinceLastCheck.end(), paramName) == paramsChangedSinceLastCheck.end()){
			paramsChangedSinceLastCheck.emplace_back(paramName);
		}
	}

	//here we update our param db
	//params[paramName] = p;
	if(newParam)
		addParamToDB(p, paramName);
	else
		params[paramName] = p;
}



vector<std::string> ofxRemoteUI::getAllParamNamesList(){

	vector<std::string>paramsList;
	//get list of params in add order
	dataMutex.lock();
	for( auto ii = orderedKeys.begin(); ii != orderedKeys.end(); ++ii ){
		const std::string & paramName = (*ii).second;
		paramsList.emplace_back(paramName);
	}
	dataMutex.unlock();
	return paramsList;
}


vector<std::string> ofxRemoteUI::scanForUpdatedParamsAndSync(){

	vector<std::string>paramsPendingUpdate;

	dataMutex.lock();
	for( auto ii = params.begin(); ii != params.end(); ++ii ){

		RemoteUIParam p = (*ii).second;
		if ( hasParamChanged(p) ){
			paramsPendingUpdate.emplace_back( (*ii).first );
			syncParamToPointer((*ii).first);
		}
	}
	dataMutex.unlock();
	return paramsPendingUpdate;
}


void ofxRemoteUI::sendUpdateForParamsInList(vector<std::string>list){

	dataMutex.lock();
	for(size_t i = 0; i < list.size(); i++){
		std::string name = list[i];
		auto it = params.find(name);
		if(it!=params.end()){
			const RemoteUIParam & p = params[list[i]];
			//cout << "ofxRemoteUIServer: sending updated param " + list[i]; p.print();
			sendParam(list[i], p);
		}else{
			RLOG_ERROR << "param not found?!";
		}
	}
	dataMutex.unlock();
}

void ofxRemoteUI::syncAllParamsToPointers(){
	dataMutex.lock();
	for( auto ii = params.begin(); ii != params.end(); ++ii ){
		syncParamToPointer( (*ii).first );
	}
	dataMutex.unlock();
}

void ofxRemoteUI::syncAllPointersToParams(){
	dataMutex.lock();
	for( auto ii = params.begin(); ii != params.end(); ++ii ){
		syncPointerToParam( (*ii).first );
	}
	dataMutex.unlock();
}

void ofxRemoteUI::syncPointerToParam(const std::string & paramName){

	RemoteUIParam &p = params[paramName];

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
}


void ofxRemoteUI::syncParamToPointer(const std::string & paramName){

	RemoteUIParam & p = params[paramName];

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
}


bool ofxRemoteUI::hasParamChanged(const RemoteUIParam & p){
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


std::string ofxRemoteUI::stringForParamType(RemoteUIParamType t){
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


bool ofxRemoteUI::paramExistsForName(const std::string & paramName){
	dataMutex.lock();
	auto it = params.find(paramName);
	dataMutex.unlock();
	return  it != params.end();
}

RemoteUIParam ofxRemoteUI::getParamForName(const std::string & paramName){

	RemoteUIParam p;
	dataMutex.lock();
	auto it = params.find(paramName);
	if ( it != params.end() ){	// found!
		p = params[paramName];
	}else{
		RLOG_ERROR << "getParamForName >> param " + paramName + " not found!";
	}
	dataMutex.unlock();
	return p;
}

RemoteUIParam& ofxRemoteUI::getParamRefForName(const std::string & paramName){
	auto it = params.find(paramName);
	if ( it != params.end() ){	// found!
		return params[paramName];
	}else{
		RLOG_ERROR << "getParamForName >> param " + paramName + " not found!";
	}
	return nullParam;
}

std::string ofxRemoteUI::getValuesAsString(const vector<std::string> & paramList){
	std::stringstream out;
	auto it = orderedKeys.begin();
	while( it != orderedKeys.end() ){
		std::string pname = it->second;
		if(paramList.size() == 0 || find(paramList.begin(), paramList.end(), pname) != paramList.end()){
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
		}
		++it;
	}
	return out.str();
}


void ofxRemoteUI::setValuesFromString( const std::string & values ){

	stringstream in(values);
	std::string name, value;
	vector<std::string>changedParam;

	while( !in.eof() ){
		getline( in, name, '=' );
		getline( in, value, '\n' );

		dataMutex.lock();

		if( params.find( name ) != params.end() ){
			RemoteUIParam param = params[name];
			RemoteUIParam original = params[name];
			changedParam.emplace_back(name);
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
				syncPointerToParam(name);
				if(std::find(paramsChangedSinceLastCheck.begin(), paramsChangedSinceLastCheck.end(), name) == paramsChangedSinceLastCheck.end()){
					paramsChangedSinceLastCheck.emplace_back(name);
				}
			}
			dataMutex.unlock();
		}else{
			if(name.size()){
				RLOG_NOTICE << "unknown param name; ignoring (" << name << ")";
			}
		}
	}

	vector<std::string>::iterator it = changedParam.begin();
	while( it != changedParam.end() ){
		dataMutex.lock();
		if ( params.find( *it ) != params.end()){
			RemoteUIParam param = params[*it];
			sendUntrackedParamUpdate(param, *it);
			RLOG_VERBOSE << "sending update for " << *it ;
			ScreenNotifArg arg;
			arg.paramName = *it;
			arg.p = param;
			arg.bgColor = (param.type == REMOTEUI_PARAM_COLOR) ? param.getColor() : ofColor(0,0,0,0);
			#ifdef OF_AVAILABLE
			ofNotifyEvent(eventShowParamUpdateNotification, arg, this);
			#endif
		}
		dataMutex.unlock();
		it++;
	}
}


void ofxRemoteUI::sendParam(std::string paramName, const RemoteUIParam & p){
	ofxOscMessage m;
	//if(verbose_){ ofLogVerbose("sending >> %s ", paramName.c_str()); p.print(); }
	m.setAddress("/SEND " + stringForParamType(p.type) + " " + paramName);
	switch (p.type) {
		case REMOTEUI_PARAM_FLOAT: m.addFloatArg(p.floatVal); m.addFloatArg(p.minFloat); m.addFloatArg(p.maxFloat); break;
		case REMOTEUI_PARAM_INT: m.addIntArg(p.intVal); m.addIntArg(p.minInt); m.addIntArg(p.maxInt); break;
		case REMOTEUI_PARAM_COLOR: m.addIntArg(p.redVal); m.addIntArg(p.greenVal); m.addIntArg(p.blueVal); m.addIntArg(p.alphaVal); break;
		case REMOTEUI_PARAM_BOOL: m.addIntArg(p.boolVal ? 1 : 0); break;
		case REMOTEUI_PARAM_STRING: m.addStringArg(p.stringVal); break;
		case REMOTEUI_PARAM_ENUM:{
			m.addIntArg(p.intVal); m.addIntArg(p.minInt); m.addIntArg(p.maxInt);
			for (size_t i = 0; i < p.enumList.size(); i++) {
				m.addStringArg(p.enumList[i]);
			}
		}break;
		case REMOTEUI_PARAM_SPACER: m.addStringArg(p.stringVal); break;
		default: break;
	}
	m.addIntArg(p.r); m.addIntArg(p.g); m.addIntArg(p.b); m.addIntArg(p.a); // set bg color!
	m.addStringArg(p.group);
	m.addStringArg(p.description);
	try{
		sendMessage(m);
		if(verbose_) RLOG_VERBOSE << "sendParam(" << paramName << ")";
		paramsSentOverOsc.insert(paramName);
	}catch(exception e){
		RLOG_ERROR << "exception sendParam " << paramName;
	}
}

//if used by server, confirmation == YES
//else, NO
void ofxRemoteUI::sendREQU(bool confirmation){
	if(verbose_) RLOG_VERBOSE << "sendREQU()";
	ofxOscMessage m;
	m.setAddress("/REQU");
	if (confirmation) m.addStringArg("OK");
	sendMessage(m);
}


void ofxRemoteUI::sendRESX(bool confirm){
	if(verbose_) RLOG_VERBOSE << "sendRESX()";
	ofxOscMessage m;
	m.setAddress("/RESX");
	if (confirm) m.addStringArg("OK");
	sendMessage(m);
}

void ofxRemoteUI::sendRESD(bool confirm){
	if(verbose_) RLOG_VERBOSE << "sendRESD()";
	ofxOscMessage m;
	m.setAddress("/RESD");
	if (confirm) m.addStringArg("OK");
	sendMessage(m);
}

void ofxRemoteUI::sendSAVE(bool confirm){
	if(verbose_) RLOG_VERBOSE << "sendSAVE()";
	ofxOscMessage m;
	m.setAddress("/SAVE");
	if (confirm) m.addStringArg("OK");
	sendMessage(m);
}


void ofxRemoteUI::sendTEST(){
	//if(verbose_) RLOG_VERBOSE << "sendTEST()";
	waitingForReply = true;
	timeSinceLastReply = 0.0f;
	ofxOscMessage m;
	m.setAddress("/TEST");
	sendMessage(m);
}

//on client call, presetNames should be empty vector (request ing the list)
//on server call, presetNames should have all the presetNames
void ofxRemoteUI::sendPREL( vector<std::string> presetNames_ ){
	if(verbose_) RLOG_VERBOSE << "sendPREL()";
	ofxOscMessage m;
	m.setAddress("/PREL");
	if (presetNames_.size() == 0){ // if we are the client requesting a preset list, delete our current list
		presetNames.clear();
		m.addStringArg("OK");
	}
	for(size_t i = 0; i < presetNames_.size(); i++){
		m.addStringArg(presetNames_[i]);
	}
	sendMessage(m);
}

//on client call, presetName should be the new preset name
//on server call, presetName should be empty string (so it will send "OK"
void ofxRemoteUI::sendSAVP(std::string presetName, bool confirm){
	if(verbose_) RLOG_VERBOSE << "sendSAVP()";
	ofxOscMessage m;
	m.setAddress("/SAVP");
	m.addStringArg(presetName);
	if (confirm){
		m.addStringArg("OK");
	}
	sendMessage(m);
}

//on client call, presetName should be the new preset name
//on server call, presetName should be empty string (so it will send "OK"
void ofxRemoteUI::sendSETP(std::string presetName, bool confirm){
	if(verbose_) RLOG_VERBOSE << "sendSETP()";
	ofxOscMessage m;
	m.setAddress("/SETP");
	m.addStringArg(presetName);
	if (confirm){
		m.addStringArg("OK");
	}
	sendMessage(m);
}

void ofxRemoteUI::sendMISP(vector<std::string> missingParamsInPreset){
	if (missingParamsInPreset.size() == 0) return; //do nothing if no params are missing
	if(verbose_) RLOG_VERBOSE << "sendMISP()";
	ofxOscMessage m;
	m.setAddress("/MISP");
	for(size_t i = 0; i < missingParamsInPreset.size(); i++){
		m.addStringArg(missingParamsInPreset[i]);
	}
	sendMessage(m);
}

void ofxRemoteUI::sendREMp(const string & paramName){
	if(verbose_) RLOG_VERBOSE << "sendREMp() " << paramName ;
	if(readyToSend){
		ofxOscMessage m;
		m.setAddress("/REMp");
		m.addStringArg(paramName);
		try{
			sendMessage(m);
		}catch(exception e){
			RLOG_ERROR << "Exception sendREMp " << e.what() ;
		}
	}
}

void ofxRemoteUI::sendDELP(std::string presetName, bool confirm){
	if(verbose_) RLOG_VERBOSE << "sendDELP()";
	ofxOscMessage m;
	m.setAddress("/DELP");
	m.addStringArg(presetName);
	if (confirm){
		m.addStringArg("OK");
	}
	sendMessage(m);
}

void ofxRemoteUI::sendHELLO(){
	ofxOscMessage m;
	m.setAddress("/HELO");
	sendMessage(m);
}

void ofxRemoteUI::sendCIAO(){
	ofxOscMessage m;
	m.setAddress("/CIAO");
	sendMessage(m);
}

//on client call, presetName should be the new preset name
//on server call, presetName should be empty string (so it will send "OK"
void ofxRemoteUI::sendSETp(std::string presetName, std::string group, bool confirm){
	if(verbose_) RLOG_VERBOSE << "sendSETp()";
	ofxOscMessage m;
	m.setAddress("/SETp");
	m.addStringArg(presetName);
	m.addStringArg(group);
	if (confirm){
		m.addStringArg("OK");
	}
	sendMessage(m);
}

//on client call, presetName should be the new preset name
//on server call, presetName should be empty string (so it will send "OK"
void ofxRemoteUI::sendSAVp(std::string presetName, std::string group, bool confirm){
	if(verbose_) RLOG_VERBOSE << "sendSAVp()";
	ofxOscMessage m;
	m.setAddress("/SAVp");
	m.addStringArg(presetName);
	m.addStringArg(group);
	if (confirm){
		m.addStringArg("OK");
	}
	sendMessage(m);
}

//on client call, presetName should be the new preset name
//on server call, presetName should be empty string (so it will send "OK"
void ofxRemoteUI::sendDELp(std::string presetName, std::string group, bool confirm){
	if(verbose_) RLOG_VERBOSE << "sendDELp()";
	ofxOscMessage m;
	m.setAddress("/DELp");
	m.addStringArg(presetName);
	m.addStringArg(group);
	if (confirm){
		m.addStringArg("OK");
	}
	sendMessage(m);
}


const char *get_filename_ext(const char *filename) {
    const char *dot = strrchr(filename, '.');
    if(!dot || dot == filename) return "";
    return dot + 1;
}
