#ifndef _TEST_APP
#define _TEST_APP

#include "ofMain.h"
#include "ofxRemoteUIServer.h"

class testApp : public ofBaseApp{

public:
		void setup();
		void update();
		void draw();
		void exit();


		float x;
		float y;
		bool drawOutlines;
		int numCircles;
		string currentFrameRate;

	int test;
};

#endif