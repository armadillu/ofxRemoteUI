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
#include "ofxRemoteUISimpleNotifications.h"

// ################################################################################################
// ## EASY ACCES MACROS ## use these instead of direct calls
// ################################################################################################

/*setup the server, set a specific port if you must. otherwise a random one will be chosen
 the first time, and it will be reused for successive launches */
#define OFX_REMOTEUI_SERVER_SETUP(port, ...)							\
( ofxRemoteUIServer::instance()->setup(port, ##__VA_ARGS__) )

//use this macro to share floats, ints, bools
#define OFX_REMOTEUI_SERVER_SHARE_PARAM(val, ...)						\
( ofxRemoteUIServer::instance()->shareParam( #val, &val, ##__VA_ARGS__ ) )

//WCN - "with custom name" share a param with a custom string
#define OFX_REMOTEUI_SERVER_SHARE_PARAM_WCN(pName, val, ...)			\
( ofxRemoteUIServer::instance()->shareParam( pName, &val, ##__VA_ARGS__ ) )

//use this macro to share enums + enumList
#define OFX_REMOTEUI_SERVER_SHARE_ENUM_PARAM(val, enumMin, enumMax, menuList, ...)	\
( ofxRemoteUIServer::instance()->shareParam( #val, (int*)&val,enumMin, enumMax,menuList, ##__VA_ARGS__ ) )

//use this macro to share ofColors
#define OFX_REMOTEUI_SERVER_SHARE_COLOR_PARAM(color, ...)				\
( ofxRemoteUIServer::instance()->shareParam( #color, (unsigned char*)&color.v[0], ##__VA_ARGS__ ) )

/*set a new group for the upcoming params, this also sets a new color*/
#define OFX_REMOTEUI_SERVER_SET_UPCOMING_PARAM_GROUP(g)					\
( ofxRemoteUIServer::instance()->setParamGroup( g ) )

/*set a new small 'hue' change for upcoming params, to create alternating rows inside a group*/
#define OFX_REMOTEUI_SERVER_SET_NEW_COLOR()								\
( ofxRemoteUIServer::instance()->setNewParamColorVariation() )
#define OFX_REMOTEUI_SERVER_SET_NEW_TONE()								\
( ofxRemoteUIServer::instance()->setNewParamColorVariation() )

/*allows you to get a param from anywhere in your code*/
#define OFX_REMOTEUI_SERVER_GET_PARAM(pname)							\
( ofxRemoteUIServer::instance()->getParamForName(pname) )

/*setup the server-client callback. This will be called on important events
 and param updates from the UI. Supplied method should look like:
	void serverCallback(RemoteUIServerCallBackArg arg); 
 See example code below. */
#define OFX_REMOTEUI_SERVER_SET_CALLBACK(serverCallback)				\
( ofxRemoteUIServer::instance()->setCallback(serverCallback) )

/*loads last saved default xml params into your variables */
#define	OFX_REMOTEUI_SERVER_LOAD_FROM_XML()								\
( ofxRemoteUIServer::instance()->loadFromXML(OFXREMOTEUI_SETTINGS_FILENAME) )

/*save current param status to default xml.
 No need to call this on app quit in OF, happens automatically */
#define	OFX_REMOTEUI_SERVER_SAVE_TO_XML()								\
( ofxRemoteUIServer::instance()->saveToXML(OFXREMOTEUI_SETTINGS_FILENAME) )

/*update the server. No need to call this from OF*/
#define OFX_REMOTEUI_SERVER_UPDATE(deltaTime)							\
( ofxRemoteUIServer::instance()->update(deltaTime) )

/*draw the server msgs. No need to call this from OF
 only call this if you disabled automatic notifications and still 
 want to see them in a custom location*/
#define OFX_REMOTEUI_SERVER_DRAW(x,y)									\
( ofxRemoteUIServer::instance()->draw(x,y) )

/*close the server. no need to call this from OF*/
#define OFX_REMOTEUI_SERVER_CLOSE()										\
( ofxRemoteUIServer::instance()->close() )

/*set if saves to XML automatically on app exit. Default is YES in OF*/
#define	OFX_REMOTEUI_SERVER_SET_SAVES_ON_EXIT(save)						\
( ofxRemoteUIServer::instance()->setSaveToXMLOnExit(save) )

/*Set which network interface to use instead of auto-picking the 1st one.
 Supply "en0" or similar. Call this before setup() */
#define	OFX_REMOTEUI_SERVER_SET_NET_INTERFACE(iface)					\
( ofxRemoteUIServer::instance()->setNetworkInterface(iface) )

/*in OF, auto draws on screen imprtant events and param updates. defaults to YES in OF*/
#define	OFX_REMOTEUI_SERVER_SET_DRAWS_NOTIF(draw)						\
( ofxRemoteUIServer::instance()->setDrawsNotificationsAutomaticallly(draw) )

/*sends all params to client, same as pressing sync on client
 updates client UI to match current param values. use this if you modify 
 params internally and want those changes reflected in the UI*/
#define OFX_REMOTEUI_SERVER_PUSH_TO_CLIENT()							\
( ofxRemoteUIServer::instance()->pushParamsToClient() )

/*get a pointer to the server, not usually needed*/
#define OFX_REMOTEUI_SERVER_GET_INSTANCE()								\
( ofxRemoteUIServer::instance() )

/*send any text as a log line your remote client*/
#define OFX_REMOTEUI_SERVER_LOG(format,...)								\
( ofxRemoteUIServer::instance()->sendLogToClient( format, ##__VA_ARGS__ ) )

/*untested! the idea behind this is to share a param, load for xml, and then unload it so that it
 doesnt clutter the client UI, but you still get to load from xml.
 its meant to be a nice way to phase out params which are kind settled down and dont need editing*/
#define OFX_REMOTEUI_SERVER_REMOVE_PARAM(paramName)						\
( ofxRemoteUIServer::instance()->removeParamFromDB(#paramName) )


#ifdef OF_AVAILABLE
/*run the server on a back thread. Useful for apps with very low framerate.
 default is disabled in OF; only works in OF! */
#define OFX_REMOTEUI_SERVER_START_THREADED()							\
( ofxRemoteUIServer::instance()->startInBackgroundThread() )
#endif


#define BG_COLOR_ALPHA			55

class ofxRemoteUIServer: public ofxRemoteUI
#ifdef OF_AVAILABLE
, ofThread
#endif
{

public:

	static ofxRemoteUIServer* instance();

	void setup(int port = -1, float updateInterval = 0.1/*sec*/);

#ifdef OF_AVAILABLE
	void startInBackgroundThread();
	/*	Calling this will run the server in a background thread.
		all param changes will run in a separate thread, this might cause issues with your app
		as parameters can be changed at any time! so be aware, especially with strings. You might get crashes.
		This can be useful in situation where your main thread is blocked for seconds, or your app runs
		at a very low framerate. In those situations, the server doesnt get updated often enough,
		and you might get disconnected. Using a background thread means you can still control your params
		as the main thread is blocked, but it also means the changes may happen at any time. Also, the
		callback method will be called from a background thread, so keep it in mind. (no GL calls in there!)
	 */
#endif

	//You shouldnt need to call any of these directly. Use the Macros supplied above instead.
	void update(float dt);
	void draw(int x = 20, int y = 20); //draws important notifications on screen
	void close();
	vector<string> loadFromXML(string fileName); //returns list of param names in current setup but not set in XML

	void saveToXML(string fileName); //save the whole list of params to an xml
	void saveGroupToXML(string fileName, string groupName); //save only a subset of params into xml

	void shareParam(string paramName, float* param, float min, float max, ofColor bgColor = ofColor(0,0,0,0) );
	void shareParam(string paramName, bool* param, ofColor bgColor = ofColor(0,0,0,0), int nothing = 0 ); //"nothing" args are just to match other methods
	void shareParam(string paramName, int* param, int min, int max, ofColor bgColor = ofColor(0,0,0,0) );
	void shareParam(string paramName, string* param, ofColor bgColor = ofColor(0,0,0,0), int nothing = 0 ); //"nothing" args are just to match other methods
	void shareParam(string paramName, int* param, int min, int max, vector<string> names, ofColor c = ofColor(0,0,0,0)); //enum!
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

	void setEnabled(bool enabled);
	void setSaveToXMLOnExit(bool save);
	void setDrawsNotificationsAutomaticallly(bool draw);
	void setNetworkInterface(string iface);
	void pushParamsToClient(); //pushes all param values to client, updating its UI
	void sendLogToClient(char* format, ...);
	void setClearXMLonSave(bool clear){clearXmlOnSaving = clear;}

	void removeParamFromDB(string paramName);	//useful for params its value is kinda set,
												//to load from xml and then remove from the list to
												//avoid crowding it too much

	void setShowInterfaceKey(char k);
	void setAutomaticBackupsEnabled(bool enabled);

private:

	ofxRemoteUIServer(); // use ofxRemoteUIServer::instance() instead! Use the MACROS defined above!
	~ofxRemoteUIServer();

	struct XmlCounter{ //used to hold xml save counts when saving to xml
		XmlCounter(){ numFloats = numInts = numStrings = numBools = numEnums = numColors = 0; }
		int numFloats, numInts, numStrings, numBools, numEnums, numColors;
	};

	void			restoreAllParamsToInitialXML();
	void			restoreAllParamsToDefaultValues();
	void			connect(string address, int port);
	void			setColorForParam(RemoteUIParam &p, ofColor c);
	vector<string>	getAvailablePresets();
	void			deletePreset(string name, string group=""); //if group is not "", then this is a global preset. otherwise its a group preset
	void			updateServer(float dt);
	void			handleBroadcast();
	void			(*callBack)(RemoteUIServerCallBackArg);
	void			threadedFunction();

	void			saveParamToXmlSettings(RemoteUIParam p, string key, ofxXmlSettings & s, XmlCounter & counter);
	void			saveSettingsBackup();

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

	bool			doBroadcast; //controls if the server advertises itself
	bool			drawNotifications;

	bool			loadedFromXML; //we start with loadedFromXML=false; once loadXML is called, this becomes true
	bool			saveToXmlOnExit;

	bool			threadedUpdate;
	bool			showValuesOnScreen; //displays all params on screen
	bool			updatedThisFrame;
	bool			clearXmlOnSaving;  //if false, allows you to keep defaults for old params that you are not sharing anymore

	char			showInterfaceKey;

	bool			enabled;
	bool			autoBackups;

#ifdef OF_AVAILABLE
	ofxRemoteUISimpleNotifications onScreenNotifications;
	void			_appExited(ofEventArgs &e);
	void			_draw(ofEventArgs &d);
	void			_update(ofEventArgs &d);
	void			_keyPressed(ofKeyEventArgs &e);

	int				selectedItem;
#endif

	static ofxRemoteUIServer* singleton;
};

#endif
