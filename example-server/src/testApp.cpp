#include "testApp.h"


//--------------------------------------------------------------
void testApp::setup(){

	ofBackground(22);
	ofSetFrameRate(60);
	ofSetVerticalSync(true);
	ofSetCircleResolution(32);
	ofEnableAlphaBlending();

	//set some default values for your params (optional)
	drawOutlines = false;
	currentSentence = "unInited";
	x = y = 66;
	numCircles = 30;
	menu = MENU_OPTION_1;

	// START THE SERVER ///////////////////////////////////////////
	OFX_REMOTEUI_SERVER_SETUP(); 	//specify a port if you want a specific one
									//if you dont specify, the server will choose a random one
									//the first time you launch it, and will use it forever

	// SETUP A CALLBACK ///////////////////////////////////////////
	OFX_REMOTEUI_SERVER_SET_CALLBACK(testApp::serverCallback); // (optional!)

	// SET PARAM GROUPS / COLORS //////////////////////////////////
	OFX_REMOTEUI_SERVER_SET_NEW_COLOR(); // set a bg color for all the upcoming params (optional)
	OFX_REMOTEUI_SERVER_SET_UPCOMING_PARAM_GROUP("POSITION"); //make a new group (optional)

	// SHARE A FLOAT PARAM ////////////////////////////////////////
	OFX_REMOTEUI_SERVER_SHARE_PARAM(x, 0, ofGetWidth() ); //add an "x" float param to the current group ("position")
	OFX_REMOTEUI_SERVER_SHARE_PARAM(y, 0, ofGetHeight()); //provide a variable, a rangeMin and a rangeMax

	OFX_REMOTEUI_SERVER_SET_UPCOMING_PARAM_GROUP("STYLE"); //make a new group (optional)
	OFX_REMOTEUI_SERVER_SET_NEW_COLOR(); // set a bg color for the upcoming params (optional)

	// SHARE A BOOL PARAM ////////////////////////////////////////
	OFX_REMOTEUI_SERVER_SHARE_PARAM(drawOutlines);

	OFX_REMOTEUI_SERVER_SET_NEW_COLOR(); //slighly change the bg color within the group
	OFX_REMOTEUI_SERVER_SHARE_PARAM(numCircles, 0, 30);	//variable, rangeMin, rangeMax


	OFX_REMOTEUI_SERVER_SET_NEW_COLOR(); //slighly change the bg color within the group
	OFX_REMOTEUI_SERVER_SHARE_PARAM(circleSize, 1, 30);	//variable, rangeMin, rangeMax

	OFX_REMOTEUI_SERVER_SET_UPCOMING_PARAM_GROUP("OTHER"); //make a new group

	// SHARE AN STRING PARAM //////////////////////////////////////
	OFX_REMOTEUI_SERVER_SHARE_PARAM(currentSentence, ofColor(0,0,255,64));	// you can also set a color on a per-param basis

	// SHARE A FLOAT //////////////////////////////////////
	OFX_REMOTEUI_SERVER_SHARE_PARAM(currentMouseX, 0, ofGetWidth());

	// SHARE AN ENUM PARAM //////////////////////////////////////
	vector<string> menuItems;
	menuItems.push_back("MENU_OPTION_0");menuItems.push_back("MENU_OPTION_1");
	menuItems.push_back("MENU_OPTION_2"); menuItems.push_back("MENU_OPTION_3");
	OFX_REMOTEUI_SERVER_SHARE_ENUM_PARAM(menu, MENU_OPTION_0, MENU_OPTION_3, menuItems);

	// SHARE A COLOR PARAM //////////////////////////////////////
	OFX_REMOTEUI_SERVER_SHARE_COLOR_PARAM(color);


	OFX_REMOTEUI_SERVER_LOAD_FROM_XML();	//load values from XML, if you want to do so
											//this will result on the UI showing the params
											//as they were when last saved (on quit in this case)


	//OFX_REMOTEUI_SERVER_START_THREADED();   //if you want all the communication to happen on a different
											//thread, call this. This has implications though.
											//your params can be changed at anytime by the client,
											//potentially leading to problems. String params are
											//especially very likely to cause crashes!
											//so don't use this unless you know you need it!
}

//--------------------------------------------------------------
void testApp::update(){

	float dt = 0.016666;

	currentMouseX = ofGetMouseX();
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
					circleSize
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
								ofColor::black, color
								);
}

//define a callback method to get notifications of client actions
void testApp::serverCallback(RemoteUIServerCallBackArg arg){

	switch (arg.action) {
		case CLIENT_CONNECTED: cout << "CLIENT_CONNECTED" << endl; break;
		case CLIENT_DISCONNECTED: cout << "CLIENT_DISCONNECTED" << endl; break;
		case CLIENT_UPDATED_PARAM: cout << "CLIENT_UPDATED_PARAM: "<< arg.paramName << ": ";
			arg.param.print();
			break;
		case CLIENT_DID_SET_PRESET: cout << "CLIENT_DID_SET_PRESET" << endl; break;
		case CLIENT_SAVED_PRESET: cout << "CLIENT_SAVED_PRESET" << endl; break;
		case CLIENT_DELETED_PRESET: cout << "CLIENT_DELETED_PRESET" << endl; break;
		case CLIENT_SAVED_STATE: cout << "CLIENT_SAVED_STATE" << endl; break;
		case CLIENT_DID_RESET_TO_XML: cout << "CLIENT_DID_RESET_TO_XML" << endl; break;
		case CLIENT_DID_RESET_TO_DEFAULTS: cout << "CLIENT_DID_RESET_TO_DEFAULTS" << endl; break;
		default:break;
	}
}

void testApp::keyPressed( int key ){
	//force an update in the client side (same as pressing sync button on osx client)
	OFX_REMOTEUI_SERVER_PUSH_TO_CLIENT();
}
