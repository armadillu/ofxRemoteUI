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



struct Neighbor{
	Neighbor(){};
	Neighbor(std::string ip, int p, float t, std::string n, std::string bin){IP = ip; port = p; timeLastSeen = t; name = n; binary = bin;}
	std::string IP, name, binary;
	int port;
	float timeLastSeen;
};

class ofxRemoteUINeigbors{

public:

	ofxRemoteUINeigbors();
	bool update(float dt); //return false if no update, true if theres an update
	bool gotPing(std::string ip, int port, std::string name, std::string binaryName); //idem
	void print();
	std::vector<Neighbor> getNeighbors();

private:

	std::unordered_map<std::string,Neighbor>neigbhors; //this will be kept updated, key is a combostring of "IP:PORT"
	float time; //secs

};

#endif /* defined(__emptyExample__ofxRemoteUINeigbors__) */
