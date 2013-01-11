#ifndef _TEST_APP
#define _TEST_APP

#include "ofMain.h"
//#import "CocoaStuff.h"
//#include "ofxCocoaWindowNibless.h"
#include "ofxRemoteUI.h"



class testApp : public ofBaseApp{
	public:
		void setup();
		void update();
		void draw();
		

		ofxRemoteUIClient client;

		float circleSize;
		float speed;
		int numCircles;
		bool drawOutlines;
		float lineWidth;
		string currentFrameRate;

};

#endif