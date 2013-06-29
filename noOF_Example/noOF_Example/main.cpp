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

int prevMyParam;
int myParam = 0;
bool quitButton = false;

void update();

int main(int argc, const char * argv[]){

	// insert code here...
	std::cout << "Hello, World!\n";

	OFX_REMOTEUI_SERVER_SETUP(10000); 	//start server
	OFX_REMOTEUI_SERVER_SHARE_PARAM(myParam, 0, 100); // share my param
	OFX_REMOTEUI_SERVER_SHARE_PARAM(quitButton);

	OFX_REMOTEUI_SERVER_LOAD_FROM_XML(); //load from XML
	while (quitButton == false) {
		update();
		usleep(100000 / 6.);
	}

	quitButton = false;
	OFX_REMOTEUI_SERVER_SAVE_TO_XML();
	OFX_REMOTEUI_SERVER_CLOSE();
    return 0;
}

void update(){

	OFX_REMOTEUI_SERVER_UPDATE(0.01666);
	if (prevMyParam != myParam){
		cout << "myParam is " << myParam << endl;
	}
	prevMyParam = myParam;

}