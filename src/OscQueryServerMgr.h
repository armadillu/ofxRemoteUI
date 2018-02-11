//
//  OscQueryServerMgr.h
//  BasicSketch
//
//  Created by Oriol Ferrer Mesià on 10/02/2018.
//
//

#pragma once

#include "ofMain.h"
#include <Poco/Util/ServerApplication.h>
#include <Poco/Net/HTTPServer.h>
#include <Poco/Net/HTTPRequestHandler.h>
#include "ofxRemoteUI.h"
#include "RemoteParam.h"
#include <Poco/Process.h>

#define OSC_QUERY_SERVER_PORT	24689

class OscQueryServerMgr : Poco::Util::ServerApplication , public ofThread{

public:
	
	OscQueryServerMgr();
	~OscQueryServerMgr();

	void setup();
	void start();
	void stop();

protected:

	static ofJson buildJSON();

	static void addFloatParam(const string & paramName, const RemoteUIParam & p, ofJson & json);
	static void addIntParam(const string & paramName, const RemoteUIParam & p, ofJson & json);
	static void addEnumParam(const string & paramName, const RemoteUIParam & p, ofJson & json);
	static void addBoolParam(const string & paramName, const RemoteUIParam & p, ofJson & json);
	static void addColorParam(const string & paramName, const RemoteUIParam & p, ofJson & json);

	static void addGroup(const string & gName, ofJson & json);

	class RUIRequestHandler : public Poco::Net::HTTPRequestHandler {
		virtual void handleRequest(Poco::Net::HTTPServerRequest &req,
								   Poco::Net::HTTPServerResponse &resp);
	};
	class RUIRequestHandlerFactory : public Poco::Net::HTTPRequestHandlerFactory {
		virtual Poco::Net::HTTPRequestHandler* createRequestHandler(const Poco::Net::HTTPServerRequest &) {  return new RUIRequestHandler; }
	};

	shared_ptr<Poco::Net::HTTPServer> server = nullptr;


	void threadedFunction();
	void stopBonjour();

	Poco::ProcessHandle * phPtr = nullptr;
};

