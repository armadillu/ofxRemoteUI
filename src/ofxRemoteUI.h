//
//  ofxRemoteUI.h
//  emptyExample
//
//  Created by Oriol Ferrer Mesià on 09/01/13.
//
//

#ifndef _ofxRemoteUI__
#define _ofxRemoteUI__

// you will need to add this to your "Header Search Path" for ofxOsc to compile
// ../../../addons/ofxOsc/libs ../../../addons/ofxOsc/libs/oscpack ../../../addons/ofxOsc/libs/oscpack/src ../../../addons/ofxOsc/libs/oscpack/src/ip ../../../addons/ofxOsc/libs/oscpack/src/ip/posix ../../../addons/ofxOsc/libs/oscpack/src/ip/win32 ../../../addons/ofxOsc/libs/oscpack/src/osc ../../../addons/ofxOsc/src
#include "ofxOsc.h"
#include <map>
#include <set>
#include <vector>
using namespace std;

#define OFXREMOTEUI_PORT					10001
#define LATENCY_TEST_RATE					0.3333
#define CONNECTION_TIMEOUT					4.0f
#define OFX_REMOTEUI_SETTINGS_FILENAME		"ofxRemoteUISettings.xml"
#define OFX_REMOTEUI_XML_TAG				"OFX_REMOTE_UI_PARAMS"
#define DEFAULT_PARAM_GROUP					"defaultGroup"
#define OFX_REMOTEUI_PRESET_DIR				"ofxRemoteUIPresets"
#define OFX_REMOTEUI_NO_PRESETS				"NO_PRESETS"
#include "RemoteParam.h"


/*

 // COMM /////////////////////////

 // init / setup connection /////
 CLIENT:	HELO								//client says HI
 SERVER:	HELO								//server says HI
 CLIENT:	REQU								//client requests all params from server
 SERVER:	SEND FLT PARAM_NAME val (float)		//server starts sending all vars one by one
 SERVER:	SEND INT PARAM_NAME2 val (int)
 SERVER:	SEND BOL PARAM_NAME3 val (bool)
 SERVER:	SEND STR PARAM_NAME4 val (string)
 SERVER:	REQU OK								//server closes REQU
 ...
 CLIENT:	PREL								//Preset List - client requests list of presets
 SERVER:	PREL PRESET_NAME_LIST(n)			//server sends all preset names
 ...

 // normal operation //////////
 CLIENT:	SEND TYP VAR_NAME val (varType)		//client sends a var change to server
 ...
 CLIENT:	TEST								//every second, client sends a msg to server to measure delay
 SERVER:	TEST								//server replies
 ...
 CLIENT:	SETP PRESET_NAME					//Set Preset - client wants to change all params according to preset "X"
 SERVER:	SETP OK								//server says ok
 CLIENT:	REQU								//client wants values for that preset
 SERVER:	SEND *****							//server sends all params
 SERVER:	REQU OK
 ...
 CLIENT:    SAVP PRESET_NAME					//Save Preset - client wants to save current params as a preset named PRESET_NAME
												//overwrites if already exists
 SERVER:	SAVP OK								//server replies OK
 CLIENT:	PREL								//Client requests full list of presets
 SERVER:	PREL PRESET_NAME_LIST(n)			//server sends all preset names
 ...
 CLIENT:	DELP PRESET_NAME					//client wants to delete preset named PRESET_NAME
 SERVER:	DELP OK								//server says ok
 CLIENT:	PREL								//Client requests full list of presets
 SERVER:	PREL PRESET_NAME_LIST(n)			//server sends all preset names
 ...
 CLIENT:	RESX								//client wants to delete load all params from the 1st loaded xml (last saved settings)
 SERVER:	RESX OK								//server says ok
 CLIENT:	REQU								//client wants ALL values
 SERVER:	SEND *****							//server sends all params
 ...
 CLIENT:	RESD								//client wants to delete load all params from the code defaults
 SERVER:	RESD OK								//server says ok
 CLIENT:	REQU								//client wants ALL values
 SERVER:	SEND *****							//server sends all params
 ...
 CLIENT:	SAVE								//client wants to save current state of params to default xml
 SERVER:	SAVE OK
 ...
 CLIENT:	CIAO								//client disconnects
 SERVER:	CIAO								//server disconnects


 // closing connection ////////

 // SERVER API ////////////////////////////////////////

	server->setup(refreshRate);
	server->shareParam("paramName", &paramName, ... );
	...
	server->update(dt);

 // CLIENT API ////////////////////////////////////////
 
	client.setup(ipAddress, refreshRate);
	client.trackParam("paramName", &paramName);
	...
	client.update();


	float getMinThresholdForParam("paramMame"); //only applies to int and float
	float getMaxThresholdForParam("paramMame"); //only applies to int and float

	//get a report of params that changed on the server side since last check
	vector<string> updatedParamsList = client.getChangedParamsList();
 
	//push a param change to the server, will send the current value of the param to server
	client.sendTrackedParamUpdate("paramName");

 */


class ofxRemoteUI{

public:

	vector<string> getAllParamNamesList();
	vector<string> getChangedParamsList(); //in user add order
	RemoteUIParam getParamForName(string paramName);
	vector<string> getPresetsList();
	
	string getValuesAsString();
	void setValuesFromString( string values );	

	virtual void restoreAllParamsToInitialXML() = 0;	//call this on client to request server to do so
	virtual void restoreAllParamsToDefaultValues() = 0;

	bool ready();
	float connectionLag();
	void setVerbose(bool b);

	virtual void sendUntrackedParamUpdate(RemoteUIParam p, string paramName){};

protected:

	virtual void update(float dt) = 0;
	void sendParam(string paramName, RemoteUIParam p);
	DecodedMessage decode(ofxOscMessage m);

	vector<string> scanForUpdatedParamsAndSync();

	void sendUpdateForParamsInList(vector<string>paramsPendingUpdate);
	bool hasParamChanged(RemoteUIParam p);	

	void updateParamFromDecodedMessage(ofxOscMessage m, DecodedMessage dm);
	void syncAllParamsToPointers();
	void syncAllPointersToParams();
	void syncParamToPointer(string paramName);
	void syncPointerToParam(string paramName);
	void addParamToDB(RemoteUIParam p, string paramName);

	void sendREQU(bool confirm = false); //a request for a complete list of server params
	void sendHELLO();
	void sendCIAO();
	void sendTEST();
	void sendPREL(vector<string> presetNames);
	void sendSAVP(string presetName);
	void sendSETP(string presetName);
	void sendDELP(string presetName);
	void sendRESX(bool confirm = false); //send a "restore fom first loaded XML" msg
	void sendRESD(bool confirm = false); //send a "restore fom code defaults" msg
	void sendSAVE(bool confirm = false); 

	bool							verbose;
	bool							readyToSend;
	ofxOscSender					oscSender;
	ofxOscReceiver					oscReceiver;

	float							time;
	float							timeSinceLastReply;
	float							avgTimeSinceLastReply;
	bool							waitingForReply;

	float							updateInterval;
	int								port;



	map<string, RemoteUIParam>		params;
	map<int, string>				orderedKeys; // used to keep the order in which the params were added
	vector<string>					presetNames;

	set<string>						paramsChangedSinceLastCheck;

	map<string, RemoteUIParam>		paramsFromCode; //this will hold a copy of all the params as they where when shared first
	map<string, RemoteUIParam>		paramsFromXML; //this will hold a copy of all the params as they where when first loaded from XML


private:

	string stringForParamType(RemoteUIParamType t);

};

#endif
