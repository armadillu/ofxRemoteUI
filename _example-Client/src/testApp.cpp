#include "testApp.h"

//get notified when server tells us something
void clientCallback(RemoteUIClientCallBackArg a){

	switch (a.action) {
		case SERVER_CONNECTED: cout << "SERVER_CONNECTED" << endl; break;
		case SERVER_DISCONNECTED: cout << "SERVER_DISCONNECTED" << endl; break;
		case SERVER_DELETED_PRESET: cout << "SERVER_DELETED_PRESET" << endl; break;
		case SERVER_SAVED_PRESET: cout << "SERVER_SAVED_PRESET" << endl; break;
		case SERVER_DID_SET_PRESET: cout << "SERVER_DID_SET_PRESET" << endl; break;
		case SERVER_SENT_FULL_PARAMS_UPDATE: cout << "SERVER_SENT_FULL_PARAMS_UPDATE" << endl; break;
		case SERVER_PRESETS_LIST_UPDATED: cout << "SERVER_PRESETS_LIST_UPDATED" << endl; break;
		case SERVER_CONFIRMED_SAVE: cout << "SERVER_CONFIRMED_SAVE" << endl;; break;
		case SERVER_DID_RESET_TO_XML: cout << "SERVER_DID_RESET_TO_XML" << endl; break;
		case SERVER_DID_RESET_TO_DEFAULTS: cout << "SERVER_DID_RESET_TO_DEFAULTS" << endl; break;
		case NEIGHBORS_UPDATED: cout << "NEIGHBORS_UPDATED" << endl; break;
		default: break;
	}
}


void testApp::setup(){

	ofBackground(22);
	ofSetFrameRate(60);
	ofSetVerticalSync(true);

	//start client
	client.setVerbose(true);
	client.setCallback(clientCallback);
	client.setup("127.0.0.1", 10000/*port*/);
	client.connect();

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
	client.updateAutoDiscovery(dt);

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

	ofDrawBitmapStringHighlight("press SPACEBAR to send param updates", 20, ofGetHeight() - 15);
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
