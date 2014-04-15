#ifndef _TEST_APP
#define _TEST_APP

#include "ofMain.h"
#include "ofxRemoteUIServer.h"

class testApp : public ofBaseApp{

enum MenuItems{
	MENU_OPTION_0, MENU_OPTION_1, MENU_OPTION_2, MENU_OPTION_3
};

public:
		void setup();
		void update();
		void draw();
		void keyPressed( int key );

		float x;
		float y;
		bool drawOutlines;
		int numCircles;
		float circleSize;
		string unloadTest; 

		ofColor color;
		MenuItems menu;

		string currentSentence; 
		int currentMouseX;
		static void serverCallback(RemoteUIServerCallBackArg arg);
};

#endif