//
//  ofxRemoteUIVars.h
//  BaseApp
//
//  Created by Mark van de Korput on 10/10/14.
//
//

#ifndef __BaseApp__ofxRemoteUIVars__
#define __BaseApp__ofxRemoteUIVars__

#include "ofxRemoteUIServer.h"


//#define OFX_REMOTEUI_SERVER_SHARE_PARAM(val, ...)						\
//( ofxRemoteUIServer::instance()->shareParam( #val, &val, ##__VA_ARGS__ ) )

//#define OFX_REMOTEUI_SERVER_GET_OR_DEFINE_VAR
#define OFX_REMOTEUI_SERVER_DEFINE_VAR(type, name, ...) \
    ( ofxRemoteUIServer::instance()->shareParam( name, ofxRemoteUIVars<type>::one().defineParam(name), ##__VA_ARGS__) )

#define OFX_REMOTEUI_SERVER_DEFINE_VAR_WITH_VALUE(type, name, value, ...) \
    ( ofxRemoteUIServer::instance()->shareParam( name, ofxRemoteUIVars<type>::one().defineParam(name, value), ##__VA_ARGS__) )

#define OFX_REMOTEUI_SERVER_GET_VAR_ADDRESS(type, name) \
    ( ofxRemoteUIVars<type>::one().getParam(name) )

#define OFX_REMOTEUI_SERVER_GET_VAR(type, name) \
    ( *OFX_REMOTEUI_SERVER_GET_VAR_ADDRESS(type, name) )

#define OFX_REMOTEUI_SERVER_SET_VAR(type, name, value) \
    if(type *__tmp_p__ = OFX_REMOTEUI_SERVER_GET_VAR_ADDRESS(type, name)){ (*__tmp_p__) = value;}

// shorter macro aliases
#define RUI_DEFINE_VAR          OFX_REMOTEUI_SERVER_DEFINE_VAR
#define RUI_DEFINE_VAR_WV       OFX_REMOTEUI_SERVER_DEFINE_VAR_WITH_VALUE
#define RUI_GET_VAR_ADDRESS     OFX_REMOTEUI_SERVER_GET_VAR_ADDRESS
#define RUI_GET_VAR             OFX_REMOTEUI_SERVER_GET_VAR
#define RUI_SET_VAR             OFX_REMOTEUI_SERVER_SET_VAR
#define RUI_VAR                 RUI_GET_VAR



template <typename VarType>
class ofxRemoteUIVars {

public:
    // Singleton stuff; usually only one instance of this class needed
    static ofxRemoteUIVars& one()
    {
        static ofxRemoteUIVars instance;
        // Instantiated on first use.
        return instance;
    }
    
public:
    ~ofxRemoteUIVars();
    VarType* getParam(string name);

    VarType* defineParam(string name);
    VarType* defineParam(string name, VarType value);


protected:
    map<string, VarType> namedPointers;
    // vector<VarType*> list;

}; // class ofxRemoteUIVars


template <typename VarType>
VarType* ofxRemoteUIVars<VarType>::getParam(string name){
    // Find existing named pointer and return its address
    try {
        return &namedPointers[name];
    } catch (std::out_of_range e) {
    }

    return NULL;
}

// So here's the promblem; on windows (this problem doesn't seem to occur on Mac)
// The ofxRemoteUIVars<***>::instance global static variables are DESTRUCTED, before
// ofxRemoteUIServer's auto-save-on-exit callback is triggered. Which means that the callback
// is saving garbage for all the parameters from ofxRemoteUIVars. Solution:
// When a singleton instance of ofxRemoteUIVars is destroyed, it's pretty save to assume
// Everything is shutting down and it's time to auto-save (but only if this is enabled of course)
// so this destructor explicitly triggers the auto-save and disables the preregistered auto-saves.
template <typename VarType>
ofxRemoteUIVars<VarType>::~ofxRemoteUIVars(){
	// only continue if this is the singleton instance of this class
	if(this != &ofxRemoteUIVars<VarType>::one()) return;
	// only continue if ofxRemoteUIServer is atually configured to automatically save on exit
	if(!OFX_REMOTEUI_SERVER_GET_SAVES_ON_EXIT()) return;
	// explicitly request to save now, don't wait until auto-scheduled on-exit-callback
	OFX_REMOTEUI_SERVER_SAVE_TO_XML();
	// we already saved, so no need for it to happen again, disable auto-save
	OFX_REMOTEUI_SERVER_SET_SAVES_ON_EXIT(false);
}

template <typename VarType>
VarType* ofxRemoteUIVars<VarType>::defineParam(string name){
    // just need a temp var to push into the vector
    // VarType tmp;
    // store address with name
    namedPointers[name] = VarType(); // = tmp;
    // return address
    return &namedPointers[name];
}

template <typename VarType>
VarType* ofxRemoteUIVars<VarType>::defineParam(string name, VarType value){
    VarType* p = defineParam(name);
    (*p) = value;
    return p;
}


#endif /* defined(__BaseApp__ofxRemoteUIVars__) */
