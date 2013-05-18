//
//  ofxRemoteUI.cpp
//  emptyExample
//
//  Created by Oriol Ferrer Mesià on 09/01/13.
//
//

#include "ofxRemoteUIClient.h"
#include <iostream>

ofxRemoteUIClient::ofxRemoteUIClient(){
	readyToSend = false;
	timeSinceLastReply = 0;
	avgTimeSinceLastReply = 0;
	waitingForReply = false;
	gotNewInfo = false;
}


void ofxRemoteUIClient::setup(string address, int port_){

	params.clear();
	orderedKeys.clear();
	port = port_;
	avgTimeSinceLastReply = timeSinceLastReply = time = 0.0f;
	waitingForReply = false;
	host = address;
	cout << "ofxRemoteUIClient listening at port " << port + 1 << " ... " << endl;
	receiver.setup(port + 1);

	cout << "ofxRemoteUIClient connecting to " << address << endl;
	sender.setup(address, port);
}


void ofxRemoteUIClient::update(float dt){

	if (!readyToSend){ // if not connected, connect

		cout << "ofxRemoteUIClient: sending HELLO!" << endl;
		sendHELLO();	//on first connect, send HI!
		sendTEST();		//and a lag test
		readyToSend = true;
	}else{

		time += dt;
		timeSinceLastReply += dt;
		//printf("waiting for reply: %d\n", waitingForReply);
		if (time > LATENCY_TEST_RATE){
			if (!waitingForReply){
				time = 0.0f;
				sendTEST();
			}else{
				if (time > CONNECTION_TIMEOUT){
					avgTimeSinceLastReply = -1;
				}
			}
		}
	}

	while( receiver.hasWaitingMessages() ){// check for waiting messages from client

		ofxOscMessage m;
		receiver.getNextMessage(&m);

		DecodedMessage dm = decode(m);

		switch (dm.action) {

			case HELO_ACTION: //server says hi back, we ask for a big update
				//cout << "ofxRemoteUIClient: " << m.getRemoteIp() << " answered HELLO!" << endl;
				requestCompleteUpdate();
				break;

			case REQUEST_ACTION: //should not happen, server doesnt request
				//cout << "ofxRemoteUIClient: " << m.getRemoteIp() << " send REQU??? WTF!!!"  << endl;
				break;

			case SEND_ACTION:{ //server is sending us an updated val
					//cout << "ofxRemoteUIClient: " << m.getRemoteIp() << " sends us a param update SEND!"  << endl;
					updateParamFromDecodedMessage(m, dm);
					gotNewInfo = true;
				}
				break;

			case CIAO_ACTION:
				cout << "ofxRemoteUIClient: " << m.getRemoteIp() << " says CIAO!" << endl;
				sendCIAO();
				params.clear();
				orderedKeys.clear();
				readyToSend = false;
				break;

			case TEST_ACTION: // we got a reply from the server, lets measure how long it took;
				waitingForReply = false;
				if (avgTimeSinceLastReply > 0.0f){
					avgTimeSinceLastReply = 0.8 * (avgTimeSinceLastReply) + 0.2 * (timeSinceLastReply);
				}else{
					avgTimeSinceLastReply = timeSinceLastReply ;
				}
				timeSinceLastReply = 0.0f;
				//cout << "ofxRemoteUIClient: " << m.getRemoteIp() << " replied TEST!" << " and took " << avgTimeSinceLastReply << endl;
				break;

			default: cout << "ofxRemoteUIClient::update >> ERR!" <<endl; break;
		}
	}
}


void ofxRemoteUIClient::trackParam(string paramName, float* param){
	RemoteUIParam p;
	map<string,RemoteUIParam>::iterator it = params.find(paramName);
	if ( it == params.end() ){	//not found! we add it
		p.type = REMOTEUI_PARAM_FLOAT;
	}else{
		p = params[paramName];
		if (p.type != REMOTEUI_PARAM_FLOAT ){
			cout << "wtf called trackParam(float) on a param that's not a float!" << endl;
		}
	}
	p.floatValAddr = param;
	
	//params[paramName] = p;
	addParamToDB(p, paramName);
}


void ofxRemoteUIClient::trackParam(string paramName, int* param){
	RemoteUIParam p;
	map<string,RemoteUIParam>::iterator it = params.find(paramName);
	if ( it == params.end() ){	//not found! we add it
		p.type = REMOTEUI_PARAM_INT;
	}else{
		p = params[paramName];
		if (p.type != REMOTEUI_PARAM_INT ){
			cout << "wtf called trackParam(int) on a param that's not a int!" << endl;
		}
	}
	p.intValAddr = param;
	//params[paramName] = p;
	addParamToDB(p, paramName);
}


void ofxRemoteUIClient::trackParam(string paramName, string* param){
	RemoteUIParam p;
	map<string,RemoteUIParam>::iterator it = params.find(paramName);
	if ( it == params.end() ){	//not found! we add it
		p.type = REMOTEUI_PARAM_STRING;
	}else{
		p = params[paramName];
		if (p.type != REMOTEUI_PARAM_STRING ){
			cout << "wtf called trackParam(string) on a param that's not a string!" << endl;
		}
	}
	p.stringValAddr = param;
	//params[paramName] = p;
	addParamToDB(p, paramName);
}


void ofxRemoteUIClient::trackParam(string paramName, bool* param){
	RemoteUIParam p;
	map<string,RemoteUIParam>::iterator it = params.find(paramName);
	if ( it == params.end() ){	//not found! we add it
		p.type = REMOTEUI_PARAM_BOOL;
	}else{
		p = params[paramName];
		if (p.type != REMOTEUI_PARAM_BOOL ){
			cout << "wtf called trackParam(bool) on a param that's not a bool!" << endl;
		}
	}
	p.boolValAddr = param;
	//params[paramName] = p;
	addParamToDB(p, paramName);
}


void ofxRemoteUIClient::sendParamUpdate(RemoteUIParam p, string paramName){

	//p.print();
	params[paramName] = p; //TODO error check!
	vector<string>list;
	list.push_back(paramName);
	sendUpdateForParamsInList(list);
}


void ofxRemoteUIClient::requestCompleteUpdate(){
	//cout << "ofxRemoteUIClient: requestCompleteUpdate()" << endl;
	if(readyToSend){
		sendREQUEST();
	}
}

bool ofxRemoteUIClient::hasReceivedUpdate(){

	if (gotNewInfo){
		return true;
		gotNewInfo = false;
	}
	return false;
}


void ofxRemoteUIClient::sendUpdatedParam(string paramName){

	map<string,RemoteUIParam>::iterator it = params.find(paramName);
	if ( it != params.end() ){
		syncParamToPointer(paramName);
		sendParam(paramName, params[paramName]);
	}else{
		cout << "ofxRemoteUIClient::sendUpdatedParam >> param '" + paramName + "' not found!" << endl;
	}
}


bool ofxRemoteUIClient::isReadyToSend(){
	return readyToSend;
}


void ofxRemoteUIClient::sendREQUEST(){
	ofxOscMessage m;
	m.setAddress("REQU");
	sender.sendMessage(m);
}

