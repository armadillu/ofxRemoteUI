//
//  ofxRemoteUI.h
//  emptyExample
//
//  Created by Oriol Ferrer Mesià on 09/01/13.
//
//

#ifndef _ofxRemoteUIServer__
#define _ofxRemoteUIServer__

// you will need to add this to your "Header Search Path" for ofxOsc to compile
// ../../../addons/ofxOsc/libs ../../../addons/ofxOsc/libs/oscpack ../../../addons/ofxOsc/libs/oscpack/src ../../../addons/ofxOsc/libs/oscpack/src/ip ../../../addons/ofxOsc/libs/oscpack/src/ip/posix ../../../addons/ofxOsc/libs/oscpack/src/ip/win32 ../../../addons/ofxOsc/libs/oscpack/src/osc ../../../addons/ofxOsc/src
#include "ofxOsc.h"
#include "ofxRemoteUI.h"
#include "ofxXmlSettings.h"
#include <map>
#include <set>
#include <vector>

#ifndef OF_VERSION_MINOR //if OF is not available, redefine ofColor to myColor
#define ofColor myColor
struct myColor{

	myColor(){}
	myColor(int rr, int gg, int bb, int aa){
		r = rr;	g = gg;	b = bb; a = aa;
	}
	myColor(int bright){
		r = g = b = bright; a = 255;
	}
	bool operator==(const myColor& c){
		return r == c.r && g == c.g && b == c.b && a == c.a;
	}
	bool operator!=(const myColor& c){
		return r != c.r || g != c.g || b != c.b || a != c.a;
	}
	union  {
		struct { unsigned char r, g, b, a; };
		unsigned char v[4];
	};
};
#else
#define OF_AVAILABLE 1 //
#endif

//easy param sharing macros, share from from anywhere!
#define OFX_REMOTEUI_SERVER_SHARE_PARAM(val,...)		( ofxRemoteUIServer::instance()->shareParam( #val, &val, ##__VA_ARGS__ ) )
#define OFX_REMOTEUI_SERVER_SHARE_ENUM_PARAM(val,enumMin,enumMax,menuList,...)	( ofxRemoteUIServer::instance()->shareParam( #val, (int*)&val,enumMin, enumMax,menuList, ##__VA_ARGS__ ) )
#define OFX_REMOTEUI_SERVER_SHARE_COLOR_PARAM(color,...)	( ofxRemoteUIServer::instance()->shareParam( #color, (unsigned char*)&color.v[0], ##__VA_ARGS__ ) )
#define OFX_REMOTEUI_SERVER_SET_UPCOMING_PARAM_COLOR(c)	( ofxRemoteUIServer::instance()->setParamColor( c ) )
#define OFX_REMOTEUI_SERVER_SET_UPCOMING_PARAM_GROUP(g)	( ofxRemoteUIServer::instance()->setParamGroup( g ) )
#define OFX_REMOTEUI_SERVER_SET_NEW_COLOR()				( ofxRemoteUIServer::instance()->setNewParamColor() )
#define OFX_REMOTEUI_SERVER_SETUP(port, ...)			( ofxRemoteUIServer::instance()->setup(port, ##__VA_ARGS__) )
#define OFX_REMOTEUI_SERVER_UPDATE(deltaTime)			( ofxRemoteUIServer::instance()->update(deltaTime) )
#define OFX_REMOTEUI_SERVER_DRAW(x,y)					( ofxRemoteUIServer::instance()->draw(x,y) ) /*only call this if you disabled automatic notifications and still want to see them*/
#define OFX_REMOTEUI_SERVER_CLOSE()						( ofxRemoteUIServer::instance()->close() )
#define	OFX_REMOTEUI_SERVER_SAVE_TO_XML()				( ofxRemoteUIServer::instance()->saveToXML(OFXREMOTEUI_SETTINGS_FILENAME) )
#define	OFX_REMOTEUI_SERVER_LOAD_FROM_XML()				( ofxRemoteUIServer::instance()->loadFromXML(OFXREMOTEUI_SETTINGS_FILENAME) )
#define	OFX_REMOTEUI_SERVER_SET_SAVES_ON_EXIT(save)		( ofxRemoteUIServer::instance()->setSaveToXMLOnExit(save) )
#define	OFX_REMOTEUI_SERVER_SET_DRAWS_NOTIF(draw)		( ofxRemoteUIServer::instance()->setDrawsNotificationsAutomaticallly(draw) )
#define OFX_REMOTEUI_SERVER_GET_INSTANCE()				( ofxRemoteUIServer::instance() )

#ifdef OF_AVAILABLE //threaded only works in OF
	#define OFX_REMOTEUI_SERVER_START_THREADED()			( ofxRemoteUIServer::instance()->startInBackgroundThread() )
#endif



class ofxRemoteUIServer: public ofxRemoteUI
#ifdef OF_AVAILABLE
, ofThread
#endif
{

public:

	static ofxRemoteUIServer* instance();

	void setup(int port = -1, float updateInterval = 0.1/*sec*/);

	#ifdef OF_AVAILABLE
	void startInBackgroundThread(); //calling this means you don't need to call update
									//all param changes will run in a separate thread
									//this might cause issues with your app
									//as parameters can be changed at any time!
									//so be aware, especially with strings you might get crashes!
									//but this can be useful in situation where your main thread is blocked for seconds
									//bc using a background therad means you can still control your params
									//as the main thread is blocked
	#endif

	void update(float dt);
	void draw(int x = 20, int y = 20); //draws important notifications on screen
	void close();
	vector<string> loadFromXML(string fileName); //returns list of param names in current setup but not set in XML
	void saveToXML(string fileName);

	void shareParam(string paramName, float* param, float min, float max, ofColor bgColor = ofColor(0,0,0,0) );
	void shareParam(string paramName, bool* param, ofColor bgColor = ofColor(0,0,0,0), int nothing = 0 ); //"nothing" args are just to match other methods
	void shareParam(string paramName, int* param, int min, int max, ofColor bgColor = ofColor(0,0,0,0) );
	void shareParam(string paramName, string* param, ofColor bgColor = ofColor(0,0,0,0), int nothing = 0 ); //"nothing" args are just to match other methods
	void shareParam(string paramName, int* param, int min, int max, vector<string> names, ofColor c = ofColor(0,0,0,0)); //enum!
	void shareParam(string paramName, unsigned char* param, ofColor bgColor = ofColor(0,0,0,0), int nothing = 0 );	//ofColor
	void setParamColor( ofColor c );
	void setNewParamColor();
	void unsetParamColor();
	void setParamGroup(string g);

	//get notified when a client changes something remotelly
	void setCallback( void (*callb)(RemoteUIServerCallBackArg) );
	//if you want to get notified when a param changes, implement a callback method like this:
	//
	//	void serverCallback(RemoteUIServerCallBackArg arg){
	//		switch (arg.action) {
	//			case CLIENT_CONNECTED: break;
	//			case CLIENT_DISCONNECTED: break;
	//			case CLIENT_UPDATED_PARAM:
	//				arg.param.print();
	//				break;
	//			...
	//			default:break;
	//		}
	//	}

	void setSaveToXMLOnExit(bool save);
	void setDrawsNotificationsAutomaticallly(bool draw);



private:

	void restoreAllParamsToInitialXML();
	void restoreAllParamsToDefaultValues();

	void connect(string address, int port);

	ofxRemoteUIServer(); // use ofxRemoteUIServer::instance() instead!
	~ofxRemoteUIServer();
	static ofxRemoteUIServer* singleton;
	void setColorForParam(RemoteUIParam &p, ofColor c);
	vector<string> getAvailablePresets();
	void deletePreset(string name);

	vector<ofColor> colorTables;
	int				colorTableIndex;
	bool			colorSet; //if user called setParamColor()
	ofColor			paramColor;
	string			upcomingGroup;
	ofxOscSender	broadcastSender;
	float			broadcastTime;
	string			computerName;
	string			binaryName;
	string			computerIP;
	bool			portIsSet;

	float			savedAnimationTimer;
	string			saveAnimationfileName;

	float			connectedAnimationTimer;
	float			disconnectedAnimationTimer;
	float			startupAnimationTimer;

	bool			doBroadcast;
	bool			drawNotifications;

	bool			loadedFromXML; //we start with loadedFromXML=false; once loadXML is called, this becomes true
	bool			saveToXmlOnExit;

	bool			threadedUpdate;
	void			threadedFunction();
	void			updateServer(float dt);

	bool			showValuesOnScreen;
	float			lastDT;

	void (*callBack)(RemoteUIServerCallBackArg);

	#ifdef OF_AVAILABLE
	void _appExited(ofEventArgs &e);
	void _draw(ofEventArgs &d);
	void _keyPressed(ofKeyEventArgs &e);
	#endif

};

#endif
