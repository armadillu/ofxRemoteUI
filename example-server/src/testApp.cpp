#include "testApp.h"


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
	unloadTest = "inited from source";

	// START THE SERVER ///////////////////////////////////////////
	RUI_SETUP(); 	//specify a port if you want a specific one
					//if you dont specify, the server will choose a random one
					//the first time you launch it, and will use it forever

	// SETUP A CALLBACK ///////////////////////////////////////////
	ofAddListener(RUI_GET_OF_EVENT(), this, &testApp::clientDidSomething);

	RUI_GET_INSTANCE()->setVerbose(true);

	// SET PARAM GROUPS / COLORS //////////////////////////////////
	RUI_NEW_GROUP("POSITION");	//make a new group (optional)
								//All items inside a group show the same background hue on the client

	// SHARE A FLOAT PARAM ////////////////////////////////////////
	RUI_SHARE_PARAM(x, 0, ofGetWidth() ); //add an "x" float param to the current group ("position")
	RUI_SHARE_PARAM(y, 0, ofGetHeight()); //provide a variable, a rangeMin and a rangeMax

	RUI_SHARE_PARAM_WCN("Z: what", z, 0, ofGetWidth() ); //add an "x" float param to the current group ("position")

	RUI_NEW_GROUP("STYLE"); //make a new group (optional)

	// SHARE A BOOL PARAM ////////////////////////////////////////
	RUI_SHARE_PARAM(drawOutlines);

	RUI_NEW_COLOR(); //slighly change the bg color within the group
	RUI_SHARE_PARAM(numCircles, 0, 30);	//variable, rangeMin, rangeMax

	RUI_NEW_COLOR(); //slighly change the bg color within the group
	RUI_SHARE_PARAM(circleSize, 1, 30);	//variable, rangeMin, rangeMax

	RUI_NEW_GROUP("OTHER"); //make a new group

	// SHARE A STRING PARAM ////////////////////////////////
	RUI_SHARE_PARAM(currentSentence, ofColor(255,0,0,64));	// you can also set a color on a per-param basis

	// SHARE A FLOAT //////////////////////////////////////
	RUI_SHARE_PARAM(currentMouseX, 0, ofGetWidth());

	// SHARE AN ENUM PARAM ////////////////////////////////
	//build a string list for the UI to show
	string menuItems[] = {"MENU_OPTION_0", "MENU_OPTION_1", "MENU_OPTION_2", "MENU_OPTION_3"};
	//privide the enum param, loweset enum, highest enum, and the Enum string list
	RUI_SHARE_ENUM_PARAM(menu, MENU_OPTION_0, MENU_OPTION_3, menuItems);

	// SHARE A COLOR PARAM ////////////////////////////////
	RUI_SHARE_COLOR_PARAM(color);

	// SHARE A string PARAM to unload it later;
	//this is useful in cases where a variable used to be shared,
	//but its value is now on the xml and you still want it loaded
	//but you dont want it to show on the client interface
	//to do so, you first share the param, then load from XML, then remove the param
	RUI_SHARE_PARAM(unloadTest);

	RUI_LOAD_FROM_XML();	//load values from XML, if you want to do so
							//this will result on the UI showing the params
							//as they were when last saved (saved on app quit by default)

	//this efectively removes all remoteUI references to this param
	//but bc it's been loaded from xml in the previous step before,
	//the end result is that you get to load its value from XML
	//but it doesnt show in the client.
	//This is meant to be a way to reduce clutter in the client,
	//allowing you to phase out params that have settled down and dont
	//need further editing, but still allowing you to load its value from the xml.
	RUI_REMOVE_PARAM(unloadTest);
	cout << "unloadTest: '" << unloadTest << "'" << endl;

	RUI_WATCH_PARAM(currentMouseX); //this will print the supplied param all the time on screen,
									//useful for debugging


	//RUI_START_THREADED(); //if you want all the osc communication to happen on a different
							//thread, call this. This has implications though.
							//your params can be changed at anytime by the client,
							//potentially leading to problems. String params are
							//especially very likely to cause crashes!
							//so don't use this unless you know you need it!

}


void testApp::update(){

	float dt = 0.016666;
	currentMouseX = ofGetMouseX();
}


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


void testApp::keyPressed( int key ){
	//force an update in the client side (same as pressing sync button on osx client)
	RUI_PUSH_TO_CLIENT();

	//and also send a text log line to the client
	RUI_LOG("key pressed at %f", ofGetElapsedTimef());
}


//define a callback method to get notifications of client actions
void testApp::clientDidSomething(RemoteUIServerCallBackArg &arg){

	switch (arg.action) {
		case CLIENT_CONNECTED: cout << "CLIENT_CONNECTED" << endl; break;
		case CLIENT_DISCONNECTED: cout << "CLIENT_DISCONNECTED" << endl; break;
		case CLIENT_UPDATED_PARAM: cout << "CLIENT_UPDATED_PARAM: "<< arg.paramName << " - ";
			arg.param.print();
			break;
		case CLIENT_DID_SET_PRESET: cout << "CLIENT_DID_SET_PRESET" << endl; break;
		case CLIENT_SAVED_PRESET: cout << "CLIENT_SAVED_PRESET" << endl; break;
		case CLIENT_DELETED_PRESET: cout << "CLIENT_DELETED_PRESET" << endl; break;
		case CLIENT_SAVED_STATE: cout << "CLIENT_SAVED_STATE" << endl; break;
		case CLIENT_DID_RESET_TO_XML: cout << "CLIENT_DID_RESET_TO_XML" << endl; break;
		case CLIENT_DID_RESET_TO_DEFAULTS: cout << "CLIENT_DID_RESET_TO_DEFAULTS" << endl; break;
		default:
			break;
	}
}