#include "testApp.h"

//--------------------------------------------------------------
void testApp::setup(){


	ofBackground(22);
	ofSetFrameRate(60);
	ofSetVerticalSync(true);

	circleSize = 30;
	numCircles = 25;
	speed = 1;
	drawOutlines = false;
	currentFrameRate = "unInited";
	
	//expose vars to ofxRemoteUI server
	server.shareParam("circlSize", &circleSize, 5, 50);
	server.shareParam("speed", &speed, 1, 10);
	server.shareParam("numCircles", &numCircles, 0, 100);
	server.shareParam("drawOutlines", &drawOutlines);
	server.shareParam("currentFrameRate", &currentFrameRate);
	server.shareParam("lineWidth", &lineWidth, 1, 20);

	//start server
	server.setup(1./30); 

}

//--------------------------------------------------------------
void testApp::update(){

	float dt = 0.016666;

	//currentFrameRate = ofToString( ofGetFrameRate() );
	server.update(dt);
}

//--------------------------------------------------------------
void testApp::draw(){

	glLineWidth(lineWidth);

	if (drawOutlines) ofNoFill();
	else ofFill();

	for(int i = 0; i < numCircles; i++){

		unsigned char r = i * 34;
		unsigned char g = i * 93;
		unsigned char b = i * 17;
		ofSetColor(r,g,b);
		ofCircle(
					ofGetWidth() * ofNoise(ofGetFrameNum() * 0.001 * speed + i),
					ofGetWidth() * ofNoise(ofGetFrameNum() * 0.004 * speed + 4 * i),
					circleSize
				 );
	}

	ofDrawBitmapStringHighlight("circleSize: " + ofToString(circleSize) + "\n" +
								"numCircles: " + ofToString(numCircles) + "\n" +
								"speed: " + ofToString(speed) + "\n" +
								"lineWidth: " + ofToString(lineWidth) + "\n" +
								"drawOutlines: " + ofToString(drawOutlines) + "\n" +
								"currentFrameRate: " + currentFrameRate ,
								20, 20, ofColor::black, ofColor::red);

}
