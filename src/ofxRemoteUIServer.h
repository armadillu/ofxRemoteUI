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
#include "ofxXmlSettings.h"
#include "ofxRemoteUI.h"
#include <map>
#include <set>
#include <vector>
#include "ofxRemoteUISimpleNotifications.h"

#if defined(__has_include) /*llvm only - query about header files being available or not*/
	#if __has_include("ofxFontStash.h") && !defined(DISABLE_AUTO_FIND_FONSTASH_HEADERS)
		#define USE_OFX_FONTSTASH
	#endif
#endif

#ifdef USE_OFX_FONTSTASH
	#include "ofxFontStash.h"
#endif

#include "ofxRemoteUIServerMacros.h"

#if defined(__has_include)
	#if __has_include("ofxTimeMeasurements.h")
		#define USE_OFX_TIME_MEASUREMENTS
	#endif
#endif

#ifdef USE_OFX_TIME_MEASUREMENTS
	#include "ofxTimeMeasurements.h"
#endif

#define BG_COLOR_ALPHA			55

//handle poco being a separate addon after 0.9.8 - and ofXML went from a pocoXML based implementation
//to a pugiXML based implementation
#if OF_VERSION_MAJOR>0 || (OF_VERSION_MAJOR==0 && OF_VERSION_MINOR>=10)
	#include "ofxXmlPoco.h"
	#define ofXmlObject ofxXmlPoco
#else
	#define ofXmlObject ofXml
#endif


// Remove to turn off websockets/webserver
#define USE_WEBSOCKETS
#define USE_WEBSERVER

#ifdef USE_WEBSOCKETS
    #include "ofxLibwebsockets.h"
#endif

#ifdef USE_WEBSERVER
    #include "ofxRemoteUIWebServer.h"
#endif


class ofxRemoteUIServer: public ofxRemoteUI
#ifdef OF_AVAILABLE
, ofThread /*if within OF, we want to be able to run threaded*/
#endif
{

public:

	static ofxRemoteUIServer* instance();

	void setup(int port = -1, float updateInterval = 0.1/*sec*/);

	//You shouldnt need to call any of these directly. Use the Macros supplied in a separate header file instead.
	void update(float dt);
	#ifdef OF_AVAILABLE
	void draw(int x = 20, int y = ofGetHeight() - 20); //x and y of where the notifications will get draw
	#else
	void draw(int x = 20, int y = 0); //x and y of where the notifications will get draw
	#endif
	void close();

	vector<string> loadFromXML(string fileName); //returns list of param names in current setup but not set in XML

	void saveToXML(string fileName, bool oldFormat = false); //save the whole list of params to an xml
	void saveGroupToXML(string fileName, string groupName, bool oldFormat = false); //save only a subset of params into xml

	void shareParam(string paramName, float* param, float min, float max, ofColor bgColor = ofColor(0,0,0,0) );
	void shareParam(string paramName, bool* param, ofColor bgColor = ofColor(0,0,0,0), int nothing = 0 ); //"nothing" args are just to match other methods
	void shareParam(string paramName, int* param, int min, int max, ofColor bgColor = ofColor(0,0,0,0) );
	void shareParam(string paramName, string* param, ofColor bgColor = ofColor(0,0,0,0), int nothing = 0 ); //"nothing" args are just to match other methods
	void shareParam(string paramName, int* param, int min, int max, vector<string> names, ofColor c = ofColor(0,0,0,0)); //enum!
	void shareParam(string paramName, int* param, int min, int max, string* names, ofColor c = ofColor(0,0,0,0)); //enum with old school string array
	void shareParam(string paramName, unsigned char* param, ofColor bgColor = ofColor(0,0,0,0), int nothing = 0 );	//ofColor

	void addSpacer(string name);

	void setParamGroup(string g);		//set for all the upcoming params

	void setNewParamColor(int num); //randomly sets a new param color for all upcoming params
	void setNewParamColorVariation(bool dontChangeVariation = false); //set a slight change to the new param, inside the same group hue
	void unsetParamColor();  //back to un-colored params (shows alternating rows on OSX client)

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

	void setEnabled(bool enabled_){enabled = enabled_;};
	bool getSaveToXMLOnExit(){ return saveToXmlOnExit; }
	void setSaveToXMLOnExit(bool save){saveToXmlOnExit = save;};
	void setDrawsNotificationsAutomaticallly(bool draw){drawNotifications = draw;}
	void setShowUIDuringEdits(bool s){showUIduringEdits = s;} 
	void setNetworkInterface(const string & iface){userSuppliedNetInterface = iface;}
	void pushParamsToClient(); //pushes all param values to client, updating its UI
	void sendLogToClient(const char* format, ...);
	void sendLogToClient(const string & message);
    void sendMessage(ofxOscMessage m);
	void setClearXMLonSave(bool clear){clearXmlOnSaving = clear;} //this only affects xml v1 - not relevant nowadays
	void setDirectoryPrefix(const string & _directoryPrefix); // set the optional directory prefix

	void removeParamFromDB(const string & paramName);	//useful for params its value is kinda set and will not change,
												//to load from xml and then remove from the list to
												//avoid crowding the UI too much

	void watchParamOnScreen(const string& paramName);

	void setShowInterfaceKey(char key){showInterfaceKey = key;};
	void setAutomaticBackupsEnabled(bool enabled){autoBackups = enabled;}
	void setAutoDraw(bool d){autoDraw = d;};
	bool getAutoDraw(){return autoDraw;}
	#ifdef USE_OFX_FONTSTASH
	void drawUiWithFontStash(string fontPath, float fontSize = 15 /*good with veraMono*/ );
	void drawUiWithBitmapFont();
	ofxFontStash & getFont(){return font;}
	#endif

	bool builtInClientIsVisible(){return showUI;}
	
	//get host info
	string getComputerIP(){return computerIP;}
	string getComputerName(){return computerName;}
	string getBinaryName(){return binaryName;}
    
    
#ifdef OF_AVAILABLE
	void toggleBuiltInClientUI(); //show hide the "built in client" GUI screen
	void setUiColumnWidth(int w);
	void setBuiltInUiScale(float s);
	void setCustomScreenHeight(int h);
	void setCustomScreenWidth(int w);

	void setNotificationScreenTime(float t){onScreenNotifications.setNotificationScreenTime(t);}
	void setLogNotificationScreenTime(float t){onScreenNotifications.setLogNotificationScreenTime(t);}

	//of style event/callback
	ofEvent<RemoteUIServerCallBackArg> clientAction;

	void startInBackgroundThread();
	/*	Calling this will run the server in a background thread.
	 all param changes will run in a separate thread, this might cause issues with your app
	 as parameters can be changed at any time! so be aware; especially with strings, you might get crashes.
	 This can be useful in situation where your main thread is blocked for seconds, or your app runs
	 at a very low framerate. In those situations, the server doesnt get updated often enough,
	 and you might get disconnected. Using a background thread means you can still control your params
	 as the main thread is blocked, but it also means the changes are not synced with the main thread. Also, the
	 callback method will be called from a background thread, so keep it in mind. (no GL calls in there!)
	 */
#endif

	//These are used to monitor on screen any value you need; similar to watchParam
	//WARNING - make sure the pointer you provide is VALID during the whole program duration
	//or you will crash!
	void addVariableWatch(const string & varName, float* varPtr, ofColor c = ofColor(0,0,0,0));
	void addVariableWatch(const string & varName, int* varPtr, ofColor c = ofColor(0,0,0,0));
	void addVariableWatch(const string & varName, bool* varPtr, ofColor c = ofColor(0,0,0,0));
	//void removeVariableWatch(const string &varName);

#ifdef USE_WEBSOCKETS
    // WebSocket Events
    void    onConnect( ofxLibwebsockets::Event& args );
    void    onOpen( ofxLibwebsockets::Event& args );
    void    onClose( ofxLibwebsockets::Event& args );
    void    onIdle( ofxLibwebsockets::Event& args );
    void    onMessage( ofxLibwebsockets::Event& args );
    void    onBroadcast( ofxLibwebsockets::Event& args );
#endif
    
protected:

	ofxRemoteUIServer(); // use ofxRemoteUIServer::instance() instead! Use the MACROS defined above!
	~ofxRemoteUIServer();

	struct XmlCounter{ //used to hold xml save counts when saving to xml
		XmlCounter(){ numFloats = numInts = numStrings = numBools = numEnums = numColors = 0; }
		int numFloats, numInts, numStrings, numBools, numEnums, numColors;
	};

	vector<string> loadFromXMLv1(string fileName); //returns list of param names in current setup but not set in XML
	#ifdef OF_AVAILABLE
	vector<string> loadFromXMLv2(string fileName); //returns list of param names in current setup but not set in XML
	#endif

	void			saveToXMLv1(string fileName); //save the whole list of params to an xml
	#ifdef OF_AVAILABLE
	void			saveToXMLv2(string fileName, string group); //save the whole list of params to an xml
	#endif

	void			saveGroupToXMLv1(string fileName, string groupName); //save only a subset of params into xml

	void			onShowParamUpdateNotification(ScreenNotifArg& a);

	void			restoreAllParamsToInitialXML();
	void			restoreAllParamsToDefaultValues();
	void			connect(string address, int port);
	void			setColorForParam(RemoteUIParam &p, ofColor c);
	vector<string>	getAvailablePresets(bool onlyGlobal = false); //all, including group presets! group presets have group/presetName name pattern
	vector<string>	getAvailablePresetsForGroup(string group);

	void			deletePreset(string name, string group=""); //if group is not "", then this is a global preset. otherwise its a group preset
	void			updateServer(float dt);
	void			handleBroadcast();
	void			(*callBack)(RemoteUIServerCallBackArg);
	void			threadedFunction();

	void			saveParamToXmlSettings(const RemoteUIParam & p, string key, ofxXmlSettings & s, XmlCounter & counter);
	#ifdef OF_AVAILABLE
	void			saveParamToXmlSettings(const RemoteUIParam & p, string key, ofXmlObject & s, int index, bool active);
	#endif
	void			saveSettingsBackup();

	void 			addParamToDB(const RemoteUIParam & p, string thisParamName);

	string 			getFinalPath(const string &);

	vector<ofColor> colorTables;
	int				colorTableIndex;
	bool			colorSet; //if user called setParamColor()

	ofColor			paramColor;					//a master hue for the current group
	ofColor			paramColorCurrentVariation; //a small hue change from the master hue for the current group
	int				newColorInGroupCounter;

	string			upcomingGroup;
	ofxOscSender	broadcastSender;
	float			broadcastTime;
	int				broadcastCount;
	bool			portIsSet;

	string			computerName;
	string			binaryName;
	string			computerIP;

	string			directoryPrefix; // the optional directory prefix where we should store data


	bool			doBroadcast; //controls if the server advertises itself
	bool			drawNotifications;

	bool			loadedFromXML; //we start with loadedFromXML=false; once loadXML is called, this becomes true
	bool			saveToXmlOnExit;

	bool			threadedUpdate;
	bool			showUI; //displays all params on screen
	bool			showUIduringEdits;
	bool			clearXmlOnSaving;  //if false, allows you to keep defaults for old params that you are not sharing anymore

	char			showInterfaceKey;

	bool			enabled;
	bool			autoBackups;

	bool			autoDraw;

	vector			<string> paramsToWatch;

#ifdef OF_AVAILABLE
	
	void			_appExited(ofEventArgs &e);
	void			_draw(ofEventArgs &d);
	void			_update(ofEventArgs &d);
	bool			_keyPressed(ofKeyEventArgs &e);

	int														selectedItem;
	ofVboMesh												uiLines;
	ofxRemoteUISimpleNotifications							onScreenNotifications;

	vector<string>											presetsCached; //for the built in client
	unordered_map<string, vector<string> > 					groupPresetsCached;
	int 													selectedGroupPreset;
	int 													selectedPreset;
	string													lastChosenPreset;
	float													uiColumnWidth;
	float													uiAlpha;
	float													uiScale;
	float													xOffset;		 //my ghetto scrolling
	float													xOffsetTarget; //smooth transitions scrolling
	int														customScreenHeight;
	int														customScreenWidth;
	int														selectedColorComp; //[0..4]
	#ifdef USE_OFX_FONTSTASH
	bool													useFontStash;
	ofxFontStash											font;
	string													fontFile;
	float													fontSize;
	#endif

	bool													headlessMode;

	void			refreshPresetsCache();
	void			drawString(const string & text, const float & x, const float & y);
	void			drawString(const string & text, const ofVec2f & pos);

	string 			cleanCharsForFileSystem(const string & s);
	const float 											ruiLineH = 20;
	float 													lineH = ruiLineH;
	float													charW = 8;

#endif

    //---WebSockets---
    bool useWebSockets = false;
    deque<ofxOscMessage> wsMessages;

#ifdef USE_WEBSOCKETS
    //---Web Sockets (OSC Port + 1)---
    void    listenWebSocket(int port);
    int     wsPort;
    class mutex    wsDequeMut;
    ofxOscMessage  jsonToOsc(Json::Value json);
    string         oscToJson(ofxOscMessage m);
    ofxLibwebsockets::Server wsServer;
#endif
    
    
    //---Web Server (OSC Port + 2)---
#ifdef USE_WEBSERVER
    ofxRemoteUIWebServer webServer;
    int  webPort;
    void startWebServer(int port);
#endif
    

	//keep track of params we added and then removed
	unordered_map<string, RemoteUIParam>				params_removed;
	unordered_map<int, string>							orderedKeys_removed; // used to keep the order in which the params were added

	map<string, RemoteUIServerValueWatch> 				varWatches;

	static ofxRemoteUIServer* 							singleton;


};

#endif
