//
//  ofxRemoteUISimpleNotifications.h
//  emptyExample
//
//  Created by Oriol Ferrer Mesià on 18/11/13.
//
//

#pragma once

#include "ofExistsTest.h"

#if defined OF_AVAILABLE /*if OF exists*/

#define RUI_NOTIFICATION_ALPHA_OVERFLOW		3.0
#define RUI_NOTIFICATION_COLOR				ofColor(200,16,16, 255 * a)
#define RUI_LOG_COLOR						ofColor(28,214,40, 255 * a)
#define RUI_NOTIFICATION_LINEHEIGHT			20
#define RUI_NOTIFICATION_FONTSIZE			15

#define _USE_MATH_DEFINES // visual studio M_PI
#include <math.h>

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

	void update(float dt);
	void draw(float x, float y);

	void addNotification(const string & msg);

	void addLogLine(const string & msg, bool merge); //if merge==true, will look for existing log lines and increase their counter
	void addParamUpdate(const string & paramName, const RemoteUIParam & p, const ofColor & bgColor, const ofColor & paramC = ofColor(0,0));
	void addParamWatch(const string &paramName, const  string& paramValue, const ofColor & paramC);
	void removeParamWatch(const string & paramName);
	void removeAllParamWatches();
	void addVariableWatch(const string &paramName, const string& paramValue, const ofColor & paramC);


	//control type render engine
	#ifdef USE_OFX_FONTSTASH
	void drawUiWithFontStash(ofxFontStash * font_);
	#endif
	#ifdef USE_OFX_FONTSTASH2
	void drawUiWithFontStash2(ofxFontStash2::Fonts * font2_);
	#endif
	void drawUiWithBitmapFont();


	void setNotificationScreenTime(float t);
	void setLogNotificationScreenTime(float t);

private:

	//return height of box
	float drawStringWithBox(const string & text, int x, int y, const ofColor& background, const ofColor& foreground, float fontSize = RUI_NOTIFICATION_FONTSIZE, float lineH = RUI_NOTIFICATION_LINEHEIGHT );

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
