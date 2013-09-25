//
//  main.cpp
//  noOF_Example
//
//  Created by Oriol Ferrer Mesià on 29/06/13.
//  Copyright (c) 2013 Oriol Ferrer Mesià. All rights reserved.
//

#include <iostream>
#include "ofxRemoteUIServer.h"
#include "unistd.h"

int myParam = 0;
ofColor color;
bool quitButton = false;

//define a callback method to get notifications of client actions
void serverCallback(RemoteUIServerCallBackArg arg){
	switch (arg.action) {
		case CLIENT_CONNECTED: cout << "CLIENT_CONNECTED" << endl; break;
		case CLIENT_DISCONNECTED: cout << "CLIENT_DISCONNECTED" << endl; break;
		case CLIENT_UPDATED_PARAM:
			cout << "CLIENT_UPDATED_PARAM "<< arg.paramName << ": ";
			arg.param.print();
			break;
		default:break;
	}
}

int main(int argc, const char * argv[]){

	std::cout << "Hello, World!\n";

	//setup our callback to get notified when client changes things
	OFX_REMOTEUI_SERVER_GET_INSTANCE()->setCallback(serverCallback);

	//OFX_REMOTEUI_SERVER_GET_INSTANCE()->setVerbose(true);
	OFX_REMOTEUI_SERVER_SETUP(10000); 	//start server
	OFX_REMOTEUI_SERVER_SET_NEW_COLOR();
	OFX_REMOTEUI_SERVER_SHARE_PARAM(myParam, 0, 100); // share my param
	OFX_REMOTEUI_SERVER_SET_NEW_COLOR();
	OFX_REMOTEUI_SERVER_SHARE_COLOR_PARAM(color);
	OFX_REMOTEUI_SERVER_SET_NEW_COLOR();
	OFX_REMOTEUI_SERVER_SHARE_PARAM(quitButton);

	OFX_REMOTEUI_SERVER_LOAD_FROM_XML(); //load from XML
	while (quitButton == false) {
		OFX_REMOTEUI_SERVER_UPDATE(0.01666);
		usleep(100000 / 6.);
	}

	quitButton = false;
	OFX_REMOTEUI_SERVER_SAVE_TO_XML();
	OFX_REMOTEUI_SERVER_CLOSE();
    return 0;
}
