//
//  ofxRemoteUIWebServer.cpp
//  projector-vis
//
//  Created by Jack O'Shea on 8/3/17.
//
//

#ifdef RUI_WEB_INTERFACE

#include "ofxRemoteUIWebServer.h"


void ofxRemoteUIWebServer::setup(int port) {
    server = make_shared<Poco::Net::HTTPServer>(new ofxRemoteUIWebServer::RUIRequestHandlerFactory,
                            Poco::Net::ServerSocket(port),
                            new Poco::Net::HTTPServerParams);
    state = kSetup;
}

void ofxRemoteUIWebServer::start() {
    switch (state) {
        case kNotSetup:
            ofLogError("PocoServer") << "Trying to start before setup";
            break;
        case kSetup:
            server->start();
            ofLogNotice("PocoServer") << "Started server on " << server->port();
            break;
        case kStarted:
            ofLogError("PocoServer") << "Server already started";
            break;
        case kStopped:
            server->start();
            break;
    }
}

void ofxRemoteUIWebServer::stop() {
    switch (state) {
        case kNotSetup:
        case kSetup:
            ofLogError("PocoServer") << "Server not started yet";
            break;
        case kStarted:
            server->stop();
            break;
        default: break;
    }
}

ofxRemoteUIWebServer::~ofxRemoteUIWebServer() {
    if (state == kStarted){
        server->stop();
    }
}


extern unsigned RUI_WEB_BINARY_SIZE;
extern unsigned char RUI_WEB_BINARY_CONTENT[];

void ofxRemoteUIWebServer::RUIRequestHandler::handleRequest(Poco::Net::HTTPServerRequest &req, Poco::Net::HTTPServerResponse &resp) {
    resp.set("Content-Encoding", "gzip");
    resp.sendBuffer(RUI_WEB_BINARY_CONTENT, RUI_WEB_BINARY_SIZE);
}

#endif
