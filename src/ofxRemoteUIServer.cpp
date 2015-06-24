//
//  ofxRemoteUI.cpp
//  emptyExample
//
//  Created by Oriol Ferrer Mesià on 09/01/13.
//
//


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

#include "ofxRemoteUIServer.h"

#ifdef OF_AVAILABLE
#include <Poco/Path.h>
#include <Poco/Environment.h>
#include <Poco/Process.h>
#include <Poco/Util/Application.h>
using Poco::Util::Application;
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

	#ifdef USE_OFX_FONTSTASH
	useFontStash = false;
	#endif
	xOffset = xOffsetTarget = 0.0f;
	selectedColorComp = 0;
	uiColumnWidth = 320;
	uiAlpha = 1.0f;
	selectedPreset = selectedGroupPreset = 0;
	selectedItem = -1;
	ofSeedRandom(1979);

	int numHues = 9;
	for(int i = 0; i < numHues; i++){
		float hue = fmod( 0 + i * ( 255.0f / float(numHues)), 255.0f );
		ofColor c = ofColor::fromHsb(hue, 255.0f, 230.0f, BG_COLOR_ALPHA);
		colorTables.push_back( c );
	}
	uiLines.setMode(OF_PRIMITIVE_LINES);
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
		RUI_LOG_NOTICE << "closing; waiting for update thread to end..." ;
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

	unordered_map<string, RemoteUIParam>::iterator it = params.find(paramName);

	if (it != params.end()){

		if(verbose_) RUI_LOG_WARNING << "removing Param '" << paramName << "' from DB!" ;

		//keep it in the removed struct
		params_removed[paramName] = it->second;
		orderedKeys_removed[orderedKeys_removed.size()] = paramName;

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
		RUI_LOG_ERROR << "removeParamFromDB >> trying to delete an unexistant param (" << paramName << ")" ;
	}
}


void ofxRemoteUIServer::setDirectoryPrefix(string _directoryPrefix){
	directoryPrefix = _directoryPrefix;
	RUI_LOG_NOTICE << "directoryPrefix set to '" << directoryPrefix << "'";
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
		case REMOTEUI_PARAM_FLOAT:
			if(verbose_) RUI_LOG_NOTICE << "saving '" << key << "' (" <<  *t.floatValAddr <<") to XML" ;
			s.setValue(OFXREMOTEUI_FLOAT_PARAM_XML_TAG, (double)*t.floatValAddr, c.numFloats);
			s.setAttribute(OFXREMOTEUI_FLOAT_PARAM_XML_TAG, OFXREMOTEUI_PARAM_NAME_XML_KEY, key, c.numFloats);
			c.numFloats++;
			break;
		case REMOTEUI_PARAM_INT:
			if(verbose_) RUI_LOG_NOTICE << "saving '" << key << "' (" <<  *t.intValAddr <<") to XML" ;
			s.setValue(OFXREMOTEUI_INT_PARAM_XML_TAG, (int)*t.intValAddr, c.numInts);
			s.setAttribute(OFXREMOTEUI_INT_PARAM_XML_TAG, OFXREMOTEUI_PARAM_NAME_XML_KEY, key, c.numInts);
			c.numInts++;
			break;
		case REMOTEUI_PARAM_COLOR:
			if(verbose_) RUI_LOG_NOTICE << "saving '" << key << "' (" << (int)*t.redValAddr << " " << (int)*(t.redValAddr+1) << " " << (int)*(t.redValAddr+2) << " " << (int)*(t.redValAddr+3) << ") to XML" ;
			s.setValue(string(OFXREMOTEUI_COLOR_PARAM_XML_TAG) + ":R", (int)*t.redValAddr, c.numColors);
			s.setValue(string(OFXREMOTEUI_COLOR_PARAM_XML_TAG) + ":G", (int)*(t.redValAddr+1), c.numColors);
			s.setValue(string(OFXREMOTEUI_COLOR_PARAM_XML_TAG) + ":B", (int)*(t.redValAddr+2), c.numColors);
			s.setValue(string(OFXREMOTEUI_COLOR_PARAM_XML_TAG) + ":A", (int)*(t.redValAddr+3), c.numColors);
			s.setAttribute(OFXREMOTEUI_COLOR_PARAM_XML_TAG, OFXREMOTEUI_PARAM_NAME_XML_KEY, key, c.numColors);
			c.numColors++;
			break;
		case REMOTEUI_PARAM_ENUM:
			if(verbose_) RUI_LOG_NOTICE << "saving '" << key << "' (" <<  *t.intValAddr <<") to XML" ;
			s.setValue(OFXREMOTEUI_ENUM_PARAM_XML_TAG, (int)*t.intValAddr, c.numEnums);
			s.setAttribute(OFXREMOTEUI_ENUM_PARAM_XML_TAG, OFXREMOTEUI_PARAM_NAME_XML_KEY, key, c.numEnums);
			c.numEnums++;
			break;
		case REMOTEUI_PARAM_BOOL:
			if(verbose_) RUI_LOG_NOTICE << "saving '" << key << "' (" <<  *t.boolValAddr <<") to XML" ;
			s.setValue(OFXREMOTEUI_BOOL_PARAM_XML_TAG, (bool)*t.boolValAddr, c.numBools);
			s.setAttribute(OFXREMOTEUI_BOOL_PARAM_XML_TAG, OFXREMOTEUI_PARAM_NAME_XML_KEY, key, c.numBools);
			c.numBools++;
			break;
		case REMOTEUI_PARAM_STRING:
			if(verbose_) RUI_LOG_NOTICE << "saving '" << key << "' (" <<  *t.stringValAddr <<") to XML" ;
			s.setValue(OFXREMOTEUI_STRING_PARAM_XML_TAG, (string)*t.stringValAddr, c.numStrings);
			s.setAttribute(OFXREMOTEUI_STRING_PARAM_XML_TAG, OFXREMOTEUI_PARAM_NAME_XML_KEY, key, c.numStrings);
			c.numStrings++;
			break;

		case REMOTEUI_PARAM_SPACER:
			if(verbose_) RUI_LOG_NOTICE << "skipping save of spacer '" << key << "' to XML" ;
			break;

		default:
			break;
	}
}

#ifdef OF_AVAILABLE
void ofxRemoteUIServer::saveParamToXmlSettings(const RemoteUIParam& t, string key, ofXml & s, int index, bool active){

	string path = "P[" + ofToString(index)  + "]";
	switch (t.type) {
		case REMOTEUI_PARAM_FLOAT:
			if(verbose_) RUI_LOG_NOTICE << "saving '" << key << "' (" <<  *t.floatValAddr <<") to XML" ;
			s.addValue("P", *t.floatValAddr);
			s.setTo(path);
			s.setAttribute("type", "float");
			break;
		case REMOTEUI_PARAM_INT:
			if(verbose_) RUI_LOG_NOTICE << "saving '" << key << "' (" <<  *t.intValAddr <<") to XML" ;
			s.addValue("P", *t.intValAddr);
			s.setTo(path);
			s.setAttribute("type", "int");
			break;
		case REMOTEUI_PARAM_COLOR:
			if(verbose_) RUI_LOG_NOTICE << "saving '" << key << "' (" << (int)*t.redValAddr << " " << (int)*(t.redValAddr+1) << " " << (int)*(t.redValAddr+2) << " " << (int)*(t.redValAddr+3) << ") to XML" ;
			s.addChild("P");
			s.setTo(path);
			s.setAttribute("type", "color");
			s.setAttribute("c0.red", ofToString((int)*t.redValAddr));
			s.setAttribute("c1.green", ofToString((int)*(t.redValAddr + 1)));
			s.setAttribute("c2.blue", ofToString((int)*(t.redValAddr + 2)));
			s.setAttribute("c3.alpha", ofToString((int)*(t.redValAddr + 3)));
			break;
		case REMOTEUI_PARAM_ENUM:
			if(verbose_) RUI_LOG_NOTICE << "saving '" << key << "' (" <<  *t.intValAddr <<") to XML" ;
			s.addValue("P", *t.intValAddr);
			s.setTo(path);
			s.setAttribute("type", "enum");
			break;
		case REMOTEUI_PARAM_BOOL:
			if(verbose_) RUI_LOG_NOTICE << "saving '" << key << "' (" <<  *t.boolValAddr <<") to XML" ;
			s.addValue("P", *t.boolValAddr);
			s.setTo(path);
			s.setAttribute("type", "bool");
			break;
		case REMOTEUI_PARAM_STRING:
			if(verbose_) RUI_LOG_NOTICE << "saving '" << key << "' (" <<  *t.stringValAddr <<") to XML" ;
			s.addValue("P", *t.stringValAddr);
			s.setTo(path);
			s.setAttribute("type", "string");
			break;
		case REMOTEUI_PARAM_SPACER:
			if(verbose_) RUI_LOG_NOTICE << "save spacer '" << key << "' to XML" ;
			s.addChild("P");
			s.setTo(path);
			s.setAttribute("type", "group");
			Poco::XML::Element *el = s.getPocoElement();
			Poco::XML::Comment *ce = s.getPocoDocument()->createComment(Poco::XML::toXMLString(" # " + ofToString(key) + " ############################ "));
			el->parentNode()->appendChild((Poco::XML::Node*)ce);
			break;
	}

	s.setAttribute("name", key);
	if(!active){
		s.setAttribute("disabled", "1");
	}
	s.setToParent();
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

	RUI_LOG_NOTICE << "saving to xml (using the V2 format) '" << fileName << "'" ;
	ofXml s;

	s.addChild(OFXREMOTEUI_XML_ROOT_TAG);
	s.setTo(OFXREMOTEUI_XML_ROOT_TAG); //get into root

	s.addValue(OFXREMOTEUI_XML_V_TAG, OFXREMOTEUI_XML_FORMAT_VER); //v2

	s.addChild(OFXREMOTEUI_XML_TAG); //add and dive into OFX_REMOTE_UI_PARAMS
	s.setTo(OFXREMOTEUI_XML_TAG);

	//save all params
	int numSaved = 0;
	for(int i = 0; i < orderedKeys.size(); i++){
		string key = orderedKeys[i];
		RemoteUIParam &t = params[key];
		bool save = false;
		if(savingGroupOnly){
			if( t.group != OFXREMOTEUI_DEFAULT_PARAM_GROUP && t.group == groupName ){
				save = true;
			}
		}else{
			save = true;
		}
		if(save){
			saveParamToXmlSettings(t, key, s, numSaved, true);
			numSaved++;
		}
	}

	if(!savingGroupOnly){ //group presets dont save disabled stuff
		//add comment separating enabled params from disabled params
		if(orderedKeys_removed.size()){
			Poco::XML::Element *el = s.getPocoElement();
			Poco::XML::Comment *ce = s.getPocoDocument()->createComment(Poco::XML::toXMLString("############################## DISABLED PARAMS ################################# "));
			el->appendChild((Poco::XML::Node*)ce);
		}
		//save removed params
		for(int i = 0; i < orderedKeys_removed.size(); i++){
			string key = orderedKeys_removed[i];
			RemoteUIParam &t = params_removed[key];
			saveParamToXmlSettings(t, key, s, i + orderedKeys.size(), false);
		}
	}

	s.setToParent(); //up 1 level
	if(!savingGroupOnly){ //group presets dont embed port
		s.addValue(OFXREMOTEUI_XML_PORT_TAG, port);
	}
	s.addValue(OFXREMOTEUI_XML_ENABLED_TAG, enabled);
	s.save(fileName );
}
#endif

vector<string> ofxRemoteUIServer::loadFromXML(string fileName){

	vector<string> empty;
	#ifdef OF_AVAILABLE
	fileName = getFinalPath(fileName);
	#endif

	ofxXmlSettings s;
	bool exists = s.loadFile(fileName);
	bool newVersion = s.getNumTags(string(OFXREMOTEUI_XML_ROOT_TAG) + ":" + string(OFXREMOTEUI_XML_V_TAG)) > 0;

	if (exists){
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
	}
	return empty;
}

#ifdef OF_AVAILABLE
vector<string> ofxRemoteUIServer::loadFromXMLv2(string fileName){

	vector<string> loadedParams;
	unordered_map<string, bool> readKeys; //to keep track of duplicated keys;

	ofXml s;
	s.load(fileName);
	s.setTo(OFXREMOTEUI_XML_ROOT_TAG);
	s.setTo(OFXREMOTEUI_XML_TAG);

	int numc = s.getNumChildren();

	for(int i = 0; i < numc; i++){

		s.setToChild(i);

		string paramName = s.getAttribute("name");
		string type = s.getAttribute("type");
		bool inactive = s.getAttribute("disabled") == "1";

		unordered_map<string, RemoteUIParam>::iterator it = params.find(paramName);
		bool isAParamWeKnowOf = it != params.end();

		if(isAParamWeKnowOf){

			if (readKeys.find(paramName) == readKeys.end()){ //lets not read keys twice, only read the first one we find in xml

				if(type.length() >	0){

					unordered_map<string, RemoteUIParam>::iterator it = params.find(paramName);

					if ( it != params.end() ){	// found!

						readKeys[paramName] = true;
						loadedParams.push_back(paramName);
						RemoteUIParam &p = params[paramName];

						switch (type[0]){

							case 'f':{ //float
								if(p.type != REMOTEUI_PARAM_FLOAT){ RUI_LOG_ERROR << "type missmatch parsing '" << paramName << "'. Ignoring it!"; break;}
								float val = ofClamp(s.getFloatValue(), p.minFloat, p.maxFloat);
								p.floatVal = *p.floatValAddr = val;
								if(verbose_) RUI_LOG_NOTICE << "loading a FLOAT '" << paramName <<"' (" << ofToString( *p.floatValAddr, 3) << ") from XML" ;
								}break;

							case 'i':{ //int
								if(p.type != REMOTEUI_PARAM_INT){ RUI_LOG_ERROR << "type missmatch parsing '" << paramName << "'. Ignoring it!"; break;}
								int val = ofClamp(s.getIntValue(), p.minInt, p.maxInt);
								p.intVal = *p.intValAddr = val;
								if(verbose_) RUI_LOG_NOTICE << "loading an INT '" << paramName <<"' (" << (int) *p.intValAddr << ") from XML" ;
								}break;

							case 's':{ //string
								if(p.type != REMOTEUI_PARAM_STRING){ RUI_LOG_ERROR << "type missmatch parsing '" << paramName << "'. Ignoring it!"; break;}
								string val = s.getValue();
								p.stringVal = *p.stringValAddr = val;
								if(verbose_) RUI_LOG_NOTICE << "loading a STRING '" << paramName <<"' (" << (string) *p.stringValAddr << ") from XML" ;
								}break;

							case 'e':{ //enum
								if(p.type != REMOTEUI_PARAM_ENUM){ RUI_LOG_ERROR << "type missmatch parsing '" << paramName << "'. Ignoring it!"; break;}
								int val = ofClamp(s.getIntValue(), p.minInt, p.maxInt);
								p.intVal = *p.intValAddr = val;
								if(verbose_) RUI_LOG_NOTICE << "loading an ENUM '" << paramName <<"' (" << (int) *p.intValAddr << ") from XML" ;
								}break;

							case 'b':{ //bool
								if(p.type != REMOTEUI_PARAM_BOOL){ RUI_LOG_ERROR << "type missmatch parsing '" << paramName << "'. Ignoring it!"; break;}
								bool val = s.getIntValue();
								p.boolVal = *p.boolValAddr = val;
								if(verbose_) RUI_LOG_NOTICE << "loading a BOOL '" << paramName <<"' (" << (bool) *p.boolValAddr << ") from XML" ;
								}break;

							case 'g':{ //group
								if(verbose_) RUI_LOG_NOTICE << "skipping GROUP '" << paramName << "' from XML" ;
							}break;


							case 'c':{ //color
								if(p.type != REMOTEUI_PARAM_COLOR){ RUI_LOG_ERROR << "type missmatch parsing '" << paramName << "'. Ignoring it!"; break;}
								unsigned char r = ofToInt(s.getAttribute("c0.red"));
								unsigned char g = ofToInt(s.getAttribute("c1.green"));
								unsigned char b = ofToInt(s.getAttribute("c2.blue"));
								unsigned char a = ofToInt(s.getAttribute("c3.alpha"));
								if(p.redValAddr != NULL){
									*p.redValAddr = p.redVal = r;
									*(p.redValAddr + 1) = p.greenVal = g;
									*(p.redValAddr + 2) = p.blueVal = b;
									*(p.redValAddr + 3) = p.alphaVal = a;
									if(verbose_) RUI_LOG_NOTICE << "loading a COLOR '" << paramName <<"' (" << (int)*p.redValAddr << " " << (int)*(p.redValAddr + 1) << " " << (int)*(p.redValAddr + 2) << " " << (int)*(p.redValAddr + 3)  << ") from XML" ;
								}else{
									RUI_LOG_ERROR << "ERROR at loading COLOR (" << paramName << ")" ;
								}
								}break;
							}

						//params[paramName] = p;
					}
					if(!loadedFromXML) paramsFromXML[paramName] = params[paramName];
				}
			}
		}else{ //params not defined from code! ignore!
			RUI_LOG_VERBOSE << "ignoring param '" << paramName << "' defined in xml but found in DB!" ;
		}
		s.setToParent();
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
#endif


void ofxRemoteUIServer::restoreAllParamsToInitialXML(){
	for( unordered_map<string, RemoteUIParam>::iterator ii = params.begin(); ii != params.end(); ++ii ){
		string key = (*ii).first;
		if (params[key].type != REMOTEUI_PARAM_SPACER){
			if (paramsFromXML.find(key) != paramsFromXML.end()){
				params[key] = paramsFromXML[key];
				syncPointerToParam(key);
			}
		}
	}
}

void ofxRemoteUIServer::restoreAllParamsToDefaultValues(){
	for( unordered_map<string, RemoteUIParam>::iterator ii = params.begin(); ii != params.end(); ++ii ){
		string key = (*ii).first;
		params[key] = paramsFromCode[key];
		syncPointerToParam(key);
	}
}

void ofxRemoteUIServer::pushParamsToClient(){

	vector<string>changedParams =  scanForUpdatedParamsAndSync();
	#ifdef OF_AVAILABLE
	for(int i = 0 ; i < changedParams.size(); i++){
		string pName = changedParams[i];
		RemoteUIParam &p = params[pName];
		onScreenNotifications.addParamUpdate(pName, p.getValueAsString(),
											 ofColor(p.r, p.g, p.b, p.a),
			p.type == REMOTEUI_PARAM_COLOR ?
			ofColor(p.redVal, p.greenVal, p.blueVal, p.alphaVal) :
			ofColor(0,0,0,0)
			);

	}
	#endif
	
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

string ofxRemoteUIServer::getFinalPath(string p){

	if(directoryPrefix.size()){
		stringstream ss;
		ss << directoryPrefix << "/" << p;
		p = ss.str();
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
			string originalPath = basePath + ofToString(i) + ".xml";
			string destPath = basePath + ofToString(i+1) + ".xml";
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
				ofFile::copyFromTo(getFinalPath(OFXREMOTEUI_SETTINGS_FILENAME), basePath + "0.xml");
			}catch(...){}
		}
		f.close();
		if(verbose_) RUI_LOG_NOTICE << "saving a backup of the current " << getFinalPath(OFXREMOTEUI_SETTINGS_FILENAME) << " in " << getFinalPath(OFXREMOTEUI_SETTINGS_BACKUP_FOLDER) ;
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
			if (!enabled){
				RUI_LOG_WARNING << "launching disabled!" ;
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
		if(subnetMask == ""){ //old way, we ignore subnet mask and assume the net's subnet mask is 255.255.255.0
			if (computerIP != RUI_LOCAL_IP_ADDRESS){ //this only handles 255.255.255.0 style networks
				vector<string>comps;
				split(comps, computerIP, '.');
				multicastIP = comps[0] + "." + comps[1] + "." + comps[2] + "." + "255";
			}else{
				multicastIP = "255.255.255.255";
			}

		}else{ //new stuff, we actually build the multicast IP based on the subnet mask too
			//this assumes all subnet components are either 0 or 255, the multicast @ will not be correct otherwise! TODO!
			if (computerIP != RUI_LOCAL_IP_ADDRESS){ //if addr is not 127.0.0.1
				vector<string>addComps;
				split(addComps, computerIP, '.');
				vector<string>subnetComps;
				split(subnetComps, subnetMask, '.');
				for(int i = 0; i < 4; i++){
					if(subnetComps[i] == "255"){
						multicastIP += addComps[i];
					}else{
						multicastIP += "255";
					}
					if (i < 3) multicastIP += ".";
				}
			}else{
				multicastIP = "255.255.255.255";
			}
		}

		#ifdef OF_AVAILABLE
		ofTargetPlatform platform = ofGetTargetPlatform();
		if ( (platform == OF_TARGET_WINVS || platform == OF_TARGET_WINGCC)
			&&
			multicastIP == "255.255.255.255"
			){
			doBroadcast = false; //windows crashes on bradcast if no devices are up!
			RUI_LOG_WARNING << "no network interface found, we will not broadcast ourselves";
		}
		#endif

		if(doBroadcast){
			broadcastSender.setup( multicastIP, OFXREMOTEUI_BROADCAST_PORT ); //multicast @
			RUI_LOG_NOTICE << "letting everyone know that I am at " << multicastIP << ":" << OFXREMOTEUI_BROADCAST_PORT ;
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
		RUI_LOG_NOTICE << "listening at port " << port << " ... " ;
		oscReceiver.setup(port);
	}
	//still get ui access despite being disabled
	#ifdef OF_AVAILABLE
	ofAddListener(ofEvents().exit, this, &ofxRemoteUIServer::_appExited); //to save to xml, disconnect, etc
	ofAddListener(ofEvents().keyPressed, this, &ofxRemoteUIServer::_keyPressed);
	ofAddListener(ofEvents().update, this, &ofxRemoteUIServer::_update);
	ofAddListener(ofEvents().draw, this, &ofxRemoteUIServer::_draw, OF_EVENT_ORDER_AFTER_APP + 110);
	#endif

	setNewParamColor(1);
	setNewParamColorVariation(true);
}

#ifdef OF_AVAILABLE
void ofxRemoteUIServer::_appExited(ofEventArgs &e){
	if(!enabled) return;
	OFX_REMOTEUI_SERVER_CLOSE();		//stop the server
	if(saveToXmlOnExit){
		OFX_REMOTEUI_SERVER_SAVE_TO_XML();	//save values to XML
	}
}


void ofxRemoteUIServer::_keyPressed(ofKeyEventArgs &e){

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
					string key = orderedKeys[selectedItem];
					RemoteUIParam &p = params[key];
					if (p.type == REMOTEUI_PARAM_SPACER){
						groupIsSelected = true;
						groupName = p.group;
					}
				}
				string presetName = ofSystemTextBoxDialog(groupIsSelected ?
														  "Create a New Group Preset For " + groupName
														  :
														  "Create a New Global Preset" ,
														  "");
				if(presetName.size()){
					RemoteUIServerCallBackArg cbArg;
					if (groupIsSelected){
						saveGroupToXML(string(OFXREMOTEUI_PRESET_DIR) + "/" + groupName + "/" + presetName + formatExt + ".xml", groupName, saveInV1);
						if(callBack) callBack(cbArg);
						cbArg.action = CLIENT_SAVED_GROUP_PRESET;
						cbArg.msg = presetName;
						cbArg.group = groupName;
						#ifdef OF_AVAILABLE
						ofNotifyEvent(clientAction, cbArg, this);
						onScreenNotifications.addNotification("SAVED PRESET '" + presetName + formatExt + ".xml' FOR GROUP '" + groupName + "'");
						#endif

					}else{
						if(verbose_) RUI_LOG_NOTICE << "saving NEW preset: " << presetName ;
						saveToXML(string(OFXREMOTEUI_PRESET_DIR) + "/" + presetName + formatExt + ".xml", saveInV1);
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

			case 'r':
				restoreAllParamsToInitialXML();
				onScreenNotifications.addNotification("RESET CONFIG TO SERVER-LAUNCH XML values");
				break;

			case 'l':{
				string p = getFinalPath(OFXREMOTEUI_SETTINGS_FILENAME);
				onScreenNotifications.addNotification("LOAD XML from " + p);
				loadFromXML(ofToDataPath(p));
				}break;

			case OF_KEY_RETURN:
				if(selectedItem == -1 && selectedPreset >= 0 && presetsCached.size()){ //global presets
					lastChosenPreset = presetsCached[selectedPreset];
					loadFromXML(string(OFXREMOTEUI_PRESET_DIR) + "/" + lastChosenPreset + ".xml");
					syncAllPointersToParams();
					uiAlpha = 0;
					if(verbose_) RUI_LOG_NOTICE << "setting preset: " << lastChosenPreset ;
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
						loadFromXML(string(OFXREMOTEUI_PRESET_DIR) + "/" + presetName + ".xml");
						syncAllPointersToParams();
						uiAlpha = 0;
						if(verbose_) RUI_LOG_NOTICE << "setting preset: " << presetName ;
						RemoteUIServerCallBackArg cbArg;
						cbArg.action = CLIENT_DID_SET_GROUP_PRESET;
						cbArg.msg = p.group;
						if(callBack) callBack(cbArg);
						#ifdef OF_AVAILABLE
						ofNotifyEvent(clientAction, cbArg, this);
						onScreenNotifications.addNotification("SET '" + p.group  + "' GROUP TO '" + presetName + ".xml' PRESET");
						#endif
					}else if(p.type == REMOTEUI_PARAM_COLOR){
						selectedColorComp++;
						if(selectedColorComp > 3) selectedColorComp = 0;
					}
				}
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

			case ',': xOffsetTarget += (uiColumnWidth); xOffsetTarget = ofClamp(xOffsetTarget, -FLT_MAX, 2 * uiColumnWidth); break;
			case '.': xOffsetTarget -= (uiColumnWidth); xOffsetTarget = ofClamp(xOffsetTarget, -FLT_MAX, 2 * uiColumnWidth); break;
			case OF_KEY_LEFT:
			case OF_KEY_RIGHT:{

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
						params[key] = p;
						syncPointerToParam(key);
						pushParamsToClient();
						RemoteUIServerCallBackArg cbArg;
						cbArg.action = CLIENT_DID_RESET_TO_XML;
						cbArg.host = "localhost";
						cbArg.action =  CLIENT_UPDATED_PARAM;
						cbArg.paramName = key;
						cbArg.param = params[key];  //copy the updated param to the callbakc arg
						#ifdef OF_AVAILABLE
						onScreenNotifications.addParamUpdate(key, cbArg.param.getValueAsString(), ofColor(p.r, p.g, p.b, p.a));
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
			}break;
		}
	}

	if(e.key == showInterfaceKey){
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
}

void ofxRemoteUIServer::refreshPresetsCache(){

	//get all group presets
	groupPresetsCached.clear();
	for( unordered_map<string, RemoteUIParam>::iterator ii = params.begin(); ii != params.end(); ++ii ){
		if((*ii).second.type == REMOTEUI_PARAM_SPACER){
			groupPresetsCached[(*ii).second.group] = getAvailablePresetsForGroup((*ii).second.group);
		};
	}

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
	useFontStash = true;
	fontFile = ofToDataPath(fontPath, true);
	fontSize = fontSize_;
	font.setup(fontFile, 1.0, 512, (uiScale > 1.0), 4, uiScale);
	if(uiScale > 1.0) font.setLodBias(-0.5);
	onScreenNotifications.drawUiWithFontStash(&font);
}

void ofxRemoteUIServer::drawUiWithBitmapFont(){
	useFontStash = false;
}
#endif


void ofxRemoteUIServer::threadedFunction(){

	while (isThreadRunning()) {
		updateServer(1./30.); //30 fps timebase
		ofSleepMillis(33);
	}
	if(verbose_) RUI_LOG_NOTICE << "threadedFunction() ending" ;
}


void ofxRemoteUIServer::_draw(ofEventArgs &e){
	if(autoDraw){
		ofSetupScreen(); //mmm this is a bit scary //TODO!
		int h = customScreenHeight;
		int w = customScreenWidth;
		if(h < 0) h = ofGetHeight();
		if(w < 0) w = ofGetWidth();
		draw( 20 / uiScale, (h - 20)/ uiScale );
	}
}

void ofxRemoteUIServer::_update(ofEventArgs &e){
	update(ofGetLastFrameTime());
}

void ofxRemoteUIServer::drawString(const string & text, const ofVec2f & pos){
	drawString(text, pos.x, pos.y);
}

void ofxRemoteUIServer::drawString(const string & text, const float & x, const float & y){
	#ifdef USE_OFX_FONTSTASH
	if(useFontStash){
		font.drawMultiLine(text, fontSize, x, y + 1);
	}else{
		ofDrawBitmapString(text, x, y);
	}
	#else
	ofDrawBitmapString(text, x, y);
	#endif
}

//x and y of where the notifications will get draw
void ofxRemoteUIServer::draw(int x, int y){

	ofPushStyle();
	ofPushMatrix();
	ofScale(uiScale,uiScale);
	ofSetDrawBitmapMode(OF_BITMAPMODE_SIMPLE);

	ofFill();
	ofEnableAlphaBlending();
	int screenH = customScreenHeight;
	if (screenH < 0) screenH = ofGetHeight();
	int screenW = customScreenWidth;
	if (screenW < 0) screenW = ofGetWidth();

	if(showUI){

		int padding = 30;
		int x = padding;
		int initialY = padding * 1.5;
		int y = initialY;
		int colw = uiColumnWidth;
		int realColW = colw * 0.9;
		int valOffset = realColW * 0.7;
		int valSpaceW = realColW - valOffset;
		int spacing = 20;
		int bottomBarHeight = padding + spacing + 36;

		//bottom bar
		if (uiAlpha > 0.99 || showUIduringEdits){
			ofSetColor(11, 245);
			ofRect(0,0, screenW / uiScale, screenH / uiScale);
			ofSetColor(44, 245);
			ofRect(0,screenH / uiScale - bottomBarHeight, screenW / uiScale, bottomBarHeight );

			ofSetColor(255);
			drawString("ofxRemoteUI built in client. " +
							   string(enabled ? ("Server reachable at " + computerIP + ":" + ofToString(port)) + "." :
									  "Sever Disabled." ) +
					   "\nPress 's' to save current config, 'S' to make a new preset. ('E' to save in old format)\n" +
					   "Press 'r' to restore all params's launch state.\n" +
					   "Press Arrow Keys to edit values. SPACEBAR + Arrow Keys for bigger increments. ',' and '.' Keys to scroll.\n" +
					   "Press 'TAB' to hide. Press 'RETURN' when editing a Color param to cycle through RGBA components.", padding, screenH / uiScale - bottomBarHeight + 20);
		}

		//preset selection / top bar
		ofSetColor(64);
		ofRect(0 , 0, screenW / uiScale, 22);
		ofColor textBlinkC ;
		if(ofGetFrameNum()%6 < 4) textBlinkC = ofColor(0,0);
		else textBlinkC = ofColor(255,0,0);

		if(presetsCached.size() > 0 && selectedPreset >= 0 && selectedPreset < presetsCached.size()){

			ofVec2f dpos = ofVec2f(192, 16);
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
		drawString("+ PRESET SELECTION: " , 30,  16);

		int linesInited = uiLines.getNumVertices() > 0 ;

		if(uiAlpha > 0.99 || showUIduringEdits){

			ofTranslate(xOffset, 0);

			//param list
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
					ofRect(x , -spacing + y + spacing * 0.33, realColW, spacing);
				}
				if (selectedItem != i){
					ofSetColor(p.r, p.g, p.b);
				}else{
					if(ofGetFrameNum()%5 < 1) ofSetColor(255);
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
					case REMOTEUI_PARAM_ENUM:
						if (p.intVal >= 0 && p.intVal < p.enumList.size() && p.enumList.size() > 0){
							string val = p.enumList[p.intVal];
							if (val.length() > column2MaxLen){
								val = val.substr(0, column2MaxLen);
							}
							drawString(val, x + valOffset, y);
						}else{
							drawString(ofToString(p.intVal), x + valOffset, y);
						}
						break;
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
						ofRect(x + valOffset, y - spacing * 0.6, valSpaceW, spacing * 0.85);
						char aux[200];
						//sprintf(aux, "[%02x%02x%02x%02x]", p.redVal, p.greenVal, p.blueVal, p.alphaVal);
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
					uiLines.addVertex(ofVec2f(x, y + spacing * 0.33));
					uiLines.addVertex(ofVec2f(x + realColW, y + spacing * 0.33));
				}
				y += spacing;
				if (y > screenH / uiScale - padding * 0.5 - bottomBarHeight){
					x += colw;
					y = initialY;
				}
			}
			ofSetColor(32);
			ofSetLineWidth(1);
			uiLines.draw();
		}

		//tiny clock top left
		if (uiAlpha < 1.0 && !showUIduringEdits){
			ofMesh m;
			int step = 30;
			m.setMode(OF_PRIMITIVE_TRIANGLE_FAN);
			ofVec2f origin = ofVec2f(15,11); //rect is 22
			m.addVertex(origin);
			float r = 8.0f;
			float ang;
			float lim = 360.0f * (1.0f - uiAlpha);
			for(ang = 0; ang < lim; ang += step){
				m.addVertex( origin + ofVec2f( r * cosf((-90.0f + ang) * DEG_TO_RAD),
											  r * sinf((-90.0f + ang) * DEG_TO_RAD)));
			}
			float lastBit = lim - ang;
			m.addVertex( origin + ofVec2f( r * cosf((-90.0f + ang + lastBit) * DEG_TO_RAD),
										  r * sinf((-90.0f + ang + lastBit) * DEG_TO_RAD)));
			ofSetColor(128);
			m.draw();
		}
	}

	if (!showUI || uiAlpha < 1.0){
		if (drawNotifications){
			for(int i = 0; i < paramsToWatch.size(); i++){
				RemoteUIParam & p = params[paramsToWatch[i]];
				string v = p.getValueAsStringFromPointer();
				ofColor c = ofColor(p.r, p.g, p.b, 255);
				onScreenNotifications.addParamWatch(paramsToWatch[i], v, c);
			}
			onScreenNotifications.draw(x, y);
		}
	}
	ofPopMatrix();
	ofPopStyle();
}
#endif

#ifdef OF_AVAILABLE

void ofxRemoteUIServer::setUiColumnWidth(int w){
	if(fabs(uiColumnWidth - w) < 0.1) uiLines.clear();
	uiColumnWidth = w;
}

void ofxRemoteUIServer::setBuiltInUiScale(float s){
	if(fabs(uiScale - s) < 0.1) uiLines.clear();
	uiScale = s;
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
    #else
        #ifdef TARGET_WIN32
				GetModuleFileNameA( NULL, pathbuf, bufsize ); //no idea why, but GetModuleFileName() is not defined?
				Poco::Path p = Poco::Path(pathbuf);
				binaryName = p[p.depth()];

        #else

            char procname[1024];
            int len = readlink("/proc/self/exe", procname, 1024-1);
            if (len > 0){
                procname[len] = '\0';
                Poco::Path p = Poco::Path(procname);
				binaryName = p[p.depth()];
            }
        #endif
    #endif
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
				cbArg.action = CLIENT_CONNECTED;
				if(callBack) callBack(cbArg);
				#ifdef OF_AVAILABLE
				onScreenNotifications.addNotification("CONNECTED (" + cbArg.host +  ")!");
				ofNotifyEvent(clientAction, cbArg, this);
				#endif
				if(verbose_) RUI_LOG_VERBOSE << m.getRemoteIp() << " says HELLO!"  ;

				break;

			case REQUEST_ACTION:{ //send all params to client
				if(verbose_) RUI_LOG_VERBOSE << m.getRemoteIp() << " sends REQU!"  ;
				pushParamsToClient();
				}break;

			case SEND_PARAM_ACTION:{ //client is sending us an updated val
				if(verbose_) RUI_LOG_VERBOSE << m.getRemoteIp() << " sends SEND!"  ;
				updateParamFromDecodedMessage(m, dm);
				cbArg.action = CLIENT_UPDATED_PARAM;
				cbArg.paramName = dm.paramName;
				cbArg.param = params[dm.paramName];  //copy the updated param to the callbakc arg
				cbArg.group = cbArg.param.group;
				if(callBack) callBack(cbArg);
				#ifdef OF_AVAILABLE
				RemoteUIParam & p = params[dm.paramName];
				onScreenNotifications.addParamUpdate(dm.paramName, p.getValueAsString(),
													 ofColor(p.r, p.g, p.b, p.a),
													 p.type == REMOTEUI_PARAM_COLOR ?
													 ofColor(p.redVal, p.greenVal, p.blueVal, p.alphaVal):
													 ofColor(0,0,0,0)
													 );
				ofNotifyEvent(clientAction, cbArg, this);
				#endif
			}
				break;

			case CIAO_ACTION:{
				sendCIAO();
				cbArg.action = CLIENT_DISCONNECTED;
				if(callBack) callBack(cbArg);
				#ifdef OF_AVAILABLE
				onScreenNotifications.addNotification("DISCONNECTED (" + cbArg.host +  ")!");
				ofNotifyEvent(clientAction, cbArg, this);
				#endif
				clearOscReceiverMsgQueue();
				readyToSend = false;
				if(verbose_) RUI_LOG_VERBOSE << m.getRemoteIp() << " says CIAO!" ;
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
				sendSETP(presetName);
				sendMISP(missingParams);
				cbArg.action = CLIENT_DID_SET_PRESET;
				cbArg.msg = presetName;
				if(callBack) callBack(cbArg);
				#ifdef OF_AVAILABLE
				onScreenNotifications.addNotification("SET PRESET to '" + getFinalPath(OFXREMOTEUI_PRESET_DIR) + "/" + presetName + ".xml'");
				ofNotifyEvent(clientAction, cbArg, this);
				#endif
				if(verbose_) RUI_LOG_NOTICE << "setting preset: " << presetName ;
			}break;

			case SAVE_PRESET_ACTION:{ //client wants to save current xml as a new preset
				string presetName = m.getArgAsString(0);
				saveToXML(string(OFXREMOTEUI_PRESET_DIR) + "/" + presetName + ".xml");
				sendSAVP(presetName);
				cbArg.action = CLIENT_SAVED_PRESET;
				cbArg.msg = presetName;
				if(callBack) callBack(cbArg);
				#ifdef OF_AVAILABLE
				onScreenNotifications.addNotification("SAVED PRESET to '" + getFinalPath(OFXREMOTEUI_PRESET_DIR) + "/" + presetName + ".xml'");
				ofNotifyEvent(clientAction, cbArg, this);
				#endif
				if(verbose_) RUI_LOG_NOTICE << "saving NEW preset: " << presetName ;
			}break;

			case DELETE_PRESET_ACTION:{
				string presetName = m.getArgAsString(0);
				deletePreset(presetName);
				sendDELP(presetName);
				cbArg.action = CLIENT_DELETED_PRESET;
				cbArg.msg = presetName;
				if(callBack) callBack(cbArg);
				#ifdef OF_AVAILABLE
				onScreenNotifications.addNotification("DELETED PRESET '" + getFinalPath(OFXREMOTEUI_PRESET_DIR) + "/" + presetName + ".xml'");
				ofNotifyEvent(clientAction, cbArg, this);
				if(verbose_) RUI_LOG_NOTICE << "DELETE preset: " << presetName ;
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
				if(verbose_) RUI_LOG_NOTICE << "SAVE CURRENT PARAMS TO DEFAULT XML: " ;
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
				if(verbose_) RUI_LOG_NOTICE << "RESET TO XML: " ;
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
				if(verbose_) RUI_LOG_NOTICE << "RESET TO DEFAULTS: " ;
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
				if(verbose_) RUI_LOG_NOTICE << "setting preset group: " << groupName << "/" <<presetName ;
			}break;

			case SAVE_GROUP_PRESET_ACTION:{ //client wants to save current xml as a new preset
				string presetName = m.getArgAsString(0);
				string groupName = m.getArgAsString(1);
				saveGroupToXML(string(OFXREMOTEUI_PRESET_DIR) + "/" + groupName + "/" + presetName + ".xml", groupName);
				sendSAVp(presetName, groupName);
				cbArg.action = CLIENT_SAVED_GROUP_PRESET;
				cbArg.msg = presetName;
				cbArg.group = groupName;
				#ifdef OF_AVAILABLE
				onScreenNotifications.addNotification("SAVED PRESET '" + presetName + ".xml' FOR GROUP '" + groupName + "'");
				ofNotifyEvent(clientAction, cbArg, this);
				#endif
				if(callBack) callBack(cbArg);
				if(verbose_) RUI_LOG_NOTICE << "saving NEW preset: " << presetName ;
			}break;

			case DELETE_GROUP_PRESET_ACTION:{
				string presetName = m.getArgAsString(0);
				string groupName = m.getArgAsString(1);
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
				if(verbose_) RUI_LOG_NOTICE << "DELETE preset: " << presetName ;
			}break;
			default: RUI_LOG_ERROR << "updateServer >> ERR!"; break;
		}
	}
}

void ofxRemoteUIServer::deletePreset(string name, string group){
	#ifdef OF_AVAILABLE
	ofDirectory dir;
	if (group == "")
		dir.open(getFinalPath(OFXREMOTEUI_PRESET_DIR) + "/" + name + ".xml");
	else
		dir.open(getFinalPath(OFXREMOTEUI_PRESET_DIR) + "/" + group + "/" + name + ".xml");
	dir.remove(true);
	#else
	string file = getFinalPath(OFXREMOTEUI_PRESET_DIR) + "/" + name + ".xml";
	if (group != "") file = getFinalPath(OFXREMOTEUI_PRESET_DIR) + "/" + group + "/" + name + ".xml";
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
		if (files[i].isFile() && extension == "xml"){
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
				if (files2[j].isFile() && extension2 == "xml"){
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
			if ( strcmp( get_filename_ext(ent->d_name), "xml") == 0 ){
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
			if (files[i].isFile() && extension == "xml"){
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

void ofxRemoteUIServer::watchParamOnScreen(string paramName){
	if (params.find(paramName) != params.end()){
		paramsToWatch.push_back(paramName);
	}else{
		RUI_LOG_ERROR << "can't watch that param; it doesnt exist! " << paramName << endl;
	}
}


void ofxRemoteUIServer::addParamToDB(const RemoteUIParam & p, string thisParamName){

	if(p.type != REMOTEUI_PARAM_SPACER && params.size() == 0){ //adding first param! and its not spacer!
		setParamGroup(OFXREMOTEUI_DEFAULT_PARAM_GROUP);
	}
	ofxRemoteUI::addParamToDB(p, thisParamName);
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
	if(verbose_) RUI_LOG_NOTICE << "Adding Group '" << title << "' ######################################" ;
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
	if(verbose_) RUI_LOG_NOTICE << "Sharing Float Param '" << paramName << "'" ;
}


void ofxRemoteUIServer::shareParam(string paramName, bool* param, ofColor c, int nothingUseful ){
	RemoteUIParam p;
	p.type = REMOTEUI_PARAM_BOOL;
	p.boolValAddr = param;
	p.boolVal = *param;
	p.group = upcomingGroup;
	setColorForParam(p, c);
	addParamToDB(p, paramName);
	if(verbose_) RUI_LOG_NOTICE << "Sharing Bool Param '" << paramName << "'" ;
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
	if(verbose_) RUI_LOG_NOTICE << "Sharing Int Param '" << paramName << "'" ;
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
	if(verbose_) RUI_LOG_NOTICE << "Sharing Enum Param '" << paramName << "'" ;
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
	if(verbose_) RUI_LOG_NOTICE << "Sharing Enum Param '" << paramName << "'" ;
}



void ofxRemoteUIServer::shareParam(string paramName, string* param, ofColor c, int nothingUseful ){
	RemoteUIParam p;
	p.type = REMOTEUI_PARAM_STRING;
	p.stringValAddr = param;
	p.stringVal = *param;
	p.group = upcomingGroup;
	setColorForParam(p, c);
	addParamToDB(p, paramName);
	if(verbose_) RUI_LOG_NOTICE << "Sharing String Param '" << paramName << "'";
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
	if(verbose_) RUI_LOG_NOTICE << "Sharing Color Param '" << paramName << "'";
}


void ofxRemoteUIServer::connect(string ipAddress, int port){
	avgTimeSinceLastReply = timeSinceLastReply = timeCounter = 0.0f;
	waitingForReply = false;
	//params.clear();
	oscSender.setup(ipAddress, port);
	readyToSend = true;
}

void ofxRemoteUIServer::sendLogToClient(const char* format, ...){
	if(readyToSend){
		// protect from crashes or memory issues
		if (strlen(format) >= 1024) {
			RUI_LOG_ERROR << "log string must be under 1024 chars" << endl;
		    return;
		}

		char line[1024];
		va_list args;
		va_start(args, format);
		vsprintf(line, format,  args);
		sendLogToClient(string(line));
	}
}

void ofxRemoteUIServer::sendLogToClient(string message){
	if(readyToSend){
		ofxOscMessage m;
		m.setAddress("LOG_");
		m.addStringArg(message);
		try{
			oscSender.sendMessage(m);
		}catch(exception e){
			RUI_LOG_ERROR << "Exception sendLogToClient " << e.what() ;
		}
	}
}



void ofxRemoteUIServer::saveToXMLv1(string fileName){

	saveSettingsBackup(); //every time , before we save

#ifdef OF_AVAILABLE
	fileName = getFinalPath(fileName);
#endif

	RUI_LOG_NOTICE << "saving to xml (using the OLD FORMAT) '" << fileName << "'" ;
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
}

vector<string> ofxRemoteUIServer::loadFromXMLv1(string fileName){

	RUI_LOG_WARNING << "Loading XML " << fileName << " with older format. Be aware that it will be saved with the new format!";
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
							if(verbose_) RUI_LOG_VERBOSE << "loading a FLOAT '" << paramName <<"' (" << ofToString( *params[paramName].floatValAddr, 3) << ") from XML" ;
						}else{
							RUI_LOG_ERROR << "ERROR at loading FLOAT (" << paramName << ")" ;
						}
					}else{
						RUI_LOG_ERROR << "float param '" << paramName << "' defined in xml not found in DB!" ;
					}
				}else{
					RUI_LOG_ERROR << "float param '" << paramName << "' defined twice in xml! Using first definition only" ;
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
							if(verbose_) RUI_LOG_VERBOSE << "loading an INT '" << paramName <<"' (" << (int) *params[paramName].intValAddr << ") from XML" ;
						}else{
							RUI_LOG_ERROR << "ERROR at loading INT (" << paramName << ")" ;
						}
					}else{
						RUI_LOG_ERROR << "int param '" <<paramName << "' defined in xml not found in DB!" ;
					}
				}else{
					RUI_LOG_ERROR << "int param '" << paramName << "' defined twice in xml! Using first definition only" ;
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
							if(verbose_) RUI_LOG_VERBOSE << "loading a COLOR '" << paramName <<"' (" << (int)*params[paramName].redValAddr << " " << (int)*(params[paramName].redValAddr+1) << " " << (int)*(params[paramName].redValAddr+2) << " " << (int)*(params[paramName].redValAddr+3)  << ") from XML" ;
						}else{
							RUI_LOG_ERROR << "ERROR at loading COLOR (" << paramName << ")" ;
						}
					}else{
						RUI_LOG_WARNING << "color param '" <<paramName << "' defined in xml not found in DB!" ;
					}
					s.popTag();
				}else{
					RUI_LOG_ERROR << "color param '" << paramName << "' defined twice in xml! Using first definition only" ;
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
							if(verbose_) RUI_LOG_VERBOSE << "loading an ENUM '" << paramName <<"' (" << (int) *params[paramName].intValAddr << ") from XML" ;
						}else{
							RUI_LOG_ERROR << "ERROR at loading ENUM (" << paramName << ")" ;
						}
					}else{
						RUI_LOG_WARNING << "enum param '" << paramName << "' defined in xml not found in DB!" ;
					}
				}else{
					RUI_LOG_ERROR << "enum param '" << paramName << "' defined twice in xml! Using first definition only" ;
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
							if(verbose_) RUI_LOG_VERBOSE << "loading a BOOL '" << paramName <<"' (" << (bool) *params[paramName].boolValAddr << ") from XML" ;
						}else{
							RUI_LOG_ERROR << "ERROR at loading BOOL (" << paramName << ")" ;
						}
					}else{
						RUI_LOG_WARNING << "bool param '" << paramName << "' defined in xml not found in DB!" ;
					}
				}else{
					RUI_LOG_ERROR << "bool param '" << paramName << "' defined twice in xml! Using first definition only" ;
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
							if(verbose_) RUI_LOG_VERBOSE << "loading a STRING '" << paramName <<"' (" << (string) *params[paramName].stringValAddr << ") from XML" ;
						}
						else RUI_LOG_ERROR << "ERROR at loading STRING (" << paramName << ")" ;
					}else{
						RUI_LOG_WARNING << "string param '" << paramName << "' defined in xml not found in DB!" ;
					}
				}else{
					RUI_LOG_ERROR << "string param '" << paramName << "' defined twice in xml! Using first definition only" ;
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



void ofxRemoteUIServer::saveGroupToXMLv1(string filePath, string groupName){

	RUI_LOG_NOTICE << "saving group to xml '" << filePath << "'" ;
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
