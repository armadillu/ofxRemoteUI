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

using namespace std;

class ofxRemoteUIClient: public ofxRemoteUI{

public:

	ofxRemoteUIClient();
	void setup(string address, int port = OFXREMOTEUI_PORT);
	void update(float dt);
	void disconnect();

	void requestCompleteUpdate(); //ask the server to send us all params and presets

	void restoreAllParamsToInitialXML();
	void restoreAllParamsToDefaultValues();
	void saveCurrentStateToDefaultXML();

	//work with presets
	void setPreset(string preset); //tell server to choose a preset
	void savePresetWithName(string presetName); //take current params and make a preset with them
	void deletePreset(string presetName); //delete preset with this name

	//by doing this you allow ofxRemoteUIClient to modify your params
	//you can find out which params got changed by calling getChangedParamsList()
	void trackParam(string paramName, float* param);
	void trackParam(string paramName, bool* param); 
	void trackParam(string paramName, int* param);
	void trackParam(string paramName, string* param);
	void trackParam(string paramName, unsigned char* param); //color! 4 components!
	void trackParam(string paramName, int* param, vector<string> list);


	//this makes the ofxRemoteUIClient fetch the current values of your param
	//and send it to the server (will take actual value from the supplied pointer in trackParam())
	void sendTrackedParamUpdate(string paramName);

	//send an "untracked" param, manually
	void sendUntrackedParamUpdate(RemoteUIParam p, string paramName);

	//get notified when server gets back to us
	void setCallback( void (*callb)(RemoteUICallBackArg) );
	//if you want to get notified when stuff happens, implement a callback method like this:
	//
	//	void clientCallback(RemoteUICallBackArg msg){
	//		switch (msg) {
	//			case PARAMS_UPDATED:break;
	//			case PRESETS_UPDATED:break;
	//			case SERVER_DISCONNECTED:break;
	//			default:break;
	//		}
	//	}

	float getMinThresholdForParam(string paramMame); //only applies to int and float
	float getMaxThresholdForParam(string paramMame); //only applies to int and float

	bool isReadyToSend();

private:


	void fillPresetListFromMessage(ofxOscMessage m);

	string					host;
	bool					gotNewInfo;
	int						pendingOperations;
	void (*callBack)(RemoteUICallBackArg);

};

#endif