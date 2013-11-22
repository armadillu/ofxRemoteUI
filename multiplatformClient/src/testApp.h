#pragma once

#include "ofMain.h"
#include "ofxRemoteUIClient.h"
#include "ofxUI.h"

#define STATIC_UI_H		60
#define GROUP_SPACE_H	3
#define EDGE_SPACE		20

//declare callback method
void clientCallback(RemoteUIClientCallBackArg a);

#ifdef TARGET_OF_IOS
#include "ofxiOS.h"
#include "ofxiOSExtras.h"
class testApp : public ofxiOSApp{
#else
class testApp : public ofBaseApp{

#endif

	public:
		void setup();
		void update();
		void draw();

		void keyPressed(int key);
		void keyReleased(int key);
		void mouseMoved(int x, int y );
		void mouseDragged(int x, int y, int button);
		void mousePressed(int x, int y, int button);
		void mouseReleased(int x, int y, int button);
		void windowResized(int w, int h);
		void dragEvent(ofDragInfo dragInfo);
		void gotMessage(ofMessage msg);


	void fullParamsUpdate();
	void updateNeighbors();
	void allocUI();


	ofxUIScrollableCanvas *gui;
	ofxUICanvas * staticUI;

	void guiEvent(ofxUIEventArgs &e);
	void staticGuiEvent(ofxUIEventArgs &e);


	bool needFullParamUpdate;
	map<string,string> neighborNames; //from "screen name" to address for neighbors

	ofxRemoteUIClient *				client;

};
