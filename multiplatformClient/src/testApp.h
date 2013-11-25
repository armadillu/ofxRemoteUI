#pragma once

#include "ofMain.h"
#include "ofxRemoteUIClient.h"
#include "ofxUI.h"

#define WIDGET_H		( retinaScale * 22 )
#define COLOR_SLIDER_H	( retinaScale * 16 )
#define SLIDER_H		( retinaScale * 18 )
#define PADDING			(4 * retinaScale)
#define WIDGET_SPACING	(4 * retinaScale)
#define GROUP_SPACE_H	3 * retinaScale
#define ENUM_SURROUNDING_SPACE 2 * retinaScale
#define EDGE_SPACE		5 * retinaScale
#define STATIC_UI_H		( 4 * WIDGET_H + 4 * PADDING + WIDGET_SPACING )
#define DYNAMIC_UI_STARTING_Y (STATIC_UI_H + 5 * retinaScale)
#define CANVAS_FULL_W	( ofGetWidth() - 2 * EDGE_SPACE)
#define WIDGET_FULL_W	( ofGetWidth() - 2 * EDGE_SPACE - 2 * PADDING)

#define FONT_FILE			"CPMono_v07 Plain.otf"
#define FONT_SIZE_MULT		1.0
#define FONT_SIZE_SMALL		5 * retinaScale * FONT_SIZE_MULT
#define FONT_SIZE_MEDIUM	6 * retinaScale * FONT_SIZE_MULT
#define FONT_SIZE_LARGE		13 * retinaScale * FONT_SIZE_MULT


#define GROUP_BG_COLOR		ofxUIColor(186, 0, 180)
#define STATIC_UI_BG_COLOR	ofxUIColor(32, 96,160,128)


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


	void savePresetConfirmed(string presetName);
	void fullParamsUpdate();
	void updateNeighbors();
	void updatePresets();
	void allocUI();
	void prepareStaticUI();
	void preparePresetUI();


	ofxUIScrollableCanvas *gui;
	ofxUICanvas * staticUI;
	ofxUICanvas * presetNameUI;

	void guiEvent(ofxUIEventArgs &e);
	void staticGuiEvent(ofxUIEventArgs &e);
	void presetGuiEvent(ofxUIEventArgs &e);


	bool needFullParamUpdate;
	map<string,string> neighborNames; //from "screen name" to address for neighbors

	ofxRemoteUIClient *				client;

	float retinaScale;

};
