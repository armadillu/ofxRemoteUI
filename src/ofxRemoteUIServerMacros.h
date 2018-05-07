//
//  ofxRemoteUIServerNacros.h
//  emptyExample
//
//  Created by Oriol Ferrer Mesià on 09/01/13.
//
//

#ifndef _ofxRemoteUIServerMacros__
#define _ofxRemoteUIServerMacros__


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

#define OFX_REMOTEUI_SERVER_SHARE_PARAM_WITH_DESC(val, desc, ...)		\
( ofxRemoteUIServer::instance()->shareParam( #val, &val, ##__VA_ARGS__ ); ofxRemoteUIServer::instance()->setParamDescription(#val, desc))

//WCN - "with custom name" share a param with a custom string instead of taking the var name
#define OFX_REMOTEUI_SERVER_SHARE_PARAM_WCN(pName, val, ...)			\
( ofxRemoteUIServer::instance()->shareParam( pName, &val, ##__VA_ARGS__ ) )

//use this macro to share enums + enumList; enum list can be vector<string> or string[]
#define OFX_REMOTEUI_SERVER_SHARE_ENUM_PARAM(val, enumMin, enumMax, menuList, ...)	\
( ofxRemoteUIServer::instance()->shareParam( #val, (int*)&val,enumMin, enumMax, menuList, ##__VA_ARGS__ ) )

//use this macro to share enums + enumList with a string for the name; enum list can be vector<string> or string[]
#define OFX_REMOTEUI_SERVER_SHARE_ENUM_PARAM_WCN(pName, val, enumMin, enumMax, menuList, ...)	\
( ofxRemoteUIServer::instance()->shareParam( pName, (int*)&val, enumMin, enumMax, menuList, ##__VA_ARGS__ ) )

//use this macro to share ofColors
#define OFX_REMOTEUI_SERVER_SHARE_COLOR_PARAM(color, ...)				\
( ofxRemoteUIServer::instance()->shareParam( #color, (unsigned char*)&color.v[0], ##__VA_ARGS__ ) )

//use this macro to share ofColors with a custom string for the name
#define OFX_REMOTEUI_SERVER_SHARE_COLOR_PARAM_WCN(pName, color, ...)    \
( ofxRemoteUIServer::instance()->shareParam( pName, (unsigned char*)&color.v[0], ##__VA_ARGS__ ) )

/*set a new group for the upcoming params, this also sets a new color*/
#define OFX_REMOTEUI_SERVER_SET_UPCOMING_PARAM_GROUP(g)					\
( ofxRemoteUIServer::instance()->setParamGroup( g ) )
#define OFX_REMOTEUI_SERVER_NEW_GROUP(g) OFX_REMOTEUI_SERVER_SET_UPCOMING_PARAM_GROUP(g)

/*set a new small 'hue' change for upcoming params, to create alternating rows inside a group*/
#define OFX_REMOTEUI_SERVER_SET_NEW_COLOR()								\
( ofxRemoteUIServer::instance()->setNewParamColorVariation() )
#define OFX_REMOTEUI_SERVER_SET_NEW_TONE()/*deprecated!*/				\
( ofxRemoteUIServer::instance()->setNewParamColorVariation() )

/*find out if a param exists*/
#define OFX_REMOTEUI_SERVER_PARAM_EXISTS(pname)							\
( ofxRemoteUIServer::instance()->paramExistsForName(pname) )

/*allows you to get a param from anywhere in your code*/
#define OFX_REMOTEUI_SERVER_GET_PARAM(pname)							\
( ofxRemoteUIServer::instance()->getParamForName(pname) )

/*allows you to get a param ref from anywhere in your code*/
#define OFX_REMOTEUI_SERVER_GET_PARAM_REF(pname)							\
( ofxRemoteUIServer::instance()->getParamRefForName(pname) )

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

/*toggle backup of main xml file at launch - defaults is off*/
#define OFX_REMOTEUI_SERVER_SET_AUTO_XML_BACKUPS(doAutoBackups)			\
( ofxRemoteUIServer::instance()->setAutomaticBackupsEnabled(doAutoBackups) )

/*set the show interface key*/
#define OFX_REMOTEUI_SERVER_SET_SHOW_INTERFACE_KEY(k)                  \
( ofxRemoteUIServer::instance()->setShowInterfaceKey(k) )

/*define where all the xml files should be saved*/
#define OFX_REMOTEUI_SERVER_SET_CONFIGS_DIR(dir)						\
( ofxRemoteUIServer::instance()->setDirectoryPrefix(dir) )

/*close the server. no need to call this from OF*/
#define OFX_REMOTEUI_SERVER_CLOSE()										\
( ofxRemoteUIServer::instance()->close() )

/*get if saves to XML automatically on app exit. Default is YES in OF*/
#define	OFX_REMOTEUI_SERVER_GET_SAVES_ON_EXIT()	 				\
( ofxRemoteUIServer::instance()->getSaveToXMLOnExit() )

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

#define OFX_REMOTEUI_SERVER_REMOVE_PARAM_WCN(paramName)						\
( ofxRemoteUIServer::instance()->removeParamFromDB(paramName) )

//sets the param as "to watch", so its value is printed on
//screen all the time. WCN version in case you want to supply a custom string instead of the var itself
#define OFX_REMOTEUI_SERVER_WATCH_PARAM(paramName)						\
( ofxRemoteUIServer::instance()->watchParamOnScreen(#paramName) )
#define OFX_REMOTEUI_SERVER_WATCH_PARAM_WCN(paramName)					\
( ofxRemoteUIServer::instance()->watchParamOnScreen(paramName) )

//add a description to the last added param
#define	OFX_REMOTEUI_SERVER_SET_PARAM_DESC(param,desc)		\
( ofxRemoteUIServer::instance()->setParamDescription(param,desc) )

#define	OFX_REMOTEUI_SERVER_SET_DESC_TO_LAST_PARAM(desc)		\
( ofxRemoteUIServer::instance()->setDescriptionForLastAddedParam(desc) )



#ifdef OF_AVAILABLE
/*run the server on a back thread. Useful for apps with very low framerate.
 default is disabled in OF; only works in OF! */
#define OFX_REMOTEUI_SERVER_START_THREADED()							\
( ofxRemoteUIServer::instance()->startInBackgroundThread() )

#define OFX_REMOTEUI_SERVER_SET_UI_COLUMN_WIDTH(w)						\
( ofxRemoteUIServer::instance()->setUiColumnWidth(w) )

#define OFX_REMOTEUI_SERVER_GET_CLIENT_OF_EVENT()						\
( ofxRemoteUIServer::instance()->clientAction )

#endif

// shorter macros //
#define RUI_SETUP					OFX_REMOTEUI_SERVER_SETUP
#define RUI_SHARE_PARAM				OFX_REMOTEUI_SERVER_SHARE_PARAM
#define RUI_SHARE_PARAM_DESC			OFX_REMOTEUI_SERVER_SHARE_PARAM_WITH_DESC
#define RUI_SHARE_PARAM_WCN			OFX_REMOTEUI_SERVER_SHARE_PARAM_WCN
#define RUI_SHARE_ENUM_PARAM		OFX_REMOTEUI_SERVER_SHARE_ENUM_PARAM
#define RUI_SHARE_ENUM_PARAM_WCN    OFX_REMOTEUI_SERVER_SHARE_ENUM_PARAM_WCN
#define RUI_SHARE_COLOR_PARAM		OFX_REMOTEUI_SERVER_SHARE_COLOR_PARAM
#define RUI_SHARE_COLOR_PARAM_WCN   OFX_REMOTEUI_SERVER_SHARE_COLOR_PARAM_WCN
#define RUI_NEW_GROUP				OFX_REMOTEUI_SERVER_SET_UPCOMING_PARAM_GROUP
#define RUI_NEW_COLOR				OFX_REMOTEUI_SERVER_SET_NEW_COLOR
#define RUI_SET_CALLBACK			OFX_REMOTEUI_SERVER_SET_CALLBACK
#define	RUI_LOAD_FROM_XML			OFX_REMOTEUI_SERVER_LOAD_FROM_XML
#define	RUI_SAVE_TO_XML				OFX_REMOTEUI_SERVER_SAVE_TO_XML
#define RUI_START_THREADED			OFX_REMOTEUI_SERVER_START_THREADED
#define RUI_SET_AUTO_XML_BACKUPS	OFX_REMOTEUI_SERVER_SET_AUTO_XML_BACKUPS
#define RUI_SET_SAVES_ON_EXIT		OFX_REMOTEUI_SERVER_SET_SAVES_ON_EXIT
#define RUI_SET_NET_INTERFACE		OFX_REMOTEUI_SERVER_SET_NET_INTERFACE
#define RUI_SET_DRAWS_NOTIF			OFX_REMOTEUI_SERVER_SET_DRAWS_NOTIF
#define RUI_PUSH_TO_CLIENT			OFX_REMOTEUI_SERVER_PUSH_TO_CLIENT
#define RUI_GET_INSTANCE			OFX_REMOTEUI_SERVER_GET_INSTANCE
#define RUI_LOG						OFX_REMOTEUI_SERVER_LOG
#define RUI_REMOVE_PARAM			OFX_REMOTEUI_SERVER_REMOVE_PARAM
#define RUI_REMOVE_PARAM_WCN		OFX_REMOTEUI_SERVER_REMOVE_PARAM_WCN
#define RUI_WATCH_PARAM				OFX_REMOTEUI_SERVER_WATCH_PARAM
#define RUI_WATCH_PARAM_WCN			OFX_REMOTEUI_SERVER_WATCH_PARAM_WCN
#define RUI_SET_INTERFACE_KEY       OFX_REMOTEUI_SERVER_SET_SHOW_INTERFACE_KEY
#define RUI_SET_CONFIGS_DIR			OFX_REMOTEUI_SERVER_SET_CONFIGS_DIR
#define RUI_GET_OF_EVENT			OFX_REMOTEUI_SERVER_GET_CLIENT_OF_EVENT
#define RUI_PARAM_EXISTS			OFX_REMOTEUI_SERVER_PARAM_EXISTS
#define RUI_GET_PARAM				OFX_REMOTEUI_SERVER_GET_PARAM
#define RUI_GET_PARAM_REF			OFX_REMOTEUI_SERVER_GET_PARAM_REF
#define RUI_UPDATE					OFX_REMOTEUI_SERVER_UPDATE
#define	RUI_CLOSE					OFX_REMOTEUI_SERVER_CLOSE
#define RUI_SET_PARAM_DESC			OFX_REMOTEUI_SERVER_SET_PARAM_DESC
#define RUI_SET_LAST_PARAM_DESC			OFX_REMOTEUI_SERVER_SET_DESC_TO_LAST_PARAM
//


#endif
