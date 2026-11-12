#pragma once

#include "ofMain.h"
#include "ofxRemoteUIClient.h"
#include "ofxRemoteUIImGui.h"
#include "ControllerBindings.h"

#include <unordered_map>
#include <unordered_set>

class ofApp : public ofBaseApp {
public:
	void setup() override;
	void update() override;
	void draw() override;
	void exit() override;
	void dragEvent(ofDragInfo dragInfo) override;

private:
	static void clientCallback(RemoteUIClientCallBackArg arg);
	void onClientEvent(const RemoteUIClientCallBackArg &arg);
	void drawParameterPanel();
	void drawLogPanel();
	void drawServerLogPanel();
	void drawSettingsPanel();
	void drawBindingsPanel();
	void applyControllerInput(const std::string &name, const ControllerBindings::Input &event);
	std::string bindingServer() const;
	void drawParameter(const std::string &name, float groupLabelWidth);
	void drawGroupHeader(const std::string &name, const RemoteUIParam &param);
	void connectTo(const std::string &host, int port);
	void disconnectFromServer();
	void clearConnectionView();
	void sendEditedParam(const std::string &name, RemoteUIParam param);
	void appendLog(const std::string &line);
	void savePreferences() const;
	void loadPreferences();
	void importPresetFile(const std::string &path);
	void queuePresetFile(const std::string &path);
	void choosePresetFile();
	void rememberCurrentValuesForDiff();
	void applyPreset(const std::string &name);
	void applyGroupPreset(const std::string &name, const std::string &group);
	void applyClipboardValues(const std::string &values, bool highlightChanges = false);
	void selectNeighbor(const Neighbor &neighbor);
	void setAlwaysOnTop(bool enabled);
	bool isLocalHost(const std::string &host) const;

	ofxRemoteUIClient client;
	ofxRemoteUIImGui gui;
	ControllerBindings controllers;

	std::string host = "127.0.0.1";
	int port = OFXREMOTEUI_PORT;
	char hostBuffer[256] = "127.0.0.1";
	char filterBuffer[256] = "";
	char presetNameBuffer[128] = "";
	char groupPresetNameBuffer[128] = "";
	char clipboardBuffer[64 * 1024] = "";

	bool autoDiscovery = true;
	bool autoConnectToNewServers = false;
	bool onlyAutoConnectToLocalHost = true;
	bool continuousRefresh = false;
	bool showOnlyChanged = false;
	bool alwaysOnTop = false;
	bool showSettings = false;
	bool showStyleEditor = false;
	bool showImGuiDemo = false;
	bool showBindings = false;
	bool focusBindings = false;
	bool showAllBindings = true;
	bool bindingPopupRequested = false;
	bool bindingPopupActive = false;
	float bindingPopupX = 0.f;
	float bindingPopupY = 0.f;
	std::string bindingParam;
	bool highlightPresetDiff = true;
	float windowColor[4] = {0.06f, 0.06f, 0.06f, 1.f};
	bool showLogPanel = false;
	bool showServerLogPanel = false;
	bool autoScrollLog = true;
	bool receivedFullSync = false;
	std::size_t lastSyncCount = 0;
	float lastSyncTime = -1.0f;
	std::string selectedGroup;
	std::string selectedPreset;
	std::string status = "Disconnected";
	std::string lastPresetWarning;
	std::vector<std::string> logLines;
	std::vector<std::string> serverLogLines;
	std::vector<std::string> pendingPresetFiles;
	std::unordered_map<std::string, std::string> groupPresetSelection;
	std::unordered_map<std::string, bool> collapsedGroups;
	std::unordered_map<std::string, RemoteUIParam> previousParams;
	std::unordered_set<std::string> diffParams;
	float diffHighlightUntil = 0.0f;
	float continuousRefreshTimer = 0.0f;
	float lastLag = -1.0f;
	bool presetReplyPending = false;
	bool autoSavePreferences = true;
	float rowHeight = 0.0f;
	float columnBaseWidth = 300.0f;
};
