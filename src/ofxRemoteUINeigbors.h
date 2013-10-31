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

#define MAX_MISSING_TIME	3	/*seconds*/

struct Neighbor{
	Neighbor(){};
	Neighbor(string ip, int p, float t, string n){IP = ip; port = p; timeLastSeen = t; name = n;}
	string IP,name;
	int port;
	float timeLastSeen;
};

class ofxRemoteUINeigbors{

public:

	ofxRemoteUINeigbors();
	bool update(float dt); //return false if no update, true if theres an update
	bool gotPing(string ip, int port, string name); //idem
	void print();
	vector<Neighbor> getNeighbors();

private:

	map<string,Neighbor>neigbhors; //this will be kept updated, key is a combostring of "IP:PORT"
	float time; //secs

};

#endif /* defined(__emptyExample__ofxRemoteUINeigbors__) */
