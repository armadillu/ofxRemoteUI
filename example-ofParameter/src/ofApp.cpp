#include "ofApp.h"


void ofApp::setup(){

	//setup ofParameters as usual
	parameters.setName("parameters");
	parameters.add(size.set("size",10,0,100));
	parameters.add(number.set("number",10,0,100));
	parameters.add(check.set("check",false));
	parameters.add(name.set("name", "myStringValue"));

	parameters.add(color.set("color",ofColor(127),ofColor(0,0),ofColor(255)));
	gui.setup(parameters);

	//setup the RemoteUIServer
	RUI_SETUP();

	//setup the syncing among them
	//note I'm feeding the gui params, to sync the gui events 3-way
	ruiBridge.setup((ofParameterGroup&)gui.getParameter());
}

void ofApp::draw(){
	gui.draw();
	ofSetColor(color);
	for(int i=0;i<number;i++){
		ofCircle(ofGetWidth()*.5-size*((number-1)*0.5-i), ofGetHeight()*.5, size);
	}
}

