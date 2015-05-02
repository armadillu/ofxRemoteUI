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

		void keyPressed(int key){}
		void keyReleased(int key){};

		//your ofParams
		ofParameter<float> size;
		ofParameter<int> number;
		ofParameter<bool> check;
		ofParameterGroup parameters;
		ofParameter<ofColor> color;
		ofParameter<string> name;
		ofxPanel gui;

		//sync ofParams to RemoteUI
		ofxRemoteUIofParamaterSync ruiBridge;
};
