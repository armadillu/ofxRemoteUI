//
//  ofxRemoteUI.h
//  emptyExample
//
//  Created by Oriol Ferrer Mesià on 09/01/13.
//
//

#ifndef __emptyExample__ofxRemoteUIServer__
#define __emptyExample__ofxRemoteUIServer__

// you will need to add this to your "Header Search Path" for ofxOsc to compile
// ../../../addons/ofxOsc/libs ../../../addons/ofxOsc/libs/oscpack ../../../addons/ofxOsc/libs/oscpack/src ../../../addons/ofxOsc/libs/oscpack/src/ip ../../../addons/ofxOsc/libs/oscpack/src/ip/posix ../../../addons/ofxOsc/libs/oscpack/src/ip/win32 ../../../addons/ofxOsc/libs/oscpack/src/osc ../../../addons/ofxOsc/src
#include "ofxRemoteUI.h"
#include "ofxOsc.h"
#include "ofxXmlSettings.h"


#include <map>
#include <set>
#include <vector>
using namespace std;


class ofxRemoteUIServer: public ofxRemoteUI{ //this is injected into your app

public:

	static ofxRemoteUIServer* instance();

	void setup(int port = OFXREMOTEUI_PORT, float updateInterval = 0.1/*sec*/);
	void update(float dt);
	void close();
	void loadFromXML();
	void saveToXML();

	void shareParam(string paramName, float* param, float min, float max, ofColor bgColor = ofColor(0,0,0,0) );
	void shareParam(string paramName, bool* param, ofColor bgColor = ofColor(0,0,0,0), int nothing = 0 ); //"nothing" args are just to match other methods
	void shareParam(string paramName, int* param, int min, int max, ofColor bgColor = ofColor(0,0,0,0) );
	void shareParam(string paramName, string* param, ofColor bgColor = ofColor(0,0,0,0), int nothing = 0 ); //"nothing" args are just to match other methods
	void setParamColor( ofColor c );

private:

	void connect(string address, int port);

	ofxRemoteUIServer(); // use ofxRemoteUIServer::instance() instead!
	static ofxRemoteUIServer* singleton;
	void setColorForParam(RemoteUIParam &p, ofColor c);

	bool colorSet; //if user called setParamColor()
	ofColor paramColor;


};
#endif
