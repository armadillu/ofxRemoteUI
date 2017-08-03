//
//  SimplePocoServer.cpp
//  projector-vis
//
//  Created by Jack O'Shea on 8/3/17.
//
//

#include "SimplePocoServer.h"


void SimplePocoServer::setup(int port) {
    server = make_shared<Poco::Net::HTTPServer>(new SimplePocoServer::RUIRequestHandlerFactory,
                            Poco::Net::ServerSocket(port),
                            new Poco::Net::HTTPServerParams);
    state = kSetup;
}

void SimplePocoServer::start() {
    switch (state) {
        case kNotSetup:
            ofLogError("PocoServer") << "Trying to start before setup";
            break;
        case kSetup:
            server->start();
            break;
        case kStarted:
            ofLogError("PocoServer") << "Server already started";
            break;
        case kStopped:
            server->start();
            break;
    }
}

void SimplePocoServer::stop() {
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

SimplePocoServer::~SimplePocoServer() {
    if (state == kStarted){
        server->stop();
    }
}


extern unsigned POCO_CONTENT_SIZE;
extern unsigned char GEN_POCO_CONTENT[];

void SimplePocoServer::RUIRequestHandler::handleRequest(Poco::Net::HTTPServerRequest &req, Poco::Net::HTTPServerResponse &resp) {
    resp.set("Content-Encoding", "gzip");
    resp.sendBuffer(GEN_POCO_CONTENT, POCO_CONTENT_SIZE);
}




