#ifndef _TEST_APP
#define _TEST_APP

#include "ofMain.h"
#include "ofxRemoteUI.h"

class testApp : public ofBaseApp{

public:
		void setup();
		void update();
		void draw();
		void exit(){ OFX_REMOTEUI_SERVER_CLOSE(); }


		float x;
		float y;
		bool drawOutlines;
		bool drawOutlines2;
		string currentFrameRate;

};

#endif