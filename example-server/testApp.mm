#include "testApp.h"

//define a callback method to get notifications of client actions
void serverCallback(RemoteUIServerCallBackArg arg){

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

//--------------------------------------------------------------
void testApp::setup(){

	ofBackground(22);
	ofSetFrameRate(60);
	ofSetVerticalSync(true);

	//set some default values
	drawOutlines = false;
	currentSentence = "unInited";
	x = y = 66;
	numCircles = 30;
	menu = MENU_OPTION_1;

	//setup our callback to get notified when client changes things
	OFX_REMOTEUI_SERVER_GET_INSTANCE()->setCallback(serverCallback);

	OFX_REMOTEUI_SERVER_SETUP(10000); 	//start server

	//expose vars to the server. Always do so after setting up the server.
	OFX_REMOTEUI_SERVER_SET_NEW_COLOR(); // set a bg color for the upcoming params
	OFX_REMOTEUI_SERVER_SET_UPCOMING_PARAM_GROUP("position"); //make a new group
	OFX_REMOTEUI_SERVER_SHARE_PARAM(x, 0, ofGetWidth() ); //add an "x" float param to this group.
	OFX_REMOTEUI_SERVER_SHARE_PARAM(y, 0, ofGetHeight());

	OFX_REMOTEUI_SERVER_SET_UPCOMING_PARAM_GROUP("style"); //make a new group
	OFX_REMOTEUI_SERVER_SET_NEW_COLOR(); // set a bg color for the upcoming params
	OFX_REMOTEUI_SERVER_SHARE_PARAM(drawOutlines);		//add a bool param
	OFX_REMOTEUI_SERVER_SHARE_PARAM(numCircles, 0, 30);	//add an int param

	OFX_REMOTEUI_SERVER_SET_UPCOMING_PARAM_GROUP("whatever"); //make a new group
	OFX_REMOTEUI_SERVER_SET_UPCOMING_PARAM_COLOR( ofColor(255,0,0,64) ); // you can set a custom upcoming color too
	OFX_REMOTEUI_SERVER_SHARE_PARAM(currentSentence, ofColor(0,0,255,64));	// you can also set a color on a per-param basis
	OFX_REMOTEUI_SERVER_SHARE_PARAM(currentMouseX, 0, ofGetWidth(), ofColor(255,64));

	//add a new Enum param, with several choices
	vector<string> menuItems;
	menuItems.push_back("MENU_OPTION_0");menuItems.push_back("MENU_OPTION_1");
	menuItems.push_back("MENU_OPTION_2"); menuItems.push_back("MENU_OPTION_3");
	OFX_REMOTEUI_SERVER_SHARE_ENUM_PARAM(menu,MENU_OPTION_0, MENU_OPTION_3, menuItems);

	OFX_REMOTEUI_SERVER_SHARE_COLOR_PARAM(color); //add a color param

	OFX_REMOTEUI_SERVER_LOAD_FROM_XML();	//load values from XML, if you want to do so
											//this will result on the UI showing the params
											//as they were when last saved (on quit in this case)

	OFX_REMOTEUI_SERVER_START_THREADED();   //if you want all the communication to happen on a different
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
								ofColor::black, color
								);

}

void testApp::exit(){
	OFX_REMOTEUI_SERVER_CLOSE();		//stop the server
	OFX_REMOTEUI_SERVER_SAVE_TO_XML();	//save values to XML (if you want to!)
}