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

	string gName = _parameters.getName();
	if(gName.size()) RUI_NEW_GROUP(gName);
	ofxRemoteUIServer * server = ofxRemoteUIServer::instance();

	for(int i=0;i<_parameters.size();i++){

		string type = _parameters.getType(i);
		string pName = gName;
		if(pName.size()) pName += SEP;

		if(type==typeid(ofParameter<int>).name()){ //INT
			ofParameter<int> p = _parameters.getInt(i);
			RUI_DEFINE_VAR_WV(int, pName + p.getName(), p, p.getMin(), p.getMax());

		}else if(type==typeid(ofParameter<float>).name()){ //FLOAT
			ofParameter<float> p = _parameters.getFloat(i);
			RUI_DEFINE_VAR_WV(float, pName + p.getName(), p, p.getMin(), p.getMax());

		}else if(type==typeid(ofParameter<bool>).name()){ //BOOL
			ofParameter<bool> p = _parameters.getBool(i);
			RUI_DEFINE_VAR_WV(bool, pName + p.getName(), p);

		}else if(type==typeid(ofParameter<string>).name()){ //STRING
			ofParameter<string> p = _parameters.getString(i);
			RUI_DEFINE_VAR_WV(string, pName + p.getName(), p);

		}else if(type==typeid(ofParameter<ofColor>).name()){ //COLOR
			ofParameter<ofColor> p = _parameters.getColor(i);
			ofColor * c = ofxRemoteUIVars<ofColor>::one().defineParam(pName + p.getName(), p);
			server->shareParam(pName + p.getName(), &(*c)[0] );

		}else if(type==typeid(ofParameter<ofVec2f>).name()){ //ofVec2f
			ofParameter<ofVec2f> p = _parameters.getVec2f(i);
			ofVec2f * v = ofxRemoteUIVars<ofVec2f>::one().defineParam(pName + p.getName(), p);
			server->shareParam(pName + p.getName() + ".2x", &(v->x), p.getMin().x, p.getMax().x);
			server->shareParam(pName + p.getName() + ".2y", &(v->y), p.getMin().y, p.getMax().y);

		}else if(type==typeid(ofParameter<ofVec3f>).name()){ //ofVec3f
			ofParameter<ofVec3f> p = _parameters.getVec3f(i);
			ofVec3f * v = ofxRemoteUIVars<ofVec3f>::one().defineParam(pName + p.getName(), p);
			server->shareParam(pName + p.getName() + ".3x", &(v->x), p.getMin().x, p.getMax().x);
			server->shareParam(pName + p.getName() + ".3y", &(v->y), p.getMin().y, p.getMax().y);
			server->shareParam(pName + p.getName() + ".3z", &(v->z), p.getMin().z, p.getMax().z);

		}else if(type==typeid(ofParameter<ofVec4f>).name()){ //ofVec4f
			ofParameter<ofVec4f> p = _parameters.getVec4f(i);
			ofVec4f * v = ofxRemoteUIVars<ofVec4f>::one().defineParam(pName + p.getName(), p);
			server->shareParam(pName + p.getName() + ".4x", &(v->x), p.getMin().x, p.getMax().x);
			server->shareParam(pName + p.getName() + ".4y", &(v->y), p.getMin().y, p.getMax().y);
			server->shareParam(pName + p.getName() + ".4z", &(v->z), p.getMin().z, p.getMax().z);
			server->shareParam(pName + p.getName() + ".4w", &(v->w), p.getMin().w, p.getMax().w);

		}else if(type==typeid(ofParameterGroup).name()){
			ofParameterGroup p = _parameters.getGroup(i);
			recursiveSetup(p);
		}else{
			ofLogError("ofxRemoteUIofParamaterSync") << "Sorry I don't support " << type << " type.";
		}
	}
}


void ofxRemoteUIofParamaterSync::parameterChanged( ofAbstractParameter & parameter ){

	string n;;
	ofParameterGroup * g = parameter.getParent();
	if (g) n = g->getName() + SEP + parameter.getName();
	else n = parameter.getName();

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

	}else if(type==typeid(ofParameter<ofVec2f>).name()){
		ofParameter<ofVec2f> p = parameter.cast<ofVec2f>();
		ofVec2f * ptr = RUI_GET_VAR_ADDRESS(ofVec2f, n); //pts to R,
		*ptr = p;

	}else if(type==typeid(ofParameter<ofVec3f>).name()){
		ofParameter<ofVec3f> p = parameter.cast<ofVec3f>();
		ofVec3f * ptr = RUI_GET_VAR_ADDRESS(ofVec3f, n); //pts to R,
		*ptr = p;

	}else if(type==typeid(ofParameter<ofVec4f>).name()){
		ofParameter<ofVec4f> p = parameter.cast<ofVec4f>();
		ofVec4f * ptr = RUI_GET_VAR_ADDRESS(ofVec4f, n); //pts to R,
		*ptr = p;

	}else{
		ofLogError("ofxRemoteUIofParamaterSync") << "Sorry I don't support " << type << " type.";
	}
	RUI_PUSH_TO_CLIENT(); //send updates to client
}


void ofxRemoteUIofParamaterSync::remoteUIClientDidSomething(RemoteUIServerCallBackArg &arg){

	ofParameterGroup deepG;
	if (arg.action == CLIENT_UPDATED_PARAM) {

		vector<string> split = ofSplitString(arg.paramName, SEP); //separate group;
		ofParameterGroup * group;
		string paramName;

		if (split.size() == 1){ //not part of group
			group = syncGroup;
			paramName = arg.paramName;
		}else{ //part of group
			deepG = syncGroup->getGroup(split[0]);
			group = &deepG;
			paramName = split[1];
		}

		if(paramName.size() == 0){
			ofLogError("ofxRemoteUIofParamaterSync") << "no name!";
		}

		switch (arg.param.type) {

			case REMOTEUI_PARAM_FLOAT:{

				//most likely an ofVecN suffix ".Dx" - TODO  #############################
				if(paramName[paramName.size() - 3] == '.'){
					char vecComp = paramName[paramName.size() - 1]; // x | y | z | w
					char vecType = paramName[paramName.size() - 2]; // 2 | 3 | 4
					string noCompPname = paramName.substr(0, paramName.size() - 3);

					switch (vecType) {
						case '2':{
							ofParameter<ofVec2f> p = group->get<ofVec2f>(noCompPname);
							ofVec2f v = p.get();
							switch (vecComp) {
								case 'x': v.x = arg.param.floatVal; p.set(v); break;
								case 'y': v.y = arg.param.floatVal; p.set(v); break;
								default: break;
						}}break;

						case '3':{
							ofParameter<ofVec3f> p = group->get<ofVec3f>(noCompPname);
							ofVec3f v = p.get();
							switch (vecComp) {
								case 'x': v.x = arg.param.floatVal; p.set(v); break;
								case 'y': v.y = arg.param.floatVal; p.set(v); break;
								case 'z': v.z = arg.param.floatVal; p.set(v); break;
								default: break;
						}}break;

						case '4':{
							ofParameter<ofVec4f> p = group->get<ofVec4f>(noCompPname);
							ofVec4f v = p.get();
							switch (vecComp) {
								case 'x': v.x = arg.param.floatVal; p.set(v); break;
								case 'y': v.y = arg.param.floatVal; p.set(v); break;
								case 'z': v.z = arg.param.floatVal; p.set(v); break;
								case 'w': v.w = arg.param.floatVal; p.set(v); break;
								default: break;
							}}break;
					}

				}else{ // regular float - not a vector #####################################
					ofParameter<float> p = group->get<float>(paramName);
					p.set(arg.param.floatVal);
				}
			}break;

			case REMOTEUI_PARAM_INT:{
				ofParameter<int> p = group->get<int>(paramName);
				p.set(arg.param.intVal);
			}break;

			case REMOTEUI_PARAM_BOOL:{
				ofParameter<bool> p = group->get<bool>(paramName);
				p.set(arg.param.boolVal);
			}break;

			case REMOTEUI_PARAM_STRING:{
				ofParameter<string> p = group->get<string>(paramName);
				p.set(arg.param.stringVal);
			}break;

			case REMOTEUI_PARAM_COLOR:{
				ofParameter<ofColor> p = group->get<ofColor>(paramName);
				p.set(ofColor(arg.param.redVal, arg.param.blueVal, arg.param.greenVal, arg.param.alphaVal));
			}break;

			default:
				ofLogError("ofxRemoteUIofParamaterSync") << "Unknown RemoteUIParamType ?";
				break;
		}
	}
}