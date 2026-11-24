#pragma once

#include "ofExistsTest.h"

#ifdef OF_AVAILABLE
#include "RemoteParam.h"
#include "ofxRemoteUISimpleNotifications.h"
#include <map>
#include <unordered_map>
#include <vector>

// Renders the server's built-in overlay and on-screen notifications.
class ofxRemoteUIBuiltInClient {
public:
	// References are borrowed for the duration of draw while the server holds its data lock.
	struct View {
		bool showUI;
		bool showDuringEdits;
		bool drawNotifications;
		int selectedItem;
		int selectedPreset;
		int selectedGroupPreset;
		int selectedColorComp;
		const std::string &computerIP;
		int port;
		#ifndef NO_RUI_WEB_INTERFACE
		int webPort;
		#endif
		const std::vector<std::string> &presets;
		const std::unordered_map<std::string, std::vector<std::string>> &groupPresets;
		const std::map<int, std::string> &orderedKeys;
		const std::unordered_map<std::string, RemoteUIParam> &params;
		const std::vector<std::string> &paramsToWatch;
		const std::map<std::string, RemoteUIServerValueWatch> &varWatches;
	};

	ofxRemoteUIBuiltInClient();

	void draw(const View &view, ofxRemoteUISimpleNotifications &notifications, int x, int y);
	void update(float elapsed);
	ofVec2f defaultNotificationPosition() const;

	float getScale() const { return uiScale; }
	float getAlpha() const { return uiAlpha; }
	void setAlpha(float alpha) { uiAlpha = alpha; }
	void clearLayout() { uiLines.clear(); }
	void scrollColumns(int columns);

	void setColumnWidth(int w);
	void setScale(float s, ofxRemoteUISimpleNotifications &notifications);
	void setScreenHeight(int h);
	void setScreenWidth(int w);
	void setCustomDrawPos(int x, int y);

	void drawUiWithBitmapFont(ofxRemoteUISimpleNotifications &notifications);
	#ifdef USE_OFX_FONTSTASH
	void drawUiWithFontStash(const std::string &fontPath, float fontSize, ofxRemoteUISimpleNotifications &notifications);
	ofxFontStash &getFont() { return font; }
	#endif
	#ifdef USE_OFX_FONTSTASH2
	void drawUiWithFontStash2(const std::string &fontPath, float fontSize, ofxRemoteUISimpleNotifications &notifications);
	ofxFontStash2::Fonts &getFont2() { return font2; }
	#endif

private:
	void drawOverlay(const View &view, int screenW, int screenH);
	void drawNotifications(const View &view, ofxRemoteUISimpleNotifications &notifications, int x, int y);
	void drawString(const std::string &text, float x, float y);
	void drawString(const std::string &text, const ofVec2f &pos);

	ofVboMesh uiLines;
	int layoutScreenWidth = -1;
	int layoutScreenHeight = -1;
	float uiColumnWidth = 320;
	float uiAlpha = 1.0f;
	float uiScale = 1.0f;
	float xOffset = 0.0f;
	float xOffsetTarget = 0.0f;
	int customScreenHeight = -1;
	int customScreenWidth = -1;
	ofVec2f customPos;

	enum FontRenderer {
		RENDER_WITH_OF_BITMAP_FONT,
		RENDER_WITH_OFXFONTSTASH,
		RENDER_WITH_OFXFONTSTASH2
	};
	FontRenderer fontRenderer = RENDER_WITH_OF_BITMAP_FONT;

	#ifdef USE_OFX_FONTSTASH
	ofxFontStash font;
	std::string fontFile;
	float fontSize;
	#endif
	#ifdef USE_OFX_FONTSTASH2
	std::string fontStashFile2;
	ofxFontStash2::Fonts font2;
	float fontSize2;
	#endif

	const float ruiLineH = 20;
	float lineH = ruiLineH;
	float charW = 8;
};
#endif
