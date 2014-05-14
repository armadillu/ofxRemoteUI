//
//  ofxRemoteUI.cpp
//  emptyExample
//
//  Created by Oriol Ferrer Mesià on 09/01/13.
//
//

#include "ofxRemoteUIServer.h"
#ifdef TARGET_WIN32
#include <winsock2.h>
#endif

#include <iostream>
#include <algorithm>
#include <string>
#include <string.h>
#ifdef __APPLE__
#include "dirent.h"
#include <mach-o/dyld.h>	/* _NSGetExecutablePath */
#elif _WIN32 || _WIN64
#include "dirent_vs.h"
#endif
#include <sys/stat.h>
#include <time.h>

#ifdef OF_AVAILABLE
#include <Poco/Path.h>
#include <Poco/Environment.h>
#include <Poco/Process.h>
#endif


ofxRemoteUIServer* ofxRemoteUIServer::singleton = NULL;
ofxRemoteUIServer* ofxRemoteUIServer::instance(){
	if (!singleton){   // Only allow one instance of class to be generated.
		singleton = new ofxRemoteUIServer();
	}
	return singleton;
}

void ofxRemoteUIServer::setEnabled(bool enabled_){
	enabled = enabled_;
}

void ofxRemoteUIServer::setAutomaticBackupsEnabled(bool enabled){
	autoBackups = enabled;
}

void ofxRemoteUIServer::setDrawsNotificationsAutomaticallly(bool draw){
	drawNotifications = draw;
}

void ofxRemoteUIServer::setShowInterfaceKey(char k){
	showInterfaceKey = k;
}

ofxRemoteUIServer::ofxRemoteUIServer(){

	enabled = true;
	readyToSend = false;
	saveToXmlOnExit = true;
	autoBackups = true;
	broadcastTime = OFXREMOTEUI_BORADCAST_INTERVAL + 0.05;
	timeSinceLastReply = avgTimeSinceLastReply = 0;
	waitingForReply = false;
	colorSet = false;
	computerName = binaryName = "";
	callBack = NULL;
	upcomingGroup = OFXREMOTEUI_DEFAULT_PARAM_GROUP;
	verbose_ = false;
	threadedUpdate = false;
	drawNotifications = true;
	showValuesOnScreen = false;
	loadedFromXML = false;
	clearXmlOnSaving = false;
	//add random colors to table
	colorTableIndex = 0;
	broadcastCount = 0;
	newColorInGroupCounter = 1;
	showInterfaceKey = '\t';
	int a = 80;
#ifdef OF_AVAILABLE
	selectedItem = 0;
	ofSeedRandom(1979);
	ofColor prevColor = ofColor::fromHsb(0, 255, 200, BG_COLOR_ALPHA);
	for(int i = 0; i < 30; i++){
		ofColor c = ofColor::fromHsb(prevColor.getHue() + 10, 255, 210, BG_COLOR_ALPHA);
		colorTables.push_back( c );
		prevColor = c;
	}
	//shuffle
	std::srand(1979);
	std::random_shuffle ( colorTables.begin(), colorTables.end() );
	ofSeedRandom();
	uiLines.setMode(OF_PRIMITIVE_LINES);
#else
	colorTables.push_back(ofColor(194,144,221,a) );
	colorTables.push_back(ofColor(202,246,70,a)  );
	colorTables.push_back(ofColor(74,236,173,a)  );
	colorTables.push_back(ofColor(253,144,150,a) );
	colorTables.push_back(ofColor(41,176,238,a)  );
	colorTables.push_back(ofColor(180,155,45,a)  );
	colorTables.push_back(ofColor(63,216,92,a)   );
	colorTables.push_back(ofColor(226,246,139,a) );
	colorTables.push_back(ofColor(239,209,46,a)  );
	colorTables.push_back(ofColor(234,127,169,a) );
	colorTables.push_back(ofColor(227,184,233,a) );
	colorTables.push_back(ofColor(165,154,206,a) );
#endif

#ifdef OF_AVAILABLE
	ofDirectory d;
	d.open(OFXREMOTEUI_PRESET_DIR);
	if(!d.exists()){
		d.create(true);
	}
#else
#if defined(_WIN32)
	_mkdir(OFXREMOTEUI_PRESET_DIR);
#else
	mkdir(OFXREMOTEUI_PRESET_DIR, 0777);
#endif
#endif

}

ofxRemoteUIServer::~ofxRemoteUIServer(){
	RUI_LOG_NOTICE << "~ofxRemoteUIServer()" ;
}


void ofxRemoteUIServer::setSaveToXMLOnExit(bool save){
	saveToXmlOnExit = save;
}


void ofxRemoteUIServer::setCallback( void (*callb)(RemoteUIServerCallBackArg) ){
	callBack = callb;
}


void ofxRemoteUIServer::close(){
	if(readyToSend)
		sendCIAO();
	if(threadedUpdate){
#ifdef OF_AVAILABLE
		stopThread();
		RUI_LOG_NOTICE << "ofxRemoteUIServer closing; waiting for update thread to end..." ;
		waitForThread();
#endif
	}
}


void ofxRemoteUIServer::setParamGroup(string g){
	upcomingGroup = g;
	newColorInGroupCounter = 1;
	setNewParamColor(1);
	setNewParamColorVariation(true);
	addSpacer(g);
}


void ofxRemoteUIServer::unsetParamColor(){
	colorSet = false;
}


void ofxRemoteUIServer::setNewParamColorVariation(bool dontChangeVariation){
	paramColorCurrentVariation = paramColor;
	if(!dontChangeVariation){
		newColorInGroupCounter++;
	}
	int offset = newColorInGroupCounter%2;
	paramColorCurrentVariation.a = BG_COLOR_ALPHA + offset * BG_COLOR_ALPHA * 0.75;
}


void ofxRemoteUIServer::setNewParamColor(int num){
	for(int i = 0; i < num; i++){
		ofColor c = colorTables[colorTableIndex];
		colorSet = true;
		paramColor = c;
		colorTableIndex++;
		if(colorTableIndex>= colorTables.size()){
			colorTableIndex = 0;
		}
	}
}


void ofxRemoteUIServer::removeParamFromDB(string paramName){

	map<string, RemoteUIParam>::iterator it = params.find(paramName);

	if (it != params.end()){

		params.erase(params.find(paramName));

		it = paramsFromCode.find(paramName);
		if (it != paramsFromCode.end()){
			paramsFromCode.erase(paramsFromCode.find(paramName));
		}

		it = paramsFromXML.find(paramName);
		if (it != paramsFromXML.end()){
			paramsFromXML.erase(paramsFromXML.find(paramName));
		}


		//re-create orderedKeys
		vector<string> myOrderedKeys;
		std::map<int, string>::iterator iterator;

		for(iterator = orderedKeys.begin(); iterator != orderedKeys.end(); iterator++) {

			if (iterator->second != paramName){
				//positionsToDelete.push_back(iterator->first);
				myOrderedKeys.push_back(iterator->second);
			}
		}

		orderedKeys.clear();
		for(int i = 0; i < myOrderedKeys.size(); i++){
			orderedKeys[i] = myOrderedKeys[i];
		}
	}else{
		RUI_LOG_ERROR << "ofxRemoteUIServer::removeParamFromDB >> trying to delete an unexistant param (" << paramName << ")" ;
	}
}


void ofxRemoteUIServer::saveParamToXmlSettings(RemoteUIParam t, string key, ofxXmlSettings & s, XmlCounter & c){

	switch (t.type) {
		case REMOTEUI_PARAM_FLOAT:
			if(verbose_) RUI_LOG_VERBOSE << "ofxRemoteUIServer saving '" << key << "' (" <<  *t.floatValAddr <<") to XML" ;
			s.setValue(OFXREMOTEUI_FLOAT_PARAM_XML_TAG, (double)*t.floatValAddr, c.numFloats);
			s.setAttribute(OFXREMOTEUI_FLOAT_PARAM_XML_TAG, OFXREMOTEUI_PARAM_NAME_XML_KEY, key, c.numFloats);
			c.numFloats++;
			break;
		case REMOTEUI_PARAM_INT:
			if(verbose_) RUI_LOG_VERBOSE << "ofxRemoteUIServer saving '" << key << "' (" <<  *t.intValAddr <<") to XML" ;
			s.setValue(OFXREMOTEUI_INT_PARAM_XML_TAG, (int)*t.intValAddr, c.numInts);
			s.setAttribute(OFXREMOTEUI_INT_PARAM_XML_TAG, OFXREMOTEUI_PARAM_NAME_XML_KEY, key, c.numInts);
			c.numInts++;
			break;
		case REMOTEUI_PARAM_COLOR:
			if(verbose_) RUI_LOG_VERBOSE << "ofxRemoteUIServer saving '" << key << "' (" << (int)*t.redValAddr << " " << (int)*(t.redValAddr+1) << " " << (int)*(t.redValAddr+2) << " " << (int)*(t.redValAddr+3) << ") to XML" ;
			s.setValue(string(OFXREMOTEUI_COLOR_PARAM_XML_TAG) + ":R", (int)*t.redValAddr, c.numColors);
			s.setValue(string(OFXREMOTEUI_COLOR_PARAM_XML_TAG) + ":G", (int)*(t.redValAddr+1), c.numColors);
			s.setValue(string(OFXREMOTEUI_COLOR_PARAM_XML_TAG) + ":B", (int)*(t.redValAddr+2), c.numColors);
			s.setValue(string(OFXREMOTEUI_COLOR_PARAM_XML_TAG) + ":A", (int)*(t.redValAddr+3), c.numColors);
			s.setAttribute(OFXREMOTEUI_COLOR_PARAM_XML_TAG, OFXREMOTEUI_PARAM_NAME_XML_KEY, key, c.numColors);
			c.numColors++;
			break;
		case REMOTEUI_PARAM_ENUM:
			if(verbose_) RUI_LOG_VERBOSE << "ofxRemoteUIServer saving '" << key << "' (" <<  *t.intValAddr <<") to XML" ;
			s.setValue(OFXREMOTEUI_ENUM_PARAM_XML_TAG, (int)*t.intValAddr, c.numEnums);
			s.setAttribute(OFXREMOTEUI_ENUM_PARAM_XML_TAG, OFXREMOTEUI_PARAM_NAME_XML_KEY, key, c.numEnums);
			c.numEnums++;
			break;
		case REMOTEUI_PARAM_BOOL:
			if(verbose_) RUI_LOG_VERBOSE << "ofxRemoteUIServer saving '" << key << "' (" <<  *t.boolValAddr <<") to XML" ;
			s.setValue(OFXREMOTEUI_BOOL_PARAM_XML_TAG, (bool)*t.boolValAddr, c.numBools);
			s.setAttribute(OFXREMOTEUI_BOOL_PARAM_XML_TAG, OFXREMOTEUI_PARAM_NAME_XML_KEY, key, c.numBools);
			c.numBools++;
			break;
		case REMOTEUI_PARAM_STRING:
			if(verbose_) RUI_LOG_VERBOSE << "ofxRemoteUIServer saving '" << key << "' (" <<  *t.stringValAddr <<") to XML" ;
			s.setValue(OFXREMOTEUI_STRING_PARAM_XML_TAG, (string)*t.stringValAddr, c.numStrings);
			s.setAttribute(OFXREMOTEUI_STRING_PARAM_XML_TAG, OFXREMOTEUI_PARAM_NAME_XML_KEY, key, c.numStrings);
			c.numStrings++;
			break;

		case REMOTEUI_PARAM_SPACER:
			if(verbose_) RUI_LOG_VERBOSE << "ofxRemoteUIServer skipping save of spacer '" << key << "' to XML" ;
			break;

		default:
			break;
	}
}

void ofxRemoteUIServer::saveGroupToXML(string fileName, string groupName){

	RUI_LOG_NOTICE << "ofxRemoteUIServer: saving group to xml '" << fileName << "'" ;
	ofxXmlSettings s;
	s.loadFile(fileName);
	s.clear();
	s.addTag(OFXREMOTEUI_XML_TAG);
	s.pushTag(OFXREMOTEUI_XML_TAG);
	XmlCounter counters;

	#ifdef OF_AVAILABLE
	ofDirectory d;
	string path = string(OFXREMOTEUI_PRESET_DIR) + "/" + groupName;
	d.open(path);
	if(!d.exists()){
		d.create(true);
	}
	#else
		#if defined(_WIN32)
		_mkdir(path.c_str());
		#else
		mkdir(fileName.c_str(), 0777);
		#endif
	#endif


	for( map<string,RemoteUIParam>::iterator ii = params.begin(); ii != params.end(); ++ii ){
		string key = (*ii).first;
		RemoteUIParam t = params[key];
		if( t.group != OFXREMOTEUI_DEFAULT_PARAM_GROUP && t.group == groupName ){
			saveParamToXmlSettings(t, key, s, counters);
		}
	}
	s.saveFile(fileName);
}


void ofxRemoteUIServer::saveToXML(string fileName){

	saveSettingsBackup();

	RUI_LOG_NOTICE << "ofxRemoteUIServer: saving to xml '" << fileName << "'" ;
	ofxXmlSettings s;
	s.loadFile(fileName);
	if(clearXmlOnSaving){
		s.clear();
	}
	if (s.getNumTags(OFXREMOTEUI_XML_TAG) == 0){
		s.addTag(OFXREMOTEUI_XML_TAG);
	}

	s.pushTag(OFXREMOTEUI_XML_TAG);

	XmlCounter counters;
	for( map<string,RemoteUIParam>::iterator ii = params.begin(); ii != params.end(); ++ii ){
		string key = (*ii).first;
		RemoteUIParam t = params[key];
		saveParamToXmlSettings(t, key, s, counters);
	}

	s.popTag(); //pop OFXREMOTEUI_XML_TAG

	if(!portIsSet){
		s.setValue(OFXREMOTEUI_XML_PORT, port, 0);
	}
	s.saveFile(fileName);
}

vector<string> ofxRemoteUIServer::loadFromXML(string fileName){

	vector<string> loadedParams;
	ofxXmlSettings s;
	bool exists = s.loadFile(fileName);

	if (exists){

		if( s.getNumTags(OFXREMOTEUI_XML_TAG) > 0 ){
			s.pushTag(OFXREMOTEUI_XML_TAG, 0);

			int numFloats = s.getNumTags(OFXREMOTEUI_FLOAT_PARAM_XML_TAG);
			for (int i=0; i< numFloats; i++){
				string paramName = s.getAttribute(OFXREMOTEUI_FLOAT_PARAM_XML_TAG, OFXREMOTEUI_PARAM_NAME_XML_KEY, OFXREMOTEUI_UNKNOWN_PARAM_NAME_XML_KEY, i);
				float val = s.getValue(OFXREMOTEUI_FLOAT_PARAM_XML_TAG, 0.0, i);
				map<string,RemoteUIParam>::iterator it = params.find(paramName);
				if ( it != params.end() ){	// found!
					loadedParams.push_back(paramName);
					if(params[paramName].floatValAddr != NULL){
						*params[paramName].floatValAddr = val;
						params[paramName].floatVal = val;
						*params[paramName].floatValAddr = ofClamp(*params[paramName].floatValAddr, params[paramName].minFloat, params[paramName].maxFloat);
						if(!loadedFromXML) paramsFromXML[paramName] = params[paramName];
						if(verbose_) RUI_LOG_VERBOSE << "ofxRemoteUIServer loading a FLOAT '" << paramName <<"' (" << ofToString( *params[paramName].floatValAddr, 3) << ") from XML" ;
					}else{
						RUI_LOG_ERROR << "ofxRemoteUIServer ERROR at loading FLOAT (" << paramName << ")" ;
					}
				}else{
					RUI_LOG_ERROR << "ofxRemoteUIServer: float param '" <<paramName << "' defined in xml not found in DB!" ;
				}
			}

			int numInts = s.getNumTags(OFXREMOTEUI_INT_PARAM_XML_TAG);
			for (int i=0; i< numInts; i++){
				string paramName = s.getAttribute(OFXREMOTEUI_INT_PARAM_XML_TAG, OFXREMOTEUI_PARAM_NAME_XML_KEY, OFXREMOTEUI_UNKNOWN_PARAM_NAME_XML_KEY, i);
				float val = s.getValue(OFXREMOTEUI_INT_PARAM_XML_TAG, 0, i);
				map<string,RemoteUIParam>::iterator it = params.find(paramName);
				if ( it != params.end() ){	// found!
					loadedParams.push_back(paramName);
					if(params[paramName].intValAddr != NULL){
						*params[paramName].intValAddr = val;
						params[paramName].intVal = val;
						*params[paramName].intValAddr = ofClamp(*params[paramName].intValAddr, params[paramName].minInt, params[paramName].maxInt);
						if(!loadedFromXML) paramsFromXML[paramName] = params[paramName];
						if(verbose_) RUI_LOG_VERBOSE << "ofxRemoteUIServer loading an INT '" << paramName <<"' (" << (int) *params[paramName].intValAddr << ") from XML" ;
					}else{
						RUI_LOG_ERROR << "ofxRemoteUIServer ERROR at loading INT (" << paramName << ")" ;
					}
				}else{
					RUI_LOG_ERROR << "ofxRemoteUIServer: int param '" <<paramName << "' defined in xml not found in DB!" ;
				}
			}

			int numColors = s.getNumTags(OFXREMOTEUI_COLOR_PARAM_XML_TAG);
			for (int i=0; i< numColors; i++){
				string paramName = s.getAttribute(OFXREMOTEUI_COLOR_PARAM_XML_TAG, OFXREMOTEUI_PARAM_NAME_XML_KEY, "OFXREMOTEUI_UNKNOWN_PARAM_NAME_XML_KEY", i);
				s.pushTag(OFXREMOTEUI_COLOR_PARAM_XML_TAG, i);
				int r = s.getValue("R", 0);
				int g = s.getValue("G", 0);
				int b = s.getValue("B", 0);
				int a = s.getValue("A", 0);
				map<string,RemoteUIParam>::iterator it = params.find(paramName);
				if ( it != params.end() ){	// found!
					loadedParams.push_back(paramName);
					if(params[paramName].redValAddr != NULL){
						*params[paramName].redValAddr = r;
						params[paramName].redVal = r;
						*(params[paramName].redValAddr+1) = g;
						params[paramName].greenVal = g;
						*(params[paramName].redValAddr+2) = b;
						params[paramName].blueVal = b;
						*(params[paramName].redValAddr+3) = a;
						params[paramName].alphaVal = a;
						if(!loadedFromXML) paramsFromXML[paramName] = params[paramName];
						if(verbose_) RUI_LOG_VERBOSE << "ofxRemoteUIServer loading a COLOR '" << paramName <<"' (" << (int)*params[paramName].redValAddr << " " << (int)*(params[paramName].redValAddr+1) << " " << (int)*(params[paramName].redValAddr+2) << " " << (int)*(params[paramName].redValAddr+3)  << ") from XML" ;
					}else{
						cout << "ofxRemoteUIServer ERROR at loading COLOR (" << paramName << ")" ;
					}
				}else{
					cout << "ofxRemoteUIServer: color param '" <<paramName << "' defined in xml not found in DB!" ;
				}
				s.popTag();
			}

			int numEnums = s.getNumTags(OFXREMOTEUI_ENUM_PARAM_XML_TAG);
			for (int i=0; i< numEnums; i++){
				string paramName = s.getAttribute(OFXREMOTEUI_ENUM_PARAM_XML_TAG, OFXREMOTEUI_PARAM_NAME_XML_KEY, OFXREMOTEUI_UNKNOWN_PARAM_NAME_XML_KEY, i);
				float val = s.getValue(OFXREMOTEUI_ENUM_PARAM_XML_TAG, 0, i);
				map<string,RemoteUIParam>::iterator it = params.find(paramName);
				if ( it != params.end() ){	// found!
					loadedParams.push_back(paramName);
					if(params[paramName].intValAddr != NULL){
						*params[paramName].intValAddr = val;
						params[paramName].intVal = val;
						*params[paramName].intValAddr = ofClamp(*params[paramName].intValAddr, params[paramName].minInt, params[paramName].maxInt);
						if(!loadedFromXML) paramsFromXML[paramName] = params[paramName];
						if(verbose_) RUI_LOG_VERBOSE << "ofxRemoteUIServer loading an ENUM '" << paramName <<"' (" << (int) *params[paramName].intValAddr << ") from XML" ;
					}else{
						cout << "ofxRemoteUIServer ERROR at loading ENUM (" << paramName << ")" ;
					}
				}else{
					cout << "ofxRemoteUIServer: enum param '" << paramName << "' defined in xml not found in DB!" ;
				}
			}


			int numBools = s.getNumTags(OFXREMOTEUI_BOOL_PARAM_XML_TAG);
			for (int i=0; i< numBools; i++){
				string paramName = s.getAttribute(OFXREMOTEUI_BOOL_PARAM_XML_TAG, OFXREMOTEUI_PARAM_NAME_XML_KEY, OFXREMOTEUI_UNKNOWN_PARAM_NAME_XML_KEY, i);
				float val = s.getValue(OFXREMOTEUI_BOOL_PARAM_XML_TAG, false, i);

				map<string,RemoteUIParam>::iterator it = params.find(paramName);
				if ( it != params.end() ){	// found!
					loadedParams.push_back(paramName);
					if(params[paramName].boolValAddr != NULL){
						*params[paramName].boolValAddr = val;
						params[paramName].boolVal = val;
						if(!loadedFromXML) paramsFromXML[paramName] = params[paramName];
						if(verbose_) RUI_LOG_VERBOSE << "ofxRemoteUIServer loading a BOOL '" << paramName <<"' (" << (bool) *params[paramName].boolValAddr << ") from XML" ;
					}else{
						cout << "ofxRemoteUIServer ERROR at loading BOOL (" << paramName << ")" ;
					}
				}else{
					cout << "ofxRemoteUIServer: bool param '" << paramName << "' defined in xml not found in DB!" ;
				}
			}

			int numStrings = s.getNumTags(OFXREMOTEUI_STRING_PARAM_XML_TAG);
			for (int i=0; i< numStrings; i++){
				string paramName = s.getAttribute(OFXREMOTEUI_STRING_PARAM_XML_TAG, OFXREMOTEUI_PARAM_NAME_XML_KEY, OFXREMOTEUI_UNKNOWN_PARAM_NAME_XML_KEY, i);
				string val = s.getValue(OFXREMOTEUI_STRING_PARAM_XML_TAG, "", i);

				map<string,RemoteUIParam>::iterator it = params.find(paramName);
				if ( it != params.end() ){	// found!
					loadedParams.push_back(paramName);
					if(params[paramName].stringValAddr != NULL){
						params[paramName].stringVal = val;
						*params[paramName].stringValAddr = val;
						if(!loadedFromXML) paramsFromXML[paramName] = params[paramName];
						if(verbose_) RUI_LOG_VERBOSE << "ofxRemoteUIServer loading a STRING '" << paramName <<"' (" << (string) *params[paramName].stringValAddr << ") from XML" ;
					}
					else cout << "ofxRemoteUIServer ERROR at loading STRING (" << paramName << ")" ;
				}else{
					cout << "ofxRemoteUIServer: string param '" << paramName << "' defined in xml not found in DB!" ;
				}
			}
		}
	}

	vector<string> paramsNotInXML;
	for( map<string,RemoteUIParam>::iterator ii = params.begin(); ii != params.end(); ++ii ){

		string paramName = (*ii).first;

		//param name found in xml
		if( find(loadedParams.begin(), loadedParams.end(), paramName) != loadedParams.end() ){

		}else{ //param name not in xml
			if ((*ii).second.type != REMOTEUI_PARAM_SPACER){ //spacers dont count as params really
				paramsNotInXML.push_back(paramName);
			}
		}
	}
	loadedFromXML = true;
	return paramsNotInXML;
}

void ofxRemoteUIServer::restoreAllParamsToInitialXML(){
	for( map<string,RemoteUIParam>::iterator ii = params.begin(); ii != params.end(); ++ii ){
		string key = (*ii).first;
		if (params[key].type != REMOTEUI_PARAM_SPACER){
			params[key] = paramsFromXML[key];
			syncPointerToParam(key);
		}
	}
}

void ofxRemoteUIServer::restoreAllParamsToDefaultValues(){
	for( map<string,RemoteUIParam>::iterator ii = params.begin(); ii != params.end(); ++ii ){
		string key = (*ii).first;
		params[key] = paramsFromCode[key];
		syncPointerToParam(key);
	}
}

void ofxRemoteUIServer::pushParamsToClient(){
	if(readyToSend){
		vector<string>paramsList = getAllParamNamesList();
		syncAllParamsToPointers();
		sendUpdateForParamsInList(paramsList);
		sendREQU(true); //once all send, confirm to close the REQU
	}
}


void ofxRemoteUIServer::setNetworkInterface(string iface){
	userSuppliedNetInterface = iface;
}


void ofxRemoteUIServer::saveSettingsBackup(){

	#ifdef OF_AVAILABLE
	if(autoBackups){
		ofDirectory d;
		d.open(OFXREMOTEUI_SETTINGS_BACKUP_FOLDER);
		if (!d.exists()){
			ofDirectory::createDirectory(OFXREMOTEUI_SETTINGS_BACKUP_FOLDER);
		}d.close();
		string basePath = OFXREMOTEUI_SETTINGS_BACKUP_FOLDER + string("/") + ofFilePath::removeExt(OFXREMOTEUI_SETTINGS_FILENAME) + ".";
		for (int i = OFXREMOTEUI_NUM_BACKUPS - 1; i >= 0; i--){
			string originalPath = basePath + ofToString(i) + ".xml";
			string destPath = basePath + ofToString(i+1) + ".xml";
			ofFile og;
			og.open(originalPath);
			if ( og.exists() ){
				ofFile::moveFromTo(originalPath, destPath, true, true);
			}
			og.close();
		}
		ofFile f;
		f.open(OFXREMOTEUI_SETTINGS_FILENAME);
		if(f.exists()){
			ofFile::copyFromTo(OFXREMOTEUI_SETTINGS_FILENAME, basePath + "0.xml");
		}
		f.close();
		if(verbose_) RUI_LOG_NOTICE << "ofxRemoteUIServer saving a backup of the current " << OFXREMOTEUI_SETTINGS_FILENAME << " in " << OFXREMOTEUI_SETTINGS_BACKUP_FOLDER ;
	}
	#endif
}



void ofxRemoteUIServer::setup(int port_, float updateInterval_){

	//setup the broadcasting
	computerIP = getMyIP(userSuppliedNetInterface);
	if (computerIP != "NOT FOUND"){
		doBroadcast = true;
		vector<string>comps;
		split(comps, computerIP, '.');
		string multicastIP = comps[0] + "." + comps[1] + "." + comps[2] + "." + "255";
		broadcastSender.setup( multicastIP, OFXREMOTEUI_BROADCAST_PORT ); //multicast @
		RUI_LOG_NOTICE << "ofxRemoteUIServer: letting everyone know that I am at " << multicastIP << ":" << OFXREMOTEUI_BROADCAST_PORT ;
	}else{
		doBroadcast = false;
	}

	//check for enabled
	ofxXmlSettings s;
	bool exists = s.loadFile(OFXREMOTEUI_SETTINGS_FILENAME);
	if(exists){
		if( s.getNumTags(OFXREMOTEUI_XML_ENABLED) > 0 ){
			enabled = ("true" == s.getValue(OFXREMOTEUI_XML_ENABLED, "true"));
			if (!enabled) RUI_LOG_WARNING << "ofxRemoteUIServer launching disabled!" ;
		}
	}

	saveSettingsBackup();

	if(port_ == -1){ //if no port specified, pick a random one, but only the very first time we get launched!
		portIsSet = false;
		ofxXmlSettings s;
		bool exists = s.loadFile(OFXREMOTEUI_SETTINGS_FILENAME);
		bool portNeedsToBePicked = false;
		if (exists){
			if( s.getNumTags(OFXREMOTEUI_XML_PORT) > 0 ){
				port_ = s.getValue(OFXREMOTEUI_XML_PORT, 10000);
			}else{
				portNeedsToBePicked = true;
			}
		}else{
			portNeedsToBePicked = true;
		}
		if(portNeedsToBePicked){
			#ifdef OF_AVAILABLE
			ofSeedRandom();
			port_ = ofRandom(5000, 60000);
			#else
			srand (time(NULL));
			port_ = 5000 + rand()%55000;
			#endif
			ofxXmlSettings s2;
			s2.loadFile(OFXREMOTEUI_SETTINGS_FILENAME);
			s2.setValue(OFXREMOTEUI_XML_PORT, port_, 0);
			s2.saveFile();
		}
	}else{
		portIsSet = true;
	}
	params.clear();
	updateInterval = updateInterval_;
	waitingForReply = false;
	avgTimeSinceLastReply = timeSinceLastReply = timeCounter = 0.0f;
	port = port_;
	RUI_LOG_NOTICE << "ofxRemoteUIServer listening at port " << port << " ... " ;
	oscReceiver.setup(port);

	#ifdef OF_AVAILABLE
	ofAddListener(ofEvents().exit, this, &ofxRemoteUIServer::_appExited); //to save to xml, disconnect, etc
	ofAddListener(ofEvents().keyPressed, this, &ofxRemoteUIServer::_keyPressed);
	ofAddListener(ofEvents().update, this, &ofxRemoteUIServer::_update);
	if(drawNotifications){
		ofAddListener(ofEvents().draw, this, &ofxRemoteUIServer::_draw);
	}
	#endif
}

#ifdef OF_AVAILABLE
void ofxRemoteUIServer::_appExited(ofEventArgs &e){
	if(!enabled) return;
	OFX_REMOTEUI_SERVER_CLOSE();		//stop the server
	if(saveToXmlOnExit){
		OFX_REMOTEUI_SERVER_SAVE_TO_XML();	//save values to XML
	}
}

void ofxRemoteUIServer::_draw(ofEventArgs &e){
	if(!enabled) return;
	ofSetupScreen(); //mmm this is a bit scary //TODO!
	draw( 20, ofGetHeight() - 20);
}

void ofxRemoteUIServer::_update(ofEventArgs &e){
	update(ofGetLastFrameTime());
}

void ofxRemoteUIServer::_keyPressed(ofKeyEventArgs &e){

	if(!enabled) return;

	if (showValuesOnScreen){
		switch(e.key){ //you can save current config from tab screen by pressing s
			case 's':
				saveToXML(OFXREMOTEUI_SETTINGS_FILENAME);
				onScreenNotifications.addNotification("SAVED CONFIG to default XML");
				break;
			case 'r':
				restoreAllParamsToInitialXML();
				onScreenNotifications.addNotification("RESET CONFIG TO SERVER-LAUNCH XML values");
				break;
			case OF_KEY_UP:
				selectedItem -= 1;
				if(selectedItem<0) selectedItem = orderedKeys.size() - 1;
				break;
			case OF_KEY_DOWN:
				selectedItem += 1;
				if(selectedItem >= orderedKeys.size()) selectedItem = 0;
				break;
			case OF_KEY_LEFT:
			case OF_KEY_RIGHT:{
				float sign = e.key == OF_KEY_RIGHT ? 1.0 : -1.0;
				string key = orderedKeys[selectedItem];
				RemoteUIParam p = params[key];
				switch (p.type) {
					case REMOTEUI_PARAM_FLOAT:
						p.floatVal += sign * (p.maxFloat - p.minFloat) * 0.0025;
						p.floatVal = ofClamp(p.floatVal, p.minFloat, p.maxFloat);
						break;
					case REMOTEUI_PARAM_ENUM:
					case REMOTEUI_PARAM_INT:
						p.intVal += sign;
						p.intVal = ofClamp(p.intVal, p.minInt, p.maxInt);
						break;
					case REMOTEUI_PARAM_BOOL:
						p.boolVal = !p.boolVal;
						break;
					default:
						break;
				}
				params[key] = p;
				syncPointerToParam(key);
				pushParamsToClient();
				if(callBack != NULL){ //send "param modified" callback to ourselves!
					RemoteUIServerCallBackArg cbArg;
					cbArg.action = CLIENT_DID_RESET_TO_XML;
					cbArg.host = "localhost";
					cbArg.action =  CLIENT_UPDATED_PARAM;
					cbArg.paramName = key;
					cbArg.param = params[key];  //copy the updated param to the callbakc arg
					#ifdef OF_AVAILABLE
					onScreenNotifications.addParamUpdate(key, cbArg.param.getValueAsString());
					#endif
					callBack(cbArg);
				}
				}break;

		}
	}
	if(e.key == showInterfaceKey){
		showValuesOnScreen = !showValuesOnScreen;
		if (showValuesOnScreen){
			uiLines.clear();
		}
	}
}

void ofxRemoteUIServer::startInBackgroundThread(){
	threadedUpdate = true;
	startThread();
}
#endif

void ofxRemoteUIServer::update(float dt){

	#ifdef OF_AVAILABLE
	if(!threadedUpdate && !updatedThisFrame){
		updateServer(dt);
	}
	updatedThisFrame = true; //this only makes sense when running threaded
	#else
	updateServer(dt);
	#endif
}

#ifdef OF_AVAILABLE
void ofxRemoteUIServer::threadedFunction(){

	while (threadRunning) {
		updateServer(1./30.); //30 fps timebase
		ofSleepMillis(33);
	}
	if(verbose_) RUI_LOG_VERBOSE << "ofxRemoteUIServer threadedFunction() ending" ;
}
#endif


void ofxRemoteUIServer::draw(int x, int y){

	if(!enabled) return;

	#ifdef OF_AVAILABLE
	ofPushStyle();
	ofFill();
	ofEnableAlphaBlending();
	if(showValuesOnScreen){
		int padding = 30;
		int x = padding;
		int initialY = padding * 2.5;
		int y = initialY;
		int colw = 320;
		int valOffset = colw * 0.6;
		int spacing = 20;

		ofSetColor(11, 245);
		ofRect(0,0, ofGetWidth(), ofGetHeight());
		ofSetColor(44, 245);
		ofRect(0,0, ofGetWidth(), padding + spacing );

		ofSetColor(255);
		ofDrawBitmapString("ofxRemoteUIServer params list : press 'TAB' to hide.\nPress 's' to save current config. Press 'r' to restore all param's launch state. Use Arrow Keys to edit values.", padding,  padding - 3);

		int linesInited = uiLines.getNumVertices() > 0 ;

		for(int i = 0; i < orderedKeys.size(); i++){
			string key = orderedKeys[i];
			RemoteUIParam p = params[key];
			int chars = key.size();
			int charw = 9;
			int stringw = chars * charw;
			if (stringw > valOffset){
				key = key.substr(0, (valOffset) / charw );
			}
			if (selectedItem != i){
				ofSetColor(p.r, p.g, p.b);
			}else{
				if(ofGetFrameNum()%5 < 1) ofSetColor(222);
				else ofSetColor(255,0,0);
			}

			if(p.type != REMOTEUI_PARAM_SPACER){
				string sel = (selectedItem == i) ? ">>" : "  ";
				ofDrawBitmapString(sel + key, x, y);
			}else{
				ofDrawBitmapString("+ " + p.stringVal, x,y);
			}

			switch (p.type) {
				case REMOTEUI_PARAM_FLOAT:
					ofDrawBitmapString(ofToString(p.floatVal), x + valOffset, y);
					break;
				case REMOTEUI_PARAM_ENUM:
				case REMOTEUI_PARAM_INT:
					ofDrawBitmapString(ofToString(p.intVal), x + valOffset, y);
					break;
				case REMOTEUI_PARAM_BOOL:
					ofDrawBitmapString(p.boolVal ? "true" : "false", x + valOffset, y);
					break;
				case REMOTEUI_PARAM_STRING:
					ofDrawBitmapString(p.stringVal, x + valOffset, y);
					break;
				case REMOTEUI_PARAM_COLOR:
					ofSetColor(p.redVal, p.greenVal, p.blueVal, p.alphaVal);
					ofRect(x + valOffset, y - spacing * 0.6, 64, spacing * 0.85);
					break;
				case REMOTEUI_PARAM_SPACER:
					break;
				default: printf("weird RemoteUIParam at draw()!\n"); break;
			}
			if(!linesInited){
				uiLines.addVertex(ofVec2f(x, y + spacing * 0.33));
				uiLines.addVertex(ofVec2f(x + colw * 0.8, y + spacing * 0.33));
			}
			y += spacing;
			if (y > ofGetHeight() - padding){
				x += colw;
				y = initialY;
			}
		}
		ofSetColor(32);
		uiLines.draw();
	}else{
		onScreenNotifications.draw(x, y);
	}

	ofPopStyle();
	#endif
	updatedThisFrame = false;
}


void ofxRemoteUIServer::handleBroadcast(){
	if(doBroadcast){
		if(broadcastTime > OFXREMOTEUI_BORADCAST_INTERVAL){
			broadcastTime = 0.0f;
			if (computerName.size() == 0){
#ifdef OF_AVAILABLE
				Poco::Environment e;
				computerName = e.nodeName();

				char pathbuf[2048];
				uint32_t  bufsize = sizeof(pathbuf);
#ifdef TARGET_OSX
				_NSGetExecutablePath(pathbuf, &bufsize);
				Poco::Path p = Poco::Path(pathbuf);
				binaryName = p[p.depth()];
#endif
#ifdef TARGET_WIN32
				GetModuleFileNameA( NULL, pathbuf, bufsize ); //no idea why, but GetModuleFileName() is not defined?
				Poco::Path p = Poco::Path(pathbuf);
				binaryName = p[p.depth()];
#endif
#else
				computerName = "Computer"; //TODO!
				binaryName = "App";
#endif
			}
			ofxOscMessage m;
			m.addIntArg(port); //0
			m.addStringArg(computerName); //1
			m.addStringArg(binaryName);	//2
			m.addIntArg(broadcastCount); // 3
			broadcastSender.sendMessage(m);
			broadcastCount++;
		}
	}
}

void ofxRemoteUIServer::updateServer(float dt){

	if(!enabled) return;

	timeCounter += dt;
	broadcastTime += dt;
	timeSinceLastReply  += dt;

	#ifdef OF_AVAILABLE
	onScreenNotifications.update(dt);
	#endif

	if(readyToSend){
		if (timeCounter > updateInterval){
			timeCounter = 0.0f;
			//vector<string> changes = scanForUpdatedParamsAndSync(); //sends changed params to client
			//cout << "ofxRemoteUIServer: sent " << ofToString(changes.size()) << " updates to client" ;
			//sendUpdateForParamsInList(changes);
		}
	}

	//let everyone know I exist and which is my port, every now and then
	handleBroadcast();

	while( oscReceiver.hasWaitingMessages() ){// check for waiting messages from client

		ofxOscMessage m;
		oscReceiver.getNextMessage(&m);
		if (!readyToSend){ // if not connected, connect to our friend so we can talk back
			connect(m.getRemoteIp(), port + 1);
		}

		DecodedMessage dm = decode(m);
		RemoteUIServerCallBackArg cbArg; // to notify our "delegate"
		cbArg.host = m.getRemoteIp();
		switch (dm.action) {

			case HELO_ACTION: //if client says hi, say hi back
				sendHELLO();
				if(callBack != NULL){
					cbArg.action = CLIENT_CONNECTED;
					callBack(cbArg);
				}
				if(verbose_) RUI_LOG_VERBOSE << "ofxRemoteUIServer: " << m.getRemoteIp() << " says HELLO!"  ;
				#ifdef OF_AVAILABLE
				onScreenNotifications.addNotification("CONNECTED (" + cbArg.host +  ")!");
				#endif
				break;

			case REQUEST_ACTION:{ //send all params to client
				if(verbose_) RUI_LOG_VERBOSE << "ofxRemoteUIServer: " << m.getRemoteIp() << " sends REQU!"  ;
				pushParamsToClient();
				}break;

			case SEND_PARAM_ACTION:{ //client is sending us an updated val
				if(verbose_) RUI_LOG_VERBOSE << "ofxRemoteUIServer: " << m.getRemoteIp() << " sends SEND!"  ;
				updateParamFromDecodedMessage(m, dm);
				if(callBack != NULL){
					cbArg.action = CLIENT_UPDATED_PARAM;
					cbArg.paramName = dm.paramName;
					cbArg.param = params[dm.paramName];  //copy the updated param to the callbakc arg
					callBack(cbArg);
					#ifdef OF_AVAILABLE
					onScreenNotifications.addParamUpdate(dm.paramName, cbArg.param.getValueAsString());
					#endif
				}
			}
				break;

			case CIAO_ACTION:{
				if(verbose_) RUI_LOG_VERBOSE << "ofxRemoteUIServer: " << m.getRemoteIp() << " says CIAO!" ;
				sendCIAO();
				#ifdef OF_AVAILABLE
				onScreenNotifications.addNotification("DISCONNECTED (" + cbArg.host +  ")!");
				#endif
				if(callBack != NULL){
					cbArg.action = CLIENT_DISCONNECTED;
					callBack(cbArg);
				}
				clearOscReceiverMsgQueue();
				readyToSend = false;
			}break;

			case TEST_ACTION: // we got a request from client, lets bounce back asap.
				sendTEST();
				//if(verbose)RUI_LOG_VERBOSE << "ofxRemoteUIServer: " << m.getRemoteIp() << " says TEST!" ;
				break;

			case PRESET_LIST_ACTION: //client wants us to send a list of all available presets
				presetNames = getAvailablePresets();
				if (presetNames.size() == 0){
					presetNames.push_back(OFXREMOTEUI_NO_PRESETS);
				}
				sendPREL(presetNames);
				break;

			case SET_PRESET_ACTION:{ // client wants to set a preset
				string presetName = m.getArgAsString(0);
				vector<string> missingParams = loadFromXML(string(OFXREMOTEUI_PRESET_DIR) + "/" + presetName + ".xml");
				if(verbose_) RUI_LOG_NOTICE << "ofxRemoteUIServer: setting preset: " << presetName ;
				sendSETP(presetName);
				sendMISP(missingParams);
				if(callBack != NULL){
					cbArg.action = CLIENT_DID_SET_PRESET;
					cbArg.msg = presetName;
					callBack(cbArg);
				}
				#ifdef OF_AVAILABLE
				onScreenNotifications.addNotification("SET PRESET to '" + string(OFXREMOTEUI_PRESET_DIR) + "/" + presetName + ".xml'");
				#endif
			}break;

			case SAVE_PRESET_ACTION:{ //client wants to save current xml as a new preset
				string presetName = m.getArgAsString(0);
				if(verbose_) RUI_LOG_NOTICE << "ofxRemoteUIServer: saving NEW preset: " << presetName ;
				saveToXML(string(OFXREMOTEUI_PRESET_DIR) + "/" + presetName + ".xml");
				sendSAVP(presetName);
				#ifdef OF_AVAILABLE
				onScreenNotifications.addNotification("SAVED PRESET to '" + string(OFXREMOTEUI_PRESET_DIR) + "/" + presetName + ".xml'");
				#endif


				if(callBack != NULL){
					cbArg.action = CLIENT_SAVED_PRESET;
					cbArg.msg = presetName;
					callBack(cbArg);
				}
			}break;

			case DELETE_PRESET_ACTION:{
				string presetName = m.getArgAsString(0);
				if(verbose_) RUI_LOG_NOTICE << "ofxRemoteUIServer: DELETE preset: " << presetName ;
				deletePreset(presetName);
				sendDELP(presetName);
				#ifdef OF_AVAILABLE
				onScreenNotifications.addNotification("DELETED PRESET '" + string(OFXREMOTEUI_PRESET_DIR) + "/" + presetName + ".xml'");
				#endif
				if(callBack != NULL){
					cbArg.action = CLIENT_DELETED_PRESET;
					cbArg.msg = presetName;
					callBack(cbArg);
				}
			}break;

			case SAVE_CURRENT_STATE_ACTION:{
				if(verbose_) RUI_LOG_NOTICE << "ofxRemoteUIServer: SAVE CURRENT PARAMS TO DEFAULT XML: " ;
				saveToXML(OFXREMOTEUI_SETTINGS_FILENAME);
				#ifdef OF_AVAILABLE
				onScreenNotifications.addNotification("SAVED CONFIG to default XML");
				#endif
				sendSAVE(true);
				if(callBack != NULL){
					cbArg.action = CLIENT_SAVED_STATE;
					callBack(cbArg);
				}
			}break;

			case RESET_TO_XML_ACTION:{
				if(verbose_) RUI_LOG_NOTICE << "ofxRemoteUIServer: RESET TO XML: " ;
				restoreAllParamsToInitialXML();
				sendRESX(true);
				#ifdef OF_AVAILABLE
				onScreenNotifications.addNotification("RESET CONFIG TO SERVER-LAUNCH XML values");
				#endif
				if(callBack != NULL){
					cbArg.action = CLIENT_DID_RESET_TO_XML;
					callBack(cbArg);
				}
			}break;

			case RESET_TO_DEFAULTS_ACTION:{
				if(verbose_) RUI_LOG_NOTICE << "ofxRemoteUIServer: RESET TO DEFAULTS: " ;
				restoreAllParamsToDefaultValues();
				sendRESD(true);
				#ifdef OF_AVAILABLE
				onScreenNotifications.addNotification("RESET CONFIG TO DEFAULTS (source defined values)");
				#endif
				if(callBack != NULL){
					cbArg.action = CLIENT_DID_RESET_TO_DEFAULTS;
					callBack(cbArg);
				}
			}break;

			case SET_GROUP_PRESET_ACTION:{ // client wants to set a preset for a group
				string presetName = m.getArgAsString(0);
				string groupName = m.getArgAsString(1);
				vector<string> missingParams = loadFromXML(string(OFXREMOTEUI_PRESET_DIR) + "/" + groupName + "/" + presetName + ".xml");
				vector<string> filtered;
				for(int i = 0; i < missingParams.size(); i++){
					if ( params[ missingParams[i] ].group == groupName ){
						filtered.push_back(missingParams[i]);
					}
				}
				if(verbose_) RUI_LOG_NOTICE << "ofxRemoteUIServer: setting preset group: " << groupName << "/" <<presetName ;
				sendSETp(presetName, groupName);
				sendMISP(filtered);
				if(callBack != NULL){
					cbArg.action = CLIENT_DID_SET_GRUP_PRESET;
					cbArg.msg = presetName;
					cbArg.group = groupName;
					callBack(cbArg);
				}
				#ifdef OF_AVAILABLE
				onScreenNotifications.addNotification("SET '" + groupName + "' GROUP TO '" + presetName + ".xml' PRESET");
				#endif
			}break;

			case SAVE_GROUP_PRESET_ACTION:{ //client wants to save current xml as a new preset
				string presetName = m.getArgAsString(0);
				string groupName = m.getArgAsString(1);
				if(verbose_) RUI_LOG_NOTICE << "ofxRemoteUIServer: saving NEW preset: " << presetName ;
				saveGroupToXML(string(OFXREMOTEUI_PRESET_DIR) + "/" + groupName + "/" + presetName + ".xml", groupName);
				sendSAVp(presetName, groupName);
				#ifdef OF_AVAILABLE
				onScreenNotifications.addNotification("SAVED PRESET '" + presetName + ".xml' FOR GROUP '" + groupName + "'");
				#endif
				if(callBack != NULL){
					cbArg.action = CLIENT_SAVED_GROUP_PRESET;
					cbArg.msg = presetName;
					cbArg.group = groupName;
					callBack(cbArg);
				}
			}break;

			case DELETE_GROUP_PRESET_ACTION:{
				string presetName = m.getArgAsString(0);
				string groupName = m.getArgAsString(1);
				if(verbose_) RUI_LOG_NOTICE << "ofxRemoteUIServer: DELETE preset: " << presetName ;
				deletePreset(presetName, groupName);
				sendDELp(presetName, groupName);
				#ifdef OF_AVAILABLE
				onScreenNotifications.addNotification("DELETED PRESET '" + presetName + ".xml' FOR GROUP'" + groupName + "'");
				#endif
				if(callBack != NULL){
					cbArg.action = CLIENT_DELETED_GROUP_PRESET;
					cbArg.msg = presetName;
					cbArg.group = groupName;
					callBack(cbArg);
				}
			}break;
			default: RUI_LOG_ERROR << "ofxRemoteUIServer::update >> ERR!"; break;
		}
	}
}

void ofxRemoteUIServer::deletePreset(string name, string group){
	#ifdef OF_AVAILABLE
	ofDirectory dir;
	if (group == "")
		dir.open(string(OFXREMOTEUI_PRESET_DIR) + "/" + name + ".xml");
	else
		dir.open(string(OFXREMOTEUI_PRESET_DIR) + "/" + group + "/" + name + ".xml");
	dir.remove(true);
	#else
	string file = string(OFXREMOTEUI_PRESET_DIR) + "/" + name + ".xml";
	if (group != "") file = string(OFXREMOTEUI_PRESET_DIR) + "/" + group + "/" + name + ".xml";
	remove( file.c_str() );
	#endif
}


vector<string> ofxRemoteUIServer::getAvailablePresets(){

	vector<string> presets;

	#ifdef OF_AVAILABLE
	ofDirectory dir;
	dir.listDir(ofToDataPath(OFXREMOTEUI_PRESET_DIR));
	vector<ofFile> files = dir.getFiles();
	for(int i = 0; i < files.size(); i++){
		string fileName = files[i].getFileName();
		string extension = files[i].getExtension();
		std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
		if (files[i].isFile() && extension == "xml"){
			string presetName = fileName.substr(0, fileName.size()-4);
			presets.push_back(presetName);
		}
		if (files[i].isDirectory()){
			ofDirectory dir2;
			dir2.listDir( ofToDataPath( string(OFXREMOTEUI_PRESET_DIR) + "/" + fileName) );
			vector<ofFile> files2 = dir2.getFiles();
			for(int j = 0; j < files2.size(); j++){
				string fileName2 = files2[j].getFileName();
				string extension2 = files2[j].getExtension();
				std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
				if (files2[j].isFile() && extension2 == "xml"){
					string presetName2 = fileName2.substr(0, fileName2.size()-4);
					presets.push_back(fileName + "/" + presetName2);
				}
			}
		}
	}
	#else
	DIR *dir;
	struct dirent *ent;
	if ((dir = opendir (OFXREMOTEUI_PRESET_DIR)) != NULL) {
		while ((ent = readdir (dir)) != NULL) {
			if ( strcmp( get_filename_ext(ent->d_name), "xml") == 0 ){
				string fileName = string(ent->d_name);
				string presetName = fileName.substr(0, fileName.size()-4);
				presets.push_back(presetName);
			}
		}
		closedir (dir);
	}
	#endif
	return presets;
}

void ofxRemoteUIServer::setColorForParam(RemoteUIParam &p, ofColor c){

	if (c.a > 0){ //if user supplied a color, override the setColor
		p.r = c.r;  p.g = c.g;  p.b = c.b;  p.a = c.a;
	}else{
		if (colorSet){
			p.r = paramColorCurrentVariation.r;
			p.g = paramColorCurrentVariation.g;
			p.b = paramColorCurrentVariation.b;
			p.a = paramColorCurrentVariation.a;
		}
	}
}


void ofxRemoteUIServer::addSpacer(string title){
	RemoteUIParam p;
	p.type = REMOTEUI_PARAM_SPACER;
	p.stringVal = title;
	p.r = paramColor.r;
	p.g = paramColor.g;
	p.b = paramColor.b;
	p.group = upcomingGroup; //to ignore those in the client app later when grouping
	p.a = 255; //spacer has full alpha
	#ifdef OF_AVAILABLE
	addParamToDB(p, title + " - " + ofToString((int)ofRandom(1000000)));
	#else
	addParamToDB(p, title + " - " + ofToString(rand()%1000000));
	#endif
	if(verbose_) RUI_LOG_NOTICE << "ofxRemoteUIServer Adding Group '" << title << "' #######################" ;
}


void ofxRemoteUIServer::shareParam(string paramName, float* param, float min, float max, ofColor c){
	RemoteUIParam p;
	p.type = REMOTEUI_PARAM_FLOAT;
	p.floatValAddr = param;
	p.maxFloat = max;
	p.minFloat = min;
	p.floatVal = *param = ofClamp(*param, min, max);
	p.group = upcomingGroup;
	setColorForParam(p, c);
	addParamToDB(p, paramName);
	if(verbose_) RUI_LOG_NOTICE << "ofxRemoteUIServer Sharing Float Param '" << paramName << "'" ;
}


void ofxRemoteUIServer::shareParam(string paramName, bool* param, ofColor c, int nothingUseful ){
	RemoteUIParam p;
	p.type = REMOTEUI_PARAM_BOOL;
	p.boolValAddr = param;
	p.boolVal = *param;
	p.group = upcomingGroup;
	setColorForParam(p, c);
	addParamToDB(p, paramName);
	if(verbose_) RUI_LOG_NOTICE << "ofxRemoteUIServer Sharing Bool Param '" << paramName << "'" ;
}


void ofxRemoteUIServer::shareParam(string paramName, int* param, int min, int max, ofColor c ){
	RemoteUIParam p;
	p.type = REMOTEUI_PARAM_INT;
	p.intValAddr = param;
	p.maxInt = max;
	p.minInt = min;
	p.group = upcomingGroup;
	setColorForParam(p, c);
	p.intVal = *param = ofClamp(*param, min, max);
	addParamToDB(p, paramName);
	if(verbose_) RUI_LOG_NOTICE << "ofxRemoteUIServer Sharing Int Param '" << paramName << "'" ;
}

void ofxRemoteUIServer::shareParam(string paramName, int* param, int min, int max, vector<string> names, ofColor c ){
	RemoteUIParam p;
	p.type = REMOTEUI_PARAM_ENUM;
	p.intValAddr = param;
	p.maxInt = max;
	p.minInt = min;
	p.enumList = names;
	p.group = upcomingGroup;
	setColorForParam(p, c);
	p.intVal = *param = ofClamp(*param, min, max);
	addParamToDB(p, paramName);
	if(verbose_) RUI_LOG_NOTICE << "ofxRemoteUIServer Sharing Enum Param '" << paramName << "'" ;
}


void ofxRemoteUIServer::shareParam(string paramName, string* param, ofColor c, int nothingUseful ){
	RemoteUIParam p;
	p.type = REMOTEUI_PARAM_STRING;
	p.stringValAddr = param;
	p.stringVal = *param;
	p.group = upcomingGroup;
	setColorForParam(p, c);
	addParamToDB(p, paramName);
	if(verbose_) RUI_LOG_NOTICE << "ofxRemoteUIServer Sharing String Param '" << paramName << "'";
}

void ofxRemoteUIServer::shareParam(string paramName, unsigned char* param, ofColor bgColor, int nothingUseful){
	RemoteUIParam p;
	p.type = REMOTEUI_PARAM_COLOR;
	p.redValAddr = param;
	p.redVal = param[0];
	p.greenVal = param[1];
	p.blueVal = param[2];
	p.alphaVal = param[3];
	p.group = upcomingGroup;
	setColorForParam(p, bgColor);
	addParamToDB(p, paramName);
	if(verbose_) RUI_LOG_NOTICE << "ofxRemoteUIServer Sharing Color Param '" << paramName << "'";
}


void ofxRemoteUIServer::connect(string ipAddress, int port){
	avgTimeSinceLastReply = timeSinceLastReply = timeCounter = 0.0f;
	waitingForReply = false;
	//params.clear();
	oscSender.setup(ipAddress, port);
	readyToSend = true;
}

void ofxRemoteUIServer::sendLogToClient(char* format, ...){

	if(readyToSend){
		char line[1024]; //this will crash (or worse, make a memory mess) if you try to log >= 1024 chars
		va_list args;
		va_start(args, format);
		vsprintf(line, format,  args);

		ofxOscMessage m;
		m.setAddress("LOG_");
		m.addStringArg(string(line));
		try{
			oscSender.sendMessage(m);
		}catch(exception e){
			RUI_LOG_ERROR << "exception " << e.what() ;
		}
	}
}

