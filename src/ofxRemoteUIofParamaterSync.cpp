//
//  ofxRemoteUIofParamaterSync.cpp
//  oscParametersReceiver
//
//  Created by Oriol Ferrer Mesià on 2/5/15.
//
//

#include "ofxRemoteUIofParamaterSync.h"

ofxRemoteUIofParamaterSync::ofxRemoteUIofParamaterSync(){


}

void ofxRemoteUIofParamaterSync::setup(ofParameterGroup & _parameters){

	ofAddListener(_parameters.parameterChangedE, this, &ofxRemoteUIofParamaterSync::parameterChanged);
	ofAddListener(RUI_GET_OF_EVENT(), this, &ofxRemoteUIofParamaterSync::remoteUIClientDidSomething);
	recursiveSetup(_parameters);
	syncGroup = &_parameters;
}


void ofxRemoteUIofParamaterSync::recursiveSetup(ofParameterGroup & _parameters){

	RUI_NEW_GROUP(_parameters.getName());

	for(int i=0;i<_parameters.size();i++){
		string type = _parameters.getType(i);
		if(type==typeid(ofParameter<int>).name()){ //INT
			ofParameter<int> p = _parameters.getInt(i);
			RUI_DEFINE_VAR_WV(int, p.getName(), p, p.getMin(), p.getMax());
		}else if(type==typeid(ofParameter<float>).name()){ //FLOAT
			ofParameter<float> p = _parameters.getFloat(i);
			RUI_DEFINE_VAR_WV(float, p.getName(), p, p.getMin(), p.getMax());
		}else if(type==typeid(ofParameter<bool>).name()){ //BOOL
			ofParameter<bool> p = _parameters.getBool(i);
			RUI_DEFINE_VAR_WV(bool, p.getName(), p);
		}else if(type==typeid(ofParameter<string>).name()){ //STRING
			ofParameter<string> p = _parameters.getString(i);
			RUI_DEFINE_VAR_WV(string, p.getName(), p);
		}else if(type==typeid(ofParameter<ofColor>).name()){ //COLOR
			ofParameter<ofColor> p = _parameters.getColor(i);
			//RUI_DEFINE_VAR_WV(ofColor, p.getName(), p);
			ofxRemoteUIServer * server = ofxRemoteUIServer::instance();
			ofColor * c = ofxRemoteUIVars<ofColor>::one().defineParam(p.getName(), p);
			server->shareParam(p.getName(), &(*c)[0] );
		}else if(type==typeid(ofParameterGroup).name()){
			ofParameterGroup p = _parameters.getGroup(i);
			recursiveSetup(p);
		}else{
			ofLogWarning() << "ofxBaseGroup; no control for parameter of type " << type;
		}
	}
}


void ofxRemoteUIofParamaterSync::parameterChanged( ofAbstractParameter & parameter ){

	string n = parameter.getName();
	RemoteUIParamType ruiType;
	string type = parameter.type();

	if(type==typeid(ofParameter<int>).name()){
		ofParameter<int> p = parameter.cast<int>();
		int * ptr = RUI_GET_VAR_ADDRESS(int, n);
		*ptr = p;
	}else if(type==typeid(ofParameter<float>).name()){
		ofParameter<float> p = parameter.cast<float>();
		float * ptr = RUI_GET_VAR_ADDRESS(float, n);
		*ptr = p;
	}else if(type==typeid(ofParameter<bool>).name()){
		ofParameter<bool> p = parameter.cast<bool>();
		bool * ptr = RUI_GET_VAR_ADDRESS(bool, n);
		*ptr = p;
	}else if(type==typeid(ofParameter<string>).name()){
		ofParameter<string> p = parameter.cast<string>();
		string * ptr = RUI_GET_VAR_ADDRESS(string, n);
		*ptr = p;
	}else if(type==typeid(ofParameter<ofColor>).name()){
		ofParameter<ofColor> p = parameter.cast<ofColor>();
		ofColor * ptr = RUI_GET_VAR_ADDRESS(ofColor, n); //pts to R,
		*ptr = p;
	}
	RUI_PUSH_TO_CLIENT(); //send updates to client
}


void ofxRemoteUIofParamaterSync::remoteUIClientDidSomething(RemoteUIServerCallBackArg &arg){

	if (arg.action == CLIENT_UPDATED_PARAM) {
		switch (arg.param.type) {
			case REMOTEUI_PARAM_FLOAT:{
				ofParameter<float> p = syncGroup->get<float>(arg.paramName);
				p.set(arg.param.floatVal);
				}break;
			case REMOTEUI_PARAM_INT:{
				ofParameter<int> p = syncGroup->get<int>(arg.paramName);
				p.set(arg.param.intVal);
				}break;
			case REMOTEUI_PARAM_BOOL:{
				ofParameter<bool> p = syncGroup->get<bool>(arg.paramName);
				p.set(arg.param.boolVal);
				}break;
			case REMOTEUI_PARAM_STRING:{
				ofParameter<string> p = syncGroup->get<string>(arg.paramName);
				p.set(arg.param.stringVal);
				}break;
			case REMOTEUI_PARAM_COLOR:{
				ofParameter<ofColor> p = syncGroup->get<ofColor>(arg.paramName);
				p.set(ofColor(arg.param.redVal, arg.param.blueVal, arg.param.greenVal, arg.param.alphaVal));
			}break;

		}
	}
}