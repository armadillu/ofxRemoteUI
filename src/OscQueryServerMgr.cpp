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
#include <Poco/Pipe.h>

using namespace std;

OscQueryServerMgr::OscQueryServerMgr(){}


void OscQueryServerMgr::setup() {
	startThread(); //starts Bonjour adv
}

OscQueryServerMgr::~OscQueryServerMgr() {
	if (server){
		try{
			stopBonjour();
			server->stop();
		}catch(std::exception e){
			ofLogError("OscQueryServerMgr") << "Exception trying to stop server and bonjour! - " << e.what();
		}
		waitForThread();
		ofSleepMillis(1);
	}
}


void OscQueryServerMgr::RUIRequestHandler::handleRequest(Poco::Net::HTTPServerRequest &req, Poco::Net::HTTPServerResponse &resp) {
	//resp.set("Content-Encoding", "gzip");
	ofLogNotice("ofxRemoteUI::OscQueryServerMgr") << "got web request!";

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

	const auto & keys = s->orderedKeys;
	const auto & params = s->params;

	std::map<string, vector<string>> paramsByGroup;

	string currentParam = "unGrouped";
	for(auto & it : keys){
		const std::string & paramName = it.second;
		const RemoteUIParam & param = params.at(paramName);
		if(param.type == REMOTEUI_PARAM_SPACER){
			currentParam = paramName;
			paramsByGroup[currentParam] = std::vector<std::string>();
		}
		if(param.type == REMOTEUI_PARAM_FLOAT || param.type == REMOTEUI_PARAM_INT ||
		   param.type == REMOTEUI_PARAM_ENUM || param.type == REMOTEUI_PARAM_BOOL ||
		   param.type == REMOTEUI_PARAM_COLOR || param.type == REMOTEUI_PARAM_STRING){
			paramsByGroup[currentParam].emplace_back(paramName);
		}
	}

	for(auto & it : paramsByGroup){
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

	#ifdef TARGET_WIN32
	#elif defined(TARGET_LINUX)
	pthread_setname_np(pthread_self(), "ofxRemoteUI::OscQueryServerMgr");
	#else
	pthread_setname_np("ofxRemoteUI::OscQueryServerMgr");
	#endif


	auto micros = ofGetSystemTimeMicros();
	webPort = OSC_QUERY_SERVER_PORT_RANGE_LO + micros%(OSC_QUERY_SERVER_PORT_RANGE_HI - OSC_QUERY_SERVER_PORT_RANGE_LO);
	server = make_shared<Poco::Net::HTTPServer>(new OscQueryServerMgr::RUIRequestHandlerFactory,
												Poco::Net::ServerSocket(webPort),
												new Poco::Net::HTTPServerParams);

	ofLogNotice("ofxRemoteUI") << "Starting OSC Query Server at port " << webPort << ".";
	server->start();

	ofxRemoteUIServer * s = RUI_GET_INSTANCE();
	string appName = s->getBinaryName();
	string computerName = s->getComputerName();

	//first off, kill any other old "dns-sd" process instances that might remain from previous unclean exits.
	//this is necessary (and dodgy) bc the way this works, it spawns a "dns-sd" process to handle the
	//bonjour advertising. This is a secondary process; if for some reason this app crashes or is killed,
	//this object's destructor is never reached and the process stays alive, owned by "init" now.
	string running = ofSystem("ps -axe -o pid -o args | grep dns-sd | grep -v grep | grep " + appName);
	//try filter as much as possible when finding candidates to kill - only kill dns-sd proceess that advertises a binary with our name
	//last awk command is to remove leading spaces and tabs - https://unix.stackexchange.com/questions/102008/how-do-i-trim-leading-and-trailing-whitespace-from-each-line-of-some-output
	if(running.size()){
		auto split = ofSplitString(running, "\n");
		for(auto &line : split){
			if(line.size() > 1){
				while (line[0] == ' ') line = line.substr(1, line.size() - 1 ); //remove leading spaces
				auto lineSplit = ofSplitString(line, " ");
				if(lineSplit.size() > 1){
					int pid = ofToInt(lineSplit[0]); //get PID of dns-sd process that most likely was spawned by us
					ofLogWarning("ofxRemoteUI::OscQueryServerMgr") << "killing \"dns-sd\" process with pid " << pid << " to avoid duplicate advertising - as its most likely ours.";
					ofSystem("kill " + ofToString(pid)); //this is quite rude, hopefully the user wasn't running "dns-sd" for anything else but this...
				}
			}
		}
	}

	//string IP = s->getComputerIP();
	string port = ofToString(s->port);
	string serverName = "ofxRemoteUI: " +appName + "@" + computerName + ":" + port;
	//string command = "dns-sd -P ofxRemoteUI _oscjson._tcp. local 3333 armadillu.local 192.168.5.30"
	//string command = "dns-sd -R ofxRemoteUI _oscjson._tcp. local 3333"
	//string command = "dns-sd -P ofxRemoteUI _oscjson._tcp. local " + ofToString(OSC_QUERY_SERVER_PORT) + " ofxRemoteUI.local " + IP;

	Poco::Pipe outPipe;
	Poco::Pipe inPipe;
	Poco::Process::Args args = { "-R", serverName, "_oscjson._tcp.",  "local", ofToString(webPort)};
	try{
		Poco::ProcessHandle ph = Poco::Process::launch("dns-sd", args, &inPipe, &outPipe, &outPipe);
		phPtr = &ph;
		try {
			int statusCode = ph.wait();
		} catch (exception e) {
			ofLogError("ofxRemoteUI::OscQueryServerMgr") << "Exception while process running \"dns-sd\"";
			ofLogError("ofxRemoteUI::OscQueryServerMgr") << e.what();
		}

	}catch(const Poco::Exception& exc){
		ofLogFatalError("ofxRemoteUI::OscQueryServerMgr") << "Exception at launch process \"dns-sd\"! : " <<  exc.displayText();
		phPtr = nullptr;
	}
	//ofLogWarning("ofxRemoteUI::OscQueryServerMgr") << "exiting Bonjour thread";
}

void OscQueryServerMgr::stopBonjour(){

	if (phPtr != nullptr && Poco::Process::isRunning(*phPtr)) {
		//ofLogWarning("ofxRemoteUI::OscQueryServerMgr") << "Trying to stop Bonjour advertising!";
		try {
			Poco::Process::kill(*phPtr);
		} catch (exception e) {
			ofLogError("ofxRemoteUI::OscQueryServerMgr") << e.what();
		}
		//ofLogWarning("ofxRemoteUI::OscQueryServerMgr") << "Done killing process!";
	}
}
