#include "testApp.h"

//--------------------------------------------------------------
void testApp::setup(){

	ofBackground(22);
	ofSetFrameRate(60);
	ofSetVerticalSync(true);

	//set some default values
	drawOutlines = false;
	currentSentence = "unInited";
	x = y = 0;
	numCircles = 30;
	menu = MENU_OPTION_1;

	OFX_REMOTEUI_SERVER_SETUP(10000); 	//start server

	//expose vars to the server. Always do so after setting up the server.
	OFX_REMOTEUI_SERVER_SET_UPCOMING_PARAM_COLOR( ofColor(255,0,0,64) ); // set a bg color for the upcoming params
	OFX_REMOTEUI_SERVER_SET_UPCOMING_PARAM_GROUP("position"); //make a new group
	OFX_REMOTEUI_SERVER_SHARE_PARAM(x, 0, ofGetWidth() );
	OFX_REMOTEUI_SERVER_SHARE_PARAM(y, 0, ofGetHeight());

	OFX_REMOTEUI_SERVER_SET_UPCOMING_PARAM_GROUP("style"); //make a new group
	OFX_REMOTEUI_SERVER_SET_UPCOMING_PARAM_COLOR( ofColor(0,255,0,64) ); // start a new "group" of params by setting a new color
	OFX_REMOTEUI_SERVER_SHARE_PARAM(drawOutlines);
	OFX_REMOTEUI_SERVER_SHARE_PARAM(numCircles, 0, 30);

	OFX_REMOTEUI_SERVER_SET_UPCOMING_PARAM_GROUP("whatever"); //make a new group
	OFX_REMOTEUI_SERVER_SHARE_PARAM(currentSentence, ofColor(0,0,255,64)); // you can also set a color on a per-param basis
	OFX_REMOTEUI_SERVER_SHARE_PARAM(currentMouseX, 0, ofGetWidth(), ofColor(255,64));

	vector<string> menuItems;
	menuItems.push_back("MENU_OPTION_0");menuItems.push_back("MENU_OPTION_1");
	menuItems.push_back("MENU_OPTION_2"); menuItems.push_back("MENU_OPTION_3");

	OFX_REMOTEUI_SERVER_SHARE_ENUM_PARAM(menu,MENU_OPTION_0, MENU_OPTION_3, menuItems);

	OFX_REMOTEUI_SERVER_SHARE_PARAM(test4, 0, 30);

	OFX_REMOTEUI_SERVER_LOAD_FROM_XML();	//load values from XML, if you want to do so
											//this will result on the UI showing the params
											//as they were when last saved (on quit in this case)
}

//--------------------------------------------------------------
void testApp::update(){

	float dt = 0.016666;

	currentMouseX = ofGetMouseX();
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
								string("SERVER\n") + 
								"x: " + ofToString(x) + "\n" +
								"y: " + ofToString(y) + "\n" +
								"drawOutlines: " + ofToString(drawOutlines) + "\n" +
								"currentSentence: " + currentSentence  + "\n" +
								"menu item: " + ofToString(menu) ,
								20, 20,
								ofColor::black, ofColor::red
								);

}

void testApp::exit(){

	OFX_REMOTEUI_SERVER_CLOSE();		//setop the server
	OFX_REMOTEUI_SERVER_SAVE_TO_XML();	//save values to XML

}