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

#define NOTIFICATION_ALPHA_OVERFLOW		3.0
#define NOTIFICATION_COLOR				ofColor(200,16,16, 255 * a)

#define PARAM_UPDATE_COLOR				ofColor(0, 200, 0, 255 * a)
#define FRESH_COLOR						ofColor(0)
#define PARAM_WATCH_COLOR				ofColor(0, 128, 255)
#define NOTIFICATION_LINEHEIGHT			20
#include "ofMain.h"

#ifdef USE_OFX_FONTSTASH
	#include "ofxFontStash.h"
#endif


class ofxRemoteUISimpleNotifications{

public:

	struct SimpleNotification{
		string msg;
		float time;
	};

	struct ParamNotification{
		string value;
		float time;
		ofColor color;
		ofColor bgColor;
	};


	ofxRemoteUISimpleNotifications(){
		#ifdef USE_OFX_FONTSTASH
		font = NULL;
		#endif
	};

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

		float spacing = NOTIFICATION_LINEHEIGHT;
		float yy = y;

		for(int i = 0; i < notifications.size(); i++){
			float a = ofClamp( NOTIFICATION_ALPHA_OVERFLOW * notifications[i].time, 0.0f, 1.0f);
			float hh = drawStringWithBox("ofxRemoteUIServer: " + notifications[i].msg,
										x,
										yy,
										ofColor(0, 255 * a),
										NOTIFICATION_COLOR
										);
			yy -= hh;
		}

		typedef std::map<string, ParamNotification>::iterator it_type;
		for(it_type it = paramNotifications.begin(); it != paramNotifications.end(); it++){

			float a = ofClamp( NOTIFICATION_ALPHA_OVERFLOW * it->second.time, 0.0f, 1.0f);
			float fresh = 1.0f - ofClamp((screenTime + 1) - it->second.time, 0.0f, 1.0f);

			string total = it->first + ": " + it->second.value ;
			float hh = drawStringWithBox( total,
								x,
								yy,
								(fresh > 0.1 ) ? it->second.bgColor : ofColor(it->second.bgColor, 255 * a),
								ofColor::black
								);
			yy -= hh;

			if (it->second.color.a != 0){
				ofPushStyle();
				ofSetColor(it->second.color, a * 255);
				ofRect(x + total.length() * 8 + 4, yy - 14, 40, 20);
				ofPopStyle();
			}
		}

		map<int, string>::iterator it2 = paramWatchOrder.begin();
		while(it2 != paramWatchOrder.end()){

			int order = it2->first;
			string name = it2->second;

			float hh = drawStringWithBox( "[" + it2->second + "] " + paramWatch[name].value,
								x,
								yy,
								ofColor::black, paramWatch[name].color
								);
			yy -= hh;
			++it2;
		}
	};

	void addNotification(string msg){
		SimpleNotification n;
		n.msg = msg;
		n.time = screenTime;
		notifications.push_back(n);
	};

	void addParamUpdate(string paramName, string paramValue, ofColor bgColor, ofColor paramC = ofColor(0,0)){
		ParamNotification n;
		n.color = paramC;
		n.bgColor = bgColor;
		n.value = paramValue;
		n.time = screenTime;
		paramNotifications[paramName] = n;
	};

	void addParamWatch(const string &paramName, const  string& paramValue, const ofColor & paramC){
		ParamNotification n;
		n.color = paramC;
		n.bgColor = ofColor(0);
		n.value = paramValue;
		n.time = screenTime;
		if(paramWatch.find(paramName) == paramWatch.end()){
			paramWatchOrder[paramWatchOrder.size()] = paramName;
			paramWatch[paramName] = n;
		}
	};


	#ifdef USE_OFX_FONTSTASH
	void drawUiWithFontStash(ofxFontStash * font_){
		font = font_;
	}
	void drawUiWithBitmapFont(){
		font = NULL;
	}
	#endif

	void setNotificationScreenTime(float t){screenTime = t;}

private:

	//return height of box
	float drawStringWithBox(string text, int x, int y, const ofColor& background, const ofColor& foreground ){
		#ifdef USE_OFX_FONTSTASH
		if(font == NULL){
			ofDrawBitmapStringHighlight(text, x, y, background, foreground);
		}else{
			ofRectangle r = font->getBBox(text, 15, x, y);
			float diff = floor(NOTIFICATION_LINEHEIGHT - r.height);
			r.x = x - 4;
			r.y -= diff * 0.5f;
			r.width += diff + 2;
			r.height += diff;
			r.height = ceil(r.height);
			ofPushStyle();
			ofSetColor(background);
			ofRect(r);
			ofSetColor(foreground);
			font->draw(text, 15, x, y);
			ofPopStyle();
			return floor(r.height);
		}
		#else
		ofDrawBitmapStringHighlight(text, x, y, background, foreground);
		#endif
		return NOTIFICATION_LINEHEIGHT;
	}

	#ifdef USE_OFX_FONTSTASH
	ofxFontStash * font;
	#endif

	vector<SimpleNotification> notifications;
	map<string, ParamNotification> paramNotifications;
	map<string, ParamNotification> paramWatch;
	map<int, string> paramWatchOrder;
	float screenTime = 5.0;
};

#endif
#endif
