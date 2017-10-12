//
//  ofxRemoteUIWebServer.h
//  projector-vis
//
//  Created by Jack O'Shea on 8/3/17.
//
//

#ifndef ofxRemoteUIWebServer_h
#define ofxRemoteUIWebServer_h

#ifdef RUI_WEB_INTERFACE

#include <ofMain.h>
#include <Poco/Net/ServerSocket.h>
#include <Poco/Net/HTTPServer.h>
#include <Poco/Net/HTTPRequestHandler.h>
#include <Poco/Net/HTTPRequestHandlerFactory.h>
#include <Poco/Net/HTTPResponse.h>
#include <Poco/Net/HTTPServerRequest.h>
#include <Poco/Net/HTTPServerResponse.h>
#include <Poco/Util/ServerApplication.h>


class ofxRemoteUIWebServer : Poco::Util::ServerApplication {
private:
    class RUIRequestHandler : public Poco::Net::HTTPRequestHandler {
        virtual void handleRequest(Poco::Net::HTTPServerRequest &req,
                                   Poco::Net::HTTPServerResponse &resp);
    };
    class RUIRequestHandlerFactory : public Poco::Net::HTTPRequestHandlerFactory {
        virtual Poco::Net::HTTPRequestHandler* createRequestHandler(const Poco::Net::HTTPServerRequest &) {  return new RUIRequestHandler; }
    };
    
    shared_ptr<Poco::Net::HTTPServer> server;
    
    enum State {
        kNotSetup,
        kSetup,
        kStarted,
        kStopped
    };
    
    State state = kNotSetup;
    
public:
    void setup(int port);
    void start();
    void stop();
    
    ~ofxRemoteUIWebServer();
};


#endif

#endif /* ofxRemoteUIWebServer_h */
