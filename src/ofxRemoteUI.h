//
//  ofxRemoteUI.h
//  emptyExample
//
//  Created by Oriol Ferrer Mesià on 09/01/13.
//
//

#ifndef __emptyExample__ofxRemoteUI__
#define __emptyExample__ofxRemoteUI__

#include "ofxOsc.h"

#include <map>
using namespace std;

#define OFXREMOTEUI_SERVER_PORT 34834
#define OFXREMOTEUI_CLIENT_PORT (OFXREMOTEUI_SERVER_PORT + 1)

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
 CLIENT:	CIAO								//client disconnects ?
 SERVER:	CIAO								//server disconnects ?

 
 // SERVER API

	server.setup(refreshRate);
	server.shareParam("paramName", &paramName, ... );
	...
	server.update(dt);

 
 // CLIENT API
 
	client.setup(ipAddress, refreshRate);
	client.trackParam("paramName", &paramName);
	...
	client.update();
 

	float getMinThresholdForParam("paramMame"); //only applies to int and float
	float getMaxThresholdForParam("paramMame"); //only applies to int and float

	//get a report of params that changed on the server side since last check
	vector<string> updatedParamsList = client.getChangeList();
 
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
	HELO_ACTION, REQUEST_ACTION, SEND_ACTION, CIAO_ACTION
};

enum ArgType{
	FLT_ARG, INT_ARG, BOL_ARG, STR_ARG, NULL_ARG
};

struct DecodedMessage{
	ActionType action;
	ArgType argument;
	string paramName;
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
	};

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
};


class ofxRemoteUI{

public:

	vector<string> getAllParamNamesList();
	RemoteUIParam getParamForName(string paramName);
	bool ready();

protected:

	virtual void update(float dt) = 0;
	void sendParam(string paramName, RemoteUIParam p);
	DecodedMessage decode(ofxOscMessage m);

	vector<string> scanForUpdatedParamsAndSync();

	void sendUpdateForParamsInList(vector<string>paramsPendingUpdate);
	bool hasParamChanged(RemoteUIParam p);

	void updateParamFromDecodedMessage(ofxOscMessage m, DecodedMessage dm);
	void syncParamToPointer(string paramName);

	void sendHELLO();
	void sendCIAO();

	void connect(string address, int port);

	bool readyToSend;
	ofxOscSender sender;
	ofxOscReceiver receiver;

	float time;
	float updateInterval;

	map<string, RemoteUIParam> params;

private:


	string stringForParamType(RemoteUIParamType t);

};


class ofxRemoteUIServer: public ofxRemoteUI{ //this is injected into your app

public:

	ofxRemoteUIServer();
	void setup(float updateInterval = 0.5/*sec*/);
	void update(float dt);

	void shareParam(string paramName, float* param, float min, float max);
	void shareParam(string paramName, bool* param );
	void shareParam(string paramName, int* param, int min, int max);
	void shareParam(string paramName, string* param );

private:


	void addParamToDB(RemoteUIParam p, string paramName);

};


class ofxRemoteUIClient: public ofxRemoteUI{

public:

	ofxRemoteUIClient();
	void setup(string address, float updateInterval_ = 0.5);
	void update(float dt);
	void requestCompleteUpdate();
	void sendParamUpdate(RemoteUIParam p, string paramName);

private:

	void sendREQUEST();
	string host;
};

#endif
