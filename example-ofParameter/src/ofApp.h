#pragma once

#include "ofMain.h"
#include "ofxGui.h"

#include "ofxRemoteUIServer.h"
#include "ofxRemoteUIofParamaterSync.h"

class ofApp : public ofBaseApp{

	public:
		void setup();
		void update(){};
		void draw();

		void keyPressed(int key){ cout << "pressed" << endl; }
		void keyReleased(int key){};

		ofParameterGroup parameters;
		ofParameterGroup amount;
		ofParameterGroup style;
		ofParameterGroup naming;

		ofParameter<float> size;
		ofParameter<int> number;
		ofParameter<bool> wireframe;

		ofParameter<ofColor> color;
		ofParameter<string> name1;
		ofParameter<string> name2;

		ofxPanel gui;

		//sync ofParams to RemoteUI
		ofxRemoteUIofParamaterSync ruiBridge;
};

