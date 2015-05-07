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

	void forceRuiToOfParamSync();

protected:

	ofParameterGroup * syncGroup;

	void remoteUIClientDidSomething(RemoteUIServerCallBackArg & arg);
	void parameterChanged( ofAbstractParameter & parameter );

	void recursiveSetup(ofParameterGroup & group);

	const string SEP = "."; //to separate ofParameter group hierarcy
	const string compSEP = "_"; //for ofVec _2x, _2y, etc


	ofAbstractParameter& findInChildren(ofParameterGroup & group,
										vector<string>& groupPathName,
										const string& paramNane);

	map<string, string> groupClusterID; //full param group path to shortVersion.
	map<string, int> uniqueWords;
	vector<string>	ofParamRuiList;

	void updateOfParamFromRuiParam(string ruiParamName);
	string getShortVersionForGroupPath(const string & groupPath);
	string getFullGroupPathForShortVersion(const string & shortV);

	string cleanParamName(string p);

	string goUpOneLevel(const string & path);
	string getFileName(const string & path);
};

#endif /* defined(__oscParametersReceiver__ofxRemoteUIofParamaterSync__) */
