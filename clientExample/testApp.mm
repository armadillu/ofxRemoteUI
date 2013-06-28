#include "testApp.h"


void testApp::setup(){

	ofBackground(22);
	ofSetFrameRate(60);
	ofSetVerticalSync(true);

	//start client
	client.setup("127.0.0.1", 10000);

	//we want the client to have acces to those values all the time, so we feed him their @
	client.trackParam("drawOutlines", &drawOutlines);
	client.trackParam("x", &x);
	client.trackParam("y", &y);
	client.trackParam("currentSentence", &currentSentence);

	time = 0.0f;
}


void testApp::update(){

	float dt = 0.016666;
	time += dt;
	client.update(dt);

	if(time > 0.2f){
		time = 0.0f;
		//request a param update to the server every 1/5th of a sec
		//in case your app updates params on its own
		//client.requestCompleteUpdate();
	}

	x = mouseX;
	y = mouseY;
	//now control some params from the client app
	//send an update to the server app
}


void testApp::draw(){

	ofDrawBitmapStringHighlight(
								string("CLIENT\n") +
								"x: " + ofToString(x) + "\n" +
								"y: " + ofToString(y) + "\n" +
								"drawOutlines: " + ofToString(drawOutlines) + "\n" +
								"currentSentence: " + currentSentence ,
								20, 20,
								ofColor::black, ofColor::red
								);

}


void testApp::mousePressed( int x, int y, int button ){
	drawOutlines = !drawOutlines;
}


void testApp::keyPressed(int key){

	if(key=='1'){
		client.savePresetWithName("a");
	}
	if(key=='2'){
		client.setPreset("a");
	} 
	if(key=='3'){
		client.deletePreset("a");
	}

	if(key=='4'){
		vector<string> params = client.getPresetsList();
		cout << "presets list" << endl;
 		for(int i = 0; i < params.size(); i++){
			cout << params[i] << endl;
		}
	}

	if(key==' '){
		client.sendTrackedParamUpdate("x");
		client.sendTrackedParamUpdate("y");
		client.sendTrackedParamUpdate("drawOutlines");
		client.sendTrackedParamUpdate("currentSentence");
	}
}
