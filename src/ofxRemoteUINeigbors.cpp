//
//  ofxRemoteUINeigbors.cpp
//  emptyExample
//
//  Created by Oriol Ferrer Mesià on 31/10/13.
//
//

#include "ofxRemoteUINeigbors.h"
#include <sstream>
#include <iostream>

ofxRemoteUINeigbors::ofxRemoteUINeigbors(){
	time = 0;
}


bool ofxRemoteUINeigbors::update(float dt){

	bool updated = false;
	time += dt;
	unordered_map<string, Neighbor>::iterator iterator;
	vector<string>idsToDelete;
	for( iterator = neigbhors.begin(); iterator != neigbhors.end(); iterator++) {
		string ID = iterator->first;
		Neighbor n = iterator->second;
		float timeLastSeen = n.timeLastSeen;
		float lastSeenSecAgo = time - timeLastSeen;
		if (lastSeenSecAgo > OFXREMOTEUI_NEIGHBOR_DEATH_BY_TIME){
			idsToDelete.push_back(ID);
			updated = true;
		}
	}

	for(int i = 0; i < idsToDelete.size(); i++){
		neigbhors.erase (idsToDelete[i]);
	}

	return updated;
}

void ofxRemoteUINeigbors::print(){
	RUI_LOG_NOTICE << "## NEIGHBORS ##############################";
	unordered_map<string, Neighbor>::iterator iterator;
	for( iterator = neigbhors.begin(); iterator != neigbhors.end(); iterator++) {
		Neighbor n = iterator->second;
		RUI_LOG_NOTICE << n.name << " (" << n.IP << ":" << n.port << ") " << time - n.timeLastSeen << " seconds ago.";
	}
	RUI_LOG_NOTICE << "###########################################";
}

bool ofxRemoteUINeigbors::gotPing(string ip, int port, string name, string binaryName){

	bool updated = false;
	std::ostringstream oss;
	oss << port;
	string myID = ip + ":" +  oss.str();

	if ( neigbhors.find(myID) == neigbhors.end() ){ //not found, add it
		updated = true;
		Neighbor n = Neighbor(ip, port, time, name, binaryName);
		neigbhors[myID] = n;
	}else{ //found!
		neigbhors[myID].timeLastSeen = time; //update time last seen for this dude
	}

	return updated;
}


vector<Neighbor> ofxRemoteUINeigbors::getNeighbors(){

	unordered_map<string, Neighbor>::iterator iterator;
	vector<Neighbor>ns;
	for( iterator = neigbhors.begin(); iterator != neigbhors.end(); iterator++) {
		ns.push_back(iterator->second);
	}
	return ns;
}

