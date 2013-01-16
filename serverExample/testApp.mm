#include "testApp.h"

//--------------------------------------------------------------
void testApp::setup(){


	ofBackground(22);
	ofSetFrameRate(60);
	ofSetVerticalSync(true);

	drawOutlines = false;
	currentFrameRate = "unInited";
	x = y = 0;

	//start server
	OFX_REMOTEUI_SERVER_SETUP(10000);

	//expose vars to ofxRemoteUI server, AFTER SETUP!
	OFX_REMOTEUI_SERVER_SHARE_PARAM(x, 0, ofGetWidth());
	OFX_REMOTEUI_SERVER_SHARE_PARAM(drawOutlines);
	OFX_REMOTEUI_SERVER_SHARE_PARAM(currentFrameRate);
	OFX_REMOTEUI_SERVER_SHARE_PARAM(drawOutlines2);
	OFX_REMOTEUI_SERVER_SHARE_PARAM(y, 0, ofGetHeight());

}

//--------------------------------------------------------------
void testApp::update(){

	float dt = 0.016666;

	currentFrameRate = ofToString( ofGetFrameRate() );
	OFX_REMOTEUI_SERVER_UPDATE(dt);
}


//--------------------------------------------------------------
void testApp::draw(){

	if (drawOutlines) ofNoFill();
	else ofFill();

	ofTranslate(x, y);

	for(int i = 0; i < 30; i++){
		unsigned char r = i * 34;
		unsigned char g = i * 93;
		unsigned char b = i * 17;
		ofSetColor(r,g,b);
		ofCircle(
					-ofGetWidth() * 0.5 + ofGetWidth() * ofNoise((ofGetFrameNum() + 20 * i) * 0.004  + i),
					-ofGetHeight() * 0.5 + ofGetHeight() * ofNoise((ofGetFrameNum() + 10 * i) * 0.004 + 4 * i),
					25
				 );
	}


	ofSetupScreen();
	ofDrawBitmapStringHighlight(
								"x: " + ofToString(x) + "\n" +
								"y: " + ofToString(y) + "\n" +
								"drawOutlines: " + ofToString(drawOutlines) + "\n" +
								"currentFrameRate: " + currentFrameRate ,
								20, 20,
								ofColor::black, ofColor::red
								);

}
