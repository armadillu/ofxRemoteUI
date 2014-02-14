//
//  ofxRemoteUISimpleNotifications.h
//  emptyExample
//
//  Created by Oriol Ferrer Mesià on 18/11/13.
//
//

#ifndef emptyExample_ofxRemoteUISimpleNotifications_h
#define emptyExample_ofxRemoteUISimpleNotifications_h


#if defined OF_VERSION_MINOR /*if OF exists*/

#include "ofMain.h"

class ofxRemoteUISimpleNotifications{

public:

	struct SimpleNotification{
		string msg;
		float time;
	};

	struct ParamNotification{
		string value;
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

		vector<string> toDeleteParams;
		typedef std::map<string, ParamNotification>::iterator it_type;
		for(it_type it = paramNotifications.begin(); it != paramNotifications.end(); it++) {
			//it->first;
			it->second.time -= dt;
			if( it->second.time < 0.0f ){
				toDeleteParams.push_back(it->first);
			}
		}

		//delete expired paramUpdates
		for(int i = 0; i < toDeleteParams.size(); i++){
			paramNotifications.erase( toDeleteParams[i] );
		}
	};

	void draw(float x, float y){

		float spacing = 16;

		for(int i = 0; i < notifications.size(); i++){
			float a = ofClamp( 3.0 * notifications[i].time, 0.0f, 1.0f);
			ofDrawBitmapStringHighlight("ofxRemoteUIServer: " + notifications[i].msg,
										x,
										y - spacing * (notifications.size() - 1) + i * spacing,
										ofColor(0, 255 * a),
										ofColor(255,0,0, 255 * a)
										);
		}

		typedef std::map<string, ParamNotification>::iterator it_type;
		int c = 0;
		for(it_type it = paramNotifications.begin(); it != paramNotifications.end(); it++){
			float a = ofClamp( 3.0 * it->second.time, 0.0f, 1.0f);
			float fresh = 1.0f - ofClamp(OFXREMOTEUI_PARAM_UPDATE_NOTIFICATION_SCREENTIME - it->second.time, 0.0f, 1.0f);
			string freshS = (fresh > 0.2 ) ? (ofGetFrameNum() % 12 < 6 ? " <<" : "") : "";
			ofDrawBitmapStringHighlight( it->first + ": " + it->second.value + freshS,
										x,
										y - spacing * ( notifications.size() + (paramNotifications.size()-1) - c ),
										ofColor(0, 255 * a),
										(fresh > 0.2 ) ? ofColor(0, 255, 0, 255 * a) : ofColor(255, 255, 255, 255 * a)
										);
			c++;
		}
	};

	void addNotification(string msg){
		SimpleNotification n;
		n.msg = msg;
		n.time = OFXREMOTEUI_EVENT_NOTIFICATION_SCREENTIME;
		notifications.push_back(n);
	};

	void addParamUpdate(string paramName, string paramValue){
		ParamNotification n;
		n.value = paramValue;
		n.time = OFXREMOTEUI_PARAM_UPDATE_NOTIFICATION_SCREENTIME;
		paramNotifications[paramName] = n;
	};

private:

	vector<SimpleNotification> notifications;
	map<string, ParamNotification> paramNotifications;
};

#endif
#endif
