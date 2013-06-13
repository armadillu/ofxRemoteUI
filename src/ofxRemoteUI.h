//
//  ofxRemoteUI.h
//  emptyExample
//
//  Created by Oriol Ferrer Mesià on 09/01/13.
//
//

#ifndef __emptyExample__ofxRemoteUI__
#define __emptyExample__ofxRemoteUI__

// you will need to add this to your "Header Search Path" for ofxOsc to compile
// ../../../addons/ofxOsc/libs ../../../addons/ofxOsc/libs/oscpack ../../../addons/ofxOsc/libs/oscpack/src ../../../addons/ofxOsc/libs/oscpack/src/ip ../../../addons/ofxOsc/libs/oscpack/src/ip/posix ../../../addons/ofxOsc/libs/oscpack/src/ip/win32 ../../../addons/ofxOsc/libs/oscpack/src/osc ../../../addons/ofxOsc/src
#include "ofxOsc.h"

#include <map>
#include <set>
#include <vector>
using namespace std;

#define OFXREMOTEUI_PORT					10001
#define LATENCY_TEST_RATE					0.3333
#define CONNECTION_TIMEOUT					6.0f
#define OFX_REMOTEUI_SETTINGS_FILENAME		"ofxRemoteUISettings.xml"
#define OFX_REMOTEUI_XML_TAG				"OFX_REMOTE_UI_PARAMS"
#define DEFAULT_PARAM_GROUP					"defaultGroup"


//easy param sharing macro, share from from anywhere!
#define OFX_REMOTEUI_SERVER_SHARE_PARAM(val,...)		( ofxRemoteUIServer::instance()->shareParam( #val, &val, ##__VA_ARGS__ ) )
#define OFX_REMOTEUI_SERVER_SET_UPCOMING_PARAM_COLOR(c)	( ofxRemoteUIServer::instance()->setParamColor( c ) )
#define OFX_REMOTEUI_SERVER_SET_UPCOMING_PARAM_GROUP(g)	( ofxRemoteUIServer::instance()->setParamGroup( g ) )
#define OFX_REMOTEUI_SERVER_SETUP(port, ...)			( ofxRemoteUIServer::instance()->setup(port, ##__VA_ARGS__) )
#define OFX_REMOTEUI_SERVER_UPDATE(deltaTime)			( ofxRemoteUIServer::instance()->update(deltaTime) )
#define OFX_REMOTEUI_SERVER_CLOSE()						( ofxRemoteUIServer::instance()->close() )
#define	OFX_REMOTEUI_SERVER_SAVE_TO_XML()				( ofxRemoteUIServer::instance()->saveToXML() )
#define	OFX_REMOTEUI_SERVER_LOAD_FROM_XML()				( ofxRemoteUIServer::instance()->loadFromXML() )

/*

 // COMM /////////////////////////

 CLIENT:	HELO								//client says HI
 SERVER:	HELO								//server says HI
 CLIENT:	REQU								//client requests all params from server
 SERVER:	SEND FLT PARAM_NAME val (float)		//server starts sending all vars one by one
 SERVER:	SEND INT PARAM_NAME2 val (int)
 SERVER:	SEND BOL PARAM_NAME3 val (bool)
 SERVER:	SEND STR PARAM_NAME4 val (string)
 ...
 CLIENT:	SEND TYP VAR_NAME val (varType)		//client sends a var change to server
 ...
 CLIENT:	TEST								//every second, client sends a msg to server to measure delay
 SERVER:	TEST
 CLIENT:	TEST								//TODO? server doesnt need to know about latency... so maybe we dont do this
 ...

 CLIENT:	CIAO								//client disconnects - not really needed? TODO
 SERVER:	CIAO								//server disconnects - not really needed? TODO

 
 // SERVER API

	server->setup(refreshRate);
	server->shareParam("paramName", &paramName, ... );
	...
	server->update(dt);

 // CLIENT API
 
	client.setup(ipAddress, refreshRate);
	client.trackParam("paramName", &paramName);
	...
	client.update();


	float getMinThresholdForParam("paramMame"); //only applies to int and float
	float getMaxThresholdForParam("paramMame"); //only applies to int and float

	//get a report of params that changed on the server side since last check
	vector<string> updatedParamsList = client.getChangedParamsList();
 
	//push a param change to the server, will send the current value of the param to server
	client.sendUpdatedParam("paramName");

 */


enum RemoteUIParamType{
	REMOTEUI_PARAM_FLOAT = 100,
	REMOTEUI_PARAM_INT,
	REMOTEUI_PARAM_BOOL,
	REMOTEUI_PARAM_STRING
};

enum ActionType{
	HELO_ACTION, REQUEST_ACTION, SEND_ACTION, CIAO_ACTION, TEST_ACTION
};

enum ArgType{
	FLT_ARG, INT_ARG, BOL_ARG, STR_ARG, NULL_ARG
};

struct DecodedMessage{
	ActionType action;
	ArgType argument;
	string paramName;
	string paramGroup;
};

class RemoteUIParam{ //I am lazy and I know it

public:

	RemoteUIParam(){
		floatValAddr = NULL;
		intValAddr = NULL;
		boolValAddr = NULL;
		stringValAddr = NULL;
		floatVal = minFloat = maxFloat = 0;
		intVal = minInt = maxInt = 0;
		boolVal = false;
		stringVal = "empty";
		r = g = b = a = 0;
		group = DEFAULT_PARAM_GROUP;
	};


	bool isEqualTo(RemoteUIParam &p){

		bool equal = true;
		switch (type) {
			case REMOTEUI_PARAM_FLOAT:
				if(p.floatVal != floatVal) equal = false;
				if(p.minFloat != minFloat) equal = false;
				if(p.maxFloat != maxFloat) equal = false;
				break;
			case REMOTEUI_PARAM_INT:
				if(p.intVal != intVal) equal = false;
				if(p.minInt != minInt) equal = false;
				if(p.maxInt != maxInt) equal = false;
				break;
			case REMOTEUI_PARAM_BOOL:
				if(p.boolVal != boolVal) equal = false;
				break;
			case REMOTEUI_PARAM_STRING:
				if(p.stringVal != stringVal) equal = false;
				break;
			default: printf("weird RemoteUIParam at isEqualTo()!\n"); break;
		}
		return equal;
	}


	void print(){
		switch (type) {
			case REMOTEUI_PARAM_FLOAT: printf("float: %.2f [%.2f, %.2f]\n", floatVal, minFloat, maxFloat); break;
			case REMOTEUI_PARAM_INT: printf("int: %d [%d, %d]\n", intVal, minInt, maxInt); break;
			case REMOTEUI_PARAM_BOOL: printf("bool: %d\n", boolVal); break;
			case REMOTEUI_PARAM_STRING: printf("string: %s\n", stringVal.c_str()); break;
			default: printf("weird RemoteUIParam at print()!\n"); break;
		}
	};

	RemoteUIParamType type;

	float * floatValAddr;	//used in server
	float floatVal;			//used in client
	float minFloat;
	float maxFloat;

	int * intValAddr;
	int intVal;
	int minInt;
	int maxInt;

	bool * boolValAddr;
	bool boolVal;

	string * stringValAddr;
	string stringVal;
	string group;

	unsigned char r,g,b,a; // color [0,255]

};


class ofxRemoteUI{

public:

	vector<string> getAllParamNamesList();
	vector<string> getChangedParamsList(); //in user add order
	RemoteUIParam getParamForName(string paramName);
	

	bool ready();
	float connectionLag();

protected:

	virtual void update(float dt) = 0;
	void sendParam(string paramName, RemoteUIParam p);
	DecodedMessage decode(ofxOscMessage m);

	vector<string> scanForUpdatedParamsAndSync();

	void sendUpdateForParamsInList(vector<string>paramsPendingUpdate);
	bool hasParamChanged(RemoteUIParam p);	

	void updateParamFromDecodedMessage(ofxOscMessage m, DecodedMessage dm);
	void syncParamToPointer(string paramName);
	void addParamToDB(RemoteUIParam p, string paramName);
	
	void sendHELLO();
	void sendCIAO();
	void sendTEST();

	bool							readyToSend;
	ofxOscSender					sender;
	ofxOscReceiver					receiver;

	float							time;
	float							timeSinceLastReply;
	float							avgTimeSinceLastReply;
	bool							waitingForReply;

	float							updateInterval;
	int								port;

	map<string, RemoteUIParam>		params;
	map<int, string>				orderedKeys; // used to keep the order in which the params were added

	set<string>						paramsChangedSinceLastCheck;


private:

	string stringForParamType(RemoteUIParamType t);

};

#endif
