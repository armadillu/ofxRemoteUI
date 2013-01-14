#include "testApp.h"

//--------------------------------------------------------------
void testApp::setup(){


	ofBackground(22);
	ofSetFrameRate(60);
	ofSetVerticalSync(true);

	//start client

	client.setup("127.0.0.1", 10000);
	client.trackParam("drawOutlines", &drawOutlines);
	client.trackParam("x", &x);
	client.trackParam("y", &y);
	client.trackParam("currentFrameRate", &currentFrameRate);

	time = 0;
}

//--------------------------------------------------------------
void testApp::update(){

	float dt = 0.016666;
	time += dt;
	client.update(dt);



	if(time > 0.2){
		//request a param update to the server every 1/5th of a sec
		//in case your app updates params on its own
		//client.requestCompleteUpdate();
	}

	//now control some params from the client app
	x = mouseX;
	y = mouseY;
	client.sendUpdatedParam("x");
	client.sendUpdatedParam("y");
	client.sendUpdatedParam("drawOutlines");

}

//--------------------------------------------------------------
void testApp::draw(){


	ofDrawBitmapStringHighlight(
								"x: " + ofToString(x) + "\n" +
								"y: " + ofToString(y) + "\n" +
								"drawOutlines: " + ofToString(drawOutlines) + "\n" +
								"currentFrameRate: " + currentFrameRate ,
								20, 20,
								ofColor::black, ofColor::red
								);

}

void testApp::mousePressed( int x, int y, int button ){
	drawOutlines = !drawOutlines;
}
