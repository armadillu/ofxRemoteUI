#ifndef _TEST_APP
#define _TEST_APP

#include "ofMain.h"
#include "ofxRemoteUI.h"


class testApp : public ofBaseApp{
	public:
		void setup();
		void update();
		void draw();

		void mousePressed( int x, int y, int button );
		
		

		ofxRemoteUIClient client;

		float x;
		float y;
		bool drawOutlines;
		string currentFrameRate;

		float time;
};

#endif