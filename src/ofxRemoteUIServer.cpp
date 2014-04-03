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

void ofxRemoteUIServer::setDrawsNotificationsAutomaticallly(bool draw){
	drawNotifications = draw;
}

ofxRemoteUIServer::ofxRemoteUIServer(){

	//cout << "serving at: " << getMyIP() << endl;
	readyToSend = false;
	saveToXmlOnExit = true;
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
	clearXmlOnSaving = true;
	//add random colors to table
	colorTableIndex = 0;
	broadcastCount = 0;
	newColorInGroupCounter = 0;
	int a = 80;
#ifdef OF_AVAILABLE
	selectedItem = 0;
	ofSeedRandom(1979);
	ofColor prevColor = ofColor::fromHsb(0, 255, 200, BG_COLOR_ALPHA);
	for(int i = 0; i < 30; i++){
		ofColor c = prevColor;
		c.setHue(  ((int) (prevColor.getHue() + 25) ) % 255 );
		//c.setSaturation(prevColor.getSaturation() + ofRandom(-0.1,0.1) );
		colorTables.push_back( c );
		prevColor = c;
	}
	//shuffle
	//std::random_shuffle ( colorTables.begin(), colorTables.end() );
	ofSeedRandom();
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
	d.create(true);
#else
#if defined(_WIN32)
	_mkdir(OFXREMOTEUI_PRESET_DIR);
#else
	mkdir(OFXREMOTEUI_PRESET_DIR, 0777);
#endif
#endif

}

ofxRemoteUIServer::~ofxRemoteUIServer(){
	cout << "~ofxRemoteUIServer()" << endl;
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
		cout << "ofxRemoteUIServer closing; waiting for update thread to end..." << endl;
		waitForThread();
#endif
	}
}


void ofxRemoteUIServer::setParamGroup(string g){
	upcomingGroup = g;
	newColorInGroupCounter = 0;
	setNewParamColor(2);
	setNewParamColorVariation();
	addSpacer(g);
}

void ofxRemoteUIServer::unsetParamColor(){
	colorSet = false;
}


void ofxRemoteUIServer::setNewParamColorVariation(){
	paramColorCurrentVariation = paramColor;
	int offset = newColorInGroupCounter%2;
	paramColorCurrentVariation.a = BG_COLOR_ALPHA + offset * BG_COLOR_ALPHA * 0.75;
	newColorInGroupCounter++;
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

void ofxRemoteUIServer::saveParamToXmlSettings(RemoteUIParam t, string key, ofxXmlSettings & s, XmlCounter & c){

	switch (t.type) {
		case REMOTEUI_PARAM_FLOAT:
			if(verbose_) cout << "ofxRemoteUIServer saving '" << key << "' (" <<  *t.floatValAddr <<") to XML" << endl;
			s.setValue("REMOTEUI_PARAM_FLOAT", (double)*t.floatValAddr, c.numFloats);
			s.setAttribute("REMOTEUI_PARAM_FLOAT", "paramName", key, c.numFloats);
			c.numFloats++;
			break;
		case REMOTEUI_PARAM_INT:
			if(verbose_) cout << "ofxRemoteUIServer saving '" << key << "' (" <<  *t.intValAddr <<") to XML" << endl;
			s.setValue("REMOTEUI_PARAM_INT", (int)*t.intValAddr, c.numInts);
			s.setAttribute("REMOTEUI_PARAM_INT", "paramName", key, c.numInts);
			c.numInts++;
			break;
		case REMOTEUI_PARAM_COLOR:
			s.addTag("REMOTEUI_PARAM_COLOR");
			s.setAttribute("REMOTEUI_PARAM_COLOR", "paramName", key, c.numColors);
			s.pushTag("REMOTEUI_PARAM_COLOR", c.numColors);
			if(verbose_) cout << "ofxRemoteUIServer saving '" << key << "' (" << (int)*t.redValAddr << " " << (int)*(t.redValAddr+1) << " " << (int)*(t.redValAddr+2) << " " << (int)*(t.redValAddr+3) << ") to XML" << endl;
			s.setValue("R", (int)*t.redValAddr);
			s.setValue("G", (int)*(t.redValAddr+1));
			s.setValue("B", (int)*(t.redValAddr+2));
			s.setValue("A", (int)*(t.redValAddr+3));
			s.popTag();
			c.numColors++;
			break;
		case REMOTEUI_PARAM_ENUM:
			if(verbose_) cout << "ofxRemoteUIServer saving '" << key << "' (" <<  *t.intValAddr <<") to XML" << endl;
			s.setValue("REMOTEUI_PARAM_ENUM", (int)*t.intValAddr, c.numEnums);
			s.setAttribute("REMOTEUI_PARAM_ENUM", "paramName", key, c.numEnums);
			c.numEnums++;
			break;
		case REMOTEUI_PARAM_BOOL:
			if(verbose_) cout << "ofxRemoteUIServer saving '" << key << "' (" <<  *t.boolValAddr <<") to XML" << endl;
			s.setValue("REMOTEUI_PARAM_BOOL", (bool)*t.boolValAddr, c.numBools);
			s.setAttribute("REMOTEUI_PARAM_BOOL", "paramName", key, c.numBools);
			c.numBools++;
			break;
		case REMOTEUI_PARAM_STRING:
			if(verbose_) cout << "ofxRemoteUIServer saving '" << key << "' (" <<  *t.stringValAddr <<") to XML" << endl;
			s.setValue("REMOTEUI_PARAM_STRING", (string)*t.stringValAddr, c.numStrings);
			s.setAttribute("REMOTEUI_PARAM_STRING", "paramName", key, c.numStrings);
			c.numStrings++;
			break;

		case REMOTEUI_PARAM_SPACER:
			break;

		default:
			break;
	}
}

void ofxRemoteUIServer::saveGroupToXML(string fileName, string groupName){

	cout << "ofxRemoteUIServer: saving group to xml '" << fileName << "'" << endl;
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
	d.create(true);
	#else
		#if defined(_WIN32)
		_mkdir(path.c_str());
		#else
		mkdir(path.c_str(), 0777);
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

	cout << "ofxRemoteUIServer: saving to xml '" << fileName << "'" << endl;
	ofxXmlSettings s;
	s.loadFile(fileName);
	if(clearXmlOnSaving){
		s.clear();
	}
	s.addTag(OFXREMOTEUI_XML_TAG);
	s.pushTag(OFXREMOTEUI_XML_TAG);

	XmlCounter counters;

	for( map<string,RemoteUIParam>::iterator ii = params.begin(); ii != params.end(); ++ii ){
		string key = (*ii).first;
		RemoteUIParam t = params[key];
		saveParamToXmlSettings(t, key, s, counters);
	}

	if(!portIsSet){
		s.popTag();
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

			int numFloats = s.getNumTags("REMOTEUI_PARAM_FLOAT");
			for (int i=0; i< numFloats; i++){
				string paramName = s.getAttribute("REMOTEUI_PARAM_FLOAT", "paramName", "", i);
				float val = s.getValue("REMOTEUI_PARAM_FLOAT", 0.0, i);
				map<string,RemoteUIParam>::iterator it = params.find(paramName);
				if ( it != params.end() ){	// found!
					loadedParams.push_back(paramName);
					if(params[paramName].floatValAddr != NULL){
						*params[paramName].floatValAddr = val;
						params[paramName].floatVal = val;
						*params[paramName].floatValAddr = ofClamp(*params[paramName].floatValAddr, params[paramName].minFloat, params[paramName].maxFloat);
						if(!loadedFromXML) paramsFromXML[paramName] = params[paramName];
						if(verbose_) cout << "ofxRemoteUIServer loading a FLOAT '" << paramName <<"' (" << ofToString( *params[paramName].floatValAddr, 3) << ") from XML" << endl;
					}else{
						cout << "ofxRemoteUIServer ERROR at loading FLOAT (" << paramName << ")" << endl;
					}
				}
			}

			int numInts = s.getNumTags("REMOTEUI_PARAM_INT");
			for (int i=0; i< numInts; i++){
				string paramName = s.getAttribute("REMOTEUI_PARAM_INT", "paramName", "", i);
				float val = s.getValue("REMOTEUI_PARAM_INT", 0, i);
				map<string,RemoteUIParam>::iterator it = params.find(paramName);
				if ( it != params.end() ){	// found!
					loadedParams.push_back(paramName);
					if(params[paramName].intValAddr != NULL){
						*params[paramName].intValAddr = val;
						params[paramName].intVal = val;
						*params[paramName].intValAddr = ofClamp(*params[paramName].intValAddr, params[paramName].minInt, params[paramName].maxInt);
						if(!loadedFromXML) paramsFromXML[paramName] = params[paramName];
						if(verbose_) cout << "ofxRemoteUIServer loading an INT '" << paramName <<"' (" << (int) *params[paramName].intValAddr << ") from XML" << endl;
					}else{
						cout << "ofxRemoteUIServer ERROR at loading INT (" << paramName << ")" << endl;
					}
				}
			}

			int numColors = s.getNumTags("REMOTEUI_PARAM_COLOR");
			for (int i=0; i< numColors; i++){
				string paramName = s.getAttribute("REMOTEUI_PARAM_COLOR", "paramName", "", i);
				s.pushTag("REMOTEUI_PARAM_COLOR", i);
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
						if(verbose_) cout << "ofxRemoteUIServer loading a COLOR '" << paramName <<"' (" << (int)*params[paramName].redValAddr << " " << (int)*(params[paramName].redValAddr+1) << " " << (int)*(params[paramName].redValAddr+2) << " " << (int)*(params[paramName].redValAddr+3)  << ") from XML" << endl;
					}else{
						cout << "ofxRemoteUIServer ERROR at loading COLOR (" << paramName << ")" << endl;
					}
				}
				s.popTag();
			}

			int numEnums = s.getNumTags("REMOTEUI_PARAM_ENUM");
			for (int i=0; i< numEnums; i++){
				string paramName = s.getAttribute("REMOTEUI_PARAM_ENUM", "paramName", "", i);
				float val = s.getValue("REMOTEUI_PARAM_ENUM", 0, i);
				map<string,RemoteUIParam>::iterator it = params.find(paramName);
				if ( it != params.end() ){	// found!
					loadedParams.push_back(paramName);
					if(params[paramName].intValAddr != NULL){
						*params[paramName].intValAddr = val;
						params[paramName].intVal = val;
						*params[paramName].intValAddr = ofClamp(*params[paramName].intValAddr, params[paramName].minInt, params[paramName].maxInt);
						if(!loadedFromXML) paramsFromXML[paramName] = params[paramName];
						if(verbose_) cout << "ofxRemoteUIServer loading an ENUM '" << paramName <<"' (" << (int) *params[paramName].intValAddr << ") from XML" << endl;
					}else{
						cout << "ofxRemoteUIServer ERROR at loading ENUM (" << paramName << ")" << endl;
					}
				}
			}


			int numBools = s.getNumTags("REMOTEUI_PARAM_BOOL");
			for (int i=0; i< numBools; i++){
				string paramName = s.getAttribute("REMOTEUI_PARAM_BOOL", "paramName", "", i);
				float val = s.getValue("REMOTEUI_PARAM_BOOL", false, i);

				map<string,RemoteUIParam>::iterator it = params.find(paramName);
				if ( it != params.end() ){	// found!
					loadedParams.push_back(paramName);
					if(params[paramName].boolValAddr != NULL){
						*params[paramName].boolValAddr = val;
						params[paramName].boolVal = val;
						if(!loadedFromXML) paramsFromXML[paramName] = params[paramName];
						if(verbose_) cout << "ofxRemoteUIServer loading a BOOL '" << paramName <<"' (" << (bool) *params[paramName].boolValAddr << ") from XML" << endl;
					}else{
						cout << "ofxRemoteUIServer ERROR at loading BOOL (" << paramName << ")" << endl;
					}
				}
			}

			int numStrings = s.getNumTags("REMOTEUI_PARAM_STRING");
			for (int i=0; i< numStrings; i++){
				string paramName = s.getAttribute("REMOTEUI_PARAM_STRING", "paramName", "", i);
				string val = s.getValue("REMOTEUI_PARAM_STRING", "", i);

				map<string,RemoteUIParam>::iterator it = params.find(paramName);
				if ( it != params.end() ){	// found!
					loadedParams.push_back(paramName);
					if(params[paramName].stringValAddr != NULL){
						params[paramName].stringVal = val;
						*params[paramName].stringValAddr = val;
						if(!loadedFromXML) paramsFromXML[paramName] = params[paramName];
						if(verbose_) cout << "ofxRemoteUIServer loading a STRING '" << paramName <<"' (" << (string) *params[paramName].stringValAddr << ") from XML" << endl;
					}
					else cout << "ofxRemoteUIServer ERROR at loading STRING (" << paramName << ")" << endl;
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

void ofxRemoteUIServer::setup(int port_, float updateInterval_){

	//setup the broadcasting
	computerIP = getMyIP(userSuppliedNetInterface);
	if (computerIP != "NOT FOUND"){
		doBroadcast = true;
		vector<string>comps;
		split(comps, computerIP, '.');
		string multicastIP = comps[0] + "." + comps[1] + "." + comps[2] + "." + "255";
		broadcastSender.setup( multicastIP, OFXREMOTEUI_BROADCAST_PORT ); //multicast @
		cout << "ofxRemoteUIServer: letting everyone know that I am at " << multicastIP << ":" << OFXREMOTEUI_BROADCAST_PORT << endl;
	}else{
		doBroadcast = false;
	}

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
	cout << "ofxRemoteUIServer listening at port " << port << " ... " << endl;
	oscReceiver.setup(port);
#ifdef OF_AVAILABLE
	ofAddListener(ofEvents().exit, this, &ofxRemoteUIServer::_appExited);
	ofAddListener(ofEvents().keyPressed, this, &ofxRemoteUIServer::_keyPressed);
	ofAddListener(ofEvents().update, this, &ofxRemoteUIServer::_update);
	if(drawNotifications){
		ofAddListener(ofEvents().draw, this, &ofxRemoteUIServer::_draw);
	}
#endif
}

#ifdef OF_AVAILABLE
void ofxRemoteUIServer::_appExited(ofEventArgs &e){
	OFX_REMOTEUI_SERVER_CLOSE();		//stop the server
	if(saveToXmlOnExit){
		OFX_REMOTEUI_SERVER_SAVE_TO_XML();	//save values to XML
	}
}

void ofxRemoteUIServer::_draw(ofEventArgs &e){
	ofSetupScreen(); //mmm this is a bit scary //TODO!
	draw( 20, ofGetHeight() - 20);
}

void ofxRemoteUIServer::_update(ofEventArgs &e){
	update(ofGetLastFrameTime());
}

void ofxRemoteUIServer::_keyPressed(ofKeyEventArgs &e){

	if (showValuesOnScreen){
		switch(e.key){ //you can save current config from tab screen by pressing s
			case 's':
				saveToXML(OFXREMOTEUI_SETTINGS_FILENAME);
				onScreenNotifications.addNotification("SAVED CONFIG to default XML");
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
				}break;

		}
	}
	if(e.key == '\t'){
		showValuesOnScreen = !showValuesOnScreen;
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
	updatedThisFrame = true; //this only makes sense when
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
	if(verbose_) cout << "ofxRemoteUIServer threadedFunction() ending" << endl;
}
#endif


void ofxRemoteUIServer::draw(int x, int y){

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
		int spacing = 24;

		ofSetColor(11, 245);
		ofRect(0,0, ofGetWidth(), ofGetHeight());
		ofSetColor(44, 245);
		ofRect(0,0, ofGetWidth(), padding + spacing );

		ofSetColor(255);
		ofDrawBitmapString("ofxRemoteUIServer params list : press 'TAB' to hide.\nPress 's' to save current config. Use Arrow Keys to edit values.", padding,  padding - 3);

		for(int i = 0; i < orderedKeys.size(); i++){
			string key = orderedKeys[i];
			RemoteUIParam p = params[key];
			int chars = key.size();
			int charw = 9;
			int stringw = chars*charw;
			if (chars*charw > valOffset){
				key = key.substr(0, (valOffset) / charw );
			}
			if (selectedItem != i) ofSetColor(p.r, p.g, p.b);
			else ofSetColor(255,0,0);

			if(p.type != REMOTEUI_PARAM_SPACER){
				string sel = (selectedItem == i) ? ">" : " ";
				ofDrawBitmapString(sel + key, x, y);
			}else{
				ofDrawBitmapString(p.stringVal, x,y);
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

				default: printf("weird RemoteUIParam at isEqualTo()!\n"); break;
			}
			ofSetColor(32);
			ofLine(x, y + spacing * 0.33, x + colw * 0.8, y + spacing * 0.33);
			y += spacing;
			if (y > ofGetHeight() - padding){
				x += colw;
				y = initialY;
			}
		}
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
			//cout << "ofxRemoteUIServer: sent " << ofToString(changes.size()) << " updates to client" << endl;
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
				if(verbose_) cout << "ofxRemoteUIServer: " << m.getRemoteIp() << " says HELLO!"  << endl;
				#ifdef OF_AVAILABLE
				onScreenNotifications.addNotification("CONNECTED (" + cbArg.host +  ")!");
				#endif
				break;

			case REQUEST_ACTION:{ //send all params to client
				if(verbose_) cout << "ofxRemoteUIServer: " << m.getRemoteIp() << " sends REQU!"  << endl;
				pushParamsToClient();
				}break;

			case SEND_PARAM_ACTION:{ //client is sending us an updated val
				if(verbose_) cout << "ofxRemoteUIServer: " << m.getRemoteIp() << " sends SEND!"  << endl;
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
				if(verbose_) cout << "ofxRemoteUIServer: " << m.getRemoteIp() << " says CIAO!" << endl;
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
				//if(verbose)cout << "ofxRemoteUIServer: " << m.getRemoteIp() << " says TEST!" << endl;
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
				if(verbose_) cout << "ofxRemoteUIServer: setting preset: " << presetName << endl;
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
				if(verbose_) cout << "ofxRemoteUIServer: saving NEW preset: " << presetName << endl;
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
				if(verbose_) cout << "ofxRemoteUIServer: DELETE preset: " << presetName << endl;
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
				if(verbose_) cout << "ofxRemoteUIServer: SAVE CURRENT PARAMS TO DEFAULT XML: " << endl;
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
				if(verbose_) cout << "ofxRemoteUIServer: RESET TO XML: " << endl;
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
				if(verbose_) cout << "ofxRemoteUIServer: RESET TO DEFAULTS: " << endl;
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
				if(verbose_) cout << "ofxRemoteUIServer: setting preset group: " << groupName << "/" <<presetName << endl;
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
				if(verbose_) cout << "ofxRemoteUIServer: saving NEW preset: " << presetName << endl;
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
				if(verbose_) cout << "ofxRemoteUIServer: DELETE preset: " << presetName << endl;
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
			default: cout << "ofxRemoteUIServer::update >> ERR!" <<endl; break;
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
	addParamToDB(p, title + " - " + ofToString((int)ofRandom(10000)));
	cout << "ofxRemoteUIServer Adding Spacer '" << title << "'" << endl;
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
			cout << "exception" << endl;
		}
	}
}

