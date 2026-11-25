// Included by ofxRemoteUIServer.cpp so existing projects with explicit source lists
// build the renderer without adding a new translation unit.
#include "ofxRemoteUIBuiltInClient.h"
#include <cfloat>

ofxRemoteUIBuiltInClient::ofxRemoteUIBuiltInClient(){
	uiLines.setMode(OF_PRIMITIVE_LINES);
}

void ofxRemoteUIBuiltInClient::update(float elapsed){
	uiAlpha += 0.3f * elapsed;
	if(uiAlpha > 1.0f) uiAlpha = 1.0f;
	xOffset = 0.3f * xOffsetTarget + 0.7f * xOffset;
}

ofVec2f ofxRemoteUIBuiltInClient::defaultNotificationPosition() const {
	int height = customScreenHeight < 0 ? ofGetHeight() : customScreenHeight;
	return ofVec2f(8 / uiScale, (height - 9) / uiScale);
}

void ofxRemoteUIBuiltInClient::scrollColumns(int columns){
	xOffsetTarget = ofClamp(xOffsetTarget + columns * uiColumnWidth, -FLT_MAX, 4 * uiColumnWidth);
}

#ifdef USE_OFX_FONTSTASH
void ofxRemoteUIBuiltInClient::drawUiWithFontStash(const std::string &fontPath, float fontSize_, ofxRemoteUISimpleNotifications &onScreenNotifications){

	if(!ofIsGLProgrammableRenderer()){
		fontRenderer = RENDER_WITH_OFXFONTSTASH;
		fontFile = ofToDataPath(fontPath, true);
		fontSize = fontSize_;
		font = ofxFontStash();
		font.setup(fontFile, 1.0, 512, false, 0, uiScale);
		onScreenNotifications.drawUiWithFontStash(&font);
		ofRectangle r = font.getBBox("M", fontSize, 0, 0);
		lineH = ceil(r.height * 1.75);
		charW = r.width;
		clearLayout();
	}else{
		ofLogError("ofxRemoteUIServer") << "Can't use ofxFontStash with the Programmable Renderer!";
	}
}

#endif

#ifdef USE_OFX_FONTSTASH2
void ofxRemoteUIBuiltInClient::drawUiWithFontStash2(const std::string &fontPath, float fontSize_, ofxRemoteUISimpleNotifications &onScreenNotifications){
	fontRenderer = RENDER_WITH_OFXFONTSTASH2; fontSize2 = fontSize_; fontStashFile2 = fontPath;
	font2 = ofxFontStash2::Fonts();
	font2.setup();
	font2.addFont("mono", ofToDataPath(fontStashFile2, true));
	onScreenNotifications.drawUiWithFontStash2(&font2);
	ofxFontStash2::Style style = ofxFontStash2::Style("mono", fontSize2);
	ofRectangle r = font2.getTextBounds("Mp", style, 0, 0);
	charW = r.width;
	lineH = ceil(r.height * 1.25);
	clearLayout();
}
#endif

void ofxRemoteUIBuiltInClient::drawUiWithBitmapFont(ofxRemoteUISimpleNotifications &onScreenNotifications){
	fontRenderer = RENDER_WITH_OF_BITMAP_FONT;
	onScreenNotifications.drawUiWithBitmapFont();
	charW = 8;
	lineH = ruiLineH;
	clearLayout();
}


void ofxRemoteUIBuiltInClient::drawString(const std::string & text, const ofVec2f & pos){
	drawString(text, pos.x, pos.y);
}

void ofxRemoteUIBuiltInClient::drawString(const std::string & text, float x, float y){

	switch (fontRenderer) {

		case RENDER_WITH_OF_BITMAP_FONT: ofDrawBitmapString(text, x, y + 1);
			break;

		#ifdef USE_OFX_FONTSTASH
		case RENDER_WITH_OFXFONTSTASH: font.drawMultiLine(text, fontSize, x, y + lineH * 0.135); break;
		#endif

		#ifdef USE_OFX_FONTSTASH2
		case RENDER_WITH_OFXFONTSTASH2:{
			ofxFontStash2::Style style = ofxFontStash2::Style("mono", fontSize2, ofGetStyle().color);
			font2.drawColumnNVG(text, style, x, y + lineH * 0.125, ofGetWidth());
		}break;
		#endif
		default:break;
	}
}

void ofxRemoteUIBuiltInClient::setCustomDrawPos(int x, int y){
	customPos.x = x;
	customPos.y = y;
}

void ofxRemoteUIBuiltInClient::draw(const View &view, ofxRemoteUISimpleNotifications &notifications, int x, int y){
	const bool showNotifications = !view.showUI || uiAlpha < 1.0f;
	if(!view.showUI && (!view.drawNotifications ||
		(notifications.empty() && view.paramsToWatch.empty() && view.varWatches.empty()))) return;

	ofSetupScreen();
	ofPushStyle();
	ofPushMatrix();
	ofTranslate(customPos.x, customPos.y);
	ofScale(uiScale, uiScale);
	ofSetDrawBitmapMode(OF_BITMAPMODE_SIMPLE);
	ofEnableBlendMode(OF_BLENDMODE_ALPHA);
	ofSetRectMode(OF_RECTMODE_CORNER);
	ofFill();

	if(view.showUI){
		const int screenW = customScreenWidth < 0 ? ofGetWidth() : customScreenWidth;
		const int screenH = customScreenHeight < 0 ? ofGetHeight() : customScreenHeight;
		drawOverlay(view, screenW, screenH);
	}
	if(showNotifications && view.drawNotifications){
		drawNotifications(view, notifications, x, y);
	}

	ofPopMatrix();
	ofPopStyle();
}

void ofxRemoteUIBuiltInClient::drawOverlay(const View &view, int screenW, int screenH){
	if(screenW != layoutScreenWidth || screenH != layoutScreenHeight){
		clearLayout();
		layoutScreenWidth = screenW;
		layoutScreenHeight = screenH;
	}
	const std::vector<std::string> noPresets;
	auto presetsForGroup = [&](const std::string &group) -> const std::vector<std::string> & {
		auto found = view.groupPresets.find(group);
		return found == view.groupPresets.end() ? noPresets : found->second;
	};


	int padding = lineH * 1.5;
	int x = padding;
	int initialY = lineH * 2;
	int y = initialY;
	int colw = uiColumnWidth;
	int realColW = colw * 0.9;
	int valOffset = realColW * 0.7;
	int valSpaceW = realColW - valOffset;
	int spacing = lineH;
	int bottomBarHeight = lineH * 4.25;
	int frameNum = ofGetFrameNum();

	//bottom bar
	if (uiAlpha > 0.99 || view.showDuringEdits){
		ofSetColor(11, 245);
		ofDrawRectangle(0,0, screenW / uiScale, screenH / uiScale);
		ofSetColor(44, 245);
		ofDrawRectangle(0,screenH / uiScale - bottomBarHeight, screenW / uiScale, bottomBarHeight );

		std::string reachableAt = "Server reachable at " + view.computerIP + ":" + ofToString(view.port) + "(OSC) ";
		#ifndef NO_RUI_WEB_INTERFACE
		reachableAt += ofToString(view.webPort) + "(WebGUI)";
		#endif

		ofSetColor(255);
		std::string instructions = "ofxRemoteUI built in client. " + reachableAt +
		"\nPress 's' to save current config, 'S' to make a new preset. ('E' to save in old format)\n" +
		"Press 'r' to restore all params's launch state. '+'/'-' to set UI Scale. 'N' to toggle screen notif.\n" +
		"Press Arrow Keys to edit values. SPACEBAR + Arrow Keys for bigger increments. ',' and '.' Keys to scroll.\n" +
		"Press 'w' to toggle watch. Press 'TAB' to hide. Press 'RETURN' to cycle RGBA components.";
		#ifndef NO_RUI_WEB_INTERFACE
		instructions += " Press 'c' to launch a web client.";
		#endif

		drawString(instructions,
				   charW * 1.5,
				   screenH / uiScale - bottomBarHeight + lineH * 0.65);
	}

	//preset selection / top bar
	ofSetColor(64);
	ofDrawRectangle(0 , 0, screenW / uiScale, lineH);
	ofColor textBlinkC ;
	if(frameNum%6 < 4) textBlinkC = ofColor(0,0);
	else textBlinkC = ofColor(255,0,0);

	ofVec2f dpos = ofVec2f(charW * 23, lineH * 0.65);
	if(view.presets.size() > 0 && view.selectedPreset >= 0 && view.selectedPreset < view.presets.size()){

		ofSetColor(180);
		if (view.selectedItem < 0){
			drawString("Press RETURN to load GLOBAL PRESET: \"" + view.presets[view.selectedPreset] + "\"", dpos);
			ofSetColor(textBlinkC);
			drawString("                                     " + view.presets[view.selectedPreset], dpos);
		}else{
			auto key = view.orderedKeys.find(view.selectedItem);
			auto param = key == view.orderedKeys.end() ? view.params.end() : view.params.find(key->second);
			bool groupPresetDisplayed = false;
			if(param != view.params.end() && param->second.type == REMOTEUI_PARAM_SPACER){
				const auto &p = param->second;
				const auto &presets = presetsForGroup(p.group);
				if (view.selectedGroupPreset >= 0 && view.selectedGroupPreset < presets.size()){
					groupPresetDisplayed = true;
					std::string msg = "Press RETURN to load \"" + p.group + "\" GROUP PRESET: \"";
					drawString( msg + presets[view.selectedGroupPreset] + "\"", dpos);
					ofSetColor(textBlinkC);
					std::string padding(msg.length(), ' ');
					drawString(padding + presets[view.selectedGroupPreset], dpos);
				}
			}
			if(!groupPresetDisplayed){
				ofSetColor(180);
				drawString("Selected Preset: NONE", dpos);
			}
		}
	}
	if (view.selectedItem != -1) ofSetColor(255);
	else ofSetColor(textBlinkC);
	drawString("+ PRESET SELECTION: ", charW * 3,  dpos.y);

	int linesInited = uiLines.getNumVertices() > 0 ;
	ofPushMatrix();

	if(uiAlpha > 0.99 || view.showDuringEdits){

		ofTranslate(xOffset, 0);

		//param list
		for(const auto &entry : view.orderedKeys){
			const int i = entry.first;
			std::string key = entry.second;
			auto param = view.params.find(key);
			if(param == view.params.end()) continue;
			const RemoteUIParam &p = param->second;
			int chars = key.size();
			int charw = 9;
			int column2MaxLen = ceil(valSpaceW / charw) + 1;
			int stringw = chars * charw;
			if (stringw > valOffset){
				key = key.substr(0, (valOffset) / charw );
			}

			if(p.type == REMOTEUI_PARAM_SPACER){
				ofColor c = ofColor(p.r, p.g, p.b);
				ofSetColor(c * 0.3);
				ofDrawRectangle(x , -spacing + y + spacing * 0.33, realColW, spacing);
			}
			if (view.selectedItem != i){
				ofSetColor(p.r, p.g, p.b);
			}else{
				if(frameNum%5 < 1) ofSetColor(255);
				else ofSetColor(255,0,0);
			}

			if(p.type != REMOTEUI_PARAM_SPACER){
				std::string sel = (view.selectedItem == i) ? ">>" : "  ";
				const bool watched = std::find(view.paramsToWatch.begin(), view.paramsToWatch.end(), entry.second) != view.paramsToWatch.end();
				drawString(sel + (watched ? "*" : " ") + key, x, y);
			}else{
				drawString("+ " + p.stringVal, x,y);
			}

			switch (p.type) {
				case REMOTEUI_PARAM_FLOAT:
					drawString(ofToString(p.floatVal), x + valOffset, y);
					break;
				case REMOTEUI_PARAM_ENUM: {
					int index = p.intVal - p.minInt;
					if (index >= 0 && index < p.enumList.size()) {
						std::string val = p.enumList[index];
						if (val.length() > column2MaxLen) {
							val = val.substr(0, column2MaxLen);
						}
						drawString(val, x + valOffset, y);
					}else {
						drawString(ofToString(p.intVal), x + valOffset, y);
					}
				}break;
				case REMOTEUI_PARAM_INT:
					drawString(ofToString(p.intVal), x + valOffset, y);
					break;
				case REMOTEUI_PARAM_BOOL:
					drawString(p.boolVal ? "true" : "false", x + valOffset, y);
					break;
				case REMOTEUI_PARAM_STRING:
					drawString(p.stringVal, x + valOffset, y);
					break;
				case REMOTEUI_PARAM_COLOR:{
					ofPushStyle();
					ofSetColor(p.redVal, p.greenVal, p.blueVal, p.alphaVal);
					ofDrawRectangle(x + valOffset, y - spacing * 0.6, valSpaceW, spacing * 0.85);
					char aux[200];
					sprintf(aux, "[%0*d,%0*d,%0*d,%0*d]", 3, p.redVal, 3, p.greenVal, 3, p.blueVal, 3, p.alphaVal);
					ofSetColor(0); //shadow
					drawString(aux, x + valOffset + 1, y + 1);
					ofSetColor(255);
					drawString(aux, x + valOffset, y);
					if(view.selectedItem == i){
						char auxSelect[200];
						switch (view.selectedColorComp) {
							case 0: sprintf(auxSelect, " ___"); break;
							case 1: sprintf(auxSelect, "     ___"); break;
							case 2: sprintf(auxSelect, "         ___"); break;
							case 3: sprintf(auxSelect, "             ___"); break;
						}
						ofSetColor(textBlinkC);
						drawString(auxSelect, x + valOffset, y);
					}
					ofPopStyle();
					}break;
				case REMOTEUI_PARAM_SPACER:{
					const auto &presets = presetsForGroup(p.group);
					int howMany = presets.size();

					if (view.selectedItem == i){ //selected
						if (view.selectedGroupPreset < howMany && view.selectedGroupPreset >= 0){
							std::string presetName = presets[view.selectedGroupPreset];
							if (presetName.length() > column2MaxLen){
								presetName = presetName.substr(0, column2MaxLen);
							}
							drawString(presetName, x + valOffset, y);
						}
					}else{ //not selected
						if(howMany > 0 ){
							drawString("(" + ofToString(howMany) + ")", x + valOffset, y);
						}
					}
					}break;
				default: printf("weird RemoteUIParam at draw()!\n"); break;
			}
			if(!linesInited){
				uiLines.addVertex(ofVec3f(x, y + spacing * 0.33));
				uiLines.addVertex(ofVec3f(x + realColW, y + spacing * 0.33));
			}
			y += spacing;
			if (y > screenH / uiScale - padding * 0.5 - bottomBarHeight){
				x += colw;
				y = initialY;
			}
		}
		ofSetColor(32);
		ofSetLineWidth(1);
		uiLines.draw();
	}

	//tiny clock top left
	if (uiAlpha < 1.0 && !view.showDuringEdits){
		ofMesh m;
		int step = 20;
		m.setMode(OF_PRIMITIVE_TRIANGLE_FAN);
		ofVec3f origin = ofVec3f(charW * 1.5, lineH * 0.5); //rect is 22
		m.addVertex(origin);
		float r = lineH * 0.4;
		float ang;
		float lim = 360.0f * (1.0f - uiAlpha);
		for(ang = 0; ang < lim; ang += step){
			m.addVertex( origin + ofVec3f( r * cosf((-90.0f + ang) * DEG_TO_RAD),
										  r * sinf((-90.0f + ang) * DEG_TO_RAD)));
		}
		float lastBit = lim - ang;
		m.addVertex( origin + ofVec3f( r * cosf((-90.0f + ang + lastBit) * DEG_TO_RAD),
									  r * sinf((-90.0f + ang + lastBit) * DEG_TO_RAD)));
		ofSetColor(128);
		m.draw();
	}

	ofPopMatrix();

	//clearly draw the corners - helpful when in multi monitor setups to see if we are cropping
	float ts = 7 * uiScale;
	ofSetColor((frameNum * 20)%255);
	ofDrawTriangle(0, 0, ts, 0, 0, ts);
	ofDrawTriangle(screenW / uiScale, 0, screenW / uiScale - ts, 0, screenW / uiScale, ts);
	ofDrawTriangle(0, screenH / uiScale, ts, screenH / uiScale, 0, screenH / uiScale - ts);
	ofDrawTriangle(screenW / uiScale, screenH / uiScale, screenW / uiScale - ts, screenH / uiScale, screenW / uiScale, screenH / uiScale - ts);
}

void ofxRemoteUIBuiltInClient::drawNotifications(const View &view, ofxRemoteUISimpleNotifications &onScreenNotifications, int x, int y){
	#if( defined(USE_OFX_TIME_MEASUREMENTS) && !defined(TIME_MEASUREMENTS_DISABLED))
	ofxTimeMeasurements * tm = TIME_SAMPLE_GET_INSTANCE();
	if (tm->getEnabled()) {
		if (tm->getDrawLocation() == TIME_MEASUREMENTS_BOTTOM_LEFT) {
			float scale = tm->getUiScale();
			ofTranslate(0, scale * (-tm->getHeight() - 10 - tm->getPlotsHeight()));
		}
		if (tm->getDrawLocation() == TIME_MEASUREMENTS_BOTTOM_RIGHT) {
			float scale = tm->getUiScale();
			ofTranslate(0, scale * (-10 - tm->getPlotsHeight()));
		}
	}
	#endif
	for(const auto &name : view.paramsToWatch){
		auto param = view.params.find(name);
		if(param == view.params.end()) continue;
		const RemoteUIParam &p = param->second;
		onScreenNotifications.addParamWatch(name, p.getValueAsStringFromPointer(), ofColor(p.r, p.g, p.b, 255));
	}

	ofColor transp = ofColor(0,0,0,0);
	for(const auto &w : view.varWatches){ // add watches
		onScreenNotifications.addVariableWatch(w.first, w.second.getValueAsString(), (w.second.color == transp) ? ofColor(0, 190) : w.second.color );
	}
	onScreenNotifications.draw(x, y);
}

void ofxRemoteUIBuiltInClient::setColumnWidth(int w){
	if(fabs(uiColumnWidth - w) >= 0.1) clearLayout();
	uiColumnWidth = w;
}

void ofxRemoteUIBuiltInClient::setScale(float s, ofxRemoteUISimpleNotifications &onScreenNotifications){
	if(fabs(s - uiScale) > FLT_EPSILON){
		uiScale = s;
		#ifdef USE_OFX_FONTSTASH
		if(fontFile.size()){ //re-create font with higher uiscale
			drawUiWithFontStash(fontFile, fontSize, onScreenNotifications);
		}
		#endif
		#ifdef USE_OFX_FONTSTASH2
		if(fontStashFile2.size()){ //re-create font with higher uiscale
			drawUiWithFontStash2(fontStashFile2, fontSize2, onScreenNotifications);
		}
		#endif
		clearLayout();
	}
}


void ofxRemoteUIBuiltInClient::setScreenHeight(int h){
	if(customScreenHeight != h) clearLayout();
	customScreenHeight = h;
}


void ofxRemoteUIBuiltInClient::setScreenWidth(int w){
	if(customScreenWidth != w) clearLayout();
	customScreenWidth = w;
}
