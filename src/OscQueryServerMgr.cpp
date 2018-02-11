//
//  OscQueryServerMgr.cpp
//  BasicSketch
//
//  Created by Oriol Ferrer Mesià on 10/02/2018.
//
//

#include "OscQueryServerMgr.h"

OscQueryServerMgr::OscQueryServerMgr(){

}


ofJson OscQueryServerMgr::buildJSON(){

	ofxRemoteUIServer * s = RUI_GET_INSTANCE();
	string IP = s->getComputerIP();

	//string bonjourName = ofSystem("hostname");
	//ofStringReplace(bonjourName, "\n", "");

	//string command = "dns-sd -P ofxRemoteUI _oscjson._tcp. local 3333 armadillu.local 192.168.5.30"
	int port = 3333;
	string command = "dns-sd -P ofxRemoteUI _oscjson._tcp. local " + ofToString(port) + " ofxRemoteUI.local " + IP + " &";
	ofLogNotice() << command;
	string ret = ofSystem(command);
	ofLogNotice() << ret;

	ofJson root;
	string currentGroup = "myGroup";

	root["ACCESS"] = 0;
	root["CONTENTS"] = ofJson::object();
	root["DESCRIPTION"] = "root node";
	root["FULL_PATH"] = "\\/";
	root["CONTENTS"][currentGroup] = ofJson::object();
	root["CONTENTS"][currentGroup]["ACCESS"] = 0;
	root["CONTENTS"][currentGroup]["DESCRIPTION"] = "My test group";
	root["CONTENTS"][currentGroup]["FULL_PATH"] = "\\/" + currentGroup;
	root["CONTENTS"][currentGroup]["CONTENTS"] = ofJson::object();

	ofJson & group = root["CONTENTS"][currentGroup]["CONTENTS"];


	const std::map<int, std::string> & keys = s->orderedKeys;
	const std::unordered_map<std::string, RemoteUIParam> & params = s->params;

	for(auto it : keys){
		ofLogNotice() << "key: " << it.second;
		const string & paramName = it.second;
		const RemoteUIParam & param = params.at(paramName);
		if(param.type == REMOTEUI_PARAM_FLOAT ){
			addParam(paramName, param, group[paramName]);
		}

	}

	std::cout << root.dump(4) << std::endl;

	return root;
}


void OscQueryServerMgr::addParam(const string & paramName, const RemoteUIParam & p, ofJson & json){

	json["ACCESS"] = 3;
	//json["CLIPMODE"] = ofJson::object();
	json["CONTENTS"] = ofJson::object();
	json["DESCRIPTION"] = paramName + " parameter";
	json["FULL_PATH"] = "\\/SEND FLT " + paramName;
	//json["RANGE"] = ofJson::array();
	json["RANGE"][0]["MAX"] = p.maxFloat;
	json["RANGE"][0]["MIN"] = p.minFloat;
	json["TAGS"] = {"float input"};
	json["TYPE"] = "f";
	json["UNIT"][0] = "percent";

}
