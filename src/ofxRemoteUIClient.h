//
//  ofxRemoteUI.h
//  emptyExample
//
//  Created by Oriol Ferrer Mesià on 09/01/13.
//
//

#ifndef __emptyExample__ofxRemoteUIClient__
#define __emptyExample__ofxRemoteUIClient__

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
	void requestCompleteUpdate();
	void sendParamUpdate(RemoteUIParam p, string paramName);

	//by doing this you allow ofxRemoteUIClient to modify your params
	//you can find out which params got changed by calling getChangedParamsList()
	void trackParam(string paramName, float* param);
	void trackParam(string paramName, bool* param); 
	void trackParam(string paramName, int* param);
	void trackParam(string paramName, string* param);

	float getMinThresholdForParam(string paramMame); //only applies to int and float
	float getMaxThresholdForParam(string paramMame); //only applies to int and float

	//send the server an update on a param (will take actual value from the supplied pointer in trackParam())
	void sendUpdatedParam(string paramName);

	bool isReadyToSend();

private:

	void sendREQUEST(); //a request for a complete list of server params
	string host;
};

#endif
