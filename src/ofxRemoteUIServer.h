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
#include "ofxRemoteUIServerMacros.h"
#if defined( OF_AVAILABLE )
	#include "ofxRemoteUISimpleNotifications.h"
	#ifdef TARGET_OSX
		#include "OscQueryServerMgr.h"
	#endif
#endif


// ofxFontStash ///////////////////////////////////////////////////////////////////////////////////

#if defined(__has_include) /*llvm only - query about header files being available or not*/
	#if __has_include("ofxFontStash.h") && !defined(DISABLE_AUTO_FIND_FONSTASH_HEADERS)
		#define USE_OFX_FONTSTASH
	#endif
#endif

#ifdef USE_OFX_FONTSTASH
	#include "ofxFontStash.h"
#endif

// ofxFontStash2 //////////////////////////////////////////////////////////////////////////////////

#if defined(__has_include) /*llvm only - query about header files being available or not*/
#if __has_include("ofxFontStash2.h") && !defined(DISABLE_AUTO_FIND_FONSTASH_HEADERS)
#define USE_OFX_FONTSTASH2
#endif
#endif

#ifdef USE_OFX_FONTSTASH2
#include "ofxFontStash2.h"
#endif

// ofxTimeMeasurements /////////////////////////////////////////////////////////////////////////////

#if defined(__has_include)
	#if __has_include("ofxTimeMeasurements.h")
		#define USE_OFX_TIME_MEASUREMENTS
	#endif
#endif

#ifdef USE_OFX_TIME_MEASUREMENTS
	#include "ofxTimeMeasurements.h"
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////

#define BG_COLOR_ALPHA			55

// Define RUI_WEB_INTERFACE to turn ON websockets/webserver
#ifdef RUI_WEB_INTERFACE
    #include "ofxLibwebsockets.h"
    #include "ofxRemoteUIWebServer.h"
#endif


class ofxRemoteUIServer: public ofxRemoteUI
#ifdef OF_AVAILABLE
, ofThread /*if within OF, we want to be able to run threaded*/
#endif
{

friend class OscQueryServerMgr;

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

	vector<std::string> loadFromXML(std::string fileName); //returns list of param names in current setup but not set in XML

	void saveToXML(std::string fileName, bool oldFormat = false); //save the whole list of params to an xml
	void saveGroupToXML(std::string fileName, std::string groupName, bool oldFormat = false); //save only a subset of params into xml

	void shareParam(std::string paramName, float* param, float min, float max, ofColor bgColor = ofColor(0,0,0,0) );
	void shareParam(std::string paramName, bool* param, ofColor bgColor = ofColor(0,0,0,0), int nothing = 0 ); //"nothing" args are just to match other methods
	void shareParam(std::string paramName, int* param, int min, int max, ofColor bgColor = ofColor(0,0,0,0) );
	void shareParam(std::string paramName, std::string* param, ofColor bgColor = ofColor(0,0,0,0), int nothing = 0 ); //"nothing" args are just to match other methods
	void shareParam(std::string paramName, int* param, int min, int max, vector<std::string> names, ofColor c = ofColor(0,0,0,0)); //enum!
	void shareParam(std::string paramName, int* param, int min, int max, std::string* names, ofColor c = ofColor(0,0,0,0)); //enum with old school string array
	void shareParam(std::string paramName, unsigned char* param, ofColor bgColor = ofColor(0,0,0,0), int nothing = 0 );	//ofColor

	void addSpacer(std::string name);

	void setParamGroup(std::string g);		//set for all the upcoming params

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
	void setNetworkInterface(const std::string & iface){userSuppliedNetInterface = iface;}
	void pushParamsToClient(); //pushes all param values to client, updating its UI
	void sendLogToClient(const char* format, ...);
	void sendLogToClient(const std::string & message);
    void sendMessage(ofxOscMessage m);
	void setClearXMLonSave(bool clear){clearXmlOnSaving = clear;} //this only affects xml v1 - not relevant nowadays
	void setDirectoryPrefix(const std::string & _directoryPrefix); // set the optional directory prefix

	void removeParamFromDB(const std::string & paramName, bool permanently = false);	//useful for params its value is kinda set and will not change,
												//to load from xml and then remove from the list to
												//avoid crowding the UI too much

	//param ignore list - use this if you want to load presets but want to ignore certain
	//params that are defined in those presets (ie a "debug" or similar param)
	void addParamToPresetLoadIgnoreList(const std::string & param);
	void removeParamFromPresetLoadIgnoreList(const std::string & param);
	bool paramIsInPresetLoadIgnoreList(const std::string & param);
	void clearParamToPresetLoadIgnoreList();

	void watchParamOnScreen(const std::string& paramName);
	void removeParamWatch(const std::string& paramName);
	void removeAllParamWatches();

	void setShowInterfaceKey(char key){showInterfaceKey = key;};
	void setAutomaticBackupsEnabled(bool enabled){autoBackups = enabled;}
	void setAutoDraw(bool d){autoDraw = d;};
	bool getAutoDraw(){return autoDraw;}

	void setLoadFromXmlClampsToValidRange(bool clamp){ loadFromXmlClampsToValidRange = clamp;}

	// Font render configs ///////

	void drawUiWithBitmapFont();
	#ifdef USE_OFX_FONTSTASH
	void drawUiWithFontStash(std::string fontPath, float fontSize = 15 /*good with veraMono*/ );
	ofxFontStash & getFont(){return font;}
	#endif

	#if defined(USE_OFX_FONTSTASH2)
	void drawUiWithFontStash2(std::string fontPath, float fontSize = 15.0f /*good with VeraMonoBold*/);
	ofxFontStash2::Fonts & getFont2(){return font2;}
	#endif

	bool builtInClientIsVisible(){return showUI;}
	
	//get host info
	std::string getComputerIP(){return computerIP;}
	std::string getComputerName(){return computerName;}
	std::string getBinaryName(){return binaryName;}

	///to load a preset from disk, supply a string "myPresetName" - not the missing file extension
	///if you want to load a local preset, supply a string like "MyGroup/myLocalPreset"
	void loadPresetNamed(std::string presetName);
    
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
	void addVariableWatch(const std::string & varName, float* varPtr, ofColor c = ofColor(0,0,0,0));
	void addVariableWatch(const std::string & varName, int* varPtr, ofColor c = ofColor(0,0,0,0));
	void addVariableWatch(const std::string & varName, bool* varPtr, ofColor c = ofColor(0,0,0,0));
	//void removeVariableWatch(const std::string &varName);

#ifdef RUI_WEB_INTERFACE
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

	vector<std::string> loadFromXMLv1(std::string fileName); //returns list of param names in current setup but not set in XML
	#ifdef OF_AVAILABLE
	vector<std::string> loadFromXMLv2(std::string fileName); //returns list of param names in current setup but not set in XML
	#endif

	void			saveToXMLv1(std::string fileName); //save the whole list of params to an xml
	#ifdef OF_AVAILABLE
	void			saveToXMLv2(std::string fileName, std::string group); //save the whole list of params to an xml
	#endif

	void			saveGroupToXMLv1(std::string fileName, std::string groupName); //save only a subset of params into xml

	void			onShowParamUpdateNotification(ScreenNotifArg& a);

	void			restoreAllParamsToInitialXML();
	void			restoreAllParamsToDefaultValues();
	void			connect(std::string address, int port);
	void			setColorForParam(RemoteUIParam &p, ofColor c);
	vector<std::string>	getAvailablePresets(bool onlyGlobal = false); //all, including group presets! group presets have group/presetName name pattern
	vector<std::string>	getAvailablePresetsForGroup(std::string group);

	void			deletePreset(std::string name, std::string group=""); //if group is not "", then this is a global preset. otherwise its a group preset
	void			updateServer(float dt);
	void			handleBroadcast();
	void			(*callBack)(RemoteUIServerCallBackArg);
	void			threadedFunction();

	void			saveParamToXmlSettings(const RemoteUIParam & p, std::string key, ofxXmlSettings & s, XmlCounter & counter);
	#ifdef OF_AVAILABLE
	void			saveParamToXmlSettings(const RemoteUIParam & p, const std::string & key, pugi::xml_node & s, int index, bool active);
	#endif
	void			saveSettingsBackup();

	void 			addParamToDB(const RemoteUIParam & p, std::string thisParamName);

	std::string 			getFinalPath(const std::string &);

	vector<ofColor> colorTables;
	int				colorTableIndex;
	bool			colorSet; //if user called setParamColor()

	ofColor			paramColor;					//a master hue for the current group
	ofColor			paramColorCurrentVariation; //a small hue change from the master hue for the current group
	int				newColorInGroupCounter;

	std::string			upcomingGroup;
	ofxOscSender	broadcastSender;
	float			broadcastTime;
	int				broadcastCount;
	bool			portIsSet;

	std::string			computerName;
	std::string			binaryName;
	std::string			computerIP;

	std::string			directoryPrefix; // the optional directory prefix where we should store data


	bool			doBroadcast; //controls if the server advertises itself
	bool			drawNotifications;

	bool			loadedFromXML; //we start with loadedFromXML=false; once loadXML is called, this becomes true
	bool			saveToXmlOnExit;

	bool			threadedUpdate;
	bool			showUI; //displays all params on screen
	bool			showUIduringEdits;
	bool			clearXmlOnSaving;  //if false, allows you to keep defaults for old params that you are not sharing anymore
	bool			loadFromXmlClampsToValidRange = true; //when you load from xml, params are clamped to the supplied min-max range

	char			showInterfaceKey;

	bool			enabled;
	bool			autoBackups;

	bool			autoDraw;

	vector			<std::string> paramsToWatch;

#ifdef OF_AVAILABLE
	
	void			_appExited(ofEventArgs &e);
	void			_draw(ofEventArgs &d);
	void			_update(ofEventArgs &d);
	bool			_keyPressed(ofKeyEventArgs &e);

	int														selectedItem;
	ofVboMesh												uiLines;
	ofxRemoteUISimpleNotifications							onScreenNotifications;

	vector<std::string>											presetsCached; //for the built in client
	unordered_map<std::string, vector<std::string> > 					groupPresetsCached;
	int 													selectedGroupPreset;
	int 													selectedPreset;
	std::string													lastChosenPreset;
	float													uiColumnWidth;
	float													uiAlpha;
	float													uiScale;
	float													xOffset;		 //my ghetto scrolling
	float													xOffsetTarget; //smooth transitions scrolling
	int														customScreenHeight;
	int														customScreenWidth;
	int														selectedColorComp; //[0..4]

	enum FontRenderer{
		RENDER_WITH_OF_BITMAP_FONT,
		RENDER_WITH_OFXFONTSTASH,
		RENDER_WITH_OFXFONTSTASH2
	};
	FontRenderer											fontRenderer = RENDER_WITH_OF_BITMAP_FONT;

	#ifdef USE_OFX_FONTSTASH
	ofxFontStash											font;
	std::string													fontFile;
	float													fontSize;
	#endif

	#ifdef USE_OFX_FONTSTASH2
	std::string 													fontStashFile2;
	ofxFontStash2::Fonts									font2;
	float													fontSize2;
	#endif

	bool													headlessMode;
	std::string													dataPath;

	void			refreshPresetsCache();
	void			drawString(const std::string & text, const float & x, const float & y);
	void			drawString(const std::string & text, const ofVec2f & pos);

	std::string 			cleanCharsForFileSystem(const std::string & s);
	const float 											ruiLineH = 20;
	float 													lineH = ruiLineH;
	float													charW = 8;

	#ifdef TARGET_OSX
	//this is OSX only as it relies on Bonjour advertizing for it to work with Vezér
	//windows possible but will require additional installs/libs so on hold for now
	OscQueryServerMgr *	oscQueryServer = nullptr;
										//this allows any other app getting a proper list of all the parameters available
										//https://imimot.com/help/vezer/osc-track-extras/#osc-query-server-support
										//follows https://github.com/mrRay/OSCQueryProposal
	#endif
#endif

    //---WebSockets---
    bool useWebSockets = false;
    deque<ofxOscMessage> wsMessages;

#ifdef RUI_WEB_INTERFACE
    //---Web Sockets (OSC Port + 1)---
    void    listenWebSocket(int port);
    int     wsPort;
    class mutex    wsDequeMut;
    ofxOscMessage  jsonToOsc(Json::Value json);
    std::string         oscToJson(ofxOscMessage m);
    ofxLibwebsockets::Server wsServer;
#endif
    
    
    //---Web Server (OSC Port + 2)---
#ifdef RUI_WEB_INTERFACE
    ofxRemoteUIWebServer webServer;
    int  webPort;
    void startWebServer(int port);
#endif
    

	//keep track of params we added and then removed
	unordered_map<std::string, RemoteUIParam>				params_removed;
	unordered_map<int, std::string>							orderedKeys_removed; // used to keep the order in which the params were added

	map<std::string, RemoteUIServerValueWatch> 				varWatches;

	static ofxRemoteUIServer* 								singleton;

	//handle params that are to be ignored when loading presets
	vector<std::string>										paramsToIgnoreWhenLoadingPresets;


};

#endif
