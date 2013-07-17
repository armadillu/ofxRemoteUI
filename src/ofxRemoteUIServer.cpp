//
//  ofxRemoteUI.cpp
//  emptyExample
//
//  Created by Oriol Ferrer Mesià on 09/01/13.
//
//

#include "ofxRemoteUIServer.h"
#include <iostream>
#include <algorithm>
#include <string>
#include <string.h>
#include "dirent.h"
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
#ifndef TARGET_WIN32
	#include <sys/time.h>
#endif
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
	colorSet = false;
	upcomingGroup = DEFAULT_PARAM_GROUP;
	verbose = false;
	loadedFromXML = false;
	
	//add random colors to table
	colorTableIndex = 0;
	int a = 64;
	#if ( OF_VERSION_MINOR > 0 )
	ofColor prevColor = ofColor::fromHsb((int)ofRandom(0,150), 255, 255, 32);
	for(int i = 0; i < 30; i++){
		ofColor c = prevColor;
		c.setHue(  ((int) (prevColor.getHue() + ofRandom(40,55) )) % 255 );
		colorTables.push_back( c );
		prevColor = c;
	}
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

	#if ( OF_VERSION_MINOR > 0 ) 
	ofDirectory d;
	d.open(OFX_REMOTEUI_PRESET_DIR);
	d.create(true);
	#else
		#if defined(_WIN32)
		_mkdir(OFX_REMOTEUI_PRESET_DIR);
		#else
		mkdir(OFX_REMOTEUI_PRESET_DIR, 0777); 
		#endif
	#endif
}


void ofxRemoteUIServer::close(){
	if(readyToSend)
		sendCIAO();
}


void ofxRemoteUIServer::setParamGroup(string g){
	upcomingGroup = g;
}

void ofxRemoteUIServer::unsetParamColor(){
	colorSet = false;
}

void ofxRemoteUIServer::setParamColor( ofColor c ){
	colorSet = true;
	paramColor = c;
}

void ofxRemoteUIServer::setNewParamColor(){

	ofColor c = colorTables[colorTableIndex];
	colorSet = true;
	paramColor = c;
	colorTableIndex++;
	if(colorTableIndex>= colorTables.size()){
		colorTableIndex = 0;
	}
}


void ofxRemoteUIServer::saveToXML(string fileName){

	cout << "ofxRemoteUIServer: saving to xml '" << fileName << "'" << endl;
	ofxXmlSettings s;
	s.loadFile(fileName);
	s.clear();
	s.addTag(OFX_REMOTEUI_XML_TAG);
	s.pushTag(OFX_REMOTEUI_XML_TAG);

	int numFloats = 0;
	int numInts = 0;
	int numStrings = 0;
	int numBools = 0;
	int numEnums = 0;
	int numColors = 0;

	for( map<string,RemoteUIParam>::iterator ii = params.begin(); ii != params.end(); ++ii ){
		string key = (*ii).first;
		RemoteUIParam t = params[key];
		switch (t.type) {
			case REMOTEUI_PARAM_FLOAT:
				if(verbose) cout << "ofxRemoteUIServer saving '" << key << "' (" <<  *t.floatValAddr <<") to XML" << endl;
				s.setValue("REMOTEUI_PARAM_FLOAT", (double)*t.floatValAddr, numFloats);
				s.setAttribute("REMOTEUI_PARAM_FLOAT", "paramName", key, numFloats);
				numFloats++;
				break;
			case REMOTEUI_PARAM_INT:
				if(verbose) cout << "ofxRemoteUIServer saving '" << key << "' (" <<  *t.intValAddr <<") to XML" << endl;
				s.setValue("REMOTEUI_PARAM_INT", (int)*t.intValAddr, numInts);
				s.setAttribute("REMOTEUI_PARAM_INT", "paramName", key, numInts);
				numInts++;
				break;
			case REMOTEUI_PARAM_COLOR:
				s.addTag("REMOTEUI_PARAM_COLOR");
				s.setAttribute("REMOTEUI_PARAM_COLOR", "paramName", key, numColors);
				s.pushTag("REMOTEUI_PARAM_COLOR", numColors);
				if(verbose) cout << "ofxRemoteUIServer saving '" << key << "' (" << (int)*t.redValAddr << " " << (int)*(t.redValAddr+1) << " " << (int)*(t.redValAddr+2) << " " << (int)*(t.redValAddr+3) << ") to XML" << endl;
				s.setValue("R", (int)*t.redValAddr);
				s.setValue("G", (int)*(t.redValAddr+1));
				s.setValue("B", (int)*(t.redValAddr+2));
				s.setValue("A", (int)*(t.redValAddr+3));
				s.popTag();
				numColors++;
				break;
			case REMOTEUI_PARAM_ENUM:
				if(verbose) cout << "ofxRemoteUIServer saving '" << key << "' (" <<  *t.intValAddr <<") to XML" << endl;
				s.setValue("REMOTEUI_PARAM_ENUM", (int)*t.intValAddr, numEnums);
				s.setAttribute("REMOTEUI_PARAM_ENUM", "paramName", key, numEnums);
				numEnums++;
				break;
			case REMOTEUI_PARAM_BOOL:
				if(verbose) cout << "ofxRemoteUIServer saving '" << key << "' (" <<  *t.boolValAddr <<") to XML" << endl;
				s.setValue("REMOTEUI_PARAM_BOOL", (bool)*t.boolValAddr, numBools);
				s.setAttribute("REMOTEUI_PARAM_BOOL", "paramName", key, numBools);
				numBools++;
				break;
			case REMOTEUI_PARAM_STRING:
				if(verbose) cout << "ofxRemoteUIServer saving '" << key << "' (" <<  *t.stringValAddr <<") to XML" << endl;
				s.setValue("REMOTEUI_PARAM_STRING", (string)*t.stringValAddr, numStrings);
				s.setAttribute("REMOTEUI_PARAM_STRING", "paramName", key, numStrings);
				numStrings++;
				break;

			default:
				break;
		}
	}
	s.saveFile(fileName);
}


void ofxRemoteUIServer::loadFromXML(string fileName){

	ofxXmlSettings s;
	bool exists = s.loadFile(fileName);

	if (exists){

		if( s.getNumTags(OFX_REMOTEUI_XML_TAG) > 0 ){
			s.pushTag(OFX_REMOTEUI_XML_TAG, 0);

			int numFloats = s.getNumTags("REMOTEUI_PARAM_FLOAT");
			if(numFloats > 0){
				for (int i=0; i< numFloats; i++){
					string paramName = s.getAttribute("REMOTEUI_PARAM_FLOAT", "paramName", "", i);
					float val = s.getValue("REMOTEUI_PARAM_FLOAT", 0.0, i);
					map<string,RemoteUIParam>::iterator it = params.find(paramName);
					if ( it != params.end() ){	// found!
						if(params[paramName].floatValAddr != NULL){
							*params[paramName].floatValAddr = val;
							params[paramName].floatVal = val;
							*params[paramName].floatValAddr = ofClamp(*params[paramName].floatValAddr, params[paramName].minFloat, params[paramName].maxFloat);
							if(!loadedFromXML) paramsFromXML[paramName] = params[paramName];
							if(verbose) cout << "ofxRemoteUIServer loading a FLOAT '" << paramName <<"' (" << ofToString( *params[paramName].floatValAddr, 3) << ") from XML" << endl;
						}else{
							cout << "ofxRemoteUIServer ERROR at loading FLOAT (" << paramName << ")" << endl;
						}
					}
				}
			}

			int numInts = s.getNumTags("REMOTEUI_PARAM_INT");
			if(numInts > 0){
				for (int i=0; i< numInts; i++){
					string paramName = s.getAttribute("REMOTEUI_PARAM_INT", "paramName", "", i);
					float val = s.getValue("REMOTEUI_PARAM_INT", 0, i);
					map<string,RemoteUIParam>::iterator it = params.find(paramName);
					if ( it != params.end() ){	// found!
						if(params[paramName].intValAddr != NULL){
							*params[paramName].intValAddr = val;
							params[paramName].intVal = val;
							*params[paramName].intValAddr = ofClamp(*params[paramName].intValAddr, params[paramName].minInt, params[paramName].maxInt);
							if(!loadedFromXML) paramsFromXML[paramName] = params[paramName];
							if(verbose) cout << "ofxRemoteUIServer loading an INT '" << paramName <<"' (" << (int) *params[paramName].intValAddr << ") from XML" << endl;
						}else{
							cout << "ofxRemoteUIServer ERROR at loading INT (" << paramName << ")" << endl;
						}
					}
				}
			}

			int numColors = s.getNumTags("REMOTEUI_PARAM_COLOR");
			if(numColors > 0){
				for (int i=0; i< numColors; i++){
					string paramName = s.getAttribute("REMOTEUI_PARAM_COLOR", "paramName", "", i);
					s.pushTag("REMOTEUI_PARAM_COLOR", i);
					int r = s.getValue("R", 0);
					int g = s.getValue("G", 0);
					int b = s.getValue("B", 0);
					int a = s.getValue("A", 0);
					map<string,RemoteUIParam>::iterator it = params.find(paramName);
					if ( it != params.end() ){	// found!
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
							if(verbose) cout << "ofxRemoteUIServer loading a COLOR '" << paramName <<"' (" << (int)*params[paramName].redValAddr << " " << (int)*(params[paramName].redValAddr+1) << " " << (int)*(params[paramName].redValAddr+2) << " " << (int)*(params[paramName].redValAddr+3)  << ") from XML" << endl;
						}else{
							cout << "ofxRemoteUIServer ERROR at loading COLOR (" << paramName << ")" << endl;
						}
					}
					s.popTag();
				}
			}

			int numEnums = s.getNumTags("REMOTEUI_PARAM_ENUM");
			if(numEnums > 0){
				for (int i=0; i< numEnums; i++){
					string paramName = s.getAttribute("REMOTEUI_PARAM_ENUM", "paramName", "", i);
					float val = s.getValue("REMOTEUI_PARAM_ENUM", 0, i);
					map<string,RemoteUIParam>::iterator it = params.find(paramName);
					if ( it != params.end() ){	// found!
						if(params[paramName].intValAddr != NULL){
							*params[paramName].intValAddr = val;
							params[paramName].intVal = val;
							*params[paramName].intValAddr = ofClamp(*params[paramName].intValAddr, params[paramName].minInt, params[paramName].maxInt);
							if(!loadedFromXML) paramsFromXML[paramName] = params[paramName];
							if(verbose) cout << "ofxRemoteUIServer loading an ENUM '" << paramName <<"' (" << (int) *params[paramName].intValAddr << ") from XML" << endl;
						}else{
							cout << "ofxRemoteUIServer ERROR at loading ENUM (" << paramName << ")" << endl;
						}
					}
				}
			}


			int numBools = s.getNumTags("REMOTEUI_PARAM_BOOL");
			if(numBools > 0){
				for (int i=0; i< numBools; i++){
					string paramName = s.getAttribute("REMOTEUI_PARAM_BOOL", "paramName", "", i);
					float val = s.getValue("REMOTEUI_PARAM_BOOL", false, i);

					map<string,RemoteUIParam>::iterator it = params.find(paramName);
					if ( it != params.end() ){	// found!
						if(params[paramName].boolValAddr != NULL){
							*params[paramName].boolValAddr = val;
							params[paramName].boolVal = val;
							if(!loadedFromXML) paramsFromXML[paramName] = params[paramName];
							if(verbose) cout << "ofxRemoteUIServer loading a BOOL '" << paramName <<"' (" << (bool) *params[paramName].boolValAddr << ") from XML" << endl;
						}else{
							cout << "ofxRemoteUIServer ERROR at loading BOOL (" << paramName << ")" << endl;
						}
					}
				}
			}

			int numStrings = s.getNumTags("REMOTEUI_PARAM_STRING");
			if(numStrings > 0){
				for (int i=0; i< numStrings; i++){
					string paramName = s.getAttribute("REMOTEUI_PARAM_STRING", "paramName", "", i);
					string val = s.getValue("REMOTEUI_PARAM_STRING", "NULL STRING", i);

					map<string,RemoteUIParam>::iterator it = params.find(paramName);
					if ( it != params.end() ){	// found!
						if(params[paramName].stringValAddr != NULL){
							params[paramName].stringVal = val;
							*params[paramName].stringValAddr = val;
							if(!loadedFromXML) paramsFromXML[paramName] = params[paramName];
							if(verbose) cout << "ofxRemoteUIServer loading a STRING '" << paramName <<"' (" << (string) *params[paramName].stringValAddr << ") from XML" << endl;
						}
						else cout << "ofxRemoteUIServer ERROR at loading STRING (" << paramName << ")" << endl;
					}
				}
			}
		}
	}
	loadedFromXML = true;
}

void ofxRemoteUIServer::restoreAllParamsToInitialXML(){
	for( map<string,RemoteUIParam>::iterator ii = params.begin(); ii != params.end(); ++ii ){
		string key = (*ii).first;
		params[key] = paramsFromXML[key];
		syncPointerToParam(key);
	}	
}

void ofxRemoteUIServer::restoreAllParamsToDefaultValues(){
	for( map<string,RemoteUIParam>::iterator ii = params.begin(); ii != params.end(); ++ii ){
		string key = (*ii).first;
		params[key] = paramsFromCode[key];
		syncPointerToParam(key);
	}
}



void ofxRemoteUIServer::setup(int port_, float updateInterval_){
	params.clear();
	updateInterval = updateInterval_;
	waitingForReply = false;
	avgTimeSinceLastReply = timeSinceLastReply = time = 0.0f;
	port = port_;
	cout << "ofxRemoteUIClient listening at port " << port << " ... " << endl;
	oscReceiver.setup(port);
}


void ofxRemoteUIServer::update(float dt){
	time += dt;
	timeSinceLastReply  += dt;
	if(readyToSend){
		if (time > updateInterval){
			time = 0.0f;
			//vector<string> changes = scanForUpdatedParamsAndSync(); //sends changed params to client
			//cout << "ofxRemoteUIServer: sent " << ofToString(changes.size()) << " updates to client" << endl;
			//sendUpdateForParamsInList(changes);
		}
	}

	while( oscReceiver.hasWaitingMessages() ){// check for waiting messages from client

		ofxOscMessage m;
		oscReceiver.getNextMessage(&m);

		if (!readyToSend){ // if not connected, connect to our friend so we can talk back
			connect(m.getRemoteIp(), port + 1);
		}

		DecodedMessage dm = decode(m);

		switch (dm.action) {

			case HELO_ACTION: //if client says hi, say hi back
				sendHELLO();
				if(verbose) cout << "ofxRemoteUIServer: " << m.getRemoteIp() << " says HELLO!"  << endl;
				break;

			case REQUEST_ACTION:{ //send all params to client
				if(verbose) cout << "ofxRemoteUIServer: " << m.getRemoteIp() << " sends REQU!"  << endl;
				vector<string>paramsList = getAllParamNamesList();
				syncAllParamsToPointers();
				sendUpdateForParamsInList(paramsList);
				sendREQU(true); //once all send, confirm to close the REQU
			}
				break;

			case SEND_PARAM_ACTION:{ //client is sending us an updated val
				if(verbose) cout << "ofxRemoteUIServer: " << m.getRemoteIp() << " sends SEND!"  << endl;
				updateParamFromDecodedMessage(m, dm);
			}
				break;

			case CIAO_ACTION:
				if(verbose) cout << "ofxRemoteUIServer: " << m.getRemoteIp() << " says CIAO!" << endl;
				sendCIAO();
				readyToSend = false;
				break;

			case TEST_ACTION: // we got a request from client, lets bounce back asap.
				sendTEST();
				//if(verbose)cout << "ofxRemoteUIServer: " << m.getRemoteIp() << " says TEST!" << endl;
				break;

			case PRESET_LIST_ACTION: //client wants us to send a list of all available presets
				presetNames = getAvailablePresets();
				if (presetNames.size() == 0){
					presetNames.push_back(OFX_REMOTEUI_NO_PRESETS);
				}
				sendPREL(presetNames);
				break;
				
			case SET_PRESET_ACTION:{ // client wants to set a preset
				//presetNames = getAvailablePresets();
				string presetName = m.getArgAsString(0);
				loadFromXML(string(OFX_REMOTEUI_PRESET_DIR) + "/" + presetName + ".xml");
				if(verbose) cout << "ofxRemoteUIServer: setting preset: " << presetName << endl;
				sendSETP("");
				}break;

			case SAVE_PRESET_ACTION:{ //client wants to save current xml as a new preset
				string presetName = m.getArgAsString(0);
				if(verbose) cout << "ofxRemoteUIServer: saving NEW preset: " << presetName << endl;
				saveToXML(string(OFX_REMOTEUI_PRESET_DIR) + "/" + presetName + ".xml");
				sendSAVP("");
				}break;

			case DELETE_PRESET_ACTION:{
				string presetName = m.getArgAsString(0);
				if(verbose) cout << "ofxRemoteUIServer: DELETE preset: " << presetName << endl;
				deletePreset(presetName);
				sendDELP("");
				}break;

			case SAVE_CURRENT_STATE_ACTION:{
				if(verbose) cout << "ofxRemoteUIServer: SAVE CURRENT PARAMS TO DEFAULT XML: " << endl;
				saveToXML(OFX_REMOTEUI_SETTINGS_FILENAME);
				sendSAVE(true);
			}break;

			case RESET_TO_XML_ACTION:{
				if(verbose) cout << "ofxRemoteUIServer: RESET TO XML: " << endl;
				restoreAllParamsToInitialXML();
				sendRESX(true);
			}break;

			case RESET_TO_DEFAULTS_ACTION:{
				if(verbose) cout << "ofxRemoteUIServer: RESET TO DEFAULTS: " << endl;
				restoreAllParamsToDefaultValues();
				sendRESD(true);
			}break;

			default: cout << "ofxRemoteUIServer::update >> ERR!" <<endl; break;
		}
	}
}

void ofxRemoteUIServer::deletePreset(string name){

	#if ( OF_VERSION_MINOR > 0 ) //TODO presets wont work outside OF
	ofDirectory dir;
	dir.open(string(OFX_REMOTEUI_PRESET_DIR) + "/" + name + ".xml");
	dir.remove(true);
	#else
		string file = string(OFX_REMOTEUI_PRESET_DIR) + "/" + name + ".xml";
		remove( file.c_str() );
	#endif
}

vector<string> ofxRemoteUIServer::getAvailablePresets(){

	vector<string> presets;

	#if ( OF_VERSION_MINOR > 0 ) //TODO presets wont work outside OF
	ofDirectory dir;
	dir.listDir(ofToDataPath(OFX_REMOTEUI_PRESET_DIR));
	vector<ofFile> files = dir.getFiles();
	for(int i = 0; i < files.size(); i++){
		string fileName = files[i].getFileName();
		string extension = files[i].getExtension();
		std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
		if (files[i].isFile() && extension == "xml"){
			//cout << "preset name: " << fileName << endl;
			string presetName = fileName.substr(0, fileName.size()-4);
			presets.push_back(presetName);
		}
	}
	#else
	DIR *dir;
	struct dirent *ent;
	if ((dir = opendir (OFX_REMOTEUI_PRESET_DIR)) != NULL) {
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
		p.r = c.r;  p.g = c.g; p.b = c.b; p.a = c.a;
	}else{
		if (colorSet){
			p.r = paramColor.r;
			p.g = paramColor.g;
			p.b = paramColor.b;
			p.a = paramColor.a;
		}
	}
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
	cout << "ofxRemoteUIServer Sharing Float Param '" << paramName << "'" << endl;
}


void ofxRemoteUIServer::shareParam(string paramName, bool* param, ofColor c, int nothingUseful ){
	RemoteUIParam p;
	p.type = REMOTEUI_PARAM_BOOL;
	p.boolValAddr = param;
	p.boolVal = *param;
	p.group = upcomingGroup;
	setColorForParam(p, c);
	addParamToDB(p, paramName);
	cout << "ofxRemoteUIServer Sharing Bool Param '" << paramName << "'" << endl;
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
	cout << "ofxRemoteUIServer Sharing Int Param '" << paramName << "'" << endl;
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
	cout << "ofxRemoteUIServer Sharing Enum Param '" << paramName << "'" << endl;
}


void ofxRemoteUIServer::shareParam(string paramName, string* param, ofColor c, int nothingUseful ){
	RemoteUIParam p;
	p.type = REMOTEUI_PARAM_STRING;
	p.stringValAddr = param;
	p.stringVal = *param;
	p.group = upcomingGroup;
	setColorForParam(p, c);
	addParamToDB(p, paramName);
	cout << "ofxRemoteUIServer Sharing String Param '" << paramName << "'" <<endl;
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
	cout << "ofxRemoteUIServer Sharing Color Param '" << paramName << "'" <<endl;
}


void ofxRemoteUIServer::connect(string ipAddress, int port){
	avgTimeSinceLastReply = timeSinceLastReply = time = 0.0f;
	waitingForReply = false;
	//params.clear();
	oscSender.setup(ipAddress, port);
	readyToSend = true;
}

