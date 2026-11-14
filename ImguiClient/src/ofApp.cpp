#include "ofApp.h"
#include "ofAppImGuiHelpers.h"
#include "imgui.h"
#include "ofxXmlSettings.h"
#include "NativeMacMenu.h"
#include "NativeMacFileOpen.h"
#include "ClientPaths.h"

#include <GLFW/glfw3.h>

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstring>
#include <cstdint>
#include <fstream>
#include <sstream>
#include <thread>

namespace {
using namespace ofAppImGuiHelpers;
ofApp *activeApp = nullptr;

int refreshRateForWindow(GLFWwindow *window) {
	if (!window) return 0;
	int x = 0, y = 0, width = 0, height = 0;
	glfwGetWindowPos(window, &x, &y);
	glfwGetWindowSize(window, &width, &height);
	if (width <= 0 || height <= 0) return 0;
	int count = 0;
	GLFWmonitor **monitors = glfwGetMonitors(&count);
	std::int64_t bestArea = 0;
	int rate = 0;
	for (int i = 0; i < count; ++i) {
		const GLFWvidmode *mode = glfwGetVideoMode(monitors[i]);
		if (!mode || mode->width <= 0 || mode->height <= 0 || mode->refreshRate <= 0) continue;
		int monitorX = 0, monitorY = 0;
		glfwGetMonitorPos(monitors[i], &monitorX, &monitorY);
		const std::int64_t left = std::max<std::int64_t>(x, monitorX);
		const std::int64_t top = std::max<std::int64_t>(y, monitorY);
		const std::int64_t right = std::min<std::int64_t>(std::int64_t(x) + width, std::int64_t(monitorX) + mode->width);
		const std::int64_t bottom = std::min<std::int64_t>(std::int64_t(y) + height, std::int64_t(monitorY) + mode->height);
		const std::int64_t area = std::max<std::int64_t>(0, right - left) * std::max<std::int64_t>(0, bottom - top);
		if (area > bestArea) { bestArea = area; rate = mode->refreshRate; }
	}
	return rate;
}


}

void ofApp::setup() {
	ofSetFrameRate(0); // The client paces frames without openFrameworks' busy-waiting limiter.
	lastActivity = std::chrono::steady_clock::now();
	ofSetEscapeQuitsApp(false);
	ofSetVerticalSync(false);
	ofSetBackgroundAuto(true);
	activeApp = this;
	client.setVerbose(false);
	client.setCallback(&ofApp::clientCallback);
	loadPreferences();
	loadCollapsedGroups();
	if (!gui.setup()) {
		status = "ImGui GLFW/OpenGL initialization failed";
		ofLogError("ofApp") << status;
		return;
	}
	auto *window = static_cast<GLFWwindow *>(ofGetWindowPtr()->getWindowContext());
	// Cocoa clears both minimum dimensions if either is GLFW_DONT_CARE.
	if (window) glfwSetWindowSizeLimits(window, 570, 300, GLFW_DONT_CARE, GLFW_DONT_CARE);
	applyWindowColor(windowColor);
	setAlwaysOnTop(alwaysOnTop);
	controllers.setup();
	#ifdef TARGET_OSX
	NativeMacMenu::setup([this](NativeMacMenu::Command command) {
		switch (command) {
			case NativeMacMenu::Connect: connectTo(host, port); break;
			case NativeMacMenu::Disconnect: disconnectFromServer(); break;
			case NativeMacMenu::Refresh: client.requestCompleteUpdate(); break;
			case NativeMacMenu::Save: client.saveCurrentStateToDefaultXML(); break;
			case NativeMacMenu::RestoreLaunch: client.restoreAllParamsToInitialXML(); break;
			case NativeMacMenu::RestoreDefaults: client.restoreAllParamsToDefaultValues(); break;
			case NativeMacMenu::OpenLocal: choosePresetFile(); break;
			case NativeMacMenu::CopyValues: glfwSetClipboardString(glfwGetCurrentContext(), client.getValuesAsString().c_str()); break;
			case NativeMacMenu::PasteValues: { const char *v = glfwGetClipboardString(glfwGetCurrentContext()); if (v) applyClipboardValues(v); break; }
			case NativeMacMenu::BlinkMidi: blinkBoundParameters(true); break;
			case NativeMacMenu::BlinkUsb: blinkBoundParameters(false); break;
			case NativeMacMenu::ToggleAllGroups: toggleAllGroups(); break;
			case NativeMacMenu::HighlightChanged: highlightChangesSinceLoad = !highlightChangesSinceLoad; savePreferences(); break;
			case NativeMacMenu::ActivityPanel: showLogPanel = !showLogPanel; break;
			case NativeMacMenu::ServerLogPanel: showServerLogPanel = !showServerLogPanel; break;
			case NativeMacMenu::PreferencesPanel: showSettings = !showSettings; break;
			case NativeMacMenu::BindingsPanel: showBindings = true; showAllBindings = true; focusBindings = true; break;
		}
	});
	#endif
	connectTo(host, port);
}

void ofApp::update() {

	const float dt = ofGetLastFrameTime();
	monitorRefreshPoll += dt;
	if (monitorRefreshPoll >= .5f) {
		monitorRefreshPoll = 0.f;
		auto *window = static_cast<GLFWwindow *>(ofGetWindowPtr()->getWindowContext());
		const int reportedRate = refreshRateForWindow(window);
		const int rate = reportedRate > 0 ? reportedRate : 60;
		monitorFrameRate = rate;
	}
	const bool wasConnected = client.isReadyToSend();
	client.update(dt);
	const auto changedParams = client.getChangedParamsList();
	if (!changedParams.empty()) lastActivity = std::chrono::steady_clock::now();
	for (const auto &name : changedParams) {
		client.markParamModifiedNow(name);
		updateChangedParameter(name);
	}
	#ifdef TARGET_OSX
	for (const auto &path : NativeMacFileOpen::takePending()) queuePresetFile(path);
	#endif
	if (receivedFullSync && client.isReadyToSend() && !pendingPresetFiles.empty()) {
		auto files = std::move(pendingPresetFiles);
		pendingPresetFiles.clear();
		for (const auto &path : files) importPresetFile(path);
	}
	if (wasConnected && !client.isReadyToSend()) {
		clearConnectionView();
		if (status != "Disconnected") {
			status = "Disconnected";
			appendLog("Connection lost");
		}
	}
	if (autoDiscovery) {
		client.updateAutoDiscovery(dt);
	}
	if (continuousRefresh && client.ready()) {
		continuousRefreshTimer += dt;
		if (continuousRefreshTimer >= 1.0f) {
			client.requestCompleteUpdate();
			continuousRefreshTimer = 0.0f;
		}
	}
	if (diffHighlightUntil > 0.0f && ofGetElapsedTimef() >= diffHighlightUntil) { diffParams.clear(); diffHighlightUntil = 0.0f; }
	if (boundBlinkUntil > 0.0f && ofGetElapsedTimef() >= boundBlinkUntil) { blinkingBoundParams.clear(); boundBlinkUntil = 0.0f; }
	controllers.update(dt, bindingServer(), [this](const std::string &name, const ControllerBindings::Input &event) { applyControllerInput(name, event); }, [this](const std::string &name, const ControllerBindings::Input &event) {
		return client.isReadyToSend() && client.paramExistsForName(name) && (!event.button || client.getParamForName(name).type == REMOTEUI_PARAM_BOOL);
	});
	#ifdef TARGET_OSX
	NativeMacMenu::update(client.isReadyToSend(), showLogPanel, showServerLogPanel, showSettings, showBindings, highlightChangesSinceLoad);
	#endif
	using Clock = std::chrono::steady_clock;
	const auto now = Clock::now();
	const bool idle = controllers.bindings.empty() && controllers.learning.empty() && now - lastActivity >= std::chrono::seconds(1);
	const int targetRate = idle ? std::min(monitorFrameRate, 15) : monitorFrameRate;
	const auto interval = std::chrono::duration_cast<Clock::duration>(std::chrono::duration<double>(1.0 / targetRate));
	if (targetRate != pacedFrameRate || frameDeadline.time_since_epoch().count() == 0 || now > frameDeadline + interval) {
		pacedFrameRate = targetRate;
		frameDeadline = now;
	}
	std::this_thread::sleep_until(frameDeadline);
	frameDeadline += interval;
}

void ofApp::drawMenuBar() {
	#ifndef TARGET_OSX
	const bool menuConnected = client.isReadyToSend();
	const ImGuiInputFlags menuShortcutFlags = ImGuiInputFlags_RouteGlobal;
	if (ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_K, menuShortcutFlags)) connectTo(host, port);
	if (menuConnected && ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiMod_Shift | ImGuiKey_K, menuShortcutFlags)) disconnectFromServer();
	if (menuConnected && ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiMod_Shift | ImGuiKey_S, menuShortcutFlags)) client.requestCompleteUpdate();
	if (menuConnected && ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_S, menuShortcutFlags)) client.saveCurrentStateToDefaultXML();
	if (menuConnected && ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_R, menuShortcutFlags)) client.restoreAllParamsToInitialXML();
	if (menuConnected && ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiMod_Shift | ImGuiKey_R, menuShortcutFlags)) client.restoreAllParamsToDefaultValues();
	if (ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_O, menuShortcutFlags)) choosePresetFile();
	if (menuConnected && ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiMod_Shift | ImGuiKey_C, menuShortcutFlags)) glfwSetClipboardString(glfwGetCurrentContext(), client.getValuesAsString().c_str());
	if (menuConnected && ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiMod_Shift | ImGuiKey_V, menuShortcutFlags)) { const char *v = glfwGetClipboardString(glfwGetCurrentContext()); if (v) applyClipboardValues(v); }
	if (menuConnected && ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiMod_Shift | ImGuiKey_G, menuShortcutFlags)) toggleAllGroups();
	if (ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_1, menuShortcutFlags)) showLogPanel = !showLogPanel;
	if (ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_2, menuShortcutFlags)) showServerLogPanel = !showServerLogPanel;
	if (ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_3, menuShortcutFlags)) { showBindings = true; showAllBindings = true; focusBindings = true; }
	if (ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_Comma, menuShortcutFlags)) showSettings = !showSettings;
	if (ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_Q, menuShortcutFlags)) ofExit();
	#endif
	#ifndef TARGET_OSX
	const float menuScale = parameterFontScale(rowHeight);
	const ImGuiStyle &menuBaseStyle = ImGui::GetStyle();
	const ImVec4 windowBg = menuBaseStyle.Colors[ImGuiCol_WindowBg];
	const ImVec4 menuBarBg(windowBg.x * .88f, windowBg.y * .88f, windowBg.z * .88f, 1.f);
	const ImVec4 menuPopupBg = menuBaseStyle.Colors[ImGuiCol_PopupBg];
	const float popupLuma = .2126f * menuPopupBg.x + .7152f * menuPopupBg.y + .0722f * menuPopupBg.z;
	const float borderLuma = popupLuma < .5f ? .72f : .24f;
	ImGui::PushFont(nullptr, menuBaseStyle.FontSizeBase * menuScale);
	ImGui::PushStyleColor(ImGuiCol_MenuBarBg, menuBarBg);
	ImGui::PushStyleColor(ImGuiCol_PopupBg, menuPopupBg);
	ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(borderLuma, borderLuma, borderLuma, 1.f));
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2((menuBaseStyle.FramePadding.x + 5.f) * menuScale, (menuBaseStyle.FramePadding.y + 3.f) * menuScale));
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2((menuBaseStyle.ItemSpacing.x + 4.f) * menuScale, menuBaseStyle.ItemSpacing.y * menuScale));
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(menuBaseStyle.WindowPadding.x * menuScale, menuBaseStyle.WindowPadding.y * menuScale));
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.f);
	if (ImGui::BeginMainMenuBar()) {
		if (ImGui::BeginMenu("File")) {
			if (ImGui::MenuItem("Quit", "Ctrl+Q")) ofExit();
			ImGui::EndMenu();
		}
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, std::max(2.f * menuScale, menuBaseStyle.WindowBorderSize * menuScale));
		if (ImGui::BeginMenu("RemoteUI")) {
			if (ImGui::MenuItem("Connect", "Ctrl+K")) connectTo(host, port);
			if (ImGui::MenuItem("Disconnect", "Ctrl+Shift+K", false, menuConnected)) disconnectFromServer();
			ImGui::Separator();
			if (ImGui::MenuItem("Refresh Parameters", "Ctrl+Shift+S", false, menuConnected)) client.requestCompleteUpdate();
			if (ImGui::MenuItem("Save Current Values to Server", "Ctrl+S", false, menuConnected)) client.saveCurrentStateToDefaultXML();
			if (ImGui::MenuItem("Restore Launch Values", "Ctrl+R", false, menuConnected)) client.restoreAllParamsToInitialXML();
			if (ImGui::MenuItem("Restore Code Defaults", "Ctrl+Shift+R", false, menuConnected)) client.restoreAllParamsToDefaultValues();
			ImGui::Separator();
			if (ImGui::MenuItem("Open Local Preset...", "Ctrl+O")) choosePresetFile();
			ImGui::Separator();
			if (ImGui::MenuItem("Copy Parameter Values", "Ctrl+Shift+C", false, menuConnected)) glfwSetClipboardString(glfwGetCurrentContext(), client.getValuesAsString().c_str());
			if (ImGui::MenuItem("Paste Parameter Values", "Ctrl+Shift+V", false, menuConnected)) { const char *v = glfwGetClipboardString(glfwGetCurrentContext()); if (v) applyClipboardValues(v); }
			if (ImGui::MenuItem("Highlight Changes Since Connection", nullptr, &highlightChangesSinceLoad, menuConnected)) savePreferences();
			if (ImGui::MenuItem("Collapse / Expand All Groups", "Ctrl+Shift+G", false, menuConnected)) toggleAllGroups();
			ImGui::Separator();
			if (ImGui::MenuItem("Blink MIDI-Bound Parameters", nullptr, false, menuConnected)) blinkBoundParameters(true);
			if (ImGui::MenuItem("Blink USB Controller-Bound Parameters", nullptr, false, menuConnected)) blinkBoundParameters(false);
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("Windows")) {
			ImGui::MenuItem("Action Log", "Ctrl+1", &showLogPanel);
			ImGui::MenuItem("Server Log", "Ctrl+2", &showServerLogPanel);
			if (ImGui::MenuItem("Controller Bindings", "Ctrl+3")) { showBindings = true; showAllBindings = true; focusBindings = true; }
			ImGui::MenuItem("Settings", "Ctrl+,", &showSettings);
			ImGui::EndMenu();
		}
		ImGui::PopStyleVar();
		ImGui::EndMainMenuBar();
	}
	ImGui::PopStyleVar(4);
	ImGui::PopStyleColor(3);
	ImGui::PopFont();
	#endif
}

void ofApp::draw() {
	if (!gui.isSetup()) return;
	gui.begin();
	const ImGuiIO &input = ImGui::GetIO();
	bool interacting = input.MouseDelta.x != 0.f || input.MouseDelta.y != 0.f || input.MouseWheel != 0.f || input.MouseWheelH != 0.f || ImGui::IsAnyItemActive();
	for (bool down : input.MouseDown) interacting |= down;
	for (int key = ImGuiKey_Tab; !interacting && key < ImGuiKey_MouseLeft; ++key) interacting = ImGui::IsKeyDown(static_cast<ImGuiKey>(key));
	if (interacting) lastActivity = std::chrono::steady_clock::now();
	if (!ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
		activeMouseSlider.clear();
		fineSliderTracking = false;
	}
	if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_DockingEnable) ImGui::DockSpaceOverViewport();
	drawMenuBar();
	drawParameterPanel();
	if (showLogPanel) drawLogPanel();
	if (showServerLogPanel) drawServerLogPanel();
	if (showSettings) drawSettingsPanel();
	if (showStyleEditor) {
		ImGui::SetNextWindowSize(ImVec2(500, 600), ImGuiCond_FirstUseEver);
		prepareSecondaryWindow();
		if (ImGui::Begin("ImGui Style Editor", &showStyleEditor, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoDocking)) {
			const ImVec4 before = ImGui::GetStyle().Colors[ImGuiCol_WindowBg];
			ImGui::ShowStyleEditor();
			const ImVec4 after = ImGui::GetStyle().Colors[ImGuiCol_WindowBg];
			if (before.x != after.x || before.y != after.y || before.z != after.z || before.w != after.w) {
				windowColor[0] = after.x; windowColor[1] = after.y; windowColor[2] = after.z; windowColor[3] = after.w;
				applyWindowColor(windowColor);
			}
		}
		ImGui::End();
	}
	if (showBindings) drawBindingsPanel();
	ImGuiViewport *viewport = ImGui::GetMainViewport();
	const float statusScale = parameterFontScale(rowHeight);
	const float padding = std::round(10.f * statusScale);
	const float fontSize = std::max(1.f, std::round(ImGui::GetStyle().FontSizeBase * statusScale * .65f));
	const std::string statusText = std::to_string(client.getAllParamNamesList().size()) + " params";

	ImFont *font = ImGui::GetFont();
	const ImVec2 textSize = font->CalcTextSizeA(fontSize, 100000.f, 0.f, statusText.c_str());
	ImVec2 position(std::round(viewport->WorkPos.x + viewport->WorkSize.x - padding - textSize.x),
		std::round(viewport->WorkPos.y + viewport->WorkSize.y - padding - textSize.y));
	ImGui::PushStyleVar(ImGuiStyleVar_Alpha, .3f);

	if (client.isReadyToSend()) {
		ImGui::GetForegroundDrawList(viewport)->AddText(font, fontSize, position,
														ImGui::GetColorU32(ImGuiCol_Text), statusText.c_str());
	}

	//draw fps
	string fps = "fps: " + ofToString(ofGetFrameRate(), 0);
	position.x -= viewport->WorkSize.x - textSize.x - 22;
	ImGui::GetForegroundDrawList(viewport)->AddText(font, fontSize, position, ImGui::GetColorU32(ImGuiCol_Text), fps.c_str());
	ImGui::PopStyleVar();

	gui.end();
}

void ofApp::exit() {
	controllers.shutdown();
	#ifdef TARGET_OSX
	NativeMacFileOpen::shutdown();
	NativeMacMenu::shutdown();
	#endif
	client.disconnect();
	gui.shutdown();
	savePreferences();
	if (activeApp == this) activeApp = nullptr;
}

void ofApp::clientCallback(RemoteUIClientCallBackArg arg) {
	if (activeApp) activeApp->onClientEvent(arg);
}

void ofApp::onClientEvent(const RemoteUIClientCallBackArg &arg) {
	if (arg.action == SERVER_SENT_LOG_LINE) {
		serverLogLines.push_back(ofGetTimestampString("%H:%M:%S") + " >> " + arg.msg);
		if (serverLogLines.size() > 300) serverLogLines.erase(serverLogLines.begin(), serverLogLines.begin() + 50);
		return;
	}
	std::ostringstream line;
	line << (arg.host.empty() ? host : arg.host) << ":" << port << " ";
	switch (arg.action) {
		case SERVER_CONNECTED: status = "Connected"; line << "Connected"; break;
		case SERVER_DISCONNECTED: clearConnectionView(); status = "Disconnected"; line << "Disconnected"; break;
		case SERVER_SENT_FULL_PARAMS_UPDATE:
			if (!receivedFullSync) captureChangeBaseline();
			receivedFullSync = true;
			lastSyncCount = client.getAllParamNamesList().size();
			lastSyncTime = ofGetElapsedTimef();
			line << "Parameter list synchronized (" << lastSyncCount << " parameters)";
			ofLogNotice("RemoteUIImGui") << "Full sync from " << host << ":" << port << ": " << lastSyncCount << " entries";
			status = lastSyncCount == 0 ? "Connected · sync completed with zero parameters" : "Connected";
			if (presetReplyPending) {
				diffParams.clear();
				if (highlightPresetDiff) for (const auto &kv : previousParams) if (client.paramExistsForName(kv.first) && !client.getParamForName(kv.first).isEqualTo(kv.second)) diffParams.insert(kv.first);
				diffHighlightUntil = ofGetElapsedTimef() + 5.f;
				presetReplyPending = false;
			}
			break;
		case SERVER_PRESETS_LIST_UPDATED: line << "Preset list updated"; break;
		case SERVER_REPORTS_MISSING_PARAMS_IN_PRESET:
			lastPresetWarning = arg.msg + " is missing " + ofToString(arg.paramList.size()) + " parameter(s)";
			line << lastPresetWarning;
			break;
		case SERVER_SAVED_PRESET: selectedPreset = arg.msg; line << "Saved preset: " << arg.msg; break;
		case SERVER_DELETED_PRESET: if (selectedPreset == arg.msg) selectedPreset.clear(); line << "Deleted preset: " << arg.msg; break;
		case SERVER_DID_SET_PRESET: line << "Loaded preset: " << arg.msg; break;
		case SERVER_SAVED_GROUP_PRESET: groupPresetSelection[arg.group] = arg.msg; line << "Saved " << arg.group << " preset: " << arg.msg; break;
		case SERVER_DELETED_GROUP_PRESET:
			if (groupPresetSelection[arg.group] == arg.msg) groupPresetSelection[arg.group].clear();
			line << "Deleted " << arg.group << " preset: " << arg.msg; break;
		case SERVER_DID_SET_GROUP_PRESET: line << "Loaded " << arg.group << " preset: " << arg.msg; break;
		case SERVER_CONFIRMED_SAVE:
			captureChangeBaseline();
			line << "Saved current values to server";
			break;
		case SERVER_DID_RESET_TO_XML: line << "Restored launch values"; break;
		case SERVER_DID_RESET_TO_DEFAULTS: line << "Restored code defaults"; break;
		case NEIGHBORS_UPDATED: line << "Server discovery updated"; break;
		case NEIGHBOR_JUST_LAUNCHED_SERVER:
			line << "Server announced: " << arg.msg;
			if (autoConnectToNewServers && !client.isReadyToSend()) {
				for (const auto &neighbor : client.getNeighbors()) if (neighbor.IP == arg.host && neighbor.port == arg.port && (!onlyAutoConnectToLocalHost || isLocalHost(neighbor.IP))) { selectNeighbor(neighbor); break; }
			}
			break;
		case SERVER_ASKED_TO_REMOVE_PARAM: changedSinceLoad.erase(arg.msg); line << "Parameter removed: " << arg.msg; break;
		default: line << "RemoteUI event " << (int)arg.action; break;
	}
	if (arg.action != SERVER_SENT_FULL_PARAMS_UPDATE && arg.action != SERVER_PRESETS_LIST_UPDATED &&
		arg.action != NEIGHBORS_UPDATED && arg.action != NEIGHBOR_JUST_LAUNCHED_SERVER)
		appendLog(line.str());
}

void ofApp::appendLog(const std::string &line) {
	logLines.push_back(ofGetTimestampString("%H:%M:%S") + "  " + line);
	if (logLines.size() > 300) logLines.erase(logLines.begin(), logLines.begin() + 50);
	if (autoSavePreferences) savePreferences();
}

void ofApp::connectTo(const std::string &newHost, int newPort) {
	client.disconnect();
	serverLogLines.push_back(ofGetTimestampString("%H:%M:%S") + " >> ## CLIENT CONNECTING ###################");
	host = newHost;
	port = ofClamp(newPort, 1000, 65534);
	clearConnectionView();
	status = "Connecting…";
	std::strncpy(hostBuffer, host.c_str(), sizeof(hostBuffer) - 1);
	hostBuffer[sizeof(hostBuffer) - 1] = '\0';
	if (!client.setup(host, port)) {
		status = "Could not open OSC client; check reply port " + ofToString(port + 1);
		appendLog(status);
		return;
	}
	client.connect();
	savePreferences();
	appendLog("Connecting to " + host + ":" + ofToString(port));
}

void ofApp::disconnectFromServer() {
	client.disconnect();
	serverLogLines.push_back(ofGetTimestampString("%H:%M:%S") + " >> ## CLIENT DISCONNECTED ###################");
	status = "Disconnected";
	clearConnectionView();
}

void ofApp::clearConnectionView() {
	controllers.learning.clear();
	receivedFullSync = false;
	lastSyncCount = 0;
	lastSyncTime = -1.0f;
	selectedPreset.clear();
	groupPresetSelection.clear();
	previousParams.clear();
	changeBaseline.clear();
	changedSinceLoad.clear();
	diffParams.clear();
	diffHighlightUntil = 0.0f;
	blinkingBoundParams.clear();
	boundBlinkUntil = 0.0f;
	groupToggleTimes.clear();
	groupContentHeights.clear();
	presetReplyPending = false;
}
