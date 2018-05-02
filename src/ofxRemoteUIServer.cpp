//
//  ofxRemoteUI.cpp
//  emptyExample
//
//  Created by Oriol Ferrer Mesià on 09/01/13.
//
//


#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <WinBase.h>
#endif

#include <iostream>
#include <algorithm>
#include <string>
#include <string.h>
#ifdef __APPLE__
	#include "dirent.h"
	#include <mach-o/dyld.h>	/* _NSGetExecutablePath */
	#include <netinet/in.h>
	#include <arpa/inet.h>
#endif

#include <sys/stat.h>
#include <time.h>

#include "ofxRemoteUIServer.h"

#ifdef TARGET_IOS
#include "ofxiOS.h"
#endif

#ifdef OF_AVAILABLE
#ifndef TARGET_OF_IOS
	#include "ofAppNoWindow.h"
#endif

#endif


using namespace std;

ofxRemoteUIServer* ofxRemoteUIServer::singleton = NULL;

ofxRemoteUIServer* ofxRemoteUIServer::instance(){
	if (!singleton){   // Only allow one instance of class to be generated.
		singleton = new ofxRemoteUIServer();
	}
	return singleton;
}


ofxRemoteUIServer::ofxRemoteUIServer(){

	enabled = true;
	showUIduringEdits = false;
	autoDraw = true;
	readyToSend = false;
	saveToXmlOnExit = true;
	autoBackups = false; //off by default
	broadcastTime = OFXREMOTEUI_BORADCAST_INTERVAL + 0.05;
	timeSinceLastReply = avgTimeSinceLastReply = 0;
	waitingForReply = false;
	colorSet = false;
	computerName = binaryName = "";
	directoryPrefix = "";
	#ifdef TARGET_IOS
	directoryPrefix = ofxiOSGetDocumentsDirectory();
	#endif
	callBack = NULL;
	upcomingGroup = OFXREMOTEUI_DEFAULT_PARAM_GROUP;
	verbose_ = false;
	threadedUpdate = false;
	drawNotifications = true;
	showUI = false;
	loadedFromXML = false;
	clearXmlOnSaving = false;
	//add random colors to table
	colorTableIndex = 0;
	broadcastCount = 0;
	newColorInGroupCounter = 1;
	showInterfaceKey = '\t';
#ifdef OF_AVAILABLE
	uiScale = 1;
	customScreenWidth = customScreenHeight = -1;

	xOffset = xOffsetTarget = 0.0f;
	selectedColorComp = 0;
	uiColumnWidth = 320;
	uiAlpha = 1.0f;
	selectedPreset = selectedGroupPreset = 0;
	selectedItem = -1;
	ofSeedRandom(1979);
	headlessMode = false;

	int numHues = 9;
	for(int i = 0; i < numHues; i++){
		float hue = fmod( i * ( 255.0f / numHues), 255.0f );
		ofColor c = ofColor::fromHsb(hue, 255.0f, 230.0f, BG_COLOR_ALPHA);
		colorTables.push_back( c );
	}
//	colorTables.push_back(ofColor(254,19,41,BG_COLOR_ALPHA) );
//	colorTables.push_back(ofColor(255,82,0,BG_COLOR_ALPHA) );
//	colorTables.push_back(ofColor(255,234,0,BG_COLOR_ALPHA) );
//	colorTables.push_back(ofColor(86,203,0,BG_COLOR_ALPHA) );
//	colorTables.push_back(ofColor(0,136,58,BG_COLOR_ALPHA) );
//	colorTables.push_back(ofColor(23,234,237,BG_COLOR_ALPHA) );
//	colorTables.push_back(ofColor(0,150,255,BG_COLOR_ALPHA) );
//	colorTables.push_back(ofColor(11,51,255,BG_COLOR_ALPHA) );
//	colorTables.push_back(ofColor(139,2,190,BG_COLOR_ALPHA) );
//	colorTables.push_back(ofColor(255,20,214,BG_COLOR_ALPHA) );

	uiLines.setMode(OF_PRIMITIVE_LINES);
	ofAddListener(eventShowParamUpdateNotification, this, &ofxRemoteUIServer::onShowParamUpdateNotification);
#else
	int a = 44;
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

}

ofxRemoteUIServer::~ofxRemoteUIServer(){
	RLOG_NOTICE << "~ofxRemoteUIServer()" ;
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
		RLOG_NOTICE << "closing; waiting for update thread to end..." ;
		waitForThread();
#endif
	}
}


void ofxRemoteUIServer::setParamGroup(string g){
	g = cleanCharsForFileSystem(g);
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
		colorSet = true;
		paramColor = colorTables[colorTableIndex];
		paramColorCurrentVariation = paramColor;
		int offset = newColorInGroupCounter%2;
		paramColorCurrentVariation.a = BG_COLOR_ALPHA + offset * BG_COLOR_ALPHA * 0.75;
		colorTableIndex++;
		if(colorTableIndex>= colorTables.size()){
			colorTableIndex = 0;
		}
	}
}


void ofxRemoteUIServer::removeParamFromDB(const string & paramName, bool permanently){

	dataMutex.lock();
	unordered_map<string, RemoteUIParam>::iterator it = params.find(paramName);

	if (it != params.end()){

		if(verbose_) RLOG_WARNING << "removing Param '" << paramName << "' from DB!" ;

        if(!permanently){
            //keep it in the removed struct
            params_removed[paramName] = it->second;
            orderedKeys_removed[orderedKeys_removed.size()] = paramName;
        }

		params.erase(it);

		it = paramsFromCode.find(paramName);
		if (it != paramsFromCode.end()){
			paramsFromCode.erase(paramsFromCode.find(paramName));
		}

		it = paramsFromXML.find(paramName);
		if (it != paramsFromXML.end()){
			paramsFromXML.erase(it);
		}


		//re-create orderedKeys
		vector<string> myOrderedKeys;
		map<int, string>::iterator iterator;

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
		RLOG_ERROR << "removeParamFromDB >> trying to delete an unexistant param (" << paramName << ")" ;
	}
	dataMutex.unlock();
}


void ofxRemoteUIServer::setDirectoryPrefix(const string & _directoryPrefix){
	directoryPrefix = _directoryPrefix;
	RLOG_NOTICE << "directoryPrefix set to '" << directoryPrefix << "'";
	#ifdef OF_AVAILABLE
	ofDirectory d;
	d.open(getFinalPath(OFXREMOTEUI_PRESET_DIR));
	if(!d.exists()){
		d.create(true);
	}
	#endif
}


void ofxRemoteUIServer::saveParamToXmlSettings(const RemoteUIParam& t, string key, ofxXmlSettings & s, XmlCounter & c){

	switch (t.type) {

		case REMOTEUI_PARAM_FLOAT:{
			float v = t.floatValAddr ? *t.floatValAddr : t.floatVal;
			if(verbose_) RLOG_NOTICE << "saving '" << key << "' (" << v <<") to XML" ;
			s.setValue(OFXREMOTEUI_FLOAT_PARAM_XML_TAG, (double)v, c.numFloats);
			s.setAttribute(OFXREMOTEUI_FLOAT_PARAM_XML_TAG, OFXREMOTEUI_PARAM_NAME_XML_KEY, key, c.numFloats);
			c.numFloats++;
			}break;
		case REMOTEUI_PARAM_INT:{
			int v = t.intValAddr ? *t.intValAddr : t.intVal;
			if(verbose_) RLOG_NOTICE << "saving '" << key << "' (" << v <<") to XML" ;
			s.setValue(OFXREMOTEUI_INT_PARAM_XML_TAG, (int)v, c.numInts);
			s.setAttribute(OFXREMOTEUI_INT_PARAM_XML_TAG, OFXREMOTEUI_PARAM_NAME_XML_KEY, key, c.numInts);
			c.numInts++;
			}break;
		case REMOTEUI_PARAM_COLOR:{
			unsigned char r, g, b, a;
			if (t.redValAddr){
				r = *t.redValAddr;
				g = *(t.redValAddr+1);
				b = *(t.redValAddr+2);
				a = *(t.redValAddr+3);
			}else{
				r = t.redVal; g = t.greenVal; b = t.blueVal; a = t.alphaVal;
			}
			if(verbose_) RLOG_NOTICE << "saving '" << key << "' (" << r << " " << g << " " << b << " " << a << ") to XML" ;
			s.setValue(string(OFXREMOTEUI_COLOR_PARAM_XML_TAG) + ":R", (int)r, c.numColors);
			s.setValue(string(OFXREMOTEUI_COLOR_PARAM_XML_TAG) + ":G", (int)g, c.numColors);
			s.setValue(string(OFXREMOTEUI_COLOR_PARAM_XML_TAG) + ":B", (int)b, c.numColors);
			s.setValue(string(OFXREMOTEUI_COLOR_PARAM_XML_TAG) + ":A", (int)a, c.numColors);
			s.setAttribute(OFXREMOTEUI_COLOR_PARAM_XML_TAG, OFXREMOTEUI_PARAM_NAME_XML_KEY, key, c.numColors);
			c.numColors++;
			}break;
		case REMOTEUI_PARAM_ENUM:{
			int v = t.intValAddr ? *t.intValAddr : t.intVal;
			if(verbose_) RLOG_NOTICE << "saving '" << key << "' (" << v <<") to XML" ;
			s.setValue(OFXREMOTEUI_ENUM_PARAM_XML_TAG, v, c.numEnums);
			s.setAttribute(OFXREMOTEUI_ENUM_PARAM_XML_TAG, OFXREMOTEUI_PARAM_NAME_XML_KEY, key, c.numEnums);
			c.numEnums++;
			}break;
		case REMOTEUI_PARAM_BOOL:{
			bool v = t.boolValAddr ? *t.boolValAddr : t.boolVal;
			if(verbose_) RLOG_NOTICE << "saving '" << key << "' (" << v <<") to XML" ;
			s.setValue(OFXREMOTEUI_BOOL_PARAM_XML_TAG, (bool)v, c.numBools);
			s.setAttribute(OFXREMOTEUI_BOOL_PARAM_XML_TAG, OFXREMOTEUI_PARAM_NAME_XML_KEY, key, c.numBools);
			c.numBools++;
			}break;
		case REMOTEUI_PARAM_STRING:{
			string v = t.stringValAddr ? *t.stringValAddr : t.stringVal;
			if(verbose_) RLOG_NOTICE << "saving '" << key << "' (" << v <<") to XML" ;
			s.setValue(OFXREMOTEUI_STRING_PARAM_XML_TAG, (string)v, c.numStrings);
			s.setAttribute(OFXREMOTEUI_STRING_PARAM_XML_TAG, OFXREMOTEUI_PARAM_NAME_XML_KEY, key, c.numStrings);
			c.numStrings++;
			}break;

		case REMOTEUI_PARAM_SPACER:
			if(verbose_) RLOG_NOTICE << "skipping save of spacer '" << key << "' to XML" ;
			break;

		default:
			break;
	}
}

#ifdef OF_AVAILABLE
void ofxRemoteUIServer::saveParamToXmlSettings(const RemoteUIParam& t, const string & key, pugi::xml_node & s, int index, bool active){

	//1st att is name
	s.append_attribute("name").set_value(key.c_str());

	switch (t.type) {

		case REMOTEUI_PARAM_FLOAT:{
			float v = t.floatValAddr ? *t.floatValAddr : t.floatVal;
			if(verbose_) RLOG_NOTICE << "saving '" << key << "' (" << v <<") to XML" ;
			s.append_child(pugi::node_pcdata).set_value(ofToString(v).c_str());
			s.append_attribute("type").set_value("float");
			}break;

		case REMOTEUI_PARAM_INT:{
			int v = t.intValAddr ? *t.intValAddr : t.intVal;
			if(verbose_) RLOG_NOTICE << "saving '" << key << "' (" << v <<") to XML" ;
			s.append_attribute("type").set_value("int");
			s.append_child(pugi::node_pcdata).set_value(ofToString(v).c_str());
			}break;

		case REMOTEUI_PARAM_COLOR:{
			unsigned char r, g, b, a;
			if (t.redValAddr){
				r = *t.redValAddr;
				g = *(t.redValAddr+1);
				b = *(t.redValAddr+2);
				a = *(t.redValAddr+3);
			}else{
				r = t.redVal; g = t.greenVal; b = t.blueVal; a = t.alphaVal;
			}
			if(verbose_) RLOG_NOTICE << "saving '" << key << "' (" << r << " " << g << " " << b << " " << a << ") to XML";
			s.append_attribute("type").set_value("color");
			s.append_attribute("c0.red").set_value(ofToString((int)r).c_str());
			s.append_attribute("c1.green").set_value(ofToString((int)g).c_str());
			s.append_attribute("c2.blue").set_value(ofToString((int)b).c_str());
			s.append_attribute("c3.alpha").set_value(ofToString((int)a).c_str());
			}break;

		case REMOTEUI_PARAM_ENUM:{
			int v = t.intValAddr ? *t.intValAddr : t.intVal;
			if(verbose_) RLOG_NOTICE << "saving '" << key << "' (" << v <<") to XML" ;
			s.append_attribute("type").set_value("enum");
			s.append_child(pugi::node_pcdata).set_value(ofToString(v).c_str());
			}break;

		case REMOTEUI_PARAM_BOOL:{
			bool v = t.boolValAddr ? *t.boolValAddr : t.boolVal;
			if(verbose_) RLOG_NOTICE << "saving '" << key << "' (" << v <<") to XML" ;
			s.append_attribute("type").set_value("bool");
			s.append_child(pugi::node_pcdata).set_value(ofToString(v).c_str());
			}break;

		case REMOTEUI_PARAM_STRING:{
			string v = t.stringValAddr ? *t.stringValAddr : t.stringVal;
			if(verbose_) RLOG_NOTICE << "saving '" << key << "' (" << v <<") to XML" ;
			s.append_attribute("type").set_value("string");
			s.append_child(pugi::node_pcdata).set_value(v.c_str());
			}break;

		case REMOTEUI_PARAM_SPACER:{
			if(verbose_) RLOG_NOTICE << "save spacer '" << key << "' to XML" ;
			s.append_attribute("type").set_value("group");
			string comment = " # " + ofToString(key) + " ################################################################################# ";
			s.parent().insert_child_before(pugi::node_comment, s).set_value(comment.c_str());
			
			}break;

		case REMOTEUI_PARAM_UNKNOWN:
			RLOG_ERROR << "unknown param type at saveParamToXmlSettings()";
			break;
	}

	if(!active){
		s.append_attribute("disabled").set_value("1");
	}
}

#endif

void ofxRemoteUIServer::saveGroupToXML(string fileName, string groupName, bool oldFormat){
	#ifdef OF_AVAILABLE
	fileName = getFinalPath(fileName);
	ofDirectory d;
	string path = getFinalPath(string(OFXREMOTEUI_PRESET_DIR) + "/" + groupName);
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

	#ifdef OF_AVAILABLE
	if(oldFormat){
		saveGroupToXMLv1(fileName, groupName);
	}else{
		saveToXMLv2(fileName, groupName); //save to v2 no matter what
	}
	#else
	saveGroupToXMLv1(fileName, groupName);
	#endif
}


void ofxRemoteUIServer::saveToXML(string fileName, bool oldFormat){

	#ifdef OF_AVAILABLE
	if(oldFormat){
		saveToXMLv1(fileName);
	}else{
		saveToXMLv2(fileName, ""); //upgrade xml files to the new format, never save with v1
	}
	#else
	saveToXMLv1(fileName);
	#endif
}


#ifdef OF_AVAILABLE
void ofxRemoteUIServer::saveToXMLv2(string fileName, string groupName){

	bool savingGroupOnly = groupName.size() != 0;
	if(!savingGroupOnly){
		saveSettingsBackup(); //every time , before we save
	}

	if(!savingGroupOnly){
		fileName = getFinalPath(fileName);
	}

	RLOG_NOTICE << "Saving to XML (using the V2 format) '" << fileName << "'" ;
	pugi::xml_document s;

	pugi::xml_node root = s.append_child(OFXREMOTEUI_XML_ROOT_TAG);
	root.append_child(OFXREMOTEUI_XML_V_TAG).append_child(pugi::node_pcdata).set_value(OFXREMOTEUI_XML_FORMAT_VER);
	pugi::xml_node paramsList = root.append_child(OFXREMOTEUI_XML_TAG);

	//save all params
	int numSaved = 0;

	dataMutex.lock();

	vector<string> savedParams;
	for(int i = 0; i < orderedKeys.size(); i++){
		string key = orderedKeys[i];
		RemoteUIParam &t = params[key];
		if(t.type != REMOTEUI_PARAM_UNKNOWN){
			bool save = false;
			if(savingGroupOnly){
				if( t.group != OFXREMOTEUI_DEFAULT_PARAM_GROUP && t.group == groupName ){
					save = true;
				}
			}else{
				save = true;
			}
			if(save){
				auto thisParamXml = paramsList.append_child("P");
				saveParamToXmlSettings(t, key, thisParamXml, numSaved, true);
				numSaved++;
				savedParams.push_back(key);
			}
		}else{
			RLOG_WARNING << "param '" << key << "' not found in DB!";
		}
	}

	if(!savingGroupOnly){ //group presets dont save disabled stuff
		//add comment separating enabled params from disabled params
		if(orderedKeys_removed.size()){
			paramsList.append_child(pugi::node_comment).set_value("                                                                                                  ");
			paramsList.append_child(pugi::node_comment).set_value(" ###################################### DISABLED PARAMS ######################################### ");
			paramsList.append_child(pugi::node_comment).set_value("                                                                                                  ");
		}
		//save removed params
		int c = 0;
		for(int i = 0; i < orderedKeys_removed.size(); i++){
			string key = orderedKeys_removed[i];
			if (find(savedParams.begin(), savedParams.end(), key) == savedParams.end()){
				RemoteUIParam &t = params_removed[key];
				auto thisParamXml = paramsList.append_child("P");
				saveParamToXmlSettings(t, key, thisParamXml, numSaved + c, false);
				c++;
			}else{
				//param is defined as both params_removed[] and params[], most likely we loaded XML b4 we finished defining all code params...
				//we just make sure we dont save it twice in the XML, and we save the params[] version
			}
		}
	}
	dataMutex.unlock();

	if(!savingGroupOnly){ //group presets dont embed port
		root.append_child(OFXREMOTEUI_XML_PORT_TAG).append_child(pugi::node_pcdata).set_value(ofToString(port).c_str());
	}

	root.append_child(OFXREMOTEUI_XML_ENABLED_TAG).append_child(pugi::node_pcdata).set_value(ofToString(enabled).c_str());

	//s.save(fileName); //this is replaced by the code below to avoid this crash on exit https://github.com/openframeworks/openFrameworks/issues/5298
	struct xml_string_writer: pugi::xml_writer{
		std::string result;
		virtual void write(const void* data, size_t size){
			result.append(static_cast<const char*>(data), size);
		}
	};

	xml_string_writer writer;
	s.print(writer);

	ofstream myfile;
	string fullPath = dataPath + "/" + fileName;
	myfile.open(fullPath.c_str());
	myfile << writer.result;
	myfile.close();
	RLOG_NOTICE << "Done saving! (using the V2 format) '" << fileName << "'" ;

}
#endif

void ofxRemoteUIServer::loadPresetNamed(std::string presetName){
	bool ok = false;
	string path = getFinalPath(OFXREMOTEUI_PRESET_DIR) + "/" + presetName;
	string ext = ofFilePath::getFileExt(path);
	if (ext != OFXREMOTEUI_PRESET_FILE_EXTENSION){
		path += "." + string(OFXREMOTEUI_PRESET_FILE_EXTENSION);
	}
	loadFromXMLv2(path);
	RemoteUIServerCallBackArg cbArg;
	cbArg.action = SERVER_DID_PROGRAMATICALLY_LOAD_PRESET;
	cbArg.msg = presetName;
	if(callBack) callBack(cbArg);
	#ifdef OF_AVAILABLE
	ofNotifyEvent(clientAction, cbArg, this);
	#endif
}

vector<string> ofxRemoteUIServer::loadFromXML(string fileName){

	vector<string> empty;
	#ifdef OF_AVAILABLE
	fileName = getFinalPath(fileName);
	#endif

	ofxXmlSettings s;
	bool exists = s.loadFile(fileName);
	RLOG_NOTICE << "Loading from XML! \"" << fileName << "\"";

	#ifdef OF_AVAILABLE
	if(!exists){
		string extension = ofFilePath::getFileExt(fileName);
		if(extension == OFXREMOTEUI_PRESET_FILE_EXTENSION){ //loading "rui" file not found
			string ruiFileName = fileName;
			fileName = fileName.substr(0, fileName.size() - extension.size());
			fileName += "xml"; //legacy file extension
			exists = s.loadFile(fileName);
			RLOG_WARNING << "Can't find file! Trying out legacy file extension at! \"" << fileName << "\"";
			if(exists){ //same file as .xml exists?
				ofFile::moveFromTo(fileName, ruiFileName); //then rename it to .rui
				fileName = ruiFileName;
			}
		}
	}
	#endif

	if (exists){
		bool newVersion = s.getNumTags(string(OFXREMOTEUI_XML_ROOT_TAG) + ":" + string(OFXREMOTEUI_XML_V_TAG)) > 0;
		#ifdef OF_AVAILABLE
		if( newVersion ){ //if we have a version tag, it must be v2
			return loadFromXMLv2(fileName);
		}else{ //no version tag, so this is v1
			return loadFromXMLv1(fileName);
		}
		#else
		loadFromXMLv1(fileName);
		newVersion = false;
		#endif
	}else{
		RLOG_ERROR << "can't load XML from \"" << fileName << "\"";
	}
	return empty;
}

#ifdef OF_AVAILABLE
vector<string> ofxRemoteUIServer::loadFromXMLv2(string fileName){

	vector<string> loadedParams;
	unordered_map<string, bool> readKeys; //to keep track of duplicated keys;

	ofXml xml;
	bool loaded = xml.load(fileName);
	if(!loaded){
		RLOG_ERROR << "can't load XML file at " << fileName;
		return loadedParams;
	}

	auto paramsXml = xml.findFirst("//" + string(OFXREMOTEUI_XML_ROOT_TAG) + "/" + string(OFXREMOTEUI_XML_TAG)).getChildren("P");

	dataMutex.lock();

	for(auto & s : paramsXml){

		string paramName = s.getAttribute("name").getValue();
		string type = s.getAttribute("type").getValue();
		bool inactive = s.getAttribute("disabled").getValue() == "1";

		if (std::find(paramsToIgnoreWhenLoadingPresets.begin(), paramsToIgnoreWhenLoadingPresets.end(), paramName) !=
			paramsToIgnoreWhenLoadingPresets.end()){
			RLOG_NOTICE << "Ignoring the param \"" << paramName << "\" defined in preset because its in the ignore list.";
			continue;
		}

		unordered_map<string, RemoteUIParam>::iterator it = params.find(paramName);
		bool isAParamWeKnowOf = it != params.end();

		if (readKeys.find(paramName) == readKeys.end()){ //lets not read keys twice, only read the first one we find in xml

			if(type.length() >	0){

				readKeys[paramName] = true;
				loadedParams.push_back(paramName);
				RemoteUIParam p;

				if(isAParamWeKnowOf){
					p = params[paramName];
				}

				switch (type[0]){

					case 'f':{ //float
						if (!isAParamWeKnowOf){
							p.type = REMOTEUI_PARAM_FLOAT;
							p.floatVal = s.getFloatValue();
						}else{
							if(p.type != REMOTEUI_PARAM_FLOAT){ RLOG_ERROR << "type missmatch parsing '" << paramName << "'. Ignoring it!"; break;}
							float val;
							if(loadFromXmlClampsToValidRange){
								val = ofClamp(s.getFloatValue(), p.minFloat, p.maxFloat);
							}else{
								val = s.getFloatValue();
							}
							p.floatVal = *p.floatValAddr = val;
							if(verbose_) RLOG_NOTICE << "loading a FLOAT '" << paramName <<"' (" << ofToString( *p.floatValAddr, 3) << ") from XML" ;
						}
					}break;

					case 'i':{ //int
						if (!isAParamWeKnowOf){
							p.type = REMOTEUI_PARAM_INT;
							p.intVal = s.getIntValue();
						}else{
							if(p.type != REMOTEUI_PARAM_INT){ RLOG_ERROR << "type missmatch parsing '" << paramName << "'. Ignoring it!"; break;}
							int val;
							if(loadFromXmlClampsToValidRange){
								val = ofClamp(s.getIntValue(), p.minInt, p.maxInt);
							}else{
								val = s.getIntValue();
							}
							p.intVal = *p.intValAddr = val;
							if(verbose_) RLOG_NOTICE << "loading an INT '" << paramName <<"' (" << (int) *p.intValAddr << ") from XML" ;
						}
					}break;

					case 's':{ //string
						if (!isAParamWeKnowOf){
							p.type = REMOTEUI_PARAM_STRING;
							p.stringVal = s.getValue();
						}else{
							if(p.type != REMOTEUI_PARAM_STRING){ RLOG_ERROR << "type missmatch parsing '" << paramName << "'. Ignoring it!"; break;}
							string val = s.getValue();
							p.stringVal = *p.stringValAddr = val;
							if(verbose_) RLOG_NOTICE << "loading a STRING '" << paramName <<"' (" << (string) *p.stringValAddr << ") from XML" ;
						}
					}break;

					case 'e':{ //enum
						if (!isAParamWeKnowOf){
							p.type = REMOTEUI_PARAM_ENUM;
							p.intVal = s.getIntValue();
						}else{
							if(p.type != REMOTEUI_PARAM_ENUM){ RLOG_ERROR << "type missmatch parsing '" << paramName << "'. Ignoring it!"; break;}
							int val = ofClamp(s.getIntValue(), p.minInt, p.maxInt);
							p.intVal = *p.intValAddr = val;
							if(verbose_) RLOG_NOTICE << "loading an ENUM '" << paramName <<"' (" << (int) *p.intValAddr << ") from XML" ;
						}
					}break;

					case 'b':{ //bool
						if (!isAParamWeKnowOf){
							p.type = REMOTEUI_PARAM_BOOL;
							p.boolVal = s.getIntValue();
						}else{
							if(p.type != REMOTEUI_PARAM_BOOL){ RLOG_ERROR << "type missmatch parsing '" << paramName << "'. Ignoring it!"; break;}
							bool val = s.getIntValue();
							p.boolVal = *p.boolValAddr = val;
							if(verbose_) RLOG_NOTICE << "loading a BOOL '" << paramName <<"' (" << (bool) *p.boolValAddr << ") from XML" ;
						}
					}break;

					case 'g':{ //group
						if (!isAParamWeKnowOf){
							p.type = REMOTEUI_PARAM_SPACER;
							p.group = p.stringVal = paramName;
						}else{
							if(verbose_) RLOG_NOTICE << "skipping GROUP '" << paramName << "' from XML" ;
						}
					}break;


					case 'c':{ //color
						if (!isAParamWeKnowOf){
							p.type = REMOTEUI_PARAM_COLOR;
							p.redVal = s.getAttribute("c0.red").getIntValue();
							p.greenVal = s.getAttribute("c1.green").getIntValue();
							p.blueVal = s.getAttribute("c2.blue").getIntValue();
							p.alphaVal = s.getAttribute("c3.alpha").getIntValue();
						}else{
							if(p.type != REMOTEUI_PARAM_COLOR){ RLOG_ERROR << "type missmatch parsing '" << paramName << "'. Ignoring it!"; break;}
							unsigned char r = s.getAttribute("c0.red").getIntValue();
							unsigned char g = s.getAttribute("c1.green").getIntValue();
							unsigned char b = s.getAttribute("c2.blue").getIntValue();
							unsigned char a = s.getAttribute("c3.alpha").getIntValue();
							if(p.redValAddr != NULL){
								*p.redValAddr = p.redVal = r;
								*(p.redValAddr + 1) = p.greenVal = g;
								*(p.redValAddr + 2) = p.blueVal = b;
								*(p.redValAddr + 3) = p.alphaVal = a;
								if(verbose_) RLOG_NOTICE << "loading a COLOR '" << paramName <<"' (" << (int)*p.redValAddr << " " << (int)*(p.redValAddr + 1) << " " << (int)*(p.redValAddr + 2) << " " << (int)*(p.redValAddr + 3)  << ") from XML" ;
							}else{
								RLOG_ERROR << "ERROR at loading COLOR (" << paramName << ")" ;
							}
						}
					}break;
				}

				if(isAParamWeKnowOf){
					params[paramName] = p;
				}else{
					RLOG_VERBOSE << "Param '" << paramName << "' found in XML but not defined in source code! Keeping it arround to save back to XML";
					if (params_removed.find(paramName) == params_removed.end()){ //not added yet
						orderedKeys_removed[orderedKeys_removed.size()] = paramName;
					}
					params_removed[paramName] = p;
				}

				if(paramsLoadedFromXML.find(paramName) == paramsLoadedFromXML.end()){
					//if(isAParamWeKnowOf){ //defined in src by calling rui_share_param()
					paramsLoadedFromXML[paramName] = true;
					//}
					paramsFromXML[paramName] = p;
				}
			}
		}
	}

	vector<string> paramsNotInXML;
	for( unordered_map<string, RemoteUIParam>::iterator ii = params.begin(); ii != params.end(); ++ii ){
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
	
	dataMutex.unlock();
	
	return paramsNotInXML;
}

#endif


void ofxRemoteUIServer::restoreAllParamsToInitialXML(){

	dataMutex.lock();

	for( unordered_map<string, RemoteUIParam>::iterator ii = params.begin(); ii != params.end(); ++ii ){
		string key = (*ii).first;
		if (params[key].type != REMOTEUI_PARAM_SPACER){
			if (paramsFromXML.find(key) != paramsFromXML.end()){
				auto & xmlP = paramsFromXML[key];
				auto & p = params[key];
				//params[key] = paramsFromXML[key];
				switch (p.type) {
					case REMOTEUI_PARAM_FLOAT:
						p.floatVal = xmlP.floatVal;
						break;
					case REMOTEUI_PARAM_ENUM:
					case REMOTEUI_PARAM_INT:
						p.intVal = xmlP.intVal;
						break;

					case REMOTEUI_PARAM_COLOR:
						p.redVal = xmlP.redVal;
						p.greenVal = xmlP.greenVal;
						p.blueVal = xmlP.blueVal;
						p.alphaVal = xmlP.alphaVal;
						break;

					case REMOTEUI_PARAM_BOOL:
						p.boolVal = xmlP.boolVal;
						break;

					case REMOTEUI_PARAM_STRING:
					case REMOTEUI_PARAM_SPACER:
						p.stringVal = xmlP.stringVal;
						break;
					default: break;
				}
				syncPointerToParam(key);
			}
		}
	}
	dataMutex.unlock();
}

void ofxRemoteUIServer::restoreAllParamsToDefaultValues(){
	dataMutex.lock();
	for( unordered_map<string, RemoteUIParam>::iterator ii = params.begin(); ii != params.end(); ++ii ){
		string key = (*ii).first;
		params[key] = paramsFromCode[key];
		syncPointerToParam(key);
	}
	dataMutex.unlock();
}

void ofxRemoteUIServer::pushParamsToClient(){

	vector<string>changedParams = scanForUpdatedParamsAndSync();
	#ifdef OF_AVAILABLE
	dataMutex.lock();
	for(int i = 0 ; i < changedParams.size(); i++){
		string pName = changedParams[i];
		RemoteUIParam &p = params[pName];
		onScreenNotifications.addParamUpdate(pName, p,
											 ofColor(p.r, p.g, p.b, p.a),
			p.type == REMOTEUI_PARAM_COLOR ?
			ofColor(p.redVal, p.greenVal, p.blueVal, p.alphaVal) :
			ofColor(0,0,0,0)
			);

	}
	dataMutex.unlock();
	#endif
	
	if(readyToSend){
		vector<string>paramsList = getAllParamNamesList();
		syncAllParamsToPointers();
		sendUpdateForParamsInList(paramsList);
		sendREQU(true); //once all send, confirm to close the REQU
		//also send the presets list
		presetNames = getAvailablePresets();
		if (presetNames.size() == 0){
			presetNames.push_back(OFXREMOTEUI_NO_PRESETS);
		}
		sendPREL(presetNames);
	}
}


string ofxRemoteUIServer::getFinalPath(const string & p){

	if(directoryPrefix.size()){
		stringstream ss;
		ss << directoryPrefix << "/" << p;
		return ss.str();
	}
	return p;
}

void ofxRemoteUIServer::saveSettingsBackup(){

	#ifdef OF_AVAILABLE
	if(autoBackups){
		ofDirectory d;
		d.open( getFinalPath(OFXREMOTEUI_SETTINGS_BACKUP_FOLDER) );
		if (!d.exists()){
			ofDirectory::createDirectory(getFinalPath(OFXREMOTEUI_SETTINGS_BACKUP_FOLDER));
		}d.close();
		string basePath = OFXREMOTEUI_SETTINGS_BACKUP_FOLDER + string("/") + ofFilePath::removeExt(OFXREMOTEUI_SETTINGS_FILENAME) + ".";
		basePath = getFinalPath(basePath);
		for (int i = OFXREMOTEUI_NUM_BACKUPS - 1; i >= 0; i--){
			string originalPath = basePath + ofToString(i) + "." + OFXREMOTEUI_PRESET_FILE_EXTENSION;
			string destPath = basePath + ofToString(i+1) + "." + OFXREMOTEUI_PRESET_FILE_EXTENSION;
			ofFile og;
			og.open(originalPath);
			if ( og.exists() ){
				try{
					ofFile::moveFromTo(originalPath, destPath, true, true); //TODO complains on windows!
				}catch(...){}
			}
			og.close();
		}
		ofFile f;
		f.open(getFinalPath(OFXREMOTEUI_SETTINGS_FILENAME));
		if(f.exists()){
			try{
				ofFile::copyFromTo(getFinalPath(OFXREMOTEUI_SETTINGS_FILENAME), basePath + "0." + OFXREMOTEUI_PRESET_FILE_EXTENSION);
			}catch(...){}
		}
		f.close();
		if(verbose_) RLOG_NOTICE << "saving a backup of the current " << getFinalPath(OFXREMOTEUI_SETTINGS_FILENAME) << " in " << getFinalPath(OFXREMOTEUI_SETTINGS_BACKUP_FOLDER) ;
	}
	#endif
}



void ofxRemoteUIServer::setup(int port_, float updateInterval_){

	#ifdef OF_AVAILABLE
		ofDirectory d;
		d.open(getFinalPath(OFXREMOTEUI_PRESET_DIR));
		if(!d.exists()){
			d.create(true);
		}

		#ifndef TARGET_OF_IOS
		const ofAppNoWindow * win = dynamic_cast<const ofAppNoWindow*>(ofGetWindowPtr());
		if(win){
			RLOG_NOTICE << "Running Headless mode!";
			headlessMode = true;
		}
		#endif
	#else
	#if defined(_WIN32)
		_mkdir(getFinalPath(OFXREMOTEUI_PRESET_DIR));
	#else
		mkdir(getFinalPath(OFXREMOTEUI_PRESET_DIR).c_str(), (mode_t)0777);
	#endif
	#endif

	//check for enabled
	string configFile;
	bool exists = true;
	ofxXmlSettings s;
	#ifdef OF_AVAILABLE

	dataPath = ofToDataPath("", true);
	bool wasEnabledB4setup = enabled;
	configFile = ofToDataPath(getFinalPath(OFXREMOTEUI_SETTINGS_FILENAME));
	exists = s.loadFile(configFile);
	
	if(exists){
		bool pushed = false;
		if(s.getNumTags(OFXREMOTEUI_XML_ROOT_TAG)){ //v2
			s.pushTag(OFXREMOTEUI_XML_ROOT_TAG);
			pushed = true;
		}
		if( s.getNumTags(OFXREMOTEUI_XML_ENABLED_TAG) > 0 ){
			enabled = (1 == s.getValue(OFXREMOTEUI_XML_ENABLED_TAG, 0));
			if(!wasEnabledB4setup){
				enabled = false;
				RLOG_WARNING << "xml config set to enabled=true, but we were disabled b4 setup. Launching disabled!";
			}
			if (!enabled){
				RLOG_WARNING << "launching disabled!" ;
			}
		}
		if(pushed) s.popTag();
	}
	#else
	configFile = getFinalPath(OFXREMOTEUI_SETTINGS_FILENAME);
	enabled = true;
	#endif

	if(enabled){

		//setup the broadcasting
		string subnetMask;
		computerIP = getMyIP(userSuppliedNetInterface, subnetMask);
		doBroadcast = true;
		string multicastIP;
		
		if (computerIP != RUI_LOCAL_IP_ADDRESS) { // if addr is not 127.0.0.1
			
			struct in_addr host, mask, broadcast;
			char broadcast_address[INET_ADDRSTRLEN];
			// get broadcast
			if (inet_pton(AF_INET, computerIP.c_str(), &host) == 1 && inet_pton(AF_INET, subnetMask.c_str(), &mask) == 1) {
				broadcast.s_addr = host.s_addr | ~mask.s_addr;
			} else {
				// Failed converting strings to ip
			}
			
			if (inet_ntop(AF_INET, &broadcast, broadcast_address, INET_ADDRSTRLEN) != NULL) {
				multicastIP = string(broadcast_address);
			} else {
				// Failed converting ip to string
			}
		} else {
			// Go with default guess
			multicastIP = "255.255.255.255";
		}

		#ifdef OF_AVAILABLE
		#ifdef _WIN32
		if (multicastIP == "255.255.255.255"){
			doBroadcast = false; //windows crashes on bradcast if no devices are up!
			RLOG_WARNING << "no network interface found, we will not broadcast ourselves";
		}
		#endif
		#endif

		//find out some info about the host
		if (computerName.size() == 0){
			#ifdef OF_AVAILABLE
				#ifdef _WIN32
					char buffer[MAX_COMPUTERNAME_LENGTH + 1];
					DWORD length = sizeof(buffer);
					GetComputerNameExA((COMPUTER_NAME_FORMAT)0, buffer, &length);
					computerName = buffer;
				#else
					computerName = ofSystem("hostname");
					ofStringReplace(computerName, "\n", "");
					ofStringReplace(computerName, ".local", "");
				#endif

				binaryName = ofFilePath::getBaseName(ofFilePath::getCurrentExePath());

				#ifdef TARGET_OF_IOS
				binaryName = "iOS App";
				#endif
			#else
				binaryName = "Unknown";
			#endif
		}

		
		if(doBroadcast){
			broadcastSender.setup( multicastIP, OFXREMOTEUI_BROADCAST_PORT ); //multicast @
			RLOG_NOTICE << "Broacasting my presence every " << OFXREMOTEUI_BORADCAST_INTERVAL << "sec at this multicast @ " << multicastIP << ":" << OFXREMOTEUI_BROADCAST_PORT ;
		}

		if(port_ == -1){ //if no port specified, pick a random one, but only the very first time we get launched!
			portIsSet = false;
			bool newVersion = true;
			if(exists){

				newVersion = s.getNumTags(string(OFXREMOTEUI_XML_ROOT_TAG)) > 0;
				if (newVersion){
					if( s.getNumTags(string(OFXREMOTEUI_XML_ROOT_TAG) + ":" + string(OFXREMOTEUI_XML_PORT_TAG)) > 0 ){
						port_ = s.getValue(string(OFXREMOTEUI_XML_ROOT_TAG) + ":" + string(OFXREMOTEUI_XML_PORT_TAG), 10000);
					}
				}else{
					if( s.getNumTags(string(OFXREMOTEUI_XML_PORT_TAG)) > 0 ){
						port_ = s.getValue(string(OFXREMOTEUI_XML_PORT_TAG), 10000);
					}
				}
			}
			if(port_ == -1){ //port still undefined, lets choose a port
				#ifdef OF_AVAILABLE
				ofSeedRandom();
				port_ = ofRandom(5000, 60000);
				#else
				srand (time(NULL));
				port_ = 5000 + rand()%55000;
				#endif
			}

		}else{
			portIsSet = true;
		}
		params.clear();
		updateInterval = updateInterval_;
		waitingForReply = false;
		avgTimeSinceLastReply = timeSinceLastReply = timeCounter = 0.0f;
		port = port_;
		RLOG_NOTICE << "Listening for commands at " << computerIP << ":" << port;
		oscReceiver.setup(port);
        
        #ifdef RUI_WEB_INTERFACE
            listenWebSocket(port + 1);
            startWebServer(port + 2);
        #endif
        
    }

	//still get ui access despite being disabled
	#ifdef OF_AVAILABLE
	ofAddListener(ofEvents().exit, this, &ofxRemoteUIServer::_appExited, OF_EVENT_ORDER_BEFORE_APP); //to save to xml, disconnect, etc
	ofAddListener(ofEvents().keyPressed, this, &ofxRemoteUIServer::_keyPressed, OF_EVENT_ORDER_BEFORE_APP);
	ofAddListener(ofEvents().update, this, &ofxRemoteUIServer::_update, OF_EVENT_ORDER_BEFORE_APP);
	ofAddListener(ofEvents().draw, this, &ofxRemoteUIServer::_draw, OF_EVENT_ORDER_AFTER_APP + 110); //last thing to draw
	#endif

	setNewParamColor(1);
	setNewParamColorVariation(true);
	RUI_LOAD_FROM_XML(); //we load at setup time - all values are store even b4 they are defined in src!

	#ifdef TARGET_OSX
	if(oscQueryServer == nullptr){
		oscQueryServer = new OscQueryServerMgr();
		oscQueryServer->setup();
	}
	#endif
}

#ifdef OF_AVAILABLE
void ofxRemoteUIServer::_appExited(ofEventArgs &e){
	if(!enabled) return;
	RLOG_NOTICE << "Closing ofxRemoteUIServer...";
	OFX_REMOTEUI_SERVER_CLOSE();		//stop the server
	if(saveToXmlOnExit){
		RLOG_NOTICE << "Saving to XML on exit...";
		OFX_REMOTEUI_SERVER_SAVE_TO_XML();	//save values to XML
	}else{
		RLOG_NOTICE << "We were supposed to Save to XML on exit, but we haven't loaded an XML yet... So not saving!";
	}

	#ifdef TARGET_OSX
	if(oscQueryServer){
		delete oscQueryServer;
	}
	#endif
}


bool ofxRemoteUIServer::_keyPressed(ofKeyEventArgs &e){

	if (showUI){
		switch(e.key){ //you can save current config from tab screen by pressing s

			case 's':
				saveToXML(OFXREMOTEUI_SETTINGS_FILENAME);
				onScreenNotifications.addNotification("SAVED CONFIG to default XML");
				break;

			case 'E': //e for export to older v
			case 'S':{
				bool saveInV1 = (e.key == 'E') ? true : false;
				bool groupIsSelected = false;
				string formatExt = string(saveInV1 ? "_v1" : "");
				string groupName;
				if (selectedItem >= 0){ //selection on params list
					dataMutex.lock();
					string key = orderedKeys[selectedItem];
					RemoteUIParam &p = params[key];
					if (p.type == REMOTEUI_PARAM_SPACER){
						groupIsSelected = true;
						groupName = p.group;
					}
					dataMutex.unlock();
				}
				string presetName = ofSystemTextBoxDialog(groupIsSelected ?
														  "Create a New Group Preset For " + groupName
														  :
														  "Create a New Global Preset" ,
														  "");

				presetName = cleanCharsForFileSystem(presetName); //remove weird chars
				if(presetName.size()){
					RemoteUIServerCallBackArg cbArg;
					if (groupIsSelected){
						saveGroupToXML(string(OFXREMOTEUI_PRESET_DIR) + "/" + groupName + "/" + presetName + formatExt + "." + OFXREMOTEUI_PRESET_FILE_EXTENSION, groupName, saveInV1);
						if(callBack) callBack(cbArg);
						cbArg.action = CLIENT_SAVED_GROUP_PRESET;
						cbArg.msg = presetName;
						cbArg.group = groupName;
						#ifdef OF_AVAILABLE
						ofNotifyEvent(clientAction, cbArg, this);
						onScreenNotifications.addNotification("SAVED PRESET '" + presetName + formatExt + ".xml' FOR GROUP '" + groupName + "'");
						#endif
					}else{
						if(verbose_) RLOG_NOTICE << "saving NEW preset: " << presetName ;
						saveToXML(string(OFXREMOTEUI_PRESET_DIR) + "/" + presetName + formatExt + "." + OFXREMOTEUI_PRESET_FILE_EXTENSION, saveInV1);
						cbArg.action = CLIENT_SAVED_PRESET;
						cbArg.msg = presetName;
						#ifdef OF_AVAILABLE
						onScreenNotifications.addNotification("SAVED PRESET to '" + getFinalPath(OFXREMOTEUI_PRESET_DIR) + "/" + presetName + formatExt + ".xml'");
						ofNotifyEvent(clientAction, cbArg, this);
						#endif
						if(callBack) callBack(cbArg);
					}
					refreshPresetsCache();
				}
			}break;

			#ifdef RUI_WEB_INTERFACE
			case 'c':{
				string url = ofToString(computerIP) + ":" + ofToString(webPort);
				string wsUrl = ofToString(computerIP) + ":" + ofToString(wsPort);
				ofLaunchBrowser("http://" + url + "?connect=" + wsUrl);
				}break;
			#endif

			case 'r':
				restoreAllParamsToInitialXML();
				onScreenNotifications.addNotification("RESET CONFIG TO SERVER-LAUNCH XML values");
				break;

			case 'l':{
				string p = getFinalPath(OFXREMOTEUI_SETTINGS_FILENAME);
				onScreenNotifications.addNotification("LOAD XML from " + p);
				loadFromXML(ofToDataPath(p));
				}break;

			case 'N': drawNotifications ^= true; break;
			case ' ': { //press spacebar to edit string fields! return didnt work well on windows
				if (selectedItem >= 0) { //selection on params list
					dataMutex.lock();
					string key = orderedKeys[selectedItem];
					RemoteUIParam & p = params[key];
					if (p.type == REMOTEUI_PARAM_STRING) {
						p.stringVal = ofSystemTextBoxDialog("Edit Value for '" + key + "' parameter:", p.stringVal);
						syncPointerToParam(key);
						pushParamsToClient();
						RemoteUIServerCallBackArg cbArg;
						cbArg.host = "localhost";
						cbArg.action = CLIENT_UPDATED_PARAM;
						cbArg.paramName = key;
						cbArg.param = p;  //copy the updated param to the callback arg
						#ifdef OF_AVAILABLE
						onScreenNotifications.addParamUpdate(key, cbArg.param, ofColor(p.r, p.g, p.b, p.a));
						ofNotifyEvent(clientAction, cbArg, this);
						#endif
						if (callBack) callBack(cbArg);
					}
					dataMutex.unlock();
				}
			}break;

			case OF_KEY_RETURN:
				dataMutex.lock();
				if(selectedItem == -1 && selectedPreset >= 0 && presetsCached.size()){ //global presets
					lastChosenPreset = presetsCached[selectedPreset];
					loadFromXML(string(OFXREMOTEUI_PRESET_DIR) + "/" + lastChosenPreset + "." + OFXREMOTEUI_PRESET_FILE_EXTENSION);
					syncAllPointersToParams();
					uiAlpha = 0;
					if(verbose_) RLOG_NOTICE << "setting preset: " << lastChosenPreset ;
					RemoteUIServerCallBackArg cbArg;
					cbArg.action = CLIENT_DID_SET_PRESET;
					cbArg.msg = lastChosenPreset;
					if(callBack) callBack(cbArg);
					#ifdef OF_AVAILABLE
					onScreenNotifications.addNotification("SET PRESET to '" + getFinalPath(OFXREMOTEUI_PRESET_DIR) + "/" + lastChosenPreset + ".xml'");
					ofNotifyEvent(clientAction, cbArg, this);
					#endif
				}
				if (selectedItem >= 0){ //selection on params list
					string key = orderedKeys[selectedItem];
					RemoteUIParam & p = params[key];

					if(p.type == REMOTEUI_PARAM_SPACER && groupPresetsCached[p.group].size() > 0){
						string presetName = p.group + "/" + groupPresetsCached[p.group][selectedGroupPreset];
						loadFromXML(string(OFXREMOTEUI_PRESET_DIR) + "/" + presetName + "." + OFXREMOTEUI_PRESET_FILE_EXTENSION);
						syncAllPointersToParams();
						uiAlpha = 0;
						if(verbose_) RLOG_NOTICE << "setting preset: " << presetName ;
						RemoteUIServerCallBackArg cbArg;
						cbArg.action = CLIENT_DID_SET_GROUP_PRESET;
						cbArg.msg = p.group;
						if(callBack) callBack(cbArg);
						#ifdef OF_AVAILABLE
						ofNotifyEvent(clientAction, cbArg, this);
						onScreenNotifications.addNotification("SET '" + p.group  + "' GROUP TO '" + presetName + ".xml' PRESET");
						#endif
					} else if (p.type == REMOTEUI_PARAM_COLOR) {
						selectedColorComp++;
						if (selectedColorComp > 3) selectedColorComp = 0;
					}
				}
				dataMutex.unlock();
				break;

			case OF_KEY_DOWN:
			case OF_KEY_UP:{
				float skip = 1;
				if(ofGetKeyPressed(' ')) skip = 10;
				float sign = e.key == OF_KEY_DOWN ? 1.0 : -1.0;
				selectedGroupPreset = 0;
				uiAlpha = 1;
				selectedItem += sign * skip;
				if(selectedItem < -1) selectedItem = orderedKeys.size() - 1;
				if(selectedItem >= orderedKeys.size()) selectedItem = -1; //presets menu >> selectedItem = -1, on top of all
				selectedGroupPreset = 0;
				}break;

			case '+': setBuiltInUiScale(uiScale + 0.1); break;
			case '-': setBuiltInUiScale(MAX(uiScale - 0.1, 0.5)); break;
			case ',': xOffsetTarget += (uiColumnWidth); xOffsetTarget = ofClamp(xOffsetTarget, -FLT_MAX, 4 * uiColumnWidth); break;
			case '.': xOffsetTarget -= (uiColumnWidth); xOffsetTarget = ofClamp(xOffsetTarget, -FLT_MAX, 4 * uiColumnWidth); break;
			case OF_KEY_LEFT:
			case OF_KEY_RIGHT:{

				dataMutex.lock();
				float sign = e.key == OF_KEY_RIGHT ? 1.0 : -1.0;
				if(ofGetKeyPressed(' ')){ //hold spacebar to increment faster
					sign *= 10;					
				}
				if (selectedItem >= 0){ //params
					string key = orderedKeys[selectedItem];
					RemoteUIParam & p = params[key];
					if (p.type != REMOTEUI_PARAM_SPACER){
						uiAlpha = 0;
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
							case REMOTEUI_PARAM_COLOR:
								switch (selectedColorComp) {
									case 0: p.redVal += sign; break;
									case 1: p.greenVal += sign; break;
									case 2: p.blueVal += sign; break;
									case 3: p.alphaVal += sign; break;
								}
								break;
							default:
								break;
						}
						syncPointerToParam(key);
						pushParamsToClient();
						RemoteUIServerCallBackArg cbArg;
						cbArg.host = "localhost";
						cbArg.action =  CLIENT_UPDATED_PARAM;
						cbArg.paramName = key;
						cbArg.param = p;  //copy the updated param to the callback arg
						#ifdef OF_AVAILABLE
						onScreenNotifications.addParamUpdate(key, cbArg.param, ofColor(p.r, p.g, p.b, p.a));
						ofNotifyEvent(clientAction, cbArg, this);
						#endif
						if(callBack) callBack(cbArg);
					}else{ //in spacer! group time, cycle through group presets
						int numGroupPresets = groupPresetsCached[p.group].size();
						if(numGroupPresets > 0){
							selectedGroupPreset += sign;
							if (selectedGroupPreset < 0) selectedGroupPreset = numGroupPresets -1;
							if (selectedGroupPreset > numGroupPresets -1) selectedGroupPreset = 0;
						}
					}
				}else{ //presets!
					if (presetsCached.size()){
						selectedPreset += sign;
						int limit = presetsCached.size() - 1;
						if (selectedPreset > limit){
							selectedPreset = 0;
						}
						if (selectedPreset < 0){
							selectedPreset = limit;
						}
					}
				}
				dataMutex.unlock();
			}break;
		}
	}

	if(e.key == showInterfaceKey){
		toggleBuiltInClientUI();
	}
	return showUI && e.key != OF_KEY_ESC;
}


void ofxRemoteUIServer::toggleBuiltInClientUI(){
	if (uiAlpha < 1.0 && showUI){
		uiAlpha = 1.0;
		showUI = false;
	}else{
		showUI = !showUI;
	}

	if (showUI){
		uiAlpha = 1;
		uiLines.clear();
		syncAllPointersToParams();
		refreshPresetsCache();
	}
}


void ofxRemoteUIServer::refreshPresetsCache(){

	//get all group presets
	groupPresetsCached.clear();
	dataMutex.lock();
	for( unordered_map<string, RemoteUIParam>::iterator ii = params.begin(); ii != params.end(); ++ii ){
		if((*ii).second.type == REMOTEUI_PARAM_SPACER){
			groupPresetsCached[(*ii).second.group] = getAvailablePresetsForGroup((*ii).second.group);
		};
	}
	dataMutex.unlock();

	presetsCached = getAvailablePresets(true);

	if (selectedPreset > presetsCached.size() -1){
		selectedPreset = 0;
	}
}

void ofxRemoteUIServer::startInBackgroundThread(){
	threadedUpdate = true;
	startThread();
}
#endif

void ofxRemoteUIServer::update(float dt){

	#ifdef OF_AVAILABLE
	if(!threadedUpdate){
		updateServer(dt);
	}
	uiAlpha += 0.3f * ofGetLastFrameTime();
	if(uiAlpha > 1.0f) uiAlpha = 1.0f;
	xOffset = 0.3f * xOffsetTarget + 0.7f * xOffset;
	#else
	updateServer(dt);
	#endif
}

#ifdef OF_AVAILABLE

#ifdef USE_OFX_FONTSTASH
void ofxRemoteUIServer::drawUiWithFontStash(string fontPath, float fontSize_){

	if(!ofIsGLProgrammableRenderer()){
		fontRenderer = RENDER_WITH_OFXFONTSTASH;
		fontFile = ofToDataPath(fontPath, true);
		fontSize = fontSize_;
		font = ofxFontStash();
		font.setup(fontFile, 1.0, 512, false, 0, uiScale);
		onScreenNotifications.drawUiWithFontStash(&font);
		ofRectangle r = font.getBBox("M", fontSize, 0, 0);
		lineH = ceil(r.height * 1.75);
		charW = r.width;
	}else{
		ofLogError("ofxRemoteUIServer") << "Can't use ofxFontStash with the Programmable Renderer!";
	}
}

#endif

#ifdef USE_OFX_FONTSTASH2
void ofxRemoteUIServer::drawUiWithFontStash2(string fontPath, float fontSize_){
	fontRenderer = RENDER_WITH_OFXFONTSTASH2; fontSize2 = fontSize_; fontStashFile2 = fontPath;
	font2 = ofxFontStash2::Fonts();
	font2.setup();
	font2.addFont("mono", ofToDataPath(fontStashFile2, true));
	onScreenNotifications.drawUiWithFontStash2(&font2);
	ofxFontStash2::Style style = ofxFontStash2::Style("mono", fontSize2);
	ofRectangle r = font2.getTextBounds("Mp", style, 0, 0);
	charW = r.width;
	lineH = ceil(r.height * 1.25);
}
#endif

void ofxRemoteUIServer::drawUiWithBitmapFont(){
	fontRenderer = RENDER_WITH_OF_BITMAP_FONT;
	onScreenNotifications.drawUiWithBitmapFont();
	charW = 8;
	lineH = ruiLineH;
}


void ofxRemoteUIServer::threadedFunction(){

	while (isThreadRunning()) {
		updateServer(1./30.); //30 fps timebase
		ofSleepMillis(33);
	}
	if(verbose_) RLOG_NOTICE << "threadedFunction() ending" ;
}


void ofxRemoteUIServer::_draw(ofEventArgs &e){
	if(autoDraw){
		int h = customScreenHeight;
		if(h < 0) h = ofGetHeight();
		draw( 8 / uiScale, (h - 9)/ uiScale );
	}
}

void ofxRemoteUIServer::_update(ofEventArgs &e){
	update(ofGetLastFrameTime());
}

void ofxRemoteUIServer::drawString(const string & text, const ofVec2f & pos){
	drawString(text, pos.x, pos.y);
}

void ofxRemoteUIServer::drawString(const string & text, const float & x, const float & y){

	switch (fontRenderer) {

		case RENDER_WITH_OF_BITMAP_FONT: ofDrawBitmapString(text, x, y + 1);
			break;

		#ifdef USE_OFX_FONTSTASH
		case RENDER_WITH_OFXFONTSTASH: font.drawMultiLine(text, fontSize, x, y + lineH * 0.135); break;
		#endif

		#ifdef USE_OFX_FONTSTASH2
		case RENDER_WITH_OFXFONTSTASH2:{
			ofxFontStash2::Style style = ofxFontStash2::Style("mono", fontSize2, ofGetStyle().color);
			font2.drawColumnNVG(text, style, x, y + lineH * 0.125, ofGetWidth());
		}break;
		#endif
		default:break;
	}
}

//x and y of where the notifications will get draw
void ofxRemoteUIServer::draw(int x, int y){

	if(headlessMode || !enabled) return;

	bool needsToDrawNotification = !showUI || uiAlpha < 1.0;
	int screenH, screenW;

	if(needsToDrawNotification | showUI){
		ofSetupScreen(); //mmm this is a bit scary //TODO!

		ofPushStyle();
		ofPushMatrix();
		ofScale(uiScale,uiScale);
		ofSetDrawBitmapMode(OF_BITMAPMODE_SIMPLE);
		ofEnableBlendMode(OF_BLENDMODE_ALPHA);
		ofSetRectMode(OF_RECTMODE_CORNER);
		
		ofFill();
		screenH = customScreenHeight;
		if (screenH < 0) screenH = ofGetHeight();
		screenW = customScreenWidth;
		if (screenW < 0) screenW = ofGetWidth();
	}

	if(showUI){

		int padding = lineH * 1.5;
		int x = padding;
		int initialY = lineH * 2;
		int y = initialY;
		int colw = uiColumnWidth;
		int realColW = colw * 0.9;
		int valOffset = realColW * 0.7;
		int valSpaceW = realColW - valOffset;
		int spacing = lineH;
		int bottomBarHeight = lineH * 4.25;
		int frameNum = ofGetFrameNum();

		//bottom bar
		if (uiAlpha > 0.99 || showUIduringEdits){
			ofSetColor(11, 245);
			ofDrawRectangle(0,0, screenW / uiScale, screenH / uiScale);
			ofSetColor(44, 245);
			ofDrawRectangle(0,screenH / uiScale - bottomBarHeight, screenW / uiScale, bottomBarHeight );

            string reachableAt;
            if (enabled) {
                reachableAt = "Server reachable at " + computerIP + ":";
				#if defined(RUI_WEB_INTERFACE)
				reachableAt += " ";
				#endif

				reachableAt += ofToString(port);

				#if defined(RUI_WEB_INTERFACE)
				reachableAt += "(OSC)";
				reachableAt += " " + ofToString(wsPort) + "(WS)";
				reachableAt += " " + ofToString(webPort) + "(Web)";
				#endif
            }else {
				reachableAt = "Server disabled";
            }
            
			ofSetColor(255);
			string instructions = "ofxRemoteUI built in client. " + reachableAt +
			"\nPress 's' to save current config, 'S' to make a new preset. ('E' to save in old format)\n" +
			"Press 'r' to restore all params's launch state. '+'/'-' to set UI Scale. 'N' to toggle screen notif.\n" +
			"Press Arrow Keys to edit values. SPACEBAR + Arrow Keys for bigger increments. ',' and '.' Keys to scroll.\n" +
			"Press 'TAB' to hide. Press 'RETURN' to cycle through RGBA components.";
			#ifdef RUI_WEB_INTERFACE
			instructions += " Press 'c' to launch a web client.";
			#endif

			drawString(instructions,
					   charW * 1.5,
					   screenH / uiScale - bottomBarHeight + lineH * 0.65);
		}

		//preset selection / top bar
		ofSetColor(64);
		ofDrawRectangle(0 , 0, screenW / uiScale, lineH);
		ofColor textBlinkC ;
		if(frameNum%6 < 4) textBlinkC = ofColor(0,0);
		else textBlinkC = ofColor(255,0,0);

		ofVec2f dpos = ofVec2f(charW * 23, lineH * 0.65);
		if(presetsCached.size() > 0 && selectedPreset >= 0 && selectedPreset < presetsCached.size()){

			ofSetColor(180);
			if (selectedItem < 0){
				drawString("Press RETURN to load GLOBAL PRESET: \"" + presetsCached[selectedPreset] + "\"", dpos);
				ofSetColor(textBlinkC);
				drawString("                                     " + presetsCached[selectedPreset], dpos);
			}else{
				RemoteUIParam & p = params[orderedKeys[selectedItem]];
				int howMany = 0;
				if(p.type == REMOTEUI_PARAM_SPACER){
					howMany = groupPresetsCached[p.group].size();
					if (howMany > 0){
						string msg = "Press RETURN to load \"" + p.group + "\" GROUP PRESET: \"";
						drawString( msg + groupPresetsCached[p.group][selectedGroupPreset] + "\"", dpos);
						ofSetColor(textBlinkC);
						std::string padding(msg.length(), ' ');
						drawString(padding + groupPresetsCached[p.group][selectedGroupPreset], dpos);
					}
				}
				if(howMany == 0){
					ofSetColor(180);
					drawString("Selected Preset: NONE", dpos);
				}
			}
		}
		if (selectedItem != -1) ofSetColor(255);
		else ofSetColor(textBlinkC);
		drawString("+ PRESET SELECTION: ", charW * 3,  dpos.y);

		int linesInited = uiLines.getNumVertices() > 0 ;
		ofPushMatrix();

		if(uiAlpha > 0.99 || showUIduringEdits){

			ofTranslate(xOffset, 0);

			//param list
			dataMutex.lock();
			for(int i = 0; i < orderedKeys.size(); i++){

				string key = orderedKeys[i];
				RemoteUIParam & p = params[key];
				int chars = key.size();
				int charw = 9;
				int column2MaxLen = ceil(valSpaceW / charw) + 1;
				int stringw = chars * charw;
				if (stringw > valOffset){
					key = key.substr(0, (valOffset) / charw );
				}

				if(p.type == REMOTEUI_PARAM_SPACER){
					ofColor c = ofColor(p.r, p.g, p.b);
					ofSetColor(c * 0.3);
					ofDrawRectangle(x , -spacing + y + spacing * 0.33, realColW, spacing);
				}
				if (selectedItem != i){
					ofSetColor(p.r, p.g, p.b);
				}else{
					if(frameNum%5 < 1) ofSetColor(255);
					else ofSetColor(255,0,0);
				}

				if(p.type != REMOTEUI_PARAM_SPACER){
					string sel = (selectedItem == i) ? ">>" : "  ";
					drawString(sel + key, x, y);
				}else{
					drawString("+ " + p.stringVal, x,y);
				}

				switch (p.type) {
					case REMOTEUI_PARAM_FLOAT:
						drawString(ofToString(p.floatVal), x + valOffset, y);
						break;
					case REMOTEUI_PARAM_ENUM: {
						int index = p.intVal - p.minInt;
						if (index >= 0 && index < p.enumList.size()) {
							string val = p.enumList[index];
							if (val.length() > column2MaxLen) {
								val = val.substr(0, column2MaxLen);
							}
							drawString(val, x + valOffset, y);
						}else {
							drawString(ofToString(p.intVal), x + valOffset, y);
						}
					}break;
					case REMOTEUI_PARAM_INT:
						drawString(ofToString(p.intVal), x + valOffset, y);
						break;
					case REMOTEUI_PARAM_BOOL:
						drawString(p.boolVal ? "true" : "false", x + valOffset, y);
						break;
					case REMOTEUI_PARAM_STRING:
						drawString(p.stringVal, x + valOffset, y);
						break;
					case REMOTEUI_PARAM_COLOR:{
						ofPushStyle();
						ofSetColor(p.redVal, p.greenVal, p.blueVal, p.alphaVal);
						ofDrawRectangle(x + valOffset, y - spacing * 0.6, valSpaceW, spacing * 0.85);
						char aux[200];
						sprintf(aux, "[%0*d,%0*d,%0*d,%0*d]", 3, p.redVal, 3, p.greenVal, 3, p.blueVal, 3, p.alphaVal);
						ofSetColor(0); //shadow
						drawString(aux, x + valOffset + 1, y + 1);
						ofSetColor(255);
						drawString(aux, x + valOffset, y);
						if(selectedItem == i){
							char auxSelect[200];
							switch (selectedColorComp) {
								case 0: sprintf(auxSelect, " ___"); break;
								case 1: sprintf(auxSelect, "     ___"); break;
								case 2: sprintf(auxSelect, "         ___"); break;
								case 3: sprintf(auxSelect, "             ___"); break;
							}
							ofSetColor(textBlinkC);
							drawString(auxSelect, x + valOffset, y);
						}
						ofPopStyle();
						}break;
					case REMOTEUI_PARAM_SPACER:{
						int howMany = groupPresetsCached[p.group].size();

						if (selectedItem == i){ //selected
							if (selectedGroupPreset < howMany && selectedGroupPreset >= 0){
								string presetName = groupPresetsCached[p.group][selectedGroupPreset];
								if (presetName.length() > column2MaxLen){
									presetName = presetName.substr(0, column2MaxLen);
								}
								drawString(presetName, x + valOffset, y);
							}
						}else{ //not selected
							if(howMany > 0 ){
								drawString("(" + ofToString(howMany) + ")", x + valOffset, y);
							}
						}
						}break;
					default: printf("weird RemoteUIParam at draw()!\n"); break;
				}
				if(!linesInited){
					uiLines.addVertex(ofVec3f(x, y + spacing * 0.33));
					uiLines.addVertex(ofVec3f(x + realColW, y + spacing * 0.33));
				}
				y += spacing;
				if (y > screenH / uiScale - padding * 0.5 - bottomBarHeight){
					x += colw;
					y = initialY;
				}
			}
			dataMutex.unlock();
			ofSetColor(32);
			ofSetLineWidth(1);
			uiLines.draw();
		}
		
		//tiny clock top left
		if (uiAlpha < 1.0 && !showUIduringEdits){
			ofMesh m;
			int step = 20;
			m.setMode(OF_PRIMITIVE_TRIANGLE_FAN);
			ofVec3f origin = ofVec3f(charW * 1.5, lineH * 0.5); //rect is 22
			m.addVertex(origin);
			float r = lineH * 0.4;
			float ang;
			float lim = 360.0f * (1.0f - uiAlpha);
			for(ang = 0; ang < lim; ang += step){
				m.addVertex( origin + ofVec3f( r * cosf((-90.0f + ang) * DEG_TO_RAD),
											  r * sinf((-90.0f + ang) * DEG_TO_RAD)));
			}
			float lastBit = lim - ang;
			m.addVertex( origin + ofVec3f( r * cosf((-90.0f + ang + lastBit) * DEG_TO_RAD),
										  r * sinf((-90.0f + ang + lastBit) * DEG_TO_RAD)));
			ofSetColor(128);
			m.draw();
		}
		
		ofPopMatrix();
		
		//clearly draw the corners - helpful when in multi monitor setups to see if we are cropping
		float ts = 7 * uiScale;
		ofSetColor((frameNum * 20)%255);
		ofDrawTriangle(0, 0, ts, 0, 0, ts);
		ofDrawTriangle(screenW / uiScale, 0, screenW / uiScale - ts, 0, screenW / uiScale, ts);
		ofDrawTriangle(0, screenH / uiScale, ts, screenH / uiScale, 0, screenH / uiScale - ts);
		ofDrawTriangle(screenW / uiScale, screenH / uiScale, screenW / uiScale - ts, screenH / uiScale, screenW / uiScale, screenH / uiScale - ts);
	}

	if (needsToDrawNotification){
		if (drawNotifications){
			#if( defined(USE_OFX_TIME_MEASUREMENTS) && !defined(TIME_MEASUREMENTS_DISABLED))
			ofxTimeMeasurements * tm = TIME_SAMPLE_GET_INSTANCE();
			if (tm->getEnabled()) {
				if (tm->getDrawLocation() == TIME_MEASUREMENTS_BOTTOM_LEFT) {
					float scale = tm->getUiScale();
					ofTranslate(0, scale * (-tm->getHeight() - 10 - tm->getPlotsHeight()));
				}
				if (tm->getDrawLocation() == TIME_MEASUREMENTS_BOTTOM_RIGHT) {
					float scale = tm->getUiScale();
					ofTranslate(0, scale * (-10 - tm->getPlotsHeight()));
				}
			}
			#endif
			for(int i = 0; i < paramsToWatch.size(); i++){
				RemoteUIParam & p = params[paramsToWatch[i]];
				string v = p.getValueAsStringFromPointer();
				ofColor c = ofColor(p.r, p.g, p.b, 255);
				onScreenNotifications.addParamWatch(paramsToWatch[i], v, c);
			}

			ofColor transp = ofColor(0,0,0,0);
			for(auto & w : varWatches){ // add watches
				onScreenNotifications.addVariableWatch(w.first, w.second.getValueAsString(), (w.second.color == transp) ? ofColor(0, 190) : w.second.color );
			}
			onScreenNotifications.draw(x, y);
		}
	}
	if(needsToDrawNotification | showUI){
		ofPopMatrix();
		ofPopStyle();
	}
}
#endif

#ifdef OF_AVAILABLE

void ofxRemoteUIServer::setUiColumnWidth(int w){
	if(fabs(uiColumnWidth - w) < 0.1) uiLines.clear();
	uiColumnWidth = w;
}

void ofxRemoteUIServer::setBuiltInUiScale(float s){
	uiScale = s;
	#ifdef USE_OFX_FONTSTASH
	if(fontFile.size()){ //re-create font with higher uiscale
		drawUiWithFontStash(fontFile, fontSize);
	}
	#endif
	#ifdef USE_OFX_FONTSTASH2
	if(fontStashFile2.size()){ //re-create font with higher uiscale
		drawUiWithFontStash2(fontStashFile2, fontSize2);
	}
	#endif
	if (fabs(uiScale - s) < 0.01) uiLines.clear();
}


void ofxRemoteUIServer::setCustomScreenHeight(int h){
	if(fabs(customScreenHeight - h) < 0.1) uiLines.clear();
	customScreenHeight = h;
}


void ofxRemoteUIServer::setCustomScreenWidth(int w){
	if(fabs(customScreenWidth - w) < 0.1) uiLines.clear();
	customScreenWidth = w;
}

#endif

void ofxRemoteUIServer::handleBroadcast(){
	if(doBroadcast){
		if(broadcastTime > OFXREMOTEUI_BORADCAST_INTERVAL){
			broadcastTime = 0.0f;

			ofxOscMessage m;
			m.setAddress("/ServerBroadcast");
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


	#ifdef OF_AVAILABLE
	onScreenNotifications.update(dt);
	#endif

	if(!enabled) return;

	timeCounter += dt;
	broadcastTime += dt;
	timeSinceLastReply  += dt;

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

	#ifdef RUI_WEB_INTERFACE
    lock_guard<std::mutex> guard(wsDequeMut);
	#endif

	dataMutex.lock();

	while( oscReceiver.hasWaitingMessages() or wsMessages.size()){// check for waiting messages from client

		ofxOscMessage m;
		string remoteIP;
        if (useWebSockets && wsMessages.size()) {
            m = ofxOscMessage(wsMessages.front());
			remoteIP = m.getRemoteIp();
            wsMessages.pop_front();
        }
        else {
            oscReceiver.getNextMessage(m);
			remoteIP = m.getRemoteIp();
            if (!readyToSend ){ // if not connected, connect to our friend so we can talk back
                connect(remoteIP, port + 1);
            }
        }

		DecodedMessage dm = decode(m);
		RemoteUIServerCallBackArg cbArg; // to notify our "delegate"
		cbArg.host = remoteIP;
		switch (dm.action) {

			case HELO_ACTION: //if client says hi, say hi back
				sendHELLO();
				cbArg.action = CLIENT_CONNECTED;
				if(callBack) callBack(cbArg);
				#ifdef OF_AVAILABLE
				onScreenNotifications.addNotification("CONNECTED (" + cbArg.host +  ")!");
				ofNotifyEvent(clientAction, cbArg, this);
				#endif
				if(verbose_) RLOG_VERBOSE << remoteIP << " says HELLO!";

				break;

			case REQUEST_ACTION:{ //send all params to client
				if(verbose_) RLOG_VERBOSE << remoteIP << " sends REQU!";
				pushParamsToClient();
				}break;

			case SEND_PARAM_ACTION:{ //client is sending us an updated val
				if(verbose_) RLOG_VERBOSE << remoteIP << " sends SEND!";
				auto it = params.find(dm.paramName);
				if(it != params.end()){ //ignore param updates that talk about params we dont know about
					updateParamFromDecodedMessage(m, dm);
					cbArg.action = CLIENT_UPDATED_PARAM;
					cbArg.paramName = dm.paramName;
					cbArg.param = params[dm.paramName];  //copy the updated param to the callbakc arg
					cbArg.group = cbArg.param.group;
					if(callBack) callBack(cbArg);
					#ifdef OF_AVAILABLE
					RemoteUIParam & p = params[dm.paramName];
					onScreenNotifications.addParamUpdate(dm.paramName, p,
														 ofColor(p.r, p.g, p.b, p.a),
														 p.type == REMOTEUI_PARAM_COLOR ?
														 ofColor(p.redVal, p.greenVal, p.blueVal, p.alphaVal):
														 ofColor(0,0,0,0)
														 );
					ofNotifyEvent(clientAction, cbArg, this);
					#endif
				}else{
					RLOG_WARNING << "ignore param update msg, as \"" << dm.paramName << "\" is not a param we know about!";
				}
				}
				break;

			case CIAO_ACTION:{
                if (!useWebSockets){
                    sendCIAO();
                } else {
                    useWebSockets = false;
                }
				cbArg.action = CLIENT_DISCONNECTED;
				if(callBack) callBack(cbArg);
				#ifdef OF_AVAILABLE
				onScreenNotifications.addNotification("DISCONNECTED (" + cbArg.host +  ")!");
				ofNotifyEvent(clientAction, cbArg, this);
				#endif
				clearOscReceiverMsgQueue();
				readyToSend = false;
				if(verbose_) RLOG_VERBOSE << remoteIP << " says CIAO!" ;
			}break;

			case TEST_ACTION: // we got a request from client, lets bounce back asap.
				sendTEST();
				//if(verbose)RLOG_VERBOSE << "ofxRemoteUIServer: " << remoteIP << " says TEST!" ;
				break;

			case PRESET_LIST_ACTION: //client wants us to send a list of all available presets
				presetNames = getAvailablePresets();
				if (presetNames.size() == 0){
					presetNames.push_back(OFXREMOTEUI_NO_PRESETS);
				}
				sendPREL(presetNames);
				break;

			case SET_PRESET_ACTION:{ // client wants to set a preset
				string presetName = cleanCharsForFileSystem(m.getArgAsString(0));
				vector<string> missingParams = loadFromXML(string(OFXREMOTEUI_PRESET_DIR) + "/" + presetName + "." + OFXREMOTEUI_PRESET_FILE_EXTENSION);
				sendSETP(presetName);
				sendMISP(missingParams);
				cbArg.action = CLIENT_DID_SET_PRESET;
				cbArg.msg = presetName;
				if(callBack) callBack(cbArg);
				#ifdef OF_AVAILABLE
				onScreenNotifications.addNotification("SET PRESET to '" + getFinalPath(OFXREMOTEUI_PRESET_DIR) + "/" + presetName + ".xml'");
				ofNotifyEvent(clientAction, cbArg, this);
				#endif
				if(verbose_) RLOG_NOTICE << "setting preset: " << presetName ;
			}break;

			case SAVE_PRESET_ACTION:{ //client wants to save current xml as a new preset
				string presetName = cleanCharsForFileSystem(m.getArgAsString(0));
				saveToXML(string(OFXREMOTEUI_PRESET_DIR) + "/" + presetName + "." + OFXREMOTEUI_PRESET_FILE_EXTENSION);
				sendSAVP(presetName);
				cbArg.action = CLIENT_SAVED_PRESET;
				cbArg.msg = presetName;
				if(callBack) callBack(cbArg);
				#ifdef OF_AVAILABLE
				onScreenNotifications.addNotification("SAVED PRESET to '" + getFinalPath(OFXREMOTEUI_PRESET_DIR) + "/" + presetName + ".xml'");
				ofNotifyEvent(clientAction, cbArg, this);
				#endif
				if(verbose_) RLOG_NOTICE << "saving NEW preset: " << presetName ;
			}break;

			case DELETE_PRESET_ACTION:{
				string presetName = cleanCharsForFileSystem(m.getArgAsString(0));
				deletePreset(presetName);
				sendDELP(presetName);
				cbArg.action = CLIENT_DELETED_PRESET;
				cbArg.msg = presetName;
				if(callBack) callBack(cbArg);
				#ifdef OF_AVAILABLE
				onScreenNotifications.addNotification("DELETED PRESET '" + getFinalPath(OFXREMOTEUI_PRESET_DIR) + "/" + presetName + ".xml'");
				ofNotifyEvent(clientAction, cbArg, this);
				if(verbose_) RLOG_NOTICE << "DELETE preset: " << presetName ;
				#endif
			}break;

			case SAVE_CURRENT_STATE_ACTION:{
				saveToXML(OFXREMOTEUI_SETTINGS_FILENAME);
				cbArg.action = CLIENT_SAVED_STATE;
				if(callBack) callBack(cbArg);
				#ifdef OF_AVAILABLE
				onScreenNotifications.addNotification("SAVED CONFIG to default XML");
				ofNotifyEvent(clientAction, cbArg, this);
				#endif
				sendSAVE(true);
				if(verbose_) RLOG_NOTICE << "SAVE CURRENT PARAMS TO DEFAULT XML: " ;
			}break;

			case RESET_TO_XML_ACTION:{
				restoreAllParamsToInitialXML();
				sendRESX(true);
				cbArg.action = CLIENT_DID_RESET_TO_XML;
				#ifdef OF_AVAILABLE
				onScreenNotifications.addNotification("RESET CONFIG TO SERVER-LAUNCH XML values");
				ofNotifyEvent(clientAction, cbArg, this);
				#endif
				if(callBack)callBack(cbArg);
				if(verbose_) RLOG_NOTICE << "RESET TO XML: " ;
			}break;

			case RESET_TO_DEFAULTS_ACTION:{
				cbArg.action = CLIENT_DID_RESET_TO_DEFAULTS;
				restoreAllParamsToDefaultValues();
				sendRESD(true);
				#ifdef OF_AVAILABLE
				onScreenNotifications.addNotification("RESET CONFIG TO DEFAULTS (source defined values)");
				ofNotifyEvent(clientAction, cbArg, this);
				#endif
				if(callBack)callBack(cbArg);
				if(verbose_) RLOG_NOTICE << "RESET TO DEFAULTS: " ;
			}break;

			case SET_GROUP_PRESET_ACTION:{ // client wants to set a preset for a group
				string presetName = cleanCharsForFileSystem(m.getArgAsString(0));
				string groupName = cleanCharsForFileSystem(m.getArgAsString(1));
				vector<string> missingParams = loadFromXML(string(OFXREMOTEUI_PRESET_DIR) + "/" + groupName + "/" + presetName + "." + OFXREMOTEUI_PRESET_FILE_EXTENSION);
				vector<string> filtered;
				for(int i = 0; i < missingParams.size(); i++){
					if ( params[ missingParams[i] ].group == groupName ){
						filtered.push_back(missingParams[i]);
					}
				}
				sendSETp(presetName, groupName);
				sendMISP(filtered);
				cbArg.action = CLIENT_DID_SET_GROUP_PRESET;
				cbArg.msg = presetName;
				cbArg.group = groupName;
				if(callBack)callBack(cbArg);
				#ifdef OF_AVAILABLE
				onScreenNotifications.addNotification("SET '" + groupName + "' GROUP TO '" + presetName + ".xml' PRESET");
				ofNotifyEvent(clientAction, cbArg, this);
				#endif
				if(verbose_) RLOG_NOTICE << "setting preset group: " << groupName << "/" <<presetName ;
			}break;

			case SAVE_GROUP_PRESET_ACTION:{ //client wants to save current xml as a new preset
				string presetName = cleanCharsForFileSystem(m.getArgAsString(0));
				string groupName = cleanCharsForFileSystem(m.getArgAsString(1));
				saveGroupToXML(string(OFXREMOTEUI_PRESET_DIR) + "/" + groupName + "/" + presetName + "." + OFXREMOTEUI_PRESET_FILE_EXTENSION, groupName);
				sendSAVp(presetName, groupName);
				cbArg.action = CLIENT_SAVED_GROUP_PRESET;
				cbArg.msg = presetName;
				cbArg.group = groupName;
				#ifdef OF_AVAILABLE
				onScreenNotifications.addNotification("SAVED PRESET '" + presetName + ".xml' FOR GROUP '" + groupName + "'");
				ofNotifyEvent(clientAction, cbArg, this);
				#endif
				if(callBack) callBack(cbArg);
				if(verbose_) RLOG_NOTICE << "saving NEW preset: " << presetName ;
			}break;

			case DELETE_GROUP_PRESET_ACTION:{
				string presetName = cleanCharsForFileSystem(m.getArgAsString(0));
				string groupName = cleanCharsForFileSystem(m.getArgAsString(1));
				deletePreset(presetName, groupName);
				sendDELp(presetName, groupName);
				cbArg.action = CLIENT_DELETED_GROUP_PRESET;
				cbArg.msg = presetName;
				cbArg.group = groupName;
				#ifdef OF_AVAILABLE
				onScreenNotifications.addNotification("DELETED PRESET '" + presetName + ".xml' FOR GROUP'" + groupName + "'");
				ofNotifyEvent(clientAction, cbArg, this);
				#endif
				if(callBack)callBack(cbArg);
				if(verbose_) RLOG_NOTICE << "DELETE preset: " << presetName ;
			}break;
			default: RLOG_ERROR << "updateServer >> ERR!"; break;
		}
	}
	dataMutex.unlock();

}

void ofxRemoteUIServer::deletePreset(string name, string group){
	#ifdef OF_AVAILABLE
	if (group == ""){ //global preset
		string path = getFinalPath(OFXREMOTEUI_PRESET_DIR) + "/" + name + "." + OFXREMOTEUI_PRESET_FILE_EXTENSION;
		ofFile::removeFile(path);
	}else{
		string path = getFinalPath(OFXREMOTEUI_PRESET_DIR) + "/" + group + "/" + name + "." + OFXREMOTEUI_PRESET_FILE_EXTENSION;
		ofFile::removeFile(path);
	}
	#else //TODO this wont work, relative path
	string file = getFinalPath(OFXREMOTEUI_PRESET_DIR) + "/" + name + "." + OFXREMOTEUI_PRESET_FILE_EXTENSION;
	if (group != "") file = getFinalPath(OFXREMOTEUI_PRESET_DIR) + "/" + group + "/" + name + "." + OFXREMOTEUI_PRESET_FILE_EXTENSION;
	remove( file.c_str() );
	#endif
}


vector<string> ofxRemoteUIServer::getAvailablePresets(bool onlyGlobal){

	vector<string> presets;

	#ifdef OF_AVAILABLE
	ofDirectory dir;
	dir.listDir(ofToDataPath(getFinalPath(OFXREMOTEUI_PRESET_DIR)));
	vector<ofFile> files = dir.getFiles();
	for(int i = 0; i < files.size(); i++){
		string fileName = files[i].getFileName();
		string extension = files[i].getExtension();
		std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
		if (files[i].isFile() && (extension == "xml" || extension == OFXREMOTEUI_PRESET_FILE_EXTENSION)){
			string presetName = fileName.substr(0, fileName.size()-4);
			presets.push_back(presetName);
		}
		if (files[i].isDirectory() && !onlyGlobal){
			ofDirectory dir2;
			dir2.listDir( ofToDataPath( getFinalPath(OFXREMOTEUI_PRESET_DIR) + "/" + fileName) );
			vector<ofFile> files2 = dir2.getFiles();
			for(int j = 0; j < files2.size(); j++){
				string fileName2 = files2[j].getFileName();
				string extension2 = files2[j].getExtension();
				std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
				if (files2[j].isFile() && (extension2 == "xml" || extension2 == OFXREMOTEUI_PRESET_FILE_EXTENSION)){
					string presetName2 = fileName2.substr(0, fileName2.size()-4);
					presets.push_back(fileName + "/" + presetName2);
				}
			}
		}
	}
	#else
	DIR *dir2;
	struct dirent *ent;
	if ((dir2 = opendir(getFinalPath(OFXREMOTEUI_PRESET_DIR).c_str() )) != NULL) {
		while ((ent = readdir (dir2)) != NULL) {
			if ( strcmp( get_filename_ext(ent->d_name), OFXREMOTEUI_PRESET_FILE_EXTENSION) == 0 ){
				string fileName = string(ent->d_name);
				string presetName = fileName.substr(0, fileName.size()-4);
				presets.push_back(presetName);
			}
		}
		closedir(dir2);
	}
	#endif
	return presets;
}

vector<string>	ofxRemoteUIServer::getAvailablePresetsForGroup(string group){

	vector<string> presets;

	#ifdef OF_AVAILABLE
	ofDirectory dir;
	string path = ofToDataPath(getFinalPath(OFXREMOTEUI_PRESET_DIR) + "/" + group );
	if(ofDirectory::doesDirectoryExist(path)){
		dir.listDir(path);
		vector<ofFile> files = dir.getFiles();
		for(int i = 0; i < files.size(); i++){
			string fileName = files[i].getFileName();
			string extension = files[i].getExtension();
			std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
			if (files[i].isFile() && (extension == "xml" || extension == OFXREMOTEUI_PRESET_FILE_EXTENSION)){
				string presetName = fileName.substr(0, fileName.size()-4);
				presets.push_back(presetName);
			}
		}
	}
	#endif
	return presets;
}


void ofxRemoteUIServer::setColorForParam(RemoteUIParam &p, ofColor c){

	if (c.a > 0){ //if user supplied a color, override the setColor
		p.setBgColor(c);
	}else{
		if (colorSet){
			p.setBgColor(paramColorCurrentVariation);
		}
	}
}

void ofxRemoteUIServer::watchParamOnScreen(const string & paramName){
	dataMutex.lock();
	if (params.find(paramName) != params.end()){
		paramsToWatch.push_back(paramName);
	}else{
		RLOG_ERROR << "can't watch that param; it doesnt exist! " << paramName << endl;
	}
	dataMutex.unlock();
}

void ofxRemoteUIServer::removeParamWatch(const string & paramName){
	dataMutex.lock();
	if (params.find(paramName) != params.end()){
		auto it = std::find(paramsToWatch.begin(), paramsToWatch.end(), paramName);
		if(it != paramsToWatch.end()){
			paramsToWatch.erase(it);
			onScreenNotifications.removeParamWatch(paramName);
		}else{
			RLOG_ERROR << "can't remove watch for that param; it isn't being watched! " << paramName;
		}
	}else{
		RLOG_ERROR << "can't remove watch for that param; it doesnt exist! " << paramName;
	}
	dataMutex.unlock();
}

void ofxRemoteUIServer::removeAllParamWatches(){
	RLOG_NOTICE << "Removing all Param Watches";
	paramsToWatch.clear();
	onScreenNotifications.removeAllParamWatches();
}

void ofxRemoteUIServer::addParamToDB(const RemoteUIParam & p, string thisParamName){


	if(p.type != REMOTEUI_PARAM_SPACER && params.size() == 0){ //adding first param! and its not spacer!
		upcomingGroup = OFXREMOTEUI_DEFAULT_PARAM_GROUP;
		newColorInGroupCounter = 1;
		addSpacer(OFXREMOTEUI_DEFAULT_PARAM_GROUP);
	}

	if(loadedFromXML){ //lets see if we had loaded this param from xml - will upate its values if so
		auto it = params_removed.find(thisParamName);
		ofxRemoteUI::addParamToDB(p, thisParamName);
		dataMutex.lock();
		if(it != params_removed.end()){
			RemoteUIParam & pRem = params_removed[thisParamName];
			RemoteUIParam & xmlP = paramsFromXML[thisParamName];
			RemoteUIParam & srcP = params[thisParamName];
			if(pRem.type == srcP.type){
				switch (pRem.type) {
					case REMOTEUI_PARAM_FLOAT:
						if (srcP.floatValAddr){
							*(srcP.floatValAddr) = srcP.floatVal = pRem.floatVal;
							xmlP.floatValAddr = pRem.floatValAddr = srcP.floatValAddr;
						}
						xmlP.minFloat = srcP.minFloat; //the xml doesnt store ranges, but the param we are adding does! so we copy the other way in this case
						xmlP.maxFloat = srcP.maxFloat;
						break;
					case REMOTEUI_PARAM_ENUM:
					case REMOTEUI_PARAM_INT:
						if (srcP.intValAddr){
							*(srcP.intValAddr) = srcP.intVal = pRem.intVal;
							xmlP.intValAddr = pRem.intValAddr = srcP.intValAddr;
						}
						xmlP.maxInt = srcP.maxInt;
						xmlP.minInt = srcP.minInt;
						xmlP.enumList = srcP.enumList;
						break;

					case REMOTEUI_PARAM_COLOR:
						if (srcP.redValAddr){
							*srcP.redValAddr = srcP.redVal = pRem.redVal;
							*(srcP.redValAddr+1) = srcP.greenVal = pRem.greenVal;
							*(srcP.redValAddr+2) = srcP.blueVal = pRem.blueVal;
							*(srcP.redValAddr+3) = srcP.alphaVal = pRem.alphaVal;
							pRem.redValAddr = srcP.redValAddr;
						}break;

					case REMOTEUI_PARAM_BOOL:
						if (srcP.boolValAddr){
							pRem.boolValAddr = srcP.boolValAddr;
							*srcP.boolValAddr = srcP.boolVal = pRem.boolVal;
						}break;

					case REMOTEUI_PARAM_SPACER:
					case REMOTEUI_PARAM_STRING:
						if (srcP.stringValAddr){
							*srcP.stringValAddr = srcP.stringVal = pRem.stringVal;
							pRem.stringValAddr = srcP.stringValAddr;
						}break;
					default: break;
				}
				xmlP.r = srcP.r;
				xmlP.g = srcP.g;
				xmlP.b = srcP.b;
				xmlP.a = srcP.a;
				pRem.group = xmlP.group = srcP.group;
				if (verbose_) RLOG_NOTICE << "updating value of param \"" << thisParamName << "\" according to the previously loaded XML!";
			}
		}
		dataMutex.unlock();
	}else{
		ofxRemoteUI::addParamToDB(p, thisParamName);
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
	addParamToDB(p, title);
	if(verbose_) RLOG_NOTICE << "Adding Group '" << title << "' ######################################" ;
}


void ofxRemoteUIServer::shareParam(string paramName, float* param, float min, float max, ofColor c){
	RemoteUIParam p;
	p.type = REMOTEUI_PARAM_FLOAT;
	p.floatValAddr = param;
	p.maxFloat = max;
	p.minFloat = min;
	if (isnan(*param)) *param = 0.0f;
	p.floatVal = *param = ofClamp(*param, min, max);
	p.group = upcomingGroup;
	setColorForParam(p, c);
	addParamToDB(p, paramName);
	if(verbose_) RLOG_NOTICE << "Sharing Float Param '" << paramName << "'" ;
}


void ofxRemoteUIServer::shareParam(string paramName, bool* param, ofColor c, int nothingUseful ){
	RemoteUIParam p;
	p.type = REMOTEUI_PARAM_BOOL;
	p.boolValAddr = param;
	p.boolVal = *param;
	p.group = upcomingGroup;
	setColorForParam(p, c);
	addParamToDB(p, paramName);
	if(verbose_) RLOG_NOTICE << "Sharing Bool Param '" << paramName << "'" ;
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
	if(verbose_) RLOG_NOTICE << "Sharing Int Param '" << paramName << "'" ;
}

void ofxRemoteUIServer::shareParam(string paramName, int* param, int min, int max, vector<string> names, ofColor c ){
	if (names.size() != max - min + 1){
		RLOG_ERROR << "Error sharing enum param '" << paramName << "': Number of supplied strings doesnt match enum range!";
	}
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
	if(verbose_) RLOG_NOTICE << "Sharing Enum Param '" << paramName << "'" ;
}

void ofxRemoteUIServer::shareParam(string paramName, int* param, int min, int max, string* names, ofColor c ){
	RemoteUIParam p;
	p.type = REMOTEUI_PARAM_ENUM;
	p.intValAddr = param;
	p.maxInt = max;
	p.minInt = min;
	vector<string> list;
	for(int i = min; i <= max; i++){
		list.push_back(names[i - min]);
	}
	p.enumList = list;
	p.group = upcomingGroup;
	setColorForParam(p, c);
	p.intVal = *param = ofClamp(*param, min, max);
	addParamToDB(p, paramName);
	if(verbose_) RLOG_NOTICE << "Sharing Enum Param '" << paramName << "'" ;
}



void ofxRemoteUIServer::shareParam(string paramName, string* param, ofColor c, int nothingUseful ){
	RemoteUIParam p;
	p.type = REMOTEUI_PARAM_STRING;
	p.stringValAddr = param;
	p.stringVal = *param;
	p.group = upcomingGroup;
	setColorForParam(p, c);
	addParamToDB(p, paramName);
	if(verbose_) RLOG_NOTICE << "Sharing String Param '" << paramName << "'";
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
	if(verbose_) RLOG_NOTICE << "Sharing Color Param '" << paramName << "'";
}


void ofxRemoteUIServer::connect(string ipAddress, int port){
	avgTimeSinceLastReply = timeSinceLastReply = timeCounter = 0.0f;
	waitingForReply = false;
	//params.clear();
	oscSender.setup(ipAddress, port);
	readyToSend = true;
}

void ofxRemoteUIServer::sendLogToClient(const char* format, ...){

	if (strlen(format) >= 1024) {
		RLOG_ERROR << "log string must be under 1024 chars" << endl;
		return;
	}
	char line[1024];
	va_list args;
	va_start(args, format);
	vsprintf(line, format,  args);
	sendLogToClient(string(line));
}

void ofxRemoteUIServer::sendLogToClient(const string & message){
	if(readyToSend){
		ofxOscMessage m;
		m.setAddress("/LOG_");
		m.addStringArg(message);
		try{
			sendMessage(m);
		}catch(exception e){
			RLOG_ERROR << "Exception sendLogToClient " << e.what() ;
		}
	}
	RLOG_WARNING << "RUI_LOG(" + message + ")";
	onScreenNotifications.addLogLine(message, true);
}



void ofxRemoteUIServer::saveToXMLv1(string fileName){

	dataMutex.lock();
	saveSettingsBackup(); //every time , before we save

#ifdef OF_AVAILABLE
	fileName = getFinalPath(fileName);
#endif

	RLOG_NOTICE << "Saving to XML (using the OLD FORMAT) '" << fileName << "'" ;
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
	//for( unordered_map<string, RemoteUIParam>::iterator ii = params.begin(); ii != params.end(); ++ii ){
	for(int i = 0; i < orderedKeys.size(); i++){

		string key = orderedKeys[i];
		RemoteUIParam &t = params[key];
		saveParamToXmlSettings(t, key, s, counters);
	}

	s.popTag(); //pop OFXREMOTEUI_XML_TAG

	if(!portIsSet){
		s.setValue(OFXREMOTEUI_XML_PORT_TAG, port, 0);
	}
	s.saveFile(fileName);
	dataMutex.unlock();
}

vector<string> ofxRemoteUIServer::loadFromXMLv1(string fileName){

	RLOG_WARNING << "Loading XML " << fileName << " with older format. Be aware that it will be saved with the new format!";
	vector<string> loadedParams;
	ofxXmlSettings s;
	bool exists = s.loadFile(fileName);
	unordered_map<string, bool> readKeys; //to keep track of duplicated keys;

	if (exists){
		if( s.getNumTags(OFXREMOTEUI_XML_TAG) > 0 ){
			s.pushTag(OFXREMOTEUI_XML_TAG, 0);

			int numFloats = s.getNumTags(OFXREMOTEUI_FLOAT_PARAM_XML_TAG);
			for (int i=0; i< numFloats; i++){
				string paramName = s.getAttribute(OFXREMOTEUI_FLOAT_PARAM_XML_TAG, OFXREMOTEUI_PARAM_NAME_XML_KEY, OFXREMOTEUI_UNKNOWN_PARAM_NAME_XML_KEY, i);
				if (readKeys.find(paramName) == readKeys.end()){
					readKeys[paramName] = true;
					float val = s.getValue(OFXREMOTEUI_FLOAT_PARAM_XML_TAG, 0.0, i);
					unordered_map<string, RemoteUIParam>::iterator it = params.find(paramName);
					if ( it != params.end() ){  // found!
						loadedParams.push_back(paramName);
						if(params[paramName].floatValAddr != NULL){
							*params[paramName].floatValAddr = val;
							params[paramName].floatVal = val;
							*params[paramName].floatValAddr = ofClamp(*params[paramName].floatValAddr, params[paramName].minFloat, params[paramName].maxFloat);
							if(!paramsLoadedFromXML[paramName]){
								paramsFromXML[paramName] = params[paramName];
								paramsLoadedFromXML[paramName] = true;
							}
							if(verbose_) RLOG_VERBOSE << "loading a FLOAT '" << paramName <<"' (" << ofToString( *params[paramName].floatValAddr, 3) << ") from XML" ;
						}else{
							RLOG_ERROR << "ERROR at loading FLOAT (" << paramName << ")" ;
						}
					}else{
						RLOG_ERROR << "float param '" << paramName << "' defined in xml not found in DB!" ;
					}
				}else{
					RLOG_ERROR << "float param '" << paramName << "' defined twice in xml! Using first definition only" ;
				}
			}

			int numInts = s.getNumTags(OFXREMOTEUI_INT_PARAM_XML_TAG);
			for (int i=0; i< numInts; i++){
				string paramName = s.getAttribute(OFXREMOTEUI_INT_PARAM_XML_TAG, OFXREMOTEUI_PARAM_NAME_XML_KEY, OFXREMOTEUI_UNKNOWN_PARAM_NAME_XML_KEY, i);
				if (readKeys.find(paramName) == readKeys.end()){
					readKeys[paramName] = true;
					float val = s.getValue(OFXREMOTEUI_INT_PARAM_XML_TAG, 0, i);
					unordered_map<string, RemoteUIParam>::iterator it = params.find(paramName);
					if ( it != params.end() ){  // found!
						loadedParams.push_back(paramName);
						if(params[paramName].intValAddr != NULL){
							*params[paramName].intValAddr = val;
							params[paramName].intVal = val;
							*params[paramName].intValAddr = ofClamp(*params[paramName].intValAddr, params[paramName].minInt, params[paramName].maxInt);
							if(!paramsLoadedFromXML[paramName]){
								paramsFromXML[paramName] = params[paramName];
								paramsLoadedFromXML[paramName] = true;
							}
							if(verbose_) RLOG_VERBOSE << "loading an INT '" << paramName <<"' (" << (int) *params[paramName].intValAddr << ") from XML" ;
						}else{
							RLOG_ERROR << "ERROR at loading INT (" << paramName << ")" ;
						}
					}else{
						RLOG_ERROR << "int param '" <<paramName << "' defined in xml not found in DB!" ;
					}
				}else{
					RLOG_ERROR << "int param '" << paramName << "' defined twice in xml! Using first definition only" ;
				}
			}

			int numColors = s.getNumTags(OFXREMOTEUI_COLOR_PARAM_XML_TAG);
			for (int i=0; i< numColors; i++){
				string paramName = s.getAttribute(OFXREMOTEUI_COLOR_PARAM_XML_TAG, OFXREMOTEUI_PARAM_NAME_XML_KEY, "OFXREMOTEUI_UNKNOWN_PARAM_NAME_XML_KEY", i);
				if (readKeys.find(paramName) == readKeys.end()){
					readKeys[paramName] = true;
					s.pushTag(OFXREMOTEUI_COLOR_PARAM_XML_TAG, i);
					int r = s.getValue("R", 0);
					int g = s.getValue("G", 0);
					int b = s.getValue("B", 0);
					int a = s.getValue("A", 0);
					unordered_map<string, RemoteUIParam>::iterator it = params.find(paramName);
					if ( it != params.end() ){  // found!
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
							if(!paramsLoadedFromXML[paramName]){
								paramsFromXML[paramName] = params[paramName];
								paramsLoadedFromXML[paramName] = true;
							}
							if(verbose_) RLOG_VERBOSE << "loading a COLOR '" << paramName <<"' (" << (int)*params[paramName].redValAddr << " " << (int)*(params[paramName].redValAddr+1) << " " << (int)*(params[paramName].redValAddr+2) << " " << (int)*(params[paramName].redValAddr+3)  << ") from XML" ;
						}else{
							RLOG_ERROR << "ERROR at loading COLOR (" << paramName << ")" ;
						}
					}else{
						RLOG_WARNING << "color param '" <<paramName << "' defined in xml not found in DB!" ;
					}
					s.popTag();
				}else{
					RLOG_ERROR << "color param '" << paramName << "' defined twice in xml! Using first definition only" ;
				}
			}

			int numEnums = s.getNumTags(OFXREMOTEUI_ENUM_PARAM_XML_TAG);
			for (int i=0; i< numEnums; i++){
				string paramName = s.getAttribute(OFXREMOTEUI_ENUM_PARAM_XML_TAG, OFXREMOTEUI_PARAM_NAME_XML_KEY, OFXREMOTEUI_UNKNOWN_PARAM_NAME_XML_KEY, i);
				if (readKeys.find(paramName) == readKeys.end()){
					readKeys[paramName] = true;
					float val = s.getValue(OFXREMOTEUI_ENUM_PARAM_XML_TAG, 0, i);
					unordered_map<string, RemoteUIParam>::iterator it = params.find(paramName);
					if ( it != params.end() ){  // found!
						loadedParams.push_back(paramName);
						if(params[paramName].intValAddr != NULL){
							*params[paramName].intValAddr = val;
							params[paramName].intVal = val;
							*params[paramName].intValAddr = ofClamp(*params[paramName].intValAddr, params[paramName].minInt, params[paramName].maxInt);
							if(!paramsLoadedFromXML[paramName]){
								paramsFromXML[paramName] = params[paramName];
								paramsLoadedFromXML[paramName] = true;
							}
							if(verbose_) RLOG_VERBOSE << "loading an ENUM '" << paramName <<"' (" << (int) *params[paramName].intValAddr << ") from XML" ;
						}else{
							RLOG_ERROR << "ERROR at loading ENUM (" << paramName << ")" ;
						}
					}else{
						RLOG_WARNING << "enum param '" << paramName << "' defined in xml not found in DB!" ;
					}
				}else{
					RLOG_ERROR << "enum param '" << paramName << "' defined twice in xml! Using first definition only" ;
				}
			}


			int numBools = s.getNumTags(OFXREMOTEUI_BOOL_PARAM_XML_TAG);
			for (int i=0; i< numBools; i++){
				string paramName = s.getAttribute(OFXREMOTEUI_BOOL_PARAM_XML_TAG, OFXREMOTEUI_PARAM_NAME_XML_KEY, OFXREMOTEUI_UNKNOWN_PARAM_NAME_XML_KEY, i);
				if (readKeys.find(paramName) == readKeys.end()){
					readKeys[paramName] = true;
					float val = s.getValue(OFXREMOTEUI_BOOL_PARAM_XML_TAG, false, i);
					unordered_map<string, RemoteUIParam>::iterator it = params.find(paramName);
					if ( it != params.end() ){  // found!
						loadedParams.push_back(paramName);
						if(params[paramName].boolValAddr != NULL){
							*params[paramName].boolValAddr = val;
							params[paramName].boolVal = val;
							if(!paramsLoadedFromXML[paramName]){
								paramsFromXML[paramName] = params[paramName];
								paramsLoadedFromXML[paramName] = true;
							}
							if(verbose_) RLOG_VERBOSE << "loading a BOOL '" << paramName <<"' (" << (bool) *params[paramName].boolValAddr << ") from XML" ;
						}else{
							RLOG_ERROR << "ERROR at loading BOOL (" << paramName << ")" ;
						}
					}else{
						RLOG_WARNING << "bool param '" << paramName << "' defined in xml not found in DB!" ;
					}
				}else{
					RLOG_ERROR << "bool param '" << paramName << "' defined twice in xml! Using first definition only" ;
				}
			}

			int numStrings = s.getNumTags(OFXREMOTEUI_STRING_PARAM_XML_TAG);
			for (int i=0; i< numStrings; i++){
				string paramName = s.getAttribute(OFXREMOTEUI_STRING_PARAM_XML_TAG, OFXREMOTEUI_PARAM_NAME_XML_KEY, OFXREMOTEUI_UNKNOWN_PARAM_NAME_XML_KEY, i);
				if (readKeys.find(paramName) == readKeys.end()){
					readKeys[paramName] = true;
					string val = s.getValue(OFXREMOTEUI_STRING_PARAM_XML_TAG, "", i);
					unordered_map<string, RemoteUIParam>::iterator it = params.find(paramName);
					if ( it != params.end() ){  // found!
						loadedParams.push_back(paramName);
						if(params[paramName].stringValAddr != NULL){
							params[paramName].stringVal = val;
							*params[paramName].stringValAddr = val;
							if(!paramsLoadedFromXML[paramName]){
								paramsFromXML[paramName] = params[paramName];
								paramsLoadedFromXML[paramName] = true;
							}
							if(verbose_) RLOG_VERBOSE << "loading a STRING '" << paramName <<"' (" << (string) *params[paramName].stringValAddr << ") from XML" ;
						}
						else RLOG_ERROR << "ERROR at loading STRING (" << paramName << ")" ;
					}else{
						RLOG_WARNING << "string param '" << paramName << "' defined in xml not found in DB!" ;
					}
				}else{
					RLOG_ERROR << "string param '" << paramName << "' defined twice in xml! Using first definition only" ;
				}
			}
		}
	}

	vector<string> paramsNotInXML;
	for( unordered_map<string, RemoteUIParam>::iterator ii = params.begin(); ii != params.end(); ++ii ){

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

string ofxRemoteUIServer::cleanCharsForFileSystem(const string & s){

	string r = s;
	//remove some chars that will confuse the fileSystem
	std::replace( r.begin(), r.end(), ':', '_');
	std::replace( r.begin(), r.end(), '/', '_');
	std::replace( r.begin(), r.end(), '\\', '_');
	std::replace( r.begin(), r.end(), '<', '_');
	std::replace( r.begin(), r.end(), '>', '_');
	std::replace( r.begin(), r.end(), '|', '_');
	return r;
}

void ofxRemoteUIServer::saveGroupToXMLv1(string filePath, string groupName){

	RLOG_NOTICE << "saving group to xml '" << filePath << "'" ;
	ofxXmlSettings s;
	s.loadFile(filePath);
	s.clear();
	s.addTag(OFXREMOTEUI_XML_TAG);
	s.pushTag(OFXREMOTEUI_XML_TAG);
	XmlCounter counters;

	for( unordered_map<string, RemoteUIParam>::iterator ii = params.begin(); ii != params.end(); ++ii ){
		string key = (*ii).first;
		RemoteUIParam t = params[key];
		if( t.group != OFXREMOTEUI_DEFAULT_PARAM_GROUP && t.group == groupName ){
			saveParamToXmlSettings(t, key, s, counters);
		}
	}
	s.saveFile(filePath);
}


void ofxRemoteUIServer::addVariableWatch(const string & varName, float* varPtr, ofColor c){
	RemoteUIServerValueWatch w;
	w.type = REMOTEUI_PARAM_FLOAT;
	w.floatAddress = varPtr;
	if(c.r == 0 && c.g == 0 && c.b == 0 && c.a == 0){
		c = ofColor::magenta;
	}
	w.color = c;
	varWatches[varName] = w;
	RLOG_NOTICE << "addVariableWatch() - added a watch for var named \"" << varName << "\"";
}


void ofxRemoteUIServer::addParamToPresetLoadIgnoreList(const std::string & param){
	paramsToIgnoreWhenLoadingPresets.push_back(param);
}

bool ofxRemoteUIServer::paramIsInPresetLoadIgnoreList(const std::string & param){
	auto it = std::find(paramsToIgnoreWhenLoadingPresets.begin(), paramsToIgnoreWhenLoadingPresets.end(), param);
	return it != paramsToIgnoreWhenLoadingPresets.end();
}


void ofxRemoteUIServer::removeParamFromPresetLoadIgnoreList(const std::string & param){
	auto it = std::find(paramsToIgnoreWhenLoadingPresets.begin(), paramsToIgnoreWhenLoadingPresets.end(), param);
	if(it != paramsToIgnoreWhenLoadingPresets.end()){
		paramsToIgnoreWhenLoadingPresets.erase(it);
	}
}

void ofxRemoteUIServer::clearParamToPresetLoadIgnoreList(){
	paramsToIgnoreWhenLoadingPresets.clear();
}


void ofxRemoteUIServer::addVariableWatch(const string & varName, int* varPtr, ofColor c){
	RemoteUIServerValueWatch w;
	w.type = REMOTEUI_PARAM_INT;
	w.intAddress = varPtr;
	if(c.r == 0 && c.g == 0 && c.b == 0 && c.a == 0){
		c = ofColor::magenta;
	}
	w.color = c;
	varWatches[varName] = w;
	RLOG_NOTICE << "addVariableWatch() - added a watch for var named \"" << varName << "\"";
}


void ofxRemoteUIServer::addVariableWatch(const string & varName, bool* varPtr, ofColor c){
	RemoteUIServerValueWatch w;
	w.type = REMOTEUI_PARAM_BOOL;
	w.boolAddress = varPtr;
	if(c.r == 0 && c.g == 0 && c.b == 0 && c.a == 0){
		c = ofColor::magenta;
	}
	w.color = c;
	varWatches[varName] = w;
	RLOG_NOTICE << "addVariableWatch() - added a watch for var named \"" << varName << "\"";
}

/*
void ofxRemoteUIServer::removeVariableWatch(const string &varName){

	auto find = varWatches.find(varName);
	if(find == varWatches.end()){
		RLOG_ERROR << "Can't removeVariableWatch() - var not found! : " << varName;
	}else{
		RLOG_NOTICE << "removeVariableWatch() - removed var '" << varName << "' from the watch list!";
		varWatches.erase(varName);
	}
}
*/


void ofxRemoteUIServer::onShowParamUpdateNotification(ScreenNotifArg& a){

	onScreenNotifications.addParamUpdate(a.paramName, a.p,
										 ofColor(a.p.r, a.p.g, a.p.b, a.p.a),
										 a.p.type == REMOTEUI_PARAM_COLOR ?
										 ofColor(a.p.redVal, a.p.greenVal, a.p.blueVal, a.p.alphaVal) :
										 ofColor(0,0,0,0)
										 );

}


#ifdef RUI_WEB_INTERFACE
void ofxRemoteUIServer::startWebServer(int _port) {
    webPort = _port;
    webServer.setup(_port);
    webServer.start();
}
#endif

void ofxRemoteUIServer::sendMessage(ofxOscMessage m) {
        if (useWebSockets){
            #ifdef RUI_WEB_INTERFACE
            string json = oscToJson(m);
            wsServer.send(json);
            #endif
        }
        else {
            oscSender.sendMessage(m);
        }
}


#ifdef RUI_WEB_INTERFACE

void ofxRemoteUIServer::listenWebSocket(int port) {
    ofxLibwebsockets::ServerOptions options = ofxLibwebsockets::defaultServerOptions();
    options.port = port;
    wsPort = port;
    wsServer.addListener(this);
    bool success = wsServer.setup( options );
    if (success)
        RLOG_NOTICE << "WebSocket server running on port " << port;
    else
        RLOG_NOTICE << "WebSocket server setup failed.";
    
}

string ofxRemoteUIServer::oscToJson(ofxOscMessage m) {
    std::string json = " { \"addr\": \"" + m.getAddress() + "\", \"args\" : [ ";
    
    int argc = m.getNumArgs();
    for (int i = 0; i < argc; i++) {
        switch(m.getArgType(i)) {
            case OFXOSC_TYPE_INT32:
            case OFXOSC_TYPE_INT64:
                json += to_string(m.getArgAsInt(i));
                break;
            case OFXOSC_TYPE_FLOAT:
                json += to_string(m.getArgAsFloat(i));
                break;
            case OFXOSC_TYPE_DOUBLE:
                json += to_string(m.getArgAsDouble(i));
                break;
            case OFXOSC_TYPE_STRING:
                json += "\"" + m.getArgAsString(i) + "\"";
                break;
            case OFXOSC_TYPE_CHAR:
                json += "\"" + to_string(m.getArgAsChar(i)) + "\"";
                break;
            case OFXOSC_TYPE_TRUE:
            case OFXOSC_TYPE_FALSE:
                json += to_string(m.getArgAsBool(i));
                break;
            case OFXOSC_TYPE_RGBA_COLOR:
                json += to_string(m.getArgAsRgbaColor(i));
                break;
            default: break;
        }
        if ( i < (argc - 1))
            json += ",";
    }
    json += "] }";

    return json;
    
}

ofxOscMessage ofxRemoteUIServer::jsonToOsc(Json::Value json){
    ofxOscMessage m;
    m.setAddress(json.get("addr", "NONE").asString());
    
    Json::Value argv = json.get("args", Json::nullValue);
    int argc = argv.size();
    
    for (int i = 0; i < argc; i++) {
        
        Json::Value arg = argv.get(i, Json::nullValue);
        
        switch (arg.type()) {
                
            case Json::intValue:
            case Json::uintValue:
                m.addIntArg(arg.asInt());
                break;
                
            case Json::realValue:
                m.addFloatArg(arg.asFloat());
                break;
                
            case Json::stringValue:
                m.addStringArg(arg.asString());
                break;
                
            case Json::booleanValue:
                m.addBoolArg(arg.asBool());
                break;
                
            default:
                RLOG_NOTICE << "Possibly malformed JSON message";
                break;
        }
    }
    
    return m;
    
}
//-----------------------------------------------
//              WebSocket Events
//-----------------------------------------------


void ofxRemoteUIServer::onConnect( ofxLibwebsockets::Event& args ) {
    readyToSend = true;
    useWebSockets = true;
}

void ofxRemoteUIServer::onOpen( ofxLibwebsockets::Event& args ){
    readyToSend = true;
    useWebSockets = true;
}
void ofxRemoteUIServer::onClose( ofxLibwebsockets::Event& args ){
    ofxOscMessage m;
    m.setAddress("/CIAO");
    m.setRemoteEndpoint(args.conn.getClientIP(), wsPort);
    wsDequeMut.lock();
    wsMessages.push_back(m);
    wsDequeMut.unlock();
}

void ofxRemoteUIServer::onMessage( ofxLibwebsockets::Event& args ){
//    ofLogNotice() << "Got WS message " << args.json;
    ofxOscMessage m = jsonToOsc(args.json);
    m.setRemoteEndpoint(args.conn.getClientIP(), wsPort);
    wsDequeMut.lock();
    wsMessages.push_back(m);
    wsDequeMut.unlock();
}

void ofxRemoteUIServer::onIdle( ofxLibwebsockets::Event& args ){}
void ofxRemoteUIServer::onBroadcast( ofxLibwebsockets::Event& args ){}

#endif
