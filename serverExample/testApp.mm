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

	numCircles = 30;
	//expose vars to ofxRemoteUI server, AFTER SETUP!
	OFX_REMOTEUI_SERVER_SHARE_PARAM(x, 0, ofGetWidth(), ofColor(255,0,0,32));
	OFX_REMOTEUI_SERVER_SHARE_PARAM(y, 0, ofGetHeight());
	OFX_REMOTEUI_SERVER_SHARE_PARAM(drawOutlines, ofColor(255,0,0,32));
	OFX_REMOTEUI_SERVER_SHARE_PARAM(numCircles, 0, 30);
	OFX_REMOTEUI_SERVER_SHARE_PARAM(currentFrameRate, ofColor(0,255,0,32));

	OFX_REMOTEUI_SERVER_LOAD_FROM_XML(); //load values from XML if you want to do so
}

//--------------------------------------------------------------
void testApp::update(){

	float dt = 0.016666;

	currentFrameRate = ofToString( ofGetFrameRate() );
	OFX_REMOTEUI_SERVER_UPDATE(dt);
}


//--------------------------------------------------------------
void testApp::draw(){

	if (drawOutlines == 1) ofNoFill();
	else ofFill();

	ofTranslate(x, y);

	for(int i = 0; i < numCircles; i++){
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

void testApp::exit(){

	OFX_REMOTEUI_SERVER_CLOSE();
	OFX_REMOTEUI_SERVER_SAVE_TO_XML(); //save values to XML

}