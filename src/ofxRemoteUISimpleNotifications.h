//
//  ofxRemoteUISimpleNotifications.h
//  emptyExample
//
//  Created by Oriol Ferrer Mesià on 18/11/13.
//
//

#ifndef emptyExample_ofxRemoteUISimpleNotifications_h
#define emptyExample_ofxRemoteUISimpleNotifications_h

#if defined OF_VERSION_MINOR

#include "ofMain.h"

class ofxRemoteUISimpleNotifications{

public:

	struct SimpleNotification{
		string msg;
		float time;
	};

	ofxRemoteUISimpleNotifications(){};

	void update(float dt){
		vector<int> toDeleteIndexes;
		//walk all notif, write down expired ones's indexes
		for(int i = 0; i < notifications.size(); i++){
			notifications[i].time -= dt;
			if( notifications[i].time < 0.0f ){
				toDeleteIndexes.push_back(i);
			}
		}
		//delete expired notifications
		for(int i = toDeleteIndexes.size() - 1; i >= 0; i--){
			notifications.erase( notifications.begin() + toDeleteIndexes[i] );
		}
	};

	void draw(float x, float y){
		for(int i = 0; i < notifications.size(); i++){
			float spacing = 20;
			float a = ofClamp( 3.0 * notifications[i].time, 0.0f, 1.0f);
			ofDrawBitmapStringHighlight("ofxRemoteUIServer: " + notifications[i].msg,
										x,
										- spacing * (notifications.size() - 1) + y + i * spacing,
										ofColor(0, 255 * a),
										ofColor(255,0,0, 255 * a)
										);
		}

	};

	void addNotification(string msg){
		SimpleNotification n;
		n.msg = msg;
		n.time = OFXREMOTEUI_NOTIFICATION_SCREENTIME;
		notifications.push_back(n);
	};

private:

	vector<SimpleNotification> notifications;
};

#endif
#endif
