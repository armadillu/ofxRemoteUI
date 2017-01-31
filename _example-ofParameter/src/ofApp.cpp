#include "ofApp.h"


void ofApp::setup(){

	//setup ofParameters as usual
	parameters.setName("parameters");

	amount.setName("amount");
	style.setName("style");
	naming.setName("naming");

	style.add(size.set("size",10,0,100));
	style.add(wireframe.set("wireframe",false));
	style.add(color.set("color",ofColor(127),ofColor(0,0),ofColor(255)));

	amount.add(number.set("number",10,1,25));
	naming.add(name1.set("name1", "myStringValue"));
	naming.add(name2.set("name2", "myStringValue"));

	parameters.add(style);
	style.add(naming);
	parameters.add(amount);

	gui.setup(parameters);

	//setup the RemoteUIServer
	RUI_SETUP();

	//setup the syncing among them
	ruiBridge.setup(parameters);

}


void ofApp::draw(){
	gui.draw();

	ofSetColor(color);

	if(wireframe) ofNoFill();
	else ofFill();

	for(int i=0;i<number;i++){
		ofCircle(ofGetWidth()*.5-size*((number-1)*0.5-i), ofGetHeight()*.5, size);
	}
}

