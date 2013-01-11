#include "testApp.h"
//#include "CocoaStuff.h"

//--------------------------------------------------------------
void testApp::setup(){


	ofBackground(22);
	ofSetFrameRate(60);
	ofSetVerticalSync(true);

	//start client

	client.setup("127.0.0.1", 0.5);

}

//--------------------------------------------------------------
void testApp::update(){

	float dt = 0.016666;

	client.update(dt);

	vector<string> list = client.getAllParamNamesList();

	for(int i = 0; i < list.size(); i++){

		RemoteUIParam p = client.getParamForName(list[i]);
		if (list[i] == "circleSize") circleSize = p.floatVal;
		if (list[i] == "speed") speed = p.intVal;
		if (list[i] == "numCircles") numCircles = p.boolVal;
		if (list[i] == "lineWidth") lineWidth = p.floatVal;
		if (list[i] == "currentFrameRate") currentFrameRate = p.floatVal;
	}
}

//--------------------------------------------------------------
void testApp::draw(){


	ofDrawBitmapStringHighlight("circleSize: " + ofToString(circleSize) + "\n" +
								"numCircles: " + ofToString(numCircles) + "\n" +
								"speed: " + ofToString(speed) + "\n" +
								"lineWidth: " + ofToString(lineWidth) + "\n" +
								"drawOutlines: " + ofToString(drawOutlines) + "\n" +
								"currentFrameRate: " + currentFrameRate ,
								20, 20, ofColor::black, ofColor::red);

}
