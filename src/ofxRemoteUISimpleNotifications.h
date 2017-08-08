//
//  ofxRemoteUISimpleNotifications.h
//  emptyExample
//
//  Created by Oriol Ferrer Mesià on 18/11/13.
//
//

#pragma once

#if defined OF_VERSION_MINOR /*if OF exists*/

#define RUI_NOTIFICATION_ALPHA_OVERFLOW		3.0
#define RUI_NOTIFICATION_COLOR				ofColor(200,16,16, 255 * a)
#define RUI_LOG_COLOR						ofColor(28,214,40, 255 * a)
#define RUI_NOTIFICATION_LINEHEIGHT			20
#define RUI_NOTIFICATION_FONTSIZE			15

#define _USE_MATH_DEFINES // visual studio M_PI
#include <math.h>

#include "ofMain.h"
#include "RemoteParam.h"

// ofxFontStash ///////////////////////////////////////////////////////////////////////////////////

#if defined(__has_include) /*llvm only - query about header files being available or not*/
#if __has_include("ofxFontStash.h") && !defined(DISABLE_AUTO_FIND_FONSTASH_HEADERS)
	#ifndef USE_OFX_FONTSTASH
		#define USE_OFX_FONTSTASH
	#endif
#endif
#endif

#ifdef USE_OFX_FONTSTASH
	#include "ofxFontStash.h"
#endif

// ofxFontStash2 //////////////////////////////////////////////////////////////////////////////////

#if defined(__has_include) /*llvm only - query about header files being available or not*/
#if __has_include("ofxFontStash2.h") && !defined(DISABLE_AUTO_FIND_FONSTASH_HEADERS)
	#ifndef USE_OFX_FONTSTASH2
		#define USE_OFX_FONTSTASH2
	#endif
#endif
#endif

#ifdef USE_OFX_FONTSTASH2
#include "ofxFontStash2.h"
#endif


class ofxRemoteUISimpleNotifications{

public:

	struct SimpleNotification{
		string msg;
		float time;
	};

	struct LogLineNotification{
		string logLine;
		float time;
		float highlight; //a decay-based timer - when a repeat notif is new, will be highlighted shortly
		int repeatCount;
	};

	struct ParamNotification{
		string value;
		float time;
		ofColor color;
		ofColor bgColor;
		bool range;
		float rangeMin;
		float rangeMax;
		float pct;
	};

	void update(float dt){

		vector<int> toDeleteIndexes;
		//walk all notif, write down expired ones's indexes
		for(size_t i = 0; i < notifications.size(); i++){
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
		for(size_t i = 0; i < toDeleteParams.size(); i++){
			paramNotifications.erase( toDeleteParams[i] );
		}

		//delete expired log lines
		toDeleteIndexes.clear();
		for(size_t i = 0; i < logLines.size(); i++){
			logLines[i].time -= dt;
			if (logLines[i].highlight > 0.0f) logLines[i].highlight -= dt * 0.75;
			if( logLines[i].time < 0.0f ){
				toDeleteIndexes.push_back(i);
			}
		}

		//delete expired logLines
		for(int i = toDeleteIndexes.size() - 1; i >= 0; i--){
			logLines.erase( logLines.begin() + toDeleteIndexes[i] );
		}
	};

	void draw(float x, float y){

		float spacing = RUI_NOTIFICATION_LINEHEIGHT;
		float yy = y;

		for(size_t i = 0; i < notifications.size(); i++){
			float a = ofClamp( RUI_NOTIFICATION_ALPHA_OVERFLOW * notifications[i].time, 0.0f, 1.0f);
			float hh = drawStringWithBox("ofxRemoteUIServer: " + notifications[i].msg,
										x,
										yy,
										ofColor(0, 255 * a),
										RUI_NOTIFICATION_COLOR
										);
			yy -= hh;
		}

		for(size_t i = 0; i < logLines.size(); i++){
			float a = ofClamp( RUI_NOTIFICATION_ALPHA_OVERFLOW * logLines[i].time, 0.0f, 1.0f);
			string repeatCount;
			if (logLines[i].repeatCount > 1) repeatCount = " (x" + ofToString(logLines[i].repeatCount) + ")";
			float highlight = ofClamp(logLines[i].highlight, 0, 1); //will decrease from 1 to 0 over time
			float blink = powf(highlight, 0.5) * fabs(sin(3 * M_PI * highlight));
			//float fontBump = RUI_NOTIFICATION_FONTSIZE + 4 * highlight;
			//float lineHeightBump = RUI_NOTIFICATION_LINEHEIGHT + 4 * highlight;
			ofColor fontColor = RUI_LOG_COLOR; fontColor.a = 255;
			ofColor rainbow;
			if(blink > 0.01){
				rainbow.setHsb((30 * ofGetFrameNum())%255, 255, 255);
			}
			float hh = drawStringWithBox("Log: " + logLines[i].logLine + repeatCount,
										 x + 20 * blink,
										 yy,
										 ofColor(0, 255 * a),
										 rainbow * blink + fontColor * (1.0 - blink)
										 );
			yy -= hh;
		}

		typedef std::map<string, ParamNotification>::iterator it_type;
		for(it_type it = paramNotifications.begin(); it != paramNotifications.end(); it++){

			float a = ofClamp( RUI_NOTIFICATION_ALPHA_OVERFLOW * it->second.time, 0.0f, 1.0f);
			float fresh = 1.0f - ofClamp((screenTime + 1) - it->second.time, 0.0f, 1.0f);

			string total = it->first + ": " + it->second.value;
			ofColor bgColor = (fresh > 0.1 ) ? it->second.bgColor : ofColor(it->second.bgColor, 255 * a);
			float hh = drawStringWithBox( total,
								x,
								yy,
								bgColor,
								ofColor::black
								);

			int xx = x + total.length() * 8 + 4;
			int yyy = yy + 6 - RUI_NOTIFICATION_LINEHEIGHT;
			if (it->second.color.a != 0){ //this is a color type param - draw swatch
				ofPushStyle();
				ofSetColor(it->second.color, a * 255);

				switch (fontRenderer) {

					case RENDER_WITH_OF_BITMAP_FONT:
						ofDrawRectangle(xx, yyy , 40, RUI_NOTIFICATION_LINEHEIGHT); break;

					#ifdef USE_OFX_FONTSTASH
					case RENDER_WITH_OFXFONTSTASH:{
						ofRectangle r = font->getBBox(total, RUI_NOTIFICATION_FONTSIZE, 0, 0);
						float diff = floor(RUI_NOTIFICATION_LINEHEIGHT - r.height);
						ofDrawRectangle(x + r.width + r.x + 4, yy + r.y - diff / 2, 40, RUI_NOTIFICATION_LINEHEIGHT);
						}break;
					#endif

					#ifdef USE_OFX_FONTSTASH2
					case RENDER_WITH_OFXFONTSTASH2:{
						ofRectangle r = font2->getTextBounds(total, ofxFontStash2::Style("mono", RUI_NOTIFICATION_FONTSIZE, ofGetStyle().color), 0, 0);
						float diff = floor(RUI_NOTIFICATION_LINEHEIGHT - r.height);
						ofDrawRectangle(x + r.width + r.x + 4, yy + r.y - diff / 2, 40, RUI_NOTIFICATION_LINEHEIGHT);
					}break;
					#endif
					default:break;
				}
				ofPopStyle();
			}
			if(it->second.range){ //draw slider
				int sliderW = 80;
				int pad = 9;
				int knobW = 6;
				int markH = 2;
				int voff = (RUI_NOTIFICATION_LINEHEIGHT - knobW) / 2;
				ofSetColor(0);
				ofDrawRectangle(xx, yyy, sliderW, RUI_NOTIFICATION_LINEHEIGHT);
				ofSetColor(45);
				ofDrawRectangle(xx + pad, yyy + pad, sliderW - 2 * pad, RUI_NOTIFICATION_LINEHEIGHT - 2 * pad);
				ofSetColor(bgColor);
				ofDrawLine(xx + sliderW/2, yyy + RUI_NOTIFICATION_LINEHEIGHT / 2 + markH, xx + sliderW/2,  yyy + RUI_NOTIFICATION_LINEHEIGHT / 2 - markH );
				ofDrawRectangle(xx + pad - knobW/2 + (sliderW - 2 * pad) * ofClamp(it->second.pct, 0, 1), yyy + voff, knobW , knobW );

			}
			yy -= hh;
		}

		auto it2 = paramWatchOrder.begin();
		while(it2 != paramWatchOrder.end()){
			string & name = it2->second;
			float hh = drawStringWithBox( "[" + name + "] " + paramWatch[name].value,
								x, yy, paramWatch[name].bgColor, paramWatch[name].color );
			yy -= hh;
			++it2;
		}

		auto it3 = variableWatchOrder.rbegin();
		while(it3 != variableWatchOrder.rend()){
			string & name = it3->second;
			float hh = drawStringWithBox( "*(" + name + ") = " + variableWatch[name].value,
										 x, yy, variableWatch[name].bgColor, variableWatch[name].color, 11, 12 );
			yy -= hh;
			++it3;
		}
	};

	void addNotification(const string & msg){
		SimpleNotification n;
		n.msg = msg;
		n.time = screenTime;
		notifications.push_back(n);
	};

	void addLogLine(const string & msg, bool merge){ //if merge==true, will look for existing log lines and increase their counter

		bool found = false;
		if(merge){
			for(size_t i = 0; i < logLines.size(); i++){
				if(logLines[i].logLine == msg){
					logLines[i].repeatCount++;
					logLines[i].time = logScreenTime; //update
					logLines[i].highlight = 1.0;
					found = true;
					break;
				}
			}
		}
		if(!found || !merge){
			LogLineNotification n;
			n.logLine = msg;
			n.time = logScreenTime;
			n.repeatCount = 1;
			n.highlight = 1.0;
			//logLines.push_back(n);
			logLines.insert(logLines.begin(), n);

		}
	};


	void addParamUpdate(const string & paramName, const RemoteUIParam & p, const ofColor & bgColor, const ofColor & paramC = ofColor(0,0)){
		ParamNotification n;
		n.color = paramC;
		n.bgColor = bgColor;
		n.value = p.getValueAsString();
		n.time = screenTime;
		switch(p.type){
			case REMOTEUI_PARAM_FLOAT:
				n.rangeMin = p.minFloat; n.rangeMax = p.maxFloat;
				n.pct = ofMap(p.floatVal, p.minFloat, p.maxFloat, 0, 1);
				n.range = true;
				if(n.value.size() < 10){
					for(size_t i = 0; i < n.value.size() - 10; i++){
						n.value += " ";
					}
				}
				break;
			case REMOTEUI_PARAM_INT:
				n.rangeMin = p.minInt; n.rangeMax = p.maxInt;
				n.pct = ofMap(p.intVal, p.minInt, p.maxInt, 0, 1);
				n.range = true;
				break;
			default:
				n.range = false;
				break;
		}
		paramNotifications[paramName] = n;
	};

	void addParamWatch(const string &paramName, const  string& paramValue, const ofColor & paramC){
		ParamNotification n;
		n.color = paramC;
		n.bgColor = ofColor(0);
		n.value = paramValue;
		n.time = screenTime;
		n.range = false;
		if(paramWatch.find(paramName) == paramWatch.end()){
			paramWatchOrder[paramWatchOrder.size()] = paramName;
		}
		paramWatch[paramName] = n;
	};

	void addVariableWatch(const string &paramName, const string& paramValue, const ofColor & paramC){
		ParamNotification n;
		n.color = paramC;
		n.bgColor = ofColor(0,190);
		n.value = paramValue;
		n.time = screenTime;
		n.range = false;
		if(variableWatch.find(paramName) == variableWatch.end()){
			variableWatchOrder[variableWatchOrder.size()] = paramName;
		}
		variableWatch[paramName] = n;
	};


	#ifdef USE_OFX_FONTSTASH
	void drawUiWithFontStash(ofxFontStash * font_){
		fontRenderer = RENDER_WITH_OFXFONTSTASH;
		font = font_;
	}
	#endif

	#ifdef USE_OFX_FONTSTASH2
	void drawUiWithFontStash2(ofxFontStash2::Fonts * font2_){
		fontRenderer = RENDER_WITH_OFXFONTSTASH2;
		font2 = font2_;
	}
	#endif

	void drawUiWithBitmapFont(){
		font = NULL;
	}

	void setNotificationScreenTime(float t){screenTime = t;}
	void setLogNotificationScreenTime(float t){logScreenTime = t;}


private:

	//return height of box
	float drawStringWithBox(const string & text, int x, int y, const ofColor& background, const ofColor& foreground, float fontSize = RUI_NOTIFICATION_FONTSIZE, float lineH = RUI_NOTIFICATION_LINEHEIGHT ){

		switch (fontRenderer) {
			case RENDER_WITH_OF_BITMAP_FONT:{
				ofDrawBitmapStringHighlight(text, x, y, background, foreground);
			}break;

			#ifdef USE_OFX_FONTSTASH
			case RENDER_WITH_OFXFONTSTASH:{
				ofRectangle r = font->getBBox(text, fontSize, x, y);
				float diff = floor(lineH - r.height);
				r.x = x - 4;
				r.y -= diff * 0.5f;
				r.width += diff + 2;
				r.height += diff;
				r.height = ceil(r.height);
				ofPushStyle();
				ofSetColor(background);
				ofDrawRectangle(r);
				ofSetColor(foreground);
				font->draw(text, fontSize, x, y);
				ofPopStyle();
				return floor(r.height);
			}break;
			#endif

			#ifdef USE_OFX_FONTSTASH2
			case RENDER_WITH_OFXFONTSTASH2:{
				ofxFontStash2::Style s = ofxFontStash2::Style("mono", fontSize, foreground);
				ofRectangle r = font2->getTextBounds(text, s, x, y);
				float diff = floor(lineH - r.height);
				r.x = x - 4;
				r.y -= diff * 0.5f;
				r.width += diff + 2;
				r.height += diff;
				r.height = ceil(r.height);
				ofPushStyle();
				ofSetColor(background);
				ofDrawRectangle(r);
				font2->draw(text, s, x, y);
				ofPopStyle();
				return floor(r.height);
			}break;
			#endif
			default:break;
		}
		return RUI_NOTIFICATION_LINEHEIGHT;
	}

	#ifdef USE_OFX_FONTSTASH
	ofxFontStash * font = nullptr;
	#endif
	#ifdef USE_OFX_FONTSTASH2
	ofxFontStash2::Fonts * font2 = nullptr;
	#endif

	enum FontRenderer{
		RENDER_WITH_OF_BITMAP_FONT,
		RENDER_WITH_OFXFONTSTASH,
		RENDER_WITH_OFXFONTSTASH2
	};

	FontRenderer fontRenderer = RENDER_WITH_OF_BITMAP_FONT;


	vector<SimpleNotification> notifications;
	vector<LogLineNotification> logLines;
	map<string, ParamNotification> paramNotifications;
	map<string, ParamNotification> paramWatch;
	map<string, ParamNotification> variableWatch;
	map<int, string> paramWatchOrder;
	map<int, string> variableWatchOrder;
	float screenTime = 5.0;
	float logScreenTime = 10;
};

#endif
