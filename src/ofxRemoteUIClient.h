//
//  ofxRemoteUI.h
//  emptyExample
//
//  Created by Oriol Ferrer Mesià on 09/01/13.
//
//

#ifndef _ofxRemoteUIClient__
#define _ofxRemoteUIClient__

// you will need to add this to your "Header Search Path" for ofxOsc to compile
// ../../../addons/ofxOsc/libs ../../../addons/ofxOsc/libs/oscpack ../../../addons/ofxOsc/libs/oscpack/src ../../../addons/ofxOsc/libs/oscpack/src/ip ../../../addons/ofxOsc/libs/oscpack/src/ip/posix ../../../addons/ofxOsc/libs/oscpack/src/ip/win32 ../../../addons/ofxOsc/libs/oscpack/src/osc ../../../addons/ofxOsc/src
#include "ofxRemoteUI.h"
#include "ofxOsc.h"
#include <map>
#include <set>
#include <vector>
#include "ofxRemoteUINeigbors.h"

#define OSC_CHECK	if(!OSCsetup) { printf("OSC NOT SETUP!\n");return; }

class ofxRemoteUIClient: public ofxRemoteUI{

public:

	ofxRemoteUIClient();
	bool setup(std::string address, int port = OFXREMOTEUI_PORT);
	void update(float dt);
	void updateAutoDiscovery(float dt);

	void connect();
	void disconnect();

	void requestCompleteUpdate(); //ask the server to send us all params and presets

	void restoreAllParamsToInitialXML();
	void restoreAllParamsToDefaultValues();
	void saveCurrentStateToDefaultXML();

	//work with presets
	void setPreset(std::string preset); //tell server to choose a preset
	void savePresetWithName(std::string presetName); //take current params and make a preset with them
	void deletePreset(std::string presetName); //delete preset with this name

	//work with group presets
	void setGroupPreset(std::string preset, std::string group); //tell server to choose a preset
	void saveGroupPresetWithName(std::string presetName, std::string group); //take current params and make a preset with them
	void deleteGroupPreset(std::string presetName, std::string group); //delete preset with this name


	//by doing this you allow ofxRemoteUIClient to modify your params
	//you can find out which params got changed by calling getChangedParamsList()
	void trackParam(std::string paramName, float* param);
	void trackParam(std::string paramName, bool* param);
	void trackParam(std::string paramName, int* param);
	void trackParam(std::string paramName, std::string* param);
	void trackParam(std::string paramName, unsigned char* param); //color! 4 components!
	void trackParam(std::string paramName, int* param, std::vector<std::string> list);

	//this makes the ofxRemoteUIClient fetch the current values of your param
	//and send it to the server (will take actual value from the supplied pointer in trackParam())
	void sendTrackedParamUpdate(std::string paramName);

	//send an "untracked" param, manually
	void sendUntrackedParamUpdate(RemoteUIParam p, std::string paramName);
    
    // Send message via OSC
    void sendMessage(ofxOscMessage m);

	//get notified when server gets back to us
	void setCallback( void (*callb)(RemoteUIClientCallBackArg) );
	//if you want to get notified when stuff happens, implement a callback method like this:
	//
	//	void clientCallback(RemoteUIClientCallBackArg arg){
	//		switch (arg.action) {
	//			case SERVER_SENT_FULL_PARAMS_UPDATE:break;
	//			case SERVER_PRESETS_LIST_UPDATED:break;
	//			case SERVER_DISCONNECTED:break;
	//			default:break;
	//		}
	//	}

	std::vector<Neighbor> getNeighbors();
	bool isReadyToSend();
	bool isSetup();

private:

	void 				removeParamFromDB(const std::string & paramName);

	void					fillPresetListFromMessage(ofxOscMessage m);

	bool					OSCsetup;
	std::string				host;
	int						pendingOperations;
	ofxOscReceiver			broadcastReceiver;
	ofxRemoteUINeigbors		closebyServers;

	void (*callBack)(RemoteUIClientCallBackArg);

};

#endif
