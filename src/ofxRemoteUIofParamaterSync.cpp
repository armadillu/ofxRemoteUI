//
//  ofxRemoteUIofParamaterSync.cpp
//  oscParametersReceiver
//
//  Created by Oriol Ferrer Mesià on 2/5/15.
//
//

#include "ofxRemoteUIofParamaterSync.h"

ofxRemoteUIofParamaterSync::ofxRemoteUIofParamaterSync(){}

void ofxRemoteUIofParamaterSync::setup(ofParameterGroup & _parameters){

	ofAddListener(_parameters.parameterChangedE, this, &ofxRemoteUIofParamaterSync::parameterChanged);
	ofAddListener(RUI_GET_OF_EVENT(), this, &ofxRemoteUIofParamaterSync::remoteUIClientDidSomething);
	recursiveSetup(_parameters);
	syncGroup = &_parameters;
}

string ofxRemoteUIofParamaterSync::getShortVersionForGroupPath(const string & groupPath){

	map<string, string>::iterator it = groupClusterID.find(groupPath);
	if ( it == groupClusterID.end()){ //never heard of that one before
		vector<string> split = ofSplitString(groupPath, SEP);
		string shortGroup;
		for(int i = 0; i < split.size() - 1; i++){
			map<string,int>::iterator it = uniqueWords.find(split[i]);
			string shortS;
			if (it == uniqueWords.end()){ //new unique word, lets make it happen
				shortS = ofToString(uniqueWords.size());
				uniqueWords[split[i]] = uniqueWords.size();
			}else{
				shortS = ofToString(it->second);
			}
			shortGroup += ofToString(shortS) + SEP;
		}
		groupClusterID[shortGroup] = groupPath;
		return shortGroup;
	}else{
		return it->second;
	}
}


string ofxRemoteUIofParamaterSync::getFullGroupPathForShortVersion(const string & shortV){
	return groupClusterID[shortV];
}


void ofxRemoteUIofParamaterSync::recursiveSetup(ofParameterGroup & _parameters){

	ofxRemoteUIServer * server = ofxRemoteUIServer::instance();

	for(int i = 0; i < _parameters.size() ; i++){

		string type = _parameters.getType(i);
		string fullGroupTreeName;
		ofAbstractParameter & absP = _parameters.get(i);
		ofParameterGroup * walker = absP.getParent();

		while (walker){ //its a group!
			fullGroupTreeName = walker->getName() + SEP + fullGroupTreeName;
			walker = walker->getParent();
		}

		string shortedGroupName = getShortVersionForGroupPath(fullGroupTreeName);
		string fullRUIparamName = shortedGroupName + absP.getName();

		if(fullGroupTreeName.size() && i == 0) RUI_NEW_GROUP(fullGroupTreeName);

		if(type==typeid(ofParameter<int>).name()){ //INT
			ofParameter<int> p = _parameters.getInt(i);
			RUI_DEFINE_VAR_WV(int, fullRUIparamName, p, p.getMin(), p.getMax());

		}else if(type==typeid(ofParameter<float>).name()){ //FLOAT
			ofParameter<float> p = _parameters.getFloat(i);
			RUI_DEFINE_VAR_WV(float, fullRUIparamName, p, p.getMin(), p.getMax());

		}else if(type==typeid(ofParameter<bool>).name()){ //BOOL
			ofParameter<bool> p = _parameters.getBool(i);
			RUI_DEFINE_VAR_WV(bool, fullRUIparamName, p);

		}else if(type==typeid(ofParameter<string>).name()){ //STRING
			ofParameter<string> p = _parameters.getString(i);
			RUI_DEFINE_VAR_WV(string, fullRUIparamName, p);

		}else if(type==typeid(ofParameter<ofColor>).name()){ //COLOR
			ofParameter<ofColor> p = _parameters.getColor(i);
			ofColor * c = ofxRemoteUIVars<ofColor>::one().defineParam(fullRUIparamName, p);
			server->shareParam(fullRUIparamName, &(*c)[0] );

		}else if(type==typeid(ofParameter<ofVec2f>).name()){ //ofVec2f
			ofParameter<ofVec2f> p = _parameters.getVec2f(i);
			ofVec2f * v = ofxRemoteUIVars<ofVec2f>::one().defineParam(fullRUIparamName, p);
			server->shareParam(fullRUIparamName + ".2x", &(v->x), p.getMin().x, p.getMax().x);
			server->shareParam(fullRUIparamName + ".2y", &(v->y), p.getMin().y, p.getMax().y);

		}else if(type==typeid(ofParameter<ofVec3f>).name()){ //ofVec3f
			ofParameter<ofVec3f> p = _parameters.getVec3f(i);
			ofVec3f * v = ofxRemoteUIVars<ofVec3f>::one().defineParam(fullRUIparamName, p);
			server->shareParam(fullRUIparamName + ".3x", &(v->x), p.getMin().x, p.getMax().x);
			server->shareParam(fullRUIparamName + ".3y", &(v->y), p.getMin().y, p.getMax().y);
			server->shareParam(fullRUIparamName + ".3z", &(v->z), p.getMin().z, p.getMax().z);

		}else if(type==typeid(ofParameter<ofVec4f>).name()){ //ofVec4f
			ofParameter<ofVec4f> p = _parameters.getVec4f(i);
			ofVec4f * v = ofxRemoteUIVars<ofVec4f>::one().defineParam(fullRUIparamName, p);
			server->shareParam(fullRUIparamName + ".4x", &(v->x), p.getMin().x, p.getMax().x);
			server->shareParam(fullRUIparamName + ".4y", &(v->y), p.getMin().y, p.getMax().y);
			server->shareParam(fullRUIparamName + ".4z", &(v->z), p.getMin().z, p.getMax().z);
			server->shareParam(fullRUIparamName + ".4w", &(v->w), p.getMin().w, p.getMax().w);

		}else if(type==typeid(ofParameterGroup).name()){
			ofParameterGroup p = _parameters.getGroup(i);
			recursiveSetup(p);
		}else{
			ofLogError("ofxRemoteUIofParamaterSync") << "Sorry I don't support " << type << " type.";
		}
	}
}


void ofxRemoteUIofParamaterSync::parameterChanged( ofAbstractParameter & parameter ){

	string fullGroupTreeName;
	ofParameterGroup * walker = parameter.getParent();
	while (walker){ //its a group!
		fullGroupTreeName = walker->getName() + SEP + fullGroupTreeName;
		walker = walker->getParent();
	}

	string pName = parameter.getName();
	string groupTree = ofFilePath::getEnclosingDirectory(fullGroupTreeName, false);
	string shortGroupTree = getShortVersionForGroupPath(fullGroupTreeName);

	string type = parameter.type();
	string n = shortGroupTree + pName;

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

	if (arg.action == CLIENT_UPDATED_PARAM) {

		string paramName = ofFilePath::getFileName(arg.paramName, false);
		if(paramName.size() == 0){
			ofLogError("ofxRemoteUIofParamaterSync") << "no name!";
		}
		string shortGroupPath = ofFilePath::getEnclosingDirectory(arg.paramName, false);
		//shortGroupPath = ofFilePath::removeTrailingSlash(shortGroupPath);
		string fullGroupPath = getFullGroupPathForShortVersion(shortGroupPath);

		vector<string> split = ofSplitString(fullGroupPath, SEP); //separate group;

		ofAbstractParameter * myParam = NULL;;

		if (split.size() == 1){ //not part of group - dont think its possible?
			paramName = arg.paramName;

		}else{ //part of group
			myParam = &findInChildren(*syncGroup, split, cleanParamName(paramName));
		}

		if(myParam == NULL){
			ofLogError("ofxRemoteUIofParamaterSync") << "cant get ofParameter! " << arg.paramName;
			return;
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
							ofParameter<ofVec2f> p = myParam->cast<ofVec2f>();
							ofVec2f v = p.get();
							switch (vecComp) {
								case 'x': v.x = arg.param.floatVal; p.set(v); break;
								case 'y': v.y = arg.param.floatVal; p.set(v); break;
								default: break;
						}}break;

						case '3':{
							ofParameter<ofVec3f> p = myParam->cast<ofVec3f>();
							ofVec3f v = p.get();
							switch (vecComp) {
								case 'x': v.x = arg.param.floatVal; p.set(v); break;
								case 'y': v.y = arg.param.floatVal; p.set(v); break;
								case 'z': v.z = arg.param.floatVal; p.set(v); break;
								default: break;
						}}break;

						case '4':{
							ofParameter<ofVec4f> p = myParam->cast<ofVec4f>();
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
					ofParameter<float> p = myParam->cast<float>();
					p.set(arg.param.floatVal);
				}
			}break;

			case REMOTEUI_PARAM_INT:{
				ofParameter<int> p = myParam->cast<int>();
				p.set(arg.param.intVal);
			}break;

			case REMOTEUI_PARAM_BOOL:{
				ofParameter<bool> p = myParam->cast<bool>();
				p.set(arg.param.boolVal);
			}break;

			case REMOTEUI_PARAM_STRING:{
				ofParameter<string> p = myParam->cast<string>();
				p.set(arg.param.stringVal);
			}break;

			case REMOTEUI_PARAM_COLOR:{
				ofParameter<ofColor> p = myParam->cast<ofColor>();
				p.set(ofColor(arg.param.redVal, arg.param.blueVal, arg.param.greenVal, arg.param.alphaVal));
			}break;

			default:
				ofLogError("ofxRemoteUIofParamaterSync") << "Unknown RemoteUIParamType ?";
				break;
		}
	}
}

string ofxRemoteUIofParamaterSync::cleanParamName(string p){
	if(p[p.size() - 3] == '.'){ //most likely
		p = p.substr(0, p.size() - 3);
	}
	return p;
}

ofAbstractParameter& ofxRemoteUIofParamaterSync::findInChildren(ofParameterGroup & group,
																vector<string>& groupTree,
																const string& paramNane){
	int index = 1;
	ofParameterGroup g = group;

	while(index < groupTree.size() - 1){
		string gName = groupTree[index];
		g = (g.getGroup(gName));
		index++;
	}
	ofAbstractParameter & param = g.get(paramNane);
	return param;
}