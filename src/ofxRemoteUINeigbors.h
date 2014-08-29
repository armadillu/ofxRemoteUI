//
//  ofxRemoteUINeigbors.h
//  emptyExample
//
//  Created by Oriol Ferrer Mesià on 31/10/13.
//
//

#ifndef __emptyExample__ofxRemoteUINeigbors__
#define __emptyExample__ofxRemoteUINeigbors__


#include "ofxRemoteUI.h"

#include "ofxOsc.h"
#if __cplusplus>=200103L
#include <unordered_map>
#include <unordered_map>
#else
#include <tr1/unordered_map>
#include <tr1/unordered_map>
using std::tr1::unordered_map;
#endif

struct Neighbor{
	Neighbor(){};
	Neighbor(string ip, int p, float t, string n, string bin){IP = ip; port = p; timeLastSeen = t; name = n; binary = bin;}
	string IP,name, binary;
	int port;
	float timeLastSeen;
};

class ofxRemoteUINeigbors{

public:

	ofxRemoteUINeigbors();
	bool update(float dt); //return false if no update, true if theres an update
	bool gotPing(string ip, int port, string name, string binaryName); //idem
	void print();
	vector<Neighbor> getNeighbors();

private:

	unordered_map<string,Neighbor>neigbhors; //this will be kept updated, key is a combostring of "IP:PORT"
	float time; //secs

};

#endif /* defined(__emptyExample__ofxRemoteUINeigbors__) */
