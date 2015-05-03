//
//  ofxRemoteUIofParamaterSync.h
//  oscParametersReceiver
//
//  Created by Oriol Ferrer Mesià on 2/5/15.
//
//

#ifndef __oscParametersReceiver__ofxRemoteUIofParamaterSync__
#define __oscParametersReceiver__ofxRemoteUIofParamaterSync__

#include "ofMain.h"
#include "ofxRemoteUIServer.h"
#include "ofxRemoteUIVars.h"



class ofxRemoteUIofParamaterSync{

public:
	
	ofxRemoteUIofParamaterSync();
	void setup(ofParameterGroup & group);
	
protected:

	ofParameterGroup * syncGroup;
	

	void remoteUIClientDidSomething(RemoteUIServerCallBackArg & arg);
	void parameterChanged( ofAbstractParameter & parameter );

	void recursiveSetup(ofParameterGroup & group);

	const string SEP = "/";

};

#endif /* defined(__oscParametersReceiver__ofxRemoteUIofParamaterSync__) */
