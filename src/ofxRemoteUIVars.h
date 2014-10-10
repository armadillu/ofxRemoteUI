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

#define OFX_REMOTEUI_SERVER_GET_VAR_ADDRESS(type, name) \
    ( ofxRemoteUIVars<type>::one().getParam(name) )

#define OFX_REMOTEUI_SERVER_GET_VAR(type, name) \
    ( *OFX_REMOTEUI_SERVER_GET_VAR_ADDRESS(type, name) )

#define OFX_REMOTEUI_SERVER_SET_VAR(type, name, value) \
    if(type *__tmp_p__ = OFX_REMOTEUI_SERVER_GET_VAR_ADDRESS(type, name)){ (*__tmp_p__) = value;}

// shorter macro aliases
#define RUI_DEFINE_VAR          OFX_REMOTEUI_SERVER_DEFINE_VAR
#define RUI_GET_VAR_ADDRESS     OFX_REMOTEUI_SERVER_GET_VAR_ADDRESS
#define RUI_GET_VAR             OFX_REMOTEUI_SERVER_GET_VAR
#define RUI_SET_VAR             OFX_REMOTEUI_SERVER_SET_VAR



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

template <typename VarType>
ofxRemoteUIVars<VarType>::~ofxRemoteUIVars(){
//    for(int i=0; i<list.size(); i++){
//        delete list[i];
//    }
//
//    list.clear();
}

template <typename VarType>
VarType* ofxRemoteUIVars<VarType>::defineParam(string name){
    // just need a temp var to push into the vector
    VarType tmp;
    // store
    // list.push_back(tmp);
    // store address with name
    namedPointers[name] = tmp;
    // return address
    return &namedPointers[name];
}


#endif /* defined(__BaseApp__ofxRemoteUIVars__) */
