//
//  OscQueryServerMgr.h
//  BasicSketch
//
//  Created by Oriol Ferrer Mesià on 10/02/2018.
//
//

#pragma once
#include "ofMain.h"
#include "ofxRemoteUIServer.h"

class OscQueryServerMgr{

public:
	
	OscQueryServerMgr();
	ofJson buildJSON();

	void addParam(const string & paramName, const RemoteUIParam & p, ofJson & json);
protected:


};

