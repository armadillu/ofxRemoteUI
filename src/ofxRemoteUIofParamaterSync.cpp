//
//  ofxRemoteUIofParamaterSync.cpp
//  oscParametersReceiver
//
//  Created by Oriol Ferrer Mesià on 2/5/15.
//
//

#include "ofxRemoteUIofParamaterSync.h"

#ifdef OF_AVAILABLE


void ofxRemoteUIofParamaterSync::setup(ofParameterGroup & _parameters){

	ofAddListener(_parameters.parameterChangedE(), this, &ofxRemoteUIofParamaterSync::parameterChanged);
	ofAddListener(RUI_GET_OF_EVENT(), this, &ofxRemoteUIofParamaterSync::onRemoteUINotification);

	RUI_NEW_GROUP(_parameters.getName());
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
		ofAbstractParameter & absP = _parameters.get(i);

		string fullGroupTreeName = getCompleteParameterPath(absP);
		string shortedGroupName = getShortVersionForGroupPath(fullGroupTreeName);
		string fullRUIparamName = shortedGroupName + absP.getName();

		if(dynamic_cast<ofParameterGroup*>(&_parameters.get(i))){
			RUI_NEW_GROUP(fullGroupTreeName);
		}

		if(type==typeid(ofParameter<int>).name()){ //INT
			ofParameter<int> p = _parameters.getInt(i);
			RUI_DEFINE_VAR_WV(int, fullRUIparamName, p, p.getMin(), p.getMax());
			ofParamRuiList.emplace_back(fullRUIparamName);

		}else if(type==typeid(ofParameter<float>).name()){ //FLOAT
			ofParameter<float> p = _parameters.getFloat(i);
			RUI_DEFINE_VAR_WV(float, fullRUIparamName, p, p.getMin(), p.getMax());
			ofParamRuiList.emplace_back(fullRUIparamName);

		}else if(type==typeid(ofParameter<bool>).name()){ //BOOL
			ofParameter<bool> p = _parameters.getBool(i);
			RUI_DEFINE_VAR_WV(bool, fullRUIparamName, p);
			ofParamRuiList.emplace_back(fullRUIparamName);

		}else if(type==typeid(ofParameter<string>).name()){ //STRING
			ofParameter<string> p = _parameters.getString(i);
			RUI_DEFINE_VAR_WV(string, fullRUIparamName, p);
			ofParamRuiList.emplace_back(fullRUIparamName);

		}else if(type==typeid(ofParameter<ofColor>).name()){ //COLOR
			ofParameter<ofColor> p = _parameters.getColor(i);
			ofColor * c = ofxRemoteUIVars<ofColor>::one().defineParam(fullRUIparamName, p);
			server->shareParam(fullRUIparamName, &(*c)[0] );
			ofParamRuiList.emplace_back(fullRUIparamName);
		#ifdef GLM_SWIZZLE
		}else if(type==typeid(ofParameter<ofDefaultVec2>).name()){ //ofVec2f
		#else
		}else if (type == typeid(ofParameter<ofVec2f>).name()) { //ofVec2f
		#endif
			auto p = _parameters.getVec2f(i);
			float * x = ofxRemoteUIVars<float>::one().defineParam(fullRUIparamName + compSEP + "2x", p->x);
			float * y = ofxRemoteUIVars<float>::one().defineParam(fullRUIparamName + compSEP + "2y", p->y);
			server->shareParam(fullRUIparamName + compSEP + "2x", x, p.getMin().x, p.getMax().x);
			server->shareParam(fullRUIparamName + compSEP + "2y", y, p.getMin().y, p.getMax().y);
			ofParamRuiList.emplace_back(fullRUIparamName + compSEP + "2x");
			ofParamRuiList.emplace_back(fullRUIparamName + compSEP + "2y");
		#ifdef GLM_SWIZZLE
		}else if (type == typeid(ofParameter<ofDefaultVec3>).name()) { //ofVec3f
		#else
		}else if (type == typeid(ofParameter<ofVec3f>).name()) { //ofVec3f
		#endif
			auto p = _parameters.getVec3f(i);
			float * x = ofxRemoteUIVars<float>::one().defineParam(fullRUIparamName + compSEP + "3x", p->x);
			float * y = ofxRemoteUIVars<float>::one().defineParam(fullRUIparamName + compSEP + "3y", p->y);
			float * z = ofxRemoteUIVars<float>::one().defineParam(fullRUIparamName + compSEP + "3z", p->z);
			server->shareParam(fullRUIparamName + compSEP + "3x", x, p.getMin().x, p.getMax().x);
			server->shareParam(fullRUIparamName + compSEP + "3y", y, p.getMin().y, p.getMax().y);
			server->shareParam(fullRUIparamName + compSEP + "3z", z, p.getMin().z, p.getMax().z);
			ofParamRuiList.emplace_back(fullRUIparamName + compSEP + "3x");
			ofParamRuiList.emplace_back(fullRUIparamName + compSEP + "3y");
			ofParamRuiList.emplace_back(fullRUIparamName + compSEP + "3z");
		#ifdef GLM_SWIZZLE
		}else if (type == typeid(ofParameter<ofDefaultVec2>).name()) { //ofVec2f
		#else
		}else if (type == typeid(ofParameter<ofVec2f>).name()) { //ofVec2f
		#endif

			auto p = _parameters.getVec4f(i);
			float * x = ofxRemoteUIVars<float>::one().defineParam(fullRUIparamName + compSEP + "4x", p->x);
			float * y = ofxRemoteUIVars<float>::one().defineParam(fullRUIparamName + compSEP + "4y", p->y);
			float * z = ofxRemoteUIVars<float>::one().defineParam(fullRUIparamName + compSEP + "4z", p->z);
			float * w = ofxRemoteUIVars<float>::one().defineParam(fullRUIparamName + compSEP + "4w", p->w);
			server->shareParam(fullRUIparamName + compSEP + "4x", x, p.getMin().x, p.getMax().x);
			server->shareParam(fullRUIparamName + compSEP + "4y", y, p.getMin().y, p.getMax().y);
			server->shareParam(fullRUIparamName + compSEP + "4z", z, p.getMin().z, p.getMax().z);
			server->shareParam(fullRUIparamName + compSEP + "4w", w, p.getMin().w, p.getMax().w);
			ofParamRuiList.emplace_back(fullRUIparamName + compSEP + "4x");
			ofParamRuiList.emplace_back(fullRUIparamName + compSEP + "4y");
			ofParamRuiList.emplace_back(fullRUIparamName + compSEP + "4z");
			ofParamRuiList.emplace_back(fullRUIparamName + compSEP + "4w");
		}else if(type==typeid(ofParameterGroup).name()){
			ofParameterGroup p = _parameters.getGroup(i);
			recursiveSetup(p);
		}else{
			ofLogError("ofxRemoteUIofParamaterSync") << "Sorry I don't support '" << type << "' type.";
		}
	}
}


string ofxRemoteUIofParamaterSync::goUpOneLevel(const string & path){

	string newPath;
	vector<string> split = ofSplitString(path, SEP);
	for(int i = 0; i < split.size() - 1; i++){
		newPath += split[i] + SEP;
	}
	//newPath = newPath.substr(0, newPath.size() -1 );
	return newPath;
}


string ofxRemoteUIofParamaterSync::getFileName(const string & path){
	string newPath;
	vector<string> split = ofSplitString(path, SEP);
	return split[split.size() -1];
}


string ofxRemoteUIofParamaterSync::getCompleteParameterPath(ofAbstractParameter & parameter){

	vector<string> paramHier = parameter.getGroupHierarchyNames();
	string fullGroupTreeName;
	for(int i = 0; i < paramHier.size(); i++){
		fullGroupTreeName += SEP + paramHier[i];
	}
	fullGroupTreeName = fullGroupTreeName.substr(1, fullGroupTreeName.size()-1);
	return fullGroupTreeName;
}


void ofxRemoteUIofParamaterSync::parameterChanged( ofAbstractParameter & parameter ){

	string fullGroupTreeName = getCompleteParameterPath(parameter);
	string pName = parameter.getName();
	//string groupTree = ofFilePath::getEnclosingDirectory(fullGroupTreeName, false);
	string groupTree = goUpOneLevel(fullGroupTreeName);

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


void ofxRemoteUIofParamaterSync::onRemoteUINotification(RemoteUIServerCallBackArg &arg){

	if (arg.action == CLIENT_UPDATED_PARAM){
		updateOfParamFromRuiParam(arg.paramName);
	}

	if (arg.action == CLIENT_DID_SET_PRESET ||
		arg.action == CLIENT_DID_RESET_TO_XML ||
		arg.action == CLIENT_DID_SET_GROUP_PRESET ||
		arg.action == CLIENT_CONNECTED) {
		forceRuiToOfParamSync();
	}
}


void ofxRemoteUIofParamaterSync::updateOfParamFromRuiParam(string ruiParamName){

	RemoteUIParam ruiP = RUI_GET_PARAM(ruiParamName);

	string paramName = getFileName(ruiParamName);
	if(paramName.size() == 0){
		ofLogError("ofxRemoteUIofParamaterSync") << "no name!";
	}
	string shortGroupPath = goUpOneLevel(ruiParamName);
	//shortGroupPath = ofFilePath::removeTrailingSlash(shortGroupPath);
	string fullGroupPath = getFullGroupPathForShortVersion(shortGroupPath);

	vector<string> split = ofSplitString(fullGroupPath, SEP); //separate group;

	ofAbstractParameter * myParam = NULL;

	if (split.size() == 1){ //not part of group - dont think its possible?
		paramName = ruiParamName;

	}else{ //part of group
		myParam = &findInChildren(*syncGroup, split, cleanParamName(paramName));
	}

	if(myParam == NULL){
		ofLogError("ofxRemoteUIofParamaterSync") << "cant get ofParameter! " << ruiParamName;
		return;
	}

	switch (ruiP.type) {

		case REMOTEUI_PARAM_FLOAT:{

			//FIXME: most likely an ofVecN suffix compSEP + "Dx"   #############################
			if(paramName[paramName.size() - 3] == compSEP[0]){
				char vecComp = paramName[paramName.size() - 1]; // x | y | z | w
				char vecType = paramName[paramName.size() - 2]; // 2 | 3 | 4
				string noCompPname = paramName.substr(0, paramName.size() - 3);

				switch (vecType) {
					case '2':{
						ofParameter<ofVec2f> p = myParam->cast<ofVec2f>();
						ofVec2f v = p.get();
						switch (vecComp) {
							case 'x': v.x = ruiP.floatVal; p.set(v); break;
							case 'y': v.y = ruiP.floatVal; p.set(v); break;
							default: break;
						}}break;

					case '3':{
						ofParameter<ofVec3f> p = myParam->cast<ofVec3f>();
						ofVec3f v = p.get();
						switch (vecComp) {
							case 'x': v.x = ruiP.floatVal; p.set(v); break;
							case 'y': v.y = ruiP.floatVal; p.set(v); break;
							case 'z': v.z = ruiP.floatVal; p.set(v); break;
							default: break;
						}}break;

					case '4':{
						ofParameter<ofVec4f> p = myParam->cast<ofVec4f>();
						ofVec4f v = p.get();
						switch (vecComp) {
							case 'x': v.x = ruiP.floatVal; p.set(v); break;
							case 'y': v.y = ruiP.floatVal; p.set(v); break;
							case 'z': v.z = ruiP.floatVal; p.set(v); break;
							case 'w': v.w = ruiP.floatVal; p.set(v); break;
							default: break;
						}}break;
				}

			}else{ // regular float - not a vector #####################################
				ofParameter<float> p = myParam->cast<float>();
				p.set(ruiP.floatVal);
			}
		}break;

		case REMOTEUI_PARAM_INT:{
			ofParameter<int> p = myParam->cast<int>();
			p.set(ruiP.intVal);
		}break;

		case REMOTEUI_PARAM_BOOL:{
			ofParameter<bool> p = myParam->cast<bool>();
			p.set(ruiP.boolVal);
		}break;

		case REMOTEUI_PARAM_STRING:{
			ofParameter<string> p = myParam->cast<string>();
			p.set(ruiP.stringVal);
		}break;

		case REMOTEUI_PARAM_COLOR:{
			ofParameter<ofColor> p = myParam->cast<ofColor>();
			p.set(ofColor(ruiP.redVal, ruiP.greenVal, ruiP.blueVal, ruiP.alphaVal));
		}break;

		default:
			ofLogError("ofxRemoteUIofParamaterSync") << "Unknown RemoteUIParamType ?";
			break;
	}
}


string ofxRemoteUIofParamaterSync::cleanParamName(string p){
	if(p[p.size() - 3] == compSEP[0]){ //most likely
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


void ofxRemoteUIofParamaterSync::forceRuiToOfParamSync(){

	for(int i = 0; i < ofParamRuiList.size(); i++ ){
		updateOfParamFromRuiParam(ofParamRuiList[i]);
	}
}

#endif
