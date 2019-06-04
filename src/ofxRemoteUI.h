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
#include <set>
#include <vector>
#include <map>

#if !defined(__APPLE__) && !defined(_WIN32)
    #ifndef __linux__
        #define __linux__
    #endif
#endif

#if defined(__APPLE__) || defined(__linux__)
#include <arpa/inet.h>
#include <net/if.h>
#include <ifaddrs.h>
#include <errno.h>
#endif


#if __cplusplus>=201103L || defined(_MSC_VER)
	#include <unordered_map>
	#include <memory>
#else
	#include <tr1/unordered_map>
	using std::tr1::unordered_map;
#endif


#define OFXREMOTEUI_PORT									10000
#define OFXREMOTEUI_BROADCAST_PORT							25748
#define OFXREMOTEUI_BORADCAST_INTERVAL						1 /*secs*/
#define OFXREMOTEUI_NEIGHBOR_DEATH_BY_TIME					1.1 /*sec*/

#define OFXREMOTEUI_LATENCY_TEST_RATE						0.3333
#define OFXREMOTEUI_DISCONNECTION_STRIKES					6
#define OFXREMOTEUI_CONNECTION_TIMEOUT						OFXREMOTEUI_LATENCY_TEST_RATE * OFXREMOTEUI_DISCONNECTION_STRIKES
#define OFXREMOTEUI_PRESET_FILE_EXTENSION					"rui"
#define OFXREMOTEUI_SETTINGS_FILENAME						string("ofxRemoteUISettings." + string(OFXREMOTEUI_PRESET_FILE_EXTENSION))
#define OFXREMOTEUI_SETTINGS_BACKUP_FOLDER					"ofxRemoteUISettings Backups"
#define OFXREMOTEUI_NUM_BACKUPS								20
#define OFXREMOTEUI_XML_FORMAT_VER							"2"
#define OFXREMOTEUI_XML_ROOT_TAG							"OFX_REMOTE_UI"
#define OFXREMOTEUI_XML_TAG									"OFX_REMOTE_UI_PARAMS"
#define OFXREMOTEUI_XML_PORT_TAG							"OFX_REMOTE_UI_SERVER_PORT"
#define OFXREMOTEUI_XML_ENABLED_TAG							"OFX_REMOTE_UI_SERVER_ENABLED"
#define OFXREMOTEUI_DEFAULT_PARAM_GROUP						"defaultGroup"
#define OFXREMOTEUI_PRESET_DIR								"ofxRemoteUIPresets"
#define OFXREMOTEUI_NO_PRESETS								"NO_PRESETS"
#define OFXREMOTEUI_XML_V_TAG								"OFX_REMOTE_UI_V"

#define OFXREMOTEUI_COLOR_PARAM_XML_TAG						"REMOTEUI_PARAM_COLOR"
#define OFXREMOTEUI_INT_PARAM_XML_TAG						"REMOTEUI_PARAM_INT"
#define OFXREMOTEUI_FLOAT_PARAM_XML_TAG						"REMOTEUI_PARAM_FLOAT"
#define	OFXREMOTEUI_BOOL_PARAM_XML_TAG						"REMOTEUI_PARAM_BOOL"
#define OFXREMOTEUI_STRING_PARAM_XML_TAG					"REMOTEUI_PARAM_STRING"
#define OFXREMOTEUI_ENUM_PARAM_XML_TAG						"REMOTEUI_PARAM_ENUM"

#define OFXREMOTEUI_PARAM_NAME_XML_KEY						"paramName"
#define OFXREMOTEUI_UNKNOWN_PARAM_NAME_XML_KEY				"unnamedParamName"


#ifdef OF_VERSION_MINOR
    #define OF_AVAILABLE
#else
    //#error "no OF"
#endif

#include "RemoteParam.h"


#define RUI_LOCAL_IP_ADDRESS "127.0.0.1"

/*

 // COMM ////////////////////////////////////////////////////////////////////////////////////

 // init / setup connection /////////////////////////////////////////////////////////////////

 CLIENT:	/HELO								//client says HI
 SERVER:	/HELO								//server says HI
 CLIENT:	/REQU								//client requests all params from server
 SERVER:	/SEND FLT PARAM_NAME val (float)		//server starts sending all vars one by one
 SERVER:	/SEND INT PARAM_NAME2 val (int)
 SERVER:	/SEND BOL PARAM_NAME3 val (bool)
 SERVER:	/SEND STR PARAM_NAME4 val (string)
 SERVER:	/SEND ENU PARAM_NAME5 val (int)
 SERVER:	/SEND COL PARAM_NAME6 val (int)
 SERVER:	/REQU OK								//server closes REQU
 ...
 CLIENT:	/PREL								//Preset List - client requests list of presets
 SERVER:	/PREL PRESET_NAME_LIST(n)			//server sends all preset names
 ...

 // normal operation ///////////////////////////////////////////////////////////////////////

 // client updates a param
 CLIENT:	/SEND *** PARAM_NAME val (*** Type)		//client sends a var change to server, where *** is FLT, INT, BOL, etc

 // keep alive
 CLIENT:	/TEST								//every OFXREMOTEUI_LATENCY_TEST_RATE, client sends a msg to server to measure delay and connectivity
 SERVER:	/TEST								//server replies

 //client sets a preset
 CLIENT:	/SETP PRESET_NAME					//Set Preset - client wants to change all params according to preset "X"
 SERVER:	/SETP PRESET_NAME OK					//server says ok
 SERVER:	/MISP PRESET_NAME (param list)		//server reports missing params not set in this preset
 CLIENT:	/REQU								//client wants values for that preset
 SERVER:	/SEND *****							//server sends all params
 SERVER:	/REQU OK								//server closes REQU

 // client saves a preset
 CLIENT:    /SAVP PRESET_NAME					//Save Preset - client wants to save current params as a preset named PRESET_NAME, overwrites if already exists
 SERVER:	/SAVP PRESET_NAME OK					//server replies OK
 CLIENT:	/PREL								//Client requests full list of presets
 SERVER:	/PREL PRESET_NAME_LIST(n)			//server sends all preset names

 // client deletes a preset
 CLIENT:	/DELP PRESET_NAME					//client wants to delete preset named PRESET_NAME
 SERVER:	/DELP PRESET_NAME OK					//server says ok
 CLIENT:	/PREL								//Client requests full list of presets
 SERVER:	/PREL PRESET_NAME_LIST(n)			//server sends all preset names

 // client resets to default xml values
 CLIENT:	/RESX								//client wants to reset all params from the 1st loaded xml (last saved settings)
 SERVER:	/RESX OK								//server says ok
 CLIENT:	/REQU								//client wants ALL values
 SERVER:	/SEND *****							//server sends all params
 SERVER:	/REQU OK								//server closes REQU

 // client resets to starting values
 CLIENT:	/RESD								//client wants to reset all params from the code defaults
 SERVER:	/RESD OK								//server says ok
 CLIENT:	/REQU								//client wants ALL values
 SERVER:	/SEND *****							//server sends all params
 SERVER:	/REQU OK								//server closes REQU

 // client saves current params to default xml
 CLIENT:	/SAVE								//client wants to save current state of params to default xml
 SERVER:	/SAVE OK

 //client creates a group preset
 CLIENT:    /SAVp PRESET_NAME					//Save Preset - client wants to save current params as a preset named PRESET_NAME, overwrites if already exists
 SERVER:	/SAVp PRESET_NAME OK					//server replies OK
 CLIENT:	/PREL								//Client requests full list of presets
 SERVER:	/PREL PRESET_NAME_LIST(n)			//server sends all preset names

 // client deletes a group preset
 CLIENT:	/DELp PRESET_NAME					//client wants to delete preset named PRESET_NAME
 SERVER:	/DELp PRESET_NAME OK					//server says ok
 CLIENT:	/PREL								//Client requests full list of presets
 SERVER:	/PREL PRESET_NAME_LIST(n)			//server sends all preset names

 //client sets a group preset
 CLIENT:	/SETp PRESET_NAME					//Set Preset - client wants to change all params according to preset "X"
 SERVER:	/SETp PRESET_NAME OK					//server says ok
 SERVER:	/MISP PRESET_NAME (param list)		//server reports missing params not set in this group preset
 CLIENT:	/REQU								//client wants values for that preset
 SERVER:	/SEND *****							//server sends all params -- FIXME: only send params in this group!
 SERVER:	/REQU OK								//server closes REQU

 //server wants to remove a param
 SERVER: /REMp PARAM_NAME
 CLIENT: /REMp PARAM_NAME OK

 // client disconnects
 CLIENT:	/CIAO								//client disconnects
 SERVER:	/CIAO								//server disconnects


 // self-advertising: server broadcasts itself every OFXREMOTEUI_BORADCAST_INTERVAL on OSC port OFXREMOTEUI_BROADCAST_PORT
 // this is for clients to see all available servers on the network
 SERVER:	send packet with contents: arg0(oscPort:int) arg1(hostName:string) arg2(appName:string)


 // SERVER API ////////////////////////////////////////

 server->setup(refreshRate);
 server->shareParam("paramName", &paramName, ... );
 ...
 server->update(dt);

 ...
 server->pushParamsToClient(); //force an update of the client UI (bc the server changed some params and wants to force an update)

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

const char *get_filename_ext(const char *filename) ;

#ifdef _WIN32
void GetHostName(std::string& host_name);
#endif

class ofxRemoteUI{

friend class OscQueryServerMgr;

public:

	std::vector<std::string> getAllParamNamesList();
	std::vector<std::string> getChangedParamsList(); //in user add order
	RemoteUIParam getParamForName(const std::string & paramName);
	RemoteUIParam& getParamRefForName(const std::string & paramName); //careful with this!
	bool paramExistsForName(const std::string & paramName);
	std::vector<std::string> getPresetsList();

	void setParamDescription(const std::string & param, const std::string & description);

	std::string getValuesAsString(const std::vector<std::string> & paramList = std::vector<std::string>()); //supply param list to get only those, supply empty list to get all params
	void setValuesFromString(const std::string & values);

	virtual void restoreAllParamsToInitialXML() = 0;	//call this on client to request server to do so
	virtual void restoreAllParamsToDefaultValues() = 0;

	bool ready();
	float connectionLag();
	void setVerbose(bool b);

	virtual void sendUntrackedParamUpdate(RemoteUIParam p, std::string paramName){};

	std::string getMyIP(std::string userChosenInteface, std::string & subnetMask);

	std::recursive_mutex & getDataMutex(){return dataMutex;}

protected:

	virtual void update(float dt) = 0;
	void sendParam(std::string paramName, const RemoteUIParam & p);
	DecodedMessage decode(const ofxOscMessage & m);

	std::vector<std::string> scanForUpdatedParamsAndSync();	//goes through all params, comparing * to real value
															//reports those that are out of sync

	void sendUpdateForParamsInList(std::vector<std::string>paramsPendingUpdate);
	bool hasParamChanged(const RemoteUIParam & p);

	void updateParamFromDecodedMessage(const ofxOscMessage & m, DecodedMessage dm);
	void syncAllParamsToPointers();
	void syncAllPointersToParams();
	void syncParamToPointer(const std::string & paramName); //copies the param0s pointer value over the value
	void syncPointerToParam(const std::string & paramName); //the other way around
	bool addParamToDB(const RemoteUIParam & p, const std::string & paramName);

	void clearOscReceiverMsgQueue();

	void sendREQU(bool confirm = false); //a request for a complete list of server params
	void sendHELLO();
	void sendCIAO();
	void sendTEST();
	void sendPREL(std::vector<std::string> presetNames);
	void sendSAVP(std::string presetName, bool confirm = false);
	void sendSETP(std::string presetName, bool confirm = false);
	void sendDELP(std::string presetName, bool confirm = false);
	void sendRESX(bool confirm = false); //send a "restore fom first loaded XML" msg
	void sendRESD(bool confirm = false); //send a "restore fom code defaults" msg
	void sendSAVE(bool confirm = false);
	void sendMISP(std::vector<std::string> missingParamsInPreset);
	void sendREMp(const std::string & paramName); //server sends client a msg about removing a param from the GUI

	//group preset methods (note lowercase p, l)
	void sendSAVp(std::string presetName, std::string group, bool confirm = false);
	void sendSETp(std::string presetName, std::string group, bool confirm = false);
	void sendDELp(std::string presetName, std::string group, bool confirm = false);
    
    virtual void sendMessage(ofxOscMessage m) = 0; // b/c native Client only uses an OSC sender, but server might use WS

	void printAllParamsDebug();

	bool							verbose_;
	bool							readyToSend;
	ofxOscSender					oscSender;
	ofxOscReceiver					oscReceiver;


	float							timeCounter;
	float							timeSinceLastReply;
	float							avgTimeSinceLastReply;
	bool							waitingForReply;
	int								disconnectStrikes;

	float							updateInterval;
	int								port;

	std::string							userSuppliedNetInterface; //store user preference on network interface to use

	std::unordered_map<std::string, RemoteUIParam>			params;
	std::map<int, std::string>								orderedKeys; // used to keep the order in which the params were added

	std::vector<std::string>								presetNames;
	std::vector<std::string>								paramsChangedSinceLastCheck;

	std::unordered_map<std::string, RemoteUIParam>		paramsFromCode; //this will hold a copy of all the params as they where when shared first
	std::unordered_map<std::string, RemoteUIParam>		paramsFromXML; //this will hold a copy of all the params as they where when first loaded from XML
	std::unordered_map<std::string, bool>				paramsWereLoadedFromXML;

	//lets keep track of all params that have been sent through OSC we are trying to solve
	//the situation where params are added and removed on the fly as the app runs
	std::set<std::string>								paramsSentOverOsc;


	struct ScreenNotifArg{
		std::string paramName;
		RemoteUIParam p;
		ofColor bgColor;
	};

	#ifdef OF_AVAILABLE
	ofFastEvent<ScreenNotifArg> eventShowParamUpdateNotification; //this is a horrible hack to be able to show
	#endif
															//screen notifications on the server triggered from the supper class

	std::recursive_mutex dataMutex;

private:

	std::string stringForParamType(RemoteUIParamType t);
	RemoteUIParam nullParam;

};

void split(std::vector<std::string> &tokens, const std::string &text, char separator);

#endif
