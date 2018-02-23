//
//  OscQueryServerMgr.cpp
//  BasicSketch
//
//  Created by Oriol Ferrer Mesià on 10/02/2018.
//
//

#include "OscQueryServerMgr.h"
#include <Poco/Net/HTTPServerResponse.h>
#include "ofxRemoteUIServer.h"

using namespace std;

OscQueryServerMgr::OscQueryServerMgr(){}


void OscQueryServerMgr::setup() {
	server = make_shared<Poco::Net::HTTPServer>(new OscQueryServerMgr::RUIRequestHandlerFactory,
												Poco::Net::ServerSocket(OSC_QUERY_SERVER_PORT),
												new Poco::Net::HTTPServerParams);
	server->start();
	startThread(); //starts Bonjour adv
}

OscQueryServerMgr::~OscQueryServerMgr() {
	if (server){
		server->stop();
		stopBonjour();
		waitForThread();
	}
}


void OscQueryServerMgr::RUIRequestHandler::handleRequest(Poco::Net::HTTPServerRequest &req, Poco::Net::HTTPServerResponse &resp) {
	//resp.set("Content-Encoding", "gzip");
	ofLogNotice("OscQueryServerMgr") << "got web request!";

	ofJson json = buildJSON();

	string jsonData = json.dump(4);

	resp.set("Content-Type", "application/json; charset=utf-8");
	resp.set("Content-Length", ofToString(jsonData.size()));
	resp.sendBuffer(&jsonData[0], jsonData.size());
}


ofJson OscQueryServerMgr::buildJSON(){

	ofxRemoteUIServer * s = RUI_GET_INSTANCE();
	((ofxRemoteUI*)s)->getDataMutex().lock();

	ofJson root;

	root["ACCESS"] = 0;
	root["CONTENTS"] = ofJson::object();
	root["DESCRIPTION"] = "root node";
	root["FULL_PATH"] = "/";

	const std::map<int, std::string> & keys = s->orderedKeys;
	const std::unordered_map<std::string, RemoteUIParam> & params = s->params;

	std::map<string, vector<string>> paramsByGroup;

	string currentParam = "unGrouped";
	for(auto it : keys){
		const std::string & paramName = it.second;
		const RemoteUIParam & param = params.at(paramName);
		if(param.type == REMOTEUI_PARAM_SPACER){
			currentParam = paramName;
			paramsByGroup[currentParam] = std::vector<std::string>();
		}
		if(param.type == REMOTEUI_PARAM_FLOAT || param.type == REMOTEUI_PARAM_INT ||
		   param.type == REMOTEUI_PARAM_ENUM || param.type == REMOTEUI_PARAM_BOOL ||
		   param.type == REMOTEUI_PARAM_COLOR || param.type == REMOTEUI_PARAM_STRING){
			paramsByGroup[currentParam].push_back(paramName);
		}
	}

	for(auto it : paramsByGroup){
		string groupName = it.first;
		vector<string> & paramsNames = it.second;

		root["CONTENTS"][groupName] = ofJson::object();
		root["CONTENTS"][groupName]["ACCESS"] = 0;
		root["CONTENTS"][groupName]["DESCRIPTION"] = "ofxRemoteUI Group " + groupName;
		root["CONTENTS"][groupName]["FULL_PATH"] = groupName;
		root["CONTENTS"][groupName]["CONTENTS"] = ofJson::object();
		ofJson & group = root["CONTENTS"][groupName]["CONTENTS"];

		for(string & paramName : paramsNames){
			const RemoteUIParam & param = params.at(paramName);
			if(param.type == REMOTEUI_PARAM_FLOAT) addFloatParam(paramName, param, group[paramName]);
			if(param.type == REMOTEUI_PARAM_INT) addIntParam(paramName, param, group[paramName]);
			if(param.type == REMOTEUI_PARAM_ENUM) addEnumParam(paramName, param, group[paramName]);
			if(param.type == REMOTEUI_PARAM_BOOL) addBoolParam(paramName, param, group[paramName]);
			if(param.type == REMOTEUI_PARAM_COLOR) addColorParam(paramName, param, group[paramName]);
			if(param.type == REMOTEUI_PARAM_STRING) addStringParam(paramName, param, group[paramName]);
		}
	}

	((ofxRemoteUI*)s)->getDataMutex().unlock();
	return root;
}


void OscQueryServerMgr::addFloatParam(const string & paramName, const RemoteUIParam & p, ofJson & json){
	json["ACCESS"] = 3;
	json["CLIPMODE"] = {"none"};
	json["CONTENTS"] = ofJson::object();
	json["DESCRIPTION"] = paramName + " float parameter";
	json["FULL_PATH"] = "/SEND/FLT/" + paramName;
	json["RANGE"][0]["MAX"] = p.maxFloat;
	json["RANGE"][0]["MIN"] = p.minFloat;
	json["TAGS"] = {"float input"};
	json["TYPE"] = "f";
	json["UNIT"][0] = "gain.linear";
}


void OscQueryServerMgr::addIntParam(const string & paramName, const RemoteUIParam & p, ofJson & json){
	json["ACCESS"] = 3;
	json["CLIPMODE"] = {"none"};
	json["CONTENTS"] = ofJson::object();
	json["FULL_PATH"] = "/SEND/INT/" + paramName;
	json["RANGE"][0]["MAX"] = p.maxInt;
	json["RANGE"][0]["MIN"] = p.minInt;
	json["TAGS"] = {"integer input"};
	json["TYPE"] = "i";
	json["DESCRIPTION"] = paramName + " integer parameter";
}

void OscQueryServerMgr::addEnumParam(const string & paramName, const RemoteUIParam & p, ofJson & json){
	json["ACCESS"] = 3;
	json["CONTENTS"] = ofJson::object();
	json["CLIPMODE"] = {"both"};
	json["DESCRIPTION"] = paramName + " enum parameter";
	json["FULL_PATH"] = "/SEND/ENU/" + paramName;
	json["RANGE"][0]["MAX"] = p.maxInt;
	json["RANGE"][0]["MIN"] = p.minInt;
	json["VALUE"][0] = p.intVal;
	json["TAGS"] = {"int input"};
	json["TYPE"] = "i";
}


void OscQueryServerMgr::addBoolParam(const string & paramName, const RemoteUIParam & p, ofJson & json){
	json["ACCESS"] = 3;
	json["CLIPMODE"] = {"none"};
	json["CONTENTS"] = ofJson::object();
	json["DESCRIPTION"] = paramName + " boolean parameter";
	json["FULL_PATH"] = "/SEND/BOL/" + paramName;
	json["RANGE"][0]["MAX"] = 1;
	json["RANGE"][0]["MIN"] = 0;
	json["TAGS"] = {"bool input"};
	json["TYPE"] = "i";
}

void OscQueryServerMgr::addColorParam(const string & paramName, const RemoteUIParam & p, ofJson & json){
	json["ACCESS"] = 3;
	json["CLIPMODE"] = {"none"};
	json["CONTENTS"] = ofJson::object();
	json["DESCRIPTION"] = paramName + " ofColor parameter";
	json["FULL_PATH"] = "/SEND/COL/" + paramName;
	json["RANGE"][0]["MAX"] = 255;
	json["RANGE"][0]["MIN"] = 0;
	json["TAGS"] = {"ofColor input"};
	json["TYPE"] = "r";
	//json["UNIT"][0] = "color.rgba8";
}

void OscQueryServerMgr::addStringParam(const string & paramName, const RemoteUIParam & p, ofJson & json){
	json["ACCESS"] = 3;
	json["CONTENTS"] = ofJson::object();
	json["DESCRIPTION"] = paramName + " string parameter";
	json["FULL_PATH"] = "/SEND/STR/" + paramName + " <\"newValue\">";
	json["TAGS"] = {"ofColor input"};
	json["TYPE"] = "s";
}


void OscQueryServerMgr::threadedFunction(){

	//first off, kill any other old "dns-sd" process instances.
	//this is necessary (and dodgy) bc the way this works, it spawns a "dns-sd" process to handle the
	//bonjour advertising. This is a secondary process; if for some reason this app crashes or is killed,
	//this object's destructor is never reached and the process stays alive, owned by "init" now.
	ofSystem("killall dns-sd"); //this is quite rude, hopefully the user wasn't running "dns-sd" for anything else but this...

	ofxRemoteUIServer * s = RUI_GET_INSTANCE();
	//string IP = s->getComputerIP();
	string port = ofToString(s->port);
	string serverName = "ofxRemoteUI: " + s->getBinaryName() + "@" + s->getComputerName() + ":" + port;
	//string command = "dns-sd -P ofxRemoteUI _oscjson._tcp. local 3333 armadillu.local 192.168.5.30"
	//string command = "dns-sd -R ofxRemoteUI _oscjson._tcp. local 3333"
	//string command = "dns-sd -P ofxRemoteUI _oscjson._tcp. local " + ofToString(OSC_QUERY_SERVER_PORT) + " ofxRemoteUI.local " + IP;

	Poco::Process::Args args = { "-R", serverName, "_oscjson._tcp.",  "local", ofToString(OSC_QUERY_SERVER_PORT)};
	try{
		Poco::ProcessHandle ph = Poco::Process::launch("dns-sd", args);
		phPtr = &ph;

		try {
			int statusCode = ph.wait();
		} catch (exception e) {
			ofLogError("OscQueryServerMgr") << "exception while process running dns-sd";
			ofLogError("OscQueryServerMgr") << e.what();
		}

	}catch(const Poco::Exception& exc){
		ofLogFatalError("OscQueryServerMgr::exception") << exc.displayText();
		phPtr = nullptr;
	}
	ofLogWarning("OscQueryServerMgr") << "exiting Bonjour thread";

}

void OscQueryServerMgr::stopBonjour(){

	if (phPtr != nullptr && Poco::Process::isRunning(*phPtr)) {
		ofLogWarning("OscQueryServerMgr") << "Trying to stop Bonjour advertising!";
		try {
			Poco::Process::kill(*phPtr);
		} catch (exception e) {
			ofLogError("OscQueryServerMgr") << e.what();
		}
		ofLogWarning("OscQueryServerMgr") << "Done killing process!";
	}
}
