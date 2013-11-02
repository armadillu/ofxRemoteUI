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
#ifdef __APPLE__
#include "dirent.h"
#elif _WIN32 || _WIN64
#include "dirent_vs.h"
#endif
#include <sys/stat.h>
#include <time.h>

#ifdef TARGET_WIN32
//	#include <sys/time.h>
#include <winsock2.h>
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

void split(vector<string> &tokens, const string &text, char separator) {
	int start = 0, end = 0;
	while ((end = text.find(separator, start)) != string::npos) {
		tokens.push_back(text.substr(start, end - start));
		start = end + 1;
	}
	tokens.push_back(text.substr(start));
}

ofxRemoteUIServer::ofxRemoteUIServer(){

	cout << "serving at: " << getMyIP() << endl;
	readyToSend = false;
	saveToXmlOnExit = true;
	broadcastTime = OFXREMOTEUI_BORADCAST_INTERVAL + 0.05;
	timeSinceLastReply = avgTimeSinceLastReply = connectedAnimationTimer = 0;
	disconnectedAnimationTimer = 0;
	startupAnimationTimer = OFXREMOTEUI_NOTIFICATION_SCREENTIME;
	waitingForReply = false;
	colorSet = false;
	computerName = "";
	callBack = NULL;
	upcomingGroup = OFXREMOTEUI_DEFAULT_PARAM_GROUP;
	verbose_ = false;
	threadedUpdate = false;
	drawNotifications = true;
	showValuesOnScreen = false;
	loadedFromXML = false;
	//add random colors to table
	colorTableIndex = 0;
	int a = 80;
#ifdef OF_AVAILABLE
	ofSeedRandom(1979);
	ofColor prevColor = ofColor::fromHsb(0, 255, 255, 30);
	for(int i = 0; i < 30; i++){
		ofColor c = prevColor;
		c.setHue(  ((int) (prevColor.getHue() + 15 )) % 255 );
		//c.setSaturation(prevColor.getSaturation() + ofRandom(-0.1,0.1) );
		colorTables.push_back( c );
		prevColor = c;
	}
	//shuffle
	std::random_shuffle ( colorTables.begin(), colorTables.end() );
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

	computerIP = getMyIP();
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
	s.addTag(OFXREMOTEUI_XML_TAG);
	s.pushTag(OFXREMOTEUI_XML_TAG);

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
				if(verbose_) cout << "ofxRemoteUIServer saving '" << key << "' (" <<  *t.floatValAddr <<") to XML" << endl;
				s.setValue("REMOTEUI_PARAM_FLOAT", (double)*t.floatValAddr, numFloats);
				s.setAttribute("REMOTEUI_PARAM_FLOAT", "paramName", key, numFloats);
				numFloats++;
				break;
			case REMOTEUI_PARAM_INT:
				if(verbose_) cout << "ofxRemoteUIServer saving '" << key << "' (" <<  *t.intValAddr <<") to XML" << endl;
				s.setValue("REMOTEUI_PARAM_INT", (int)*t.intValAddr, numInts);
				s.setAttribute("REMOTEUI_PARAM_INT", "paramName", key, numInts);
				numInts++;
				break;
			case REMOTEUI_PARAM_COLOR:
				s.addTag("REMOTEUI_PARAM_COLOR");
				s.setAttribute("REMOTEUI_PARAM_COLOR", "paramName", key, numColors);
				s.pushTag("REMOTEUI_PARAM_COLOR", numColors);
				if(verbose_) cout << "ofxRemoteUIServer saving '" << key << "' (" << (int)*t.redValAddr << " " << (int)*(t.redValAddr+1) << " " << (int)*(t.redValAddr+2) << " " << (int)*(t.redValAddr+3) << ") to XML" << endl;
				s.setValue("R", (int)*t.redValAddr);
				s.setValue("G", (int)*(t.redValAddr+1));
				s.setValue("B", (int)*(t.redValAddr+2));
				s.setValue("A", (int)*(t.redValAddr+3));
				s.popTag();
				numColors++;
				break;
			case REMOTEUI_PARAM_ENUM:
				if(verbose_) cout << "ofxRemoteUIServer saving '" << key << "' (" <<  *t.intValAddr <<") to XML" << endl;
				s.setValue("REMOTEUI_PARAM_ENUM", (int)*t.intValAddr, numEnums);
				s.setAttribute("REMOTEUI_PARAM_ENUM", "paramName", key, numEnums);
				numEnums++;
				break;
			case REMOTEUI_PARAM_BOOL:
				if(verbose_) cout << "ofxRemoteUIServer saving '" << key << "' (" <<  *t.boolValAddr <<") to XML" << endl;
				s.setValue("REMOTEUI_PARAM_BOOL", (bool)*t.boolValAddr, numBools);
				s.setAttribute("REMOTEUI_PARAM_BOOL", "paramName", key, numBools);
				numBools++;
				break;
			case REMOTEUI_PARAM_STRING:
				if(verbose_) cout << "ofxRemoteUIServer saving '" << key << "' (" <<  *t.stringValAddr <<") to XML" << endl;
				s.setValue("REMOTEUI_PARAM_STRING", (string)*t.stringValAddr, numStrings);
				s.setAttribute("REMOTEUI_PARAM_STRING", "paramName", key, numStrings);
				numStrings++;
				break;

			default:
				break;
		}
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
				string val = s.getValue("REMOTEUI_PARAM_STRING", "NULL STRING", i);

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
			paramsNotInXML.push_back(paramName);
		}
	}
	loadedFromXML = true;
	return paramsNotInXML;
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

void ofxRemoteUIServer::_keyPressed(ofKeyEventArgs &e){
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

	if(!threadedUpdate){
		updateServer(dt);
	}
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

	if(showValuesOnScreen){
		int padding = 30;
		int x = padding;
		int y = padding * 2;
		int colw = 320;
		int valOffset = colw * 0.6;
		int spacing = 24;

		ofSetColor(11, 245);
		ofRect(0,0, ofGetWidth(), ofGetHeight());
		ofSetColor(44, 245);
		ofRect(0,0, ofGetWidth(), padding + spacing * 0.5);

		ofSetColor(255);
		ofDrawBitmapString("ofxRemoteUIServer params list : press TAB to hide", padding,  padding - 3);

		for(int i = 0; i < orderedKeys.size(); i++){
			string key = orderedKeys[i];
			RemoteUIParam p = params[key];
			int chars = key.size();
			int charw = 9;
			int stringw = chars*charw;
			if (chars*charw > valOffset){
				key = key.substr(0, (valOffset) / charw );
			}
			ofSetColor(200);
			ofDrawBitmapString(key, x,y);
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
					p.stringVal;
					break;
				case REMOTEUI_PARAM_COLOR:
					ofSetColor(p.redVal, p.greenVal, p.blueVal, p.alphaVal);
					ofRect(x + valOffset, y - spacing * 0.6, 64, spacing * 0.85);
					break;
				default: printf("weird RemoteUIParam at isEqualTo()!\n"); break;
			}
			ofSetColor(32);
			ofLine(x, y + spacing * 0.33, x + colw * 0.8, y + spacing * 0.33);
			y += spacing;
			if (y > ofGetHeight() - padding){
				x += colw;
				y = 2 * padding;
			}
		}
	}
	string msg = "";
	float a = 0.0f;
	if(startupAnimationTimer > 0){
		a = ofClamp(startupAnimationTimer,0,1);
		msg = "ofxRemoteUIServer started at " + computerIP  + ":" + ofToString(port) + " (" + computerName + ")";
	}
	if(savedAnimationTimer > 0){
		a = ofClamp(savedAnimationTimer,0,1);
		msg = "ofxRemoteUIServer: Client Saved config to '" + saveAnimationfileName + "'";
	}
	if(connectedAnimationTimer > 0){
		a = ofClamp(connectedAnimationTimer,0,1);
		msg = "ofxRemoteUIServer: Client Connected!";
	}
	if(disconnectedAnimationTimer > 0){
		a = ofClamp(disconnectedAnimationTimer,0,1);
		msg = "ofxRemoteUIServer: Client Disconnected!";
	}
	if (a > 0){
		ofDrawBitmapStringHighlight(msg, x, y,
									ofColor(0, 255 * ofClamp(a,0,1)),
									ofColor(255,0,0, 255 * ofClamp(a,0,1)) );
	}
	#endif
}


void ofxRemoteUIServer::updateServer(float dt){

	//timers //TODO fix this mess
	startupAnimationTimer -= dt;
	savedAnimationTimer -= dt;
	connectedAnimationTimer -= dt;
	disconnectedAnimationTimer -= dt;
	timeCounter += dt;
	broadcastTime += dt;
	timeSinceLastReply  += dt;

	if(readyToSend){
		if (timeCounter > updateInterval){
			timeCounter = 0.0f;
			//vector<string> changes = scanForUpdatedParamsAndSync(); //sends changed params to client
			//cout << "ofxRemoteUIServer: sent " << ofToString(changes.size()) << " updates to client" << endl;
			//sendUpdateForParamsInList(changes);
		}
	}

	//let everyone know I exist and which is my port, every now and then
	if(broadcastTime > OFXREMOTEUI_BORADCAST_INTERVAL){
		if(doBroadcast){
			broadcastTime = 0.0f;
			if (computerName.size() == 0){
#ifdef TARGET_OSX
				computerName = ofSystem("hostname -s ");
				computerName = computerName.substr(0, computerName.size()-2); //mmm this is weird, 10.8
#endif
#ifdef TARGET_WIN32
				GetHostName(computerName);
#endif
			}
			ofxOscMessage m;
			m.addIntArg(port);
			m.addStringArg(computerName);
			broadcastSender.sendMessage(m);
		}
	}

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
				connectedAnimationTimer = OFXREMOTEUI_NOTIFICATION_SCREENTIME;
				break;

			case REQUEST_ACTION:{ //send all params to client
				if(verbose_) cout << "ofxRemoteUIServer: " << m.getRemoteIp() << " sends REQU!"  << endl;
				vector<string>paramsList = getAllParamNamesList();
				syncAllParamsToPointers();
				sendUpdateForParamsInList(paramsList);
				sendREQU(true); //once all send, confirm to close the REQU
				connectedAnimationTimer = OFXREMOTEUI_NOTIFICATION_SCREENTIME;
			}
				break;

			case SEND_PARAM_ACTION:{ //client is sending us an updated val
				if(verbose_) cout << "ofxRemoteUIServer: " << m.getRemoteIp() << " sends SEND!"  << endl;
				updateParamFromDecodedMessage(m, dm);
				if(callBack != NULL){
					cbArg.action = CLIENT_UPDATED_PARAM;
					cbArg.paramName = dm.paramName;
					cbArg.param = params[dm.paramName];  //copy the updated param to the callbakc arg
					callBack(cbArg);
				}
			}
				break;

			case CIAO_ACTION:{
				if(verbose_) cout << "ofxRemoteUIServer: " << m.getRemoteIp() << " says CIAO!" << endl;
				sendCIAO();
				disconnectedAnimationTimer = OFXREMOTEUI_NOTIFICATION_SCREENTIME;
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
				//presetNames = getAvailablePresets();
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
			}break;

			case SAVE_PRESET_ACTION:{ //client wants to save current xml as a new preset
				string presetName = m.getArgAsString(0);
				if(verbose_) cout << "ofxRemoteUIServer: saving NEW preset: " << presetName << endl;
				saveToXML(string(OFXREMOTEUI_PRESET_DIR) + "/" + presetName + ".xml");
				sendSAVP(presetName);
				savedAnimationTimer = OFXREMOTEUI_NOTIFICATION_SCREENTIME;
				saveAnimationfileName = string(OFXREMOTEUI_PRESET_DIR) + "/" + presetName + ".xml";
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
				if(callBack != NULL){
					cbArg.action = CLIENT_DELETED_PRESET;
					cbArg.msg = presetName;
					callBack(cbArg);
				}
			}break;

			case SAVE_CURRENT_STATE_ACTION:{
				if(verbose_) cout << "ofxRemoteUIServer: SAVE CURRENT PARAMS TO DEFAULT XML: " << endl;
				saveToXML(OFXREMOTEUI_SETTINGS_FILENAME);
				savedAnimationTimer = OFXREMOTEUI_NOTIFICATION_SCREENTIME;
				saveAnimationfileName = OFXREMOTEUI_SETTINGS_FILENAME;
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
				if(callBack != NULL){
					cbArg.action = CLIENT_DID_RESET_TO_XML;
					callBack(cbArg);
				}
			}break;

			case RESET_TO_DEFAULTS_ACTION:{
				if(verbose_) cout << "ofxRemoteUIServer: RESET TO DEFAULTS: " << endl;
				restoreAllParamsToDefaultValues();
				sendRESD(true);
				if(callBack != NULL){
					cbArg.action = CLIENT_DID_RESET_TO_DEFAULTS;
					callBack(cbArg);
				}
			}break;

			default: cout << "ofxRemoteUIServer::update >> ERR!" <<endl; break;
		}
	}
}

void ofxRemoteUIServer::deletePreset(string name){

#ifdef OF_AVAILABLE
	ofDirectory dir;
	dir.open(string(OFXREMOTEUI_PRESET_DIR) + "/" + name + ".xml");
	dir.remove(true);
#else
	string file = string(OFXREMOTEUI_PRESET_DIR) + "/" + name + ".xml";
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
			//cout << "preset name: " << fileName << endl;
			string presetName = fileName.substr(0, fileName.size()-4);
			presets.push_back(presetName);
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
	avgTimeSinceLastReply = timeSinceLastReply = timeCounter = 0.0f;
	waitingForReply = false;
	//params.clear();
	oscSender.setup(ipAddress, port);
	readyToSend = true;
}

