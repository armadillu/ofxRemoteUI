#include "ofApp.h"
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
ofApp *activeApp = nullptr;
constexpr float groupAnimationDuration = .15f;

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

bool sameParameterValue(const RemoteUIParam &a, const RemoteUIParam &b) {
	if (a.type != b.type) return false;
	switch (a.type) {
		case REMOTEUI_PARAM_FLOAT: return a.floatVal == b.floatVal;
		case REMOTEUI_PARAM_INT:
		case REMOTEUI_PARAM_ENUM: return a.intVal == b.intVal;
		case REMOTEUI_PARAM_BOOL: return a.boolVal == b.boolVal;
		case REMOTEUI_PARAM_STRING: return a.stringVal == b.stringVal;
		case REMOTEUI_PARAM_COLOR: return a.redVal == b.redVal && a.greenVal == b.greenVal && a.blueVal == b.blueVal && a.alphaVal == b.alphaVal;
		default: return true;
	}
}

bool copyParameterValue(RemoteUIParam &target, const RemoteUIParam &source) {
	if (target.type != source.type) return false;
	switch (target.type) {
		case REMOTEUI_PARAM_FLOAT: target.floatVal = source.floatVal; break;
		case REMOTEUI_PARAM_INT:
		case REMOTEUI_PARAM_ENUM: target.intVal = source.intVal; break;
		case REMOTEUI_PARAM_BOOL: target.boolVal = source.boolVal; break;
		case REMOTEUI_PARAM_STRING: target.stringVal = source.stringVal; break;
		case REMOTEUI_PARAM_COLOR:
			target.redVal = source.redVal; target.greenVal = source.greenVal;
			target.blueVal = source.blueVal; target.alphaVal = source.alphaVal; break;
		default: return false;
	}
	return true;
}

float parameterFontScale(float rowHeight) {
	// Keep the row-size control relative to the client's 17px reference font.
	return rowHeight > 0.f ? std::max(10.f, rowHeight - 2.f) / 17.f : 1.f;
}

bool framedCheckbox(const char *label, bool *value) {
	ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.f);
	const bool changed = ImGui::Checkbox(label, value);
	ImGui::PopStyleVar();
	return changed;
}

void prepareSecondaryWindow() {
	static const ImGuiWindowClass windowClass = [] {
		ImGuiWindowClass result;
		result.ViewportFlagsOverrideSet = ImGuiViewportFlags_NoAutoMerge;
		return result;
	}();
	ImGui::SetNextWindowClass(&windowClass);
}

void applyWindowColor(const float color[4]) {
	ImGuiStyle &style = ImGui::GetStyle();
	const auto linearChannel = [](float channel) {
		channel = std::clamp(channel, 0.f, 1.f);
		return channel <= .04045f ? channel / 12.92f : std::pow((channel + .055f) / 1.055f, 2.4f);
	};
	const float luminance = .2126f * linearChannel(color[0]) + .7152f * linearChannel(color[1]) + .0722f * linearChannel(color[2]);
	const bool lightWindow = luminance > .179f;
	float hue, saturation, value;
	ImGui::ColorConvertRGBtoHSV(color[0], color[1], color[2], hue, saturation, value);
	const auto tone = [&](float brightness, float alpha = 1.f) {
		if (lightWindow) brightness = 1.f - brightness;
		float r, g, b;
		ImGui::ColorConvertHSVtoRGB(hue, saturation * .8f, brightness, r, g, b);
		return ImVec4(r, g, b, alpha);
	};
	style.Colors[ImGuiCol_WindowBg] = ImVec4(color[0], color[1], color[2], color[3]);
	style.Colors[ImGuiCol_Text] = lightWindow ? ImVec4(.06f, .06f, .06f, 1.f) : ImVec4(.94f, .94f, .94f, 1.f);
	style.Colors[ImGuiCol_TextDisabled] = lightWindow ? ImVec4(.30f, .30f, .30f, 1.f) : ImVec4(.68f, .68f, .68f, 1.f);
	style.Colors[ImGuiCol_PopupBg] = tone(.16f, .98f);
	style.Colors[ImGuiCol_FrameBg] = tone(.23f);
	style.Colors[ImGuiCol_FrameBgHovered] = tone(.34f);
	style.Colors[ImGuiCol_FrameBgActive] = tone(.40f);
	style.Colors[ImGuiCol_TitleBg] = tone(.18f);
	style.Colors[ImGuiCol_TitleBgActive] = tone(.28f);
	style.Colors[ImGuiCol_TitleBgCollapsed] = tone(.18f);
	style.Colors[ImGuiCol_MenuBarBg] = tone(.22f);
	style.Colors[ImGuiCol_ScrollbarBg] = tone(.10f, .5f);
	style.Colors[ImGuiCol_ScrollbarGrab] = tone(.38f);
	style.Colors[ImGuiCol_ScrollbarGrabHovered] = tone(.50f);
	style.Colors[ImGuiCol_ScrollbarGrabActive] = tone(.62f);
	style.Colors[ImGuiCol_CheckMark] = tone(.98f);
	style.Colors[ImGuiCol_CheckboxSelectedBg] = tone(.32f);
	style.Colors[ImGuiCol_SliderGrab] = tone(.55f);
	style.Colors[ImGuiCol_SliderGrabActive] = tone(.72f);
	style.Colors[ImGuiCol_Button] = tone(.29f);
	style.Colors[ImGuiCol_ButtonHovered] = tone(.40f);
	style.Colors[ImGuiCol_ButtonActive] = tone(.48f);
	style.Colors[ImGuiCol_Header] = tone(.30f);
	style.Colors[ImGuiCol_HeaderHovered] = tone(.40f);
	style.Colors[ImGuiCol_HeaderActive] = tone(.48f);
	style.Colors[ImGuiCol_Separator] = tone(.42f, .7f);
	style.Colors[ImGuiCol_SeparatorHovered] = tone(.60f);
	style.Colors[ImGuiCol_SeparatorActive] = tone(.75f);
	style.Colors[ImGuiCol_ResizeGrip] = tone(.55f, .4f);
	style.Colors[ImGuiCol_ResizeGripHovered] = tone(.65f, .75f);
	style.Colors[ImGuiCol_ResizeGripActive] = tone(.80f);
	style.Colors[ImGuiCol_InputTextCursor] = tone(.98f);
	style.Colors[ImGuiCol_Tab] = tone(.24f);
	style.Colors[ImGuiCol_TabHovered] = tone(.40f);
	style.Colors[ImGuiCol_TabSelected] = tone(.34f);
	style.Colors[ImGuiCol_TabSelectedOverline] = tone(.75f);
	style.Colors[ImGuiCol_TabDimmed] = tone(.18f);
	style.Colors[ImGuiCol_TabDimmedSelected] = tone(.26f);
	style.Colors[ImGuiCol_TabDimmedSelectedOverline] = tone(.50f);
	style.Colors[ImGuiCol_DockingPreview] = tone(.70f, .6f);
	style.Colors[ImGuiCol_DockingEmptyBg] = tone(.14f);
	style.Colors[ImGuiCol_PlotLines] = tone(.70f);
	style.Colors[ImGuiCol_PlotLinesHovered] = tone(.95f);
	style.Colors[ImGuiCol_PlotHistogram] = tone(.70f);
	style.Colors[ImGuiCol_PlotHistogramHovered] = tone(.95f);
	style.Colors[ImGuiCol_TableHeaderBg] = tone(.25f);
	style.Colors[ImGuiCol_TableBorderStrong] = tone(.42f);
	style.Colors[ImGuiCol_TableBorderLight] = tone(.32f);
	style.Colors[ImGuiCol_TableRowBgAlt] = tone(.45f, .12f);
	style.Colors[ImGuiCol_TextLink] = tone(.90f);
	style.Colors[ImGuiCol_TextSelectedBg] = tone(.65f, .55f);
	style.Colors[ImGuiCol_TreeLines] = tone(.45f);
	style.Colors[ImGuiCol_DragDropTarget] = tone(.90f);
	style.Colors[ImGuiCol_DragDropTargetBg] = tone(.65f, .25f);
	style.Colors[ImGuiCol_UnsavedMarker] = tone(.90f);
	style.Colors[ImGuiCol_NavCursor] = tone(.85f);
}

const char *paramTypeName(RemoteUIParamType type) {
	switch (type) {
		case REMOTEUI_PARAM_FLOAT: return "Float";
		case REMOTEUI_PARAM_INT: return "Int";
		case REMOTEUI_PARAM_BOOL: return "Bool";
		case REMOTEUI_PARAM_STRING: return "String";
		case REMOTEUI_PARAM_ENUM: return "Enum";
		case REMOTEUI_PARAM_COLOR: return "Color";
		case REMOTEUI_PARAM_SPACER: return "Group";
		default: return "Unknown";
	}
}

bool containsCaseInsensitive(const std::string &text, const std::string &query) {
	if (query.empty()) return true;
	std::string a = text, b = query;
	std::transform(a.begin(), a.end(), a.begin(), [](unsigned char c) { return (char)std::tolower(c); });
	std::transform(b.begin(), b.end(), b.begin(), [](unsigned char c) { return (char)std::tolower(c); });
	return a.find(b) != std::string::npos;
}
}

void ofApp::setup() {
	ofSetFrameRate(0); // The client paces frames without openFrameworks' busy-waiting limiter.
	lastActivity = std::chrono::steady_clock::now();
	ofSetEscapeQuitsApp(false);
	ofSetVerticalSync(true);
	ofSetBackgroundColor(27, 29, 34);
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
	for (const auto &name : changedParams) updateChangedParameter(name);
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
	const int targetRate = idle ? std::min(monitorFrameRate, 30) : monitorFrameRate;
	const auto interval = std::chrono::duration_cast<Clock::duration>(std::chrono::duration<double>(1.0 / targetRate));
	if (targetRate != pacedFrameRate || frameDeadline.time_since_epoch().count() == 0 || now > frameDeadline + interval) {
		pacedFrameRate = targetRate;
		frameDeadline = now;
	}
	std::this_thread::sleep_until(frameDeadline);
	frameDeadline += interval;
}

void ofApp::draw() {
	if (!gui.isSetup()) return;
	gui.begin();
	const ImGuiIO &input = ImGui::GetIO();
	bool interacting = input.MouseDelta.x != 0.f || input.MouseDelta.y != 0.f || input.MouseWheel != 0.f || input.MouseWheelH != 0.f || ImGui::IsAnyItemActive();
	for (bool down : input.MouseDown) interacting |= down;
	for (int key = ImGuiKey_Tab; !interacting && key < ImGuiKey_MouseLeft; ++key) interacting = ImGui::IsKeyDown(static_cast<ImGuiKey>(key));
	if (interacting) lastActivity = std::chrono::steady_clock::now();
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
	if (!ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
		activeMouseSlider.clear();
		fineSliderTracking = false;
	}
	if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_DockingEnable) ImGui::DockSpaceOverViewport();
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
	if (client.isReadyToSend()) {
		ImGuiViewport *viewport = ImGui::GetMainViewport();
		const float statusScale = parameterFontScale(rowHeight);
		const float padding = std::round(10.f * statusScale);
		const float fontSize = std::max(1.f, std::round(ImGui::GetStyle().FontSizeBase * statusScale * .8f));
		const std::string statusText = "Connected with " + std::to_string(client.getAllParamNamesList().size()) + " parameters.";
		ImFont *font = ImGui::GetFont();
		const ImVec2 textSize = font->CalcTextSizeA(fontSize, 100000.f, 0.f, statusText.c_str());
		const ImVec2 position(std::round(viewport->WorkPos.x + viewport->WorkSize.x - padding - textSize.x),
			std::round(viewport->WorkPos.y + viewport->WorkSize.y - padding - textSize.y));
		ImGui::PushStyleVar(ImGuiStyleVar_Alpha, .4f);
		ImGui::GetForegroundDrawList(viewport)->AddText(font, fontSize, position,
			ImGui::GetColorU32(ImGuiCol_Text), statusText.c_str());
		ImGui::PopStyleVar();
	}
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

void ofApp::drawParameterPanel() {
	const ImGuiViewport *viewport = ImGui::GetMainViewport();
	ImGui::SetNextWindowPos(viewport->WorkPos);
	ImGui::SetNextWindowSize(viewport->WorkSize);
	if (!ImGui::Begin("Parameters", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoDocking)) { ImGui::End(); return; }
	auto names = client.isReadyToSend() ? client.getAllParamNamesList() : std::vector<std::string>{};
	// Presets and parameter filtering occupy the top bar.
	const float topScale = parameterFontScale(rowHeight);
	ImGui::PushFont(nullptr, ImGui::GetStyle().FontSizeBase * topScale);
	const ImVec2 windowPadding = ImGui::GetStyle().WindowPadding;
	const ImVec2 framePadding = ImGui::GetStyle().FramePadding;
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(windowPadding.x * topScale, windowPadding.y * topScale));
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(framePadding.x * topScale, framePadding.y * topScale));
	const float topSectionGap = 10.f * topScale;
	const float topSectionWidth = std::max(1.f, (ImGui::GetContentRegionAvail().x - topSectionGap) * .5f);
	const float topSectionHeight = ImGui::GetTextLineHeight() + ImGui::GetFrameHeight() + ImGui::GetStyle().ItemSpacing.y + ImGui::GetStyle().WindowPadding.y * 2.f;
	ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.f, 0.f, 0.f, .16f));
	ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 5.f);
	ImGui::BeginChild("preset-controls", ImVec2(topSectionWidth, topSectionHeight), ImGuiChildFlags_AlwaysUseWindowPadding);
	ImGui::TextUnformatted("Global Presets");
	const float presetButtonWidths = ImGui::CalcTextSize("+").x + ImGui::CalcTextSize("-").x + 4.f * ImGui::GetStyle().FramePadding.x;
	ImGui::SetNextItemWidth(std::max(1.f, ImGui::GetContentRegionAvail().x - presetButtonWidths - 2.f * ImGui::GetStyle().ItemSpacing.x));
	const auto presets = client.getPresetsList();
	ImGui::SetNextWindowSizeConstraints(ImVec2(0.f, 0.f), ImVec2(FLT_MAX, viewport->WorkSize.y * (2.f / 3.f)));
	if (ImGui::BeginCombo("##preset", selectedPreset.empty() ? "No preset" : selectedPreset.c_str(), ImGuiComboFlags_HeightLargest)) {
		for (const auto &preset : presets) if (preset != OFXREMOTEUI_NO_PRESETS && preset.find('/') == std::string::npos && ImGui::Selectable(preset.c_str(), selectedPreset == preset)) applyPreset(preset);
		ImGui::EndCombo();
	}
	ImGui::SameLine();
	if (!client.isReadyToSend()) ImGui::BeginDisabled();
	if (ImGui::Button("+##global")) { presetNameBuffer[0] = '\0'; ImGui::OpenPopup("save-global-preset"); }
	if (ImGui::BeginPopup("save-global-preset")) {
		ImGui::TextUnformatted("Save global preset");
		ImGui::SetNextItemWidth(200.f * topScale);
		if (ImGui::IsWindowAppearing()) ImGui::SetKeyboardFocusHere();
		const bool submit = ImGui::InputText("##global-preset-name", presetNameBuffer, sizeof(presetNameBuffer), ImGuiInputTextFlags_EnterReturnsTrue);
		if ((ImGui::Button("Save") || submit) && presetNameBuffer[0]) { client.savePresetWithName(presetNameBuffer); ImGui::CloseCurrentPopup(); }
		ImGui::SameLine(); if (ImGui::Button("Cancel")) ImGui::CloseCurrentPopup();
		ImGui::EndPopup();
	}
	ImGui::SameLine();
	if (selectedPreset.empty()) ImGui::BeginDisabled();
	if (ImGui::Button("-##global")) client.deletePreset(selectedPreset);
	if (selectedPreset.empty()) ImGui::EndDisabled();
	if (!client.isReadyToSend()) ImGui::EndDisabled();
	ImGui::EndChild();
	ImGui::SameLine(0.f, topSectionGap);
	ImGui::BeginChild("group-filter-controls", ImVec2(0, topSectionHeight), ImGuiChildFlags_AlwaysUseWindowPadding);
	ImGui::TextUnformatted("Filter Parameters");
	const float clearWidth = ImGui::CalcTextSize("Clear").x + ImGui::GetStyle().FramePadding.x * 2.f;
	ImGui::SetNextItemWidth(std::max(1.f, ImGui::GetContentRegionAvail().x - clearWidth - ImGui::GetStyle().ItemSpacing.x));
	ImGui::InputTextWithHint("##filter", "Filter parameters...", filterBuffer, sizeof(filterBuffer));
	ImGui::SameLine();
	if (ImGui::Button("Clear")) filterBuffer[0] = '\0';
	ImGui::EndChild();
	ImGui::PopStyleVar();
	ImGui::PopStyleColor();
	ImGui::PopStyleVar(2);
	ImGui::PopFont();
	if (highlightChangesSinceLoad && !changedSinceLoad.empty())
		changedHighlightPulse = .5f + .5f * std::sin(ofGetElapsedTimef() * 2.f * 3.14159265f);
	const auto &collapsedGroups = collapsedGroupsByServer[bindingServer()];
	const float groupAnimationNow = ofGetElapsedTimef();
	for (auto it = groupToggleTimes.begin(); it != groupToggleTimes.end();) {
		if (groupAnimationNow - it->second >= groupAnimationDuration) it = groupToggleTimes.erase(it);
		else ++it;
	}
	std::vector<std::string> visible;
	for (const auto &name : names) {
		const auto p = client.getParamForName(name);
		if (p.type == REMOTEUI_PARAM_SPACER) {
			bool matches = containsCaseInsensitive(p.stringVal, filterBuffer);
			if (!matches) for (const auto &candidate : names) {
				const auto item = client.getParamForName(candidate);
				if (item.type != REMOTEUI_PARAM_SPACER && item.group == p.group && (containsCaseInsensitive(candidate, filterBuffer) || containsCaseInsensitive(item.description, filterBuffer))) { matches = true; break; }
			}
			if (matches) visible.push_back(name);
		} else if ((!collapsedGroups.count(p.group) || groupToggleTimes.count(p.group)) && (containsCaseInsensitive(name, filterBuffer) || containsCaseInsensitive(p.description, filterBuffer))) visible.push_back(name);
	}
	const float footerScale = topScale;
	ImGui::PushFont(nullptr, ImGui::GetStyle().FontSizeBase * footerScale);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(windowPadding.x * footerScale, windowPadding.y * footerScale));
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(framePadding.x * footerScale, framePadding.y * footerScale));
	const float footerWidth = std::max(1.f, ImGui::GetContentRegionAvail().x - ImGui::GetStyle().WindowPadding.x * 2.f);
	const float footerSpacing = ImGui::GetStyle().ItemSpacing.x * 2;
	auto footerButtonWidth = [](const char *label) { return ImGui::CalcTextSize(label).x + ImGui::GetStyle().FramePadding.x * 2.f; };
	const float hostWidth = std::max(1.f, std::min(110.f * footerScale, footerWidth - ImGui::CalcTextSize("Host:").x - footerSpacing));
	const float portWidth = std::max(1.f, std::min(55.f * footerScale, footerWidth - ImGui::CalcTextSize("Port:").x - footerSpacing));
	const float hostPortGap = 10.f * footerScale;
	const std::string pendingText = pendingPresetFiles.empty() ? "" : ofToString(pendingPresetFiles.size()) + " preset file(s) waiting for sync";
	const auto neighbors = client.getNeighbors();
	std::string serverPreview = "Servers: (" + ofToString(neighbors.size()) + ")";
	if (client.isReadyToSend()) {
		std::string connectedServer = host + ":" + ofToString(port);
		for (const auto &neighbor : neighbors) {
			if (neighbor.IP == host && neighbor.port == port) {
				if (!neighbor.binary.empty()) connectedServer = neighbor.binary;
				else if (!neighbor.name.empty()) connectedServer = neighbor.name;
				break;
			}
		}
		serverPreview += "  \xC2\xB7  " + connectedServer;
	}
	const float dividerWidth = 12.f * footerScale;
	const float footerRowGap = 6.f * footerScale;
	const float syncContinuousGap = 10.f * footerScale;
	const float connectButtonWidth = std::max(footerButtonWidth("Connect"), footerButtonWidth("Disconnect"));
	const float syncControlsWidth = dividerWidth + footerSpacing + footerButtonWidth("Sync") + syncContinuousGap + ImGui::GetFrameHeight() + ImGui::GetStyle().ItemInnerSpacing.x + ImGui::CalcTextSize("Non-Stop").x;
	const std::vector<float> connectionWidths = {
		ImGui::CalcTextSize("Host:").x + footerSpacing + hostWidth,
		hostPortGap + ImGui::CalcTextSize("Port:").x + footerSpacing + portWidth,
		ImGui::GetFrameHeight() + footerSpacing + connectButtonWidth,
		pendingText.empty() ? 0.f : ImGui::CalcTextSize(pendingText.c_str()).x
	};
	const float connectionRowWidth = connectionWidths[0] + connectionWidths[1] + connectionWidths[2] + footerSpacing * 2.f;
	const float serverDropdownWidth = std::min(footerWidth, std::max(1.f, connectionRowWidth - footerSpacing - syncControlsWidth));
	const std::vector<float> actionWidths = {serverDropdownWidth, syncControlsWidth};
	auto footerLineWidths = [footerWidth, footerSpacing](const std::vector<float> &widths) {
		std::vector<float> lines;
		float used = 0.f;
		for (float width : widths) {
			if (width <= 0.f) continue;
			if (used > 0.f && used + footerSpacing + width > footerWidth) { lines.push_back(used); used = 0.f; }
			used += (used > 0.f ? footerSpacing : 0.f) + width;
		}
		lines.push_back(used);
		return lines;
	};
	const auto connectionLines = footerLineWidths(connectionWidths);
	const auto actionLines = footerLineWidths(actionWidths);
	const int footerLines = static_cast<int>(connectionLines.size() + actionLines.size());
	const float footerHeight = ImGui::GetFrameHeight() * footerLines + ImGui::GetStyle().ItemSpacing.y * (footerLines - 1) + ImGui::GetStyle().WindowPadding.y * 2.f + footerRowGap;
	ImGui::PopStyleVar(2);
	ImGui::PopFont();
	const float parameterListGap = 6.f;
	ImGui::SetCursorPosY(ImGui::GetCursorPosY() + parameterListGap);
	ImGui::PushStyleColor(ImGuiCol_ScrollbarBg, ImVec4(0.f, 0.f, 0.f, 0.f));
	ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.f, 0.f, 0.f, .16f));
	ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 5.f);
	if (ImGui::BeginChild("param-list", ImVec2(0, -footerHeight - ImGui::GetStyle().ItemSpacing.y - parameterListGap), ImGuiChildFlags_AlwaysUseWindowPadding)) {
		if (client.isReadyToSend() && !receivedFullSync) {
			ImGui::PushFont(nullptr, ImGui::GetStyle().FontSizeBase * topScale);
			ImGui::TextDisabled("Waiting for server parameter data...");
			ImGui::PopFont();
		} else if (names.empty()) {
			ImGui::PushFont(nullptr, ImGui::GetStyle().FontSizeBase * topScale);
			ImGui::TextDisabled("Sync completed with no parameters. Check the server and try Sync.");
			ImGui::PopFont();
		}
		std::vector<std::vector<std::string>> blocks;
		for (const auto &name : visible) {
			const auto param = client.getParamForName(name);
			if (blocks.empty() || param.type == REMOTEUI_PARAM_SPACER) blocks.emplace_back();
			blocks.back().push_back(name);
		}
		// Keep each group intact and give its header, preset controls and slider
		// enough room before adding another column. Larger parameter text needs
		// proportionally more space, while the fixed group controls set a minimum.
		const float defaultRowHeight = ImGui::GetFontSize() / .8f + 2.f;
		const float effectiveRowHeight = rowHeight > 0.f ? rowHeight : defaultRowHeight;
		const float minimumColumnWidth = 160.f + (columnBaseWidth - 160.f) * effectiveRowHeight / defaultRowHeight;
		const int columnCount = std::max(1, std::min(static_cast<int>(blocks.size()), static_cast<int>(ImGui::GetContentRegionAvail().x / minimumColumnWidth)));
		if (columnCount > 1) ImGui::Columns(columnCount, "parameter-columns", false);
		std::vector<size_t> columnStarts;
		size_t remainingRows = visible.size() + blocks.size(), rowsInColumn = 0;
		int columnsRemaining = columnCount;
		for (size_t i = 0; i < blocks.size(); ++i) {
			const size_t blockRows = blocks[i].size() + 1; // Include spacing between groups.
			if (rowsInColumn && columnsRemaining > 1) {
				const double target = static_cast<double>(remainingRows) / columnsRemaining;
				const bool mustAdvance = blocks.size() - i == static_cast<size_t>(columnsRemaining - 1);
				if (mustAdvance || std::abs(static_cast<double>(rowsInColumn) - target) <= std::abs(static_cast<double>(rowsInColumn + blockRows) - target)) {
					columnStarts.push_back(i);
					remainingRows -= rowsInColumn;
					rowsInColumn = 0;
					--columnsRemaining;
				}
			}
			rowsInColumn += blockRows;
		}
		ImDrawList *drawList = ImGui::GetWindowDrawList();
		float longestLabelWidth = 0.f;
		ImGui::PushFont(nullptr, ImGui::GetStyle().FontSizeBase * parameterFontScale(rowHeight));
		for (const auto &block : blocks) {
			for (const auto &name : block) {
				if (client.getParamForName(name).type != REMOTEUI_PARAM_SPACER)
					longestLabelWidth = std::max(longestLabelWidth, ImGui::CalcTextSize(name.c_str(), nullptr, true).x);
			}
		}
		ImGui::PopFont();
		size_t nextColumn = 0;
		for (size_t i = 0; i < blocks.size(); ++i) {
			if (nextColumn < columnStarts.size() && i == columnStarts[nextColumn]) { ImGui::NextColumn(); ++nextColumn; }
			ImDrawListSplitter background;
			background.Split(drawList, 2);
			background.SetCurrentChannel(drawList, 1);
			const float groupRight = ImGui::GetCursorScreenPos().x + ImGui::GetContentRegionAvail().x;
			ImGui::BeginGroup();
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.f, 1.f, 1.f, 1.f));
			ImGui::PushID(static_cast<int>(i));
			const float groupContentWidth = std::max(1.f, ImGui::GetContentRegionAvail().x - 12.f);
			const float groupLabelWidth = std::min(groupContentWidth * .4f, std::max(groupContentWidth * .25f, longestLabelWidth + 6.f * parameterFontScale(rowHeight)));
			const auto firstParam = client.getParamForName(blocks[i].front());
			const bool hasHeader = firstParam.type == REMOTEUI_PARAM_SPACER;
			if (hasHeader) {
				ImGui::PushID(0);
				drawGroupHeader(blocks[i].front(), firstParam);
				ImGui::PopID();
			}
			const size_t firstRow = hasHeader ? 1 : 0;
			const bool hasContent = firstRow < blocks[i].size();
			const std::string &group = firstParam.group;
			const bool collapsing = collapsedGroups.count(group) != 0;
			const auto animation = groupToggleTimes.find(group);
			const bool animating = hasContent && animation != groupToggleTimes.end();
			if (animating) {
				const float progress = ofClamp((ofGetElapsedTimef() - animation->second) / groupAnimationDuration, 0.f, 1.f);
				const float eased = .5f + .5f * std::tanh((progress * 2.f - 1.f) * 3.f) / std::tanh(3.f);
				const float fraction = collapsing ? 1.f - eased : eased;
				const auto height = groupContentHeights.find(group);
				const float fullHeight = height != groupContentHeights.end() ? height->second : 8.f + (blocks[i].size() - firstRow) * (effectiveRowHeight + ImGui::GetStyle().ItemSpacing.y);
				ImGui::BeginChild("group-animation", ImVec2(0.f, std::max(1.f, fullHeight * fraction)), ImGuiChildFlags_None, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
			}
			const float contentStartY = ImGui::GetCursorPosY();
			if (hasContent) {
				ImGui::Dummy(ImVec2(0.f, 4.f));
				ImGui::Indent(6.f);
			}
			for (size_t row = firstRow; row < blocks[i].size(); ++row) {
				const auto &name = blocks[i][row];
				ImGui::PushID(static_cast<int>(row));
				if (continuousRefresh) ImGui::BeginDisabled();
				if (animating) ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
				drawParameter(name, groupLabelWidth);
				if (animating) ImGui::PopItemFlag();
				if (continuousRefresh) ImGui::EndDisabled();
				ImGui::PopID();
			}
			if (hasContent) {
				ImGui::Unindent(6.f);
				ImGui::Dummy(ImVec2(0.f, 4.f));
				const float measuredHeight = ImGui::GetCursorPosY() - contentStartY;
				if (measuredHeight > 8.f) groupContentHeights[group] = measuredHeight;
			}
			if (animating) ImGui::EndChild();
			ImGui::PopID();
			ImGui::EndGroup();
			ImGui::PopStyleColor();
			const ImVec2 min = ImGui::GetItemRectMin(), measuredMax = ImGui::GetItemRectMax();
			const ImVec2 max(groupRight, measuredMax.y);
			const auto groupColor = client.getParamForName(blocks[i].front());
			background.SetCurrentChannel(drawList, 0);
			if (groupColor.a) {
				// Keep the box darker than the controls while retaining the header's hue.
				drawList->AddRectFilled(min, max, IM_COL32(25 + groupColor.r * 18 / 100, 25 + groupColor.g * 18 / 100, 25 + groupColor.b * 18 / 100, 235), 5.f);
			} else {
				drawList->AddRectFilled(min, max, IM_COL32(0, 0, 0, 35), 5.f);
			}
			background.Merge(drawList);
			ImGui::Spacing();
		}
		if (columnCount > 1) ImGui::Columns(1);
	}
	ImGui::EndChild();
	ImGui::PopStyleVar();
	ImGui::PopStyleColor(2);
	if (bindingPopupRequested) {
		ImGui::OpenPopup("learn-control");
		bindingPopupRequested = false;
		bindingPopupActive = true;
	}
	if (bindingPopupActive) {
		ImGui::SetNextWindowPos(ImVec2(bindingPopupX, bindingPopupY), ImGuiCond_Appearing);
		if (ImGui::BeginPopup("learn-control")) {
			if (controllers.learning.empty()) {
				ImGui::CloseCurrentPopup();
				bindingPopupActive = false;
			} else {
				ImGui::TextUnformatted(controllers.learning.c_str());
				ImGui::TextUnformatted("Move a MIDI or joystick control to bind it.");
				if (ImGui::Button("Cancel")) {
					controllers.learning.clear();
					bindingPopupActive = false;
					ImGui::CloseCurrentPopup();
				}
			}
			ImGui::EndPopup();
		} else {
			controllers.learning.clear();
			bindingPopupActive = false;
		}
	}
	ImGui::SetCursorPosY(ImGui::GetCursorPosY() + parameterListGap);
	ImGui::PushFont(nullptr, ImGui::GetStyle().FontSizeBase * footerScale);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(windowPadding.x * footerScale, windowPadding.y * footerScale));
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(framePadding.x * footerScale, framePadding.y * footerScale));
	ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.f, 0.f, 0.f, .16f));
	ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 5.f);
	ImGui::BeginChild("bottom-controls", ImVec2(0, footerHeight), ImGuiChildFlags_AlwaysUseWindowPadding, ImGuiWindowFlags_NoScrollbar);
	float footerUsed = 0.f;
	size_t footerLine = 0;
	const std::vector<float> *footerLayout = &connectionLines;
	auto footerNext = [&](float width) {
		if (footerUsed > 0.f && footerUsed + footerSpacing + width <= footerWidth) { ImGui::SameLine(); footerUsed += footerSpacing; }
		else {
			if (footerUsed > 0.f) { footerUsed = 0.f; ++footerLine; }
			ImGui::SetCursorPosX(ImGui::GetCursorPosX() + std::max(0.f, (footerWidth - (*footerLayout)[footerLine]) * .5f));
		}
		footerUsed += width;
	};
	footerNext(connectionWidths[0]); ImGui::AlignTextToFramePadding(); ImGui::TextUnformatted("Host:"); ImGui::SameLine(); ImGui::SetNextItemWidth(hostWidth); ImGui::InputTextWithHint("##host", "Host / IP", hostBuffer, sizeof(hostBuffer));
	footerNext(connectionWidths[1]); ImGui::SetCursorPosX(ImGui::GetCursorPosX() + hostPortGap); ImGui::AlignTextToFramePadding(); ImGui::TextUnformatted("Port:"); ImGui::SameLine(); ImGui::SetNextItemWidth(portWidth); ImGui::InputInt("##port", &port, 0, 0);
	footerNext(connectionWidths[2]);
	ImGui::Dummy(ImVec2(ImGui::GetFrameHeight(), ImGui::GetFrameHeight()));
	const ImVec2 indicatorMin = ImGui::GetItemRectMin(), indicatorMax = ImGui::GetItemRectMax();
	const ImVec2 indicatorCenter((indicatorMin.x + indicatorMax.x) * .5f, (indicatorMin.y + indicatorMax.y) * .5f);
	const ImU32 indicatorColor = status == "Connecting…" ? IM_COL32(240, 190, 45, 255) :
		(client.isReadyToSend() ? IM_COL32(75, 210, 100, 255) : IM_COL32(145, 145, 145, 255));
	ImGui::GetWindowDrawList()->AddCircleFilled(indicatorCenter, ImGui::GetFrameHeight() * .25f,
		indicatorColor);
	if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", status.c_str());
	ImGui::SameLine(); if (client.isReadyToSend()) { if (ImGui::Button("Disconnect", ImVec2(connectButtonWidth, 0))) disconnectFromServer(); } else if (ImGui::Button("Connect", ImVec2(connectButtonWidth, 0))) connectTo(hostBuffer, port);
	if (!pendingText.empty()) { footerNext(connectionWidths[3]); ImGui::TextDisabled("%s", pendingText.c_str()); }
	ImGui::SetCursorPosY(ImGui::GetCursorPosY() + footerRowGap);
	footerUsed = 0.f;
	footerLine = 0;
	footerLayout = &actionLines;
	footerNext(actionWidths[0]);
	ImGui::SetNextItemWidth(serverDropdownWidth);
	const bool serverDropdownOpen = ImGui::BeginCombo("##servers", serverPreview.c_str());
		if (ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip)) {
			ImGui::SetNextWindowViewport(ImGui::GetMainViewport()->ID);
			ImGui::SetTooltip("%s", serverPreview.c_str());
		}
	if (serverDropdownOpen) {
		framedCheckbox("Discover servers", &autoDiscovery);
		if (neighbors.empty()) ImGui::TextDisabled("No servers discovered yet");
		for (size_t i = 0; i < neighbors.size(); ++i) {
			const auto &neighbor = neighbors[i];
			std::string label = neighbor.binary.empty() ? "Server" : neighbor.binary;
			if (!neighbor.name.empty()) label += " on " + neighbor.name;
			label += " @ " + neighbor.IP + ":" + ofToString(neighbor.port);
			const bool selected = client.isReadyToSend() && neighbor.IP == host && neighbor.port == port;
			ImGui::PushID(static_cast<int>(i));
			if (ImGui::Selectable(label.c_str(), selected) && !selected) selectNeighbor(neighbor);
			ImGui::PopID();
		}
		ImGui::EndCombo();
	}
	footerNext(actionWidths[1]);
	ImGui::Dummy(ImVec2(dividerWidth, ImGui::GetFrameHeight()));
	const ImVec2 dividerMin = ImGui::GetItemRectMin(), dividerMax = ImGui::GetItemRectMax();
	const float dividerX = (dividerMin.x + dividerMax.x) * .5f;
	ImGui::GetWindowDrawList()->AddLine(ImVec2(dividerX, dividerMin.y + ImGui::GetFrameHeight() * .1f),
		ImVec2(dividerX, dividerMax.y - ImGui::GetFrameHeight() * .1f), ImGui::GetColorU32(ImGuiCol_Separator));
	ImGui::SameLine(); if (ImGui::Button("Sync") && client.isReadyToSend()) client.requestCompleteUpdate();
	if (ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip)) {
		ImGui::SetNextWindowViewport(ImGui::GetMainViewport()->ID);
		ImGui::SetTooltip("Pulls the Current Param values from the server");
	}
	ImGui::SameLine(0.f, syncContinuousGap); framedCheckbox("Non-Stop", &continuousRefresh);
	ImGui::EndChild();
	ImGui::PopStyleVar();
	ImGui::PopStyleColor();
	ImGui::PopStyleVar(2);
	ImGui::PopFont();
	ImGui::End();
}

void ofApp::drawGroupHeader(const std::string &name, const RemoteUIParam &param) {
	ImGui::PushID(name.c_str());
	ImGui::PushFont(gui.getGroupHeaderFont(), ImGui::GetStyle().FontSizeBase * parameterFontScale(rowHeight));
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(3.f, 1.f + ImGui::GetFontSize() * .125f));
	const ImVec2 pos = ImGui::GetCursorScreenPos();
	const float width = ImGui::GetContentRegionAvail().x;
	const float height = ImGui::GetFrameHeight();
	if (param.a) ImGui::GetWindowDrawList()->AddRectFilled(pos, ImVec2(pos.x + width, pos.y + height), IM_COL32(param.r * 3 / 4, param.g * 3 / 4, param.b * 3 / 4, param.a), 5.f);
	const std::string &group = param.group;
	const float controlsWidth = std::min(185.f, std::max(100.f, width * .52f));
	const float titleWidth = std::max(1.f, width - controlsWidth - ImGui::GetStyle().ItemSpacing.x);
	auto &collapsedGroups = collapsedGroupsByServer[bindingServer()];
	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.f, 0.f, 0.f, 0.f));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.f, 1.f, 1.f, .16f));
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(1.f, 1.f, 1.f, .24f));
	if (ImGui::Button("##toggle-group", ImVec2(titleWidth, height))) {
		if (collapsedGroups.count(group)) collapsedGroups.erase(group); else collapsedGroups.insert(group);
		groupToggleTimes[group] = ofGetElapsedTimef();
		saveCollapsedGroups();
	}
	const float fontSize = ImGui::GetFontSize();
	const float inset = std::max(4.f, fontSize * .35f);
	const float arrowSize = fontSize * .44f;
	const float arrowX = pos.x + inset, arrowY = pos.y + (height - arrowSize) * .5f;
	ImDrawList *headerDrawList = ImGui::GetWindowDrawList();
	headerDrawList->PushClipRect(pos, ImVec2(pos.x + titleWidth, pos.y + height), true);
	const ImU32 textColor = ImGui::GetColorU32(ImGuiCol_Text);
	if (collapsedGroups.count(group)) {
		headerDrawList->AddTriangleFilled(ImVec2(arrowX, arrowY), ImVec2(arrowX, arrowY + arrowSize), ImVec2(arrowX + arrowSize, arrowY + arrowSize * .5f), textColor);
	} else {
		headerDrawList->AddTriangleFilled(ImVec2(arrowX, arrowY), ImVec2(arrowX + arrowSize, arrowY), ImVec2(arrowX + arrowSize * .5f, arrowY + arrowSize), textColor);
	}
	headerDrawList->AddText(ImVec2(arrowX + arrowSize + inset, pos.y + (height - fontSize) * .5f), textColor, param.stringVal.c_str());
	headerDrawList->PopClipRect();
	const float titleTextWidth = std::max(0.f, pos.x + titleWidth - (arrowX + arrowSize + inset));
	if (ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip) && ImGui::CalcTextSize(param.stringVal.c_str()).x > titleTextWidth) {
		ImGui::SetNextWindowViewport(ImGui::GetMainViewport()->ID);
		ImGui::SetTooltip("%s", param.stringVal.c_str());
	}
	ImGui::PopStyleColor(3);
	ImGui::PopFont();
	ImGui::PushFont(nullptr, ImGui::GetStyle().FontSizeBase * parameterFontScale(rowHeight));
	ImGui::SameLine();
	ImGui::SetCursorScreenPos(ImVec2(pos.x + titleWidth + ImGui::GetStyle().ItemSpacing.x, ImGui::GetCursorScreenPos().y));
	const bool canUseGroup = client.isReadyToSend() && !group.empty();
	const float presetButtonWidth = std::min(std::max(24.f, fontSize * 1.5f), std::max(1.f, (controlsWidth - 40.f - ImGui::GetStyle().ItemSpacing.x * 2.f) * .5f));
	const float presetButtonHeight = std::min(height, std::max(ImGui::GetFontSize() + 2.f, height * .75f));
	const float buttonR = param.r / 255.f, buttonG = param.g / 255.f, buttonB = param.b / 255.f;
	auto presetButton = [&](const char *label) {
		const ImVec2 buttonPos = ImGui::GetCursorScreenPos();
		ImGui::SetCursorScreenPos(ImVec2(buttonPos.x, pos.y + (height - presetButtonHeight) * .5f));
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(3.f, 1.f));
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(.08f + buttonR * .35f, .08f + buttonG * .35f, .08f + buttonB * .35f, 1.f));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(.16f + buttonR * .45f, .16f + buttonG * .45f, .16f + buttonB * .45f, 1.f));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(.22f + buttonR * .5f, .22f + buttonG * .5f, .22f + buttonB * .5f, 1.f));
		const bool pressed = ImGui::Button(label, ImVec2(presetButtonWidth, presetButtonHeight));
		ImGui::PopStyleColor(3);
		ImGui::PopStyleVar();
		return pressed;
	};
	if (!canUseGroup) ImGui::BeginDisabled();
	if (presetButton("+")) { groupPresetNameBuffer[0] = '\0'; ImGui::OpenPopup("save-group-preset"); }
	if (ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip)) {
		ImGui::SetNextWindowViewport(ImGui::GetMainViewport()->ID);
		ImGui::SetTooltip("Save a preset for %s", group.c_str());
	}
	if (ImGui::BeginPopup("save-group-preset")) {
		ImGui::Text("Save preset for %s", group.c_str());
		ImGui::SetNextItemWidth(200.f);
		if (ImGui::IsWindowAppearing()) ImGui::SetKeyboardFocusHere();
		const bool submit = ImGui::InputText("##preset-name", groupPresetNameBuffer, sizeof(groupPresetNameBuffer), ImGuiInputTextFlags_EnterReturnsTrue);
		if ((ImGui::Button("Save") || submit) && groupPresetNameBuffer[0]) {
			client.saveGroupPresetWithName(groupPresetNameBuffer, group);
			ImGui::CloseCurrentPopup();
		}
		ImGui::SameLine(); if (ImGui::Button("Cancel")) ImGui::CloseCurrentPopup();
		ImGui::EndPopup();
	}
	ImGui::SameLine();
	auto &selected = groupPresetSelection[group];
	if (selected.empty()) ImGui::BeginDisabled();
	if (presetButton("-")) client.deleteGroupPreset(selected, group);
	if (selected.empty()) ImGui::EndDisabled();
	if (ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip)) {
		ImGui::SetNextWindowViewport(ImGui::GetMainViewport()->ID);
		ImGui::SetTooltip("Delete selected group preset");
	}
	ImGui::SameLine();
	ImGui::SetCursorScreenPos(ImVec2(ImGui::GetCursorScreenPos().x, pos.y + (height - presetButtonHeight) * .5f));
	ImGui::SetNextItemWidth(std::max(1.f, ImGui::GetContentRegionAvail().x - 1.5f * parameterFontScale(rowHeight)));
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(ImGui::GetStyle().FramePadding.x + 2.f * parameterFontScale(rowHeight), std::max(0.f, (presetButtonHeight - ImGui::GetFontSize()) * .5f)));
	const float r = param.r / 255.f, g = param.g / 255.f, b = param.b / 255.f;
	ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(.12f + r * .32f, .12f + g * .32f, .12f + b * .32f, 1.f));
	ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(.16f + r * .38f, .16f + g * .38f, .16f + b * .38f, 1.f));
	ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(.20f + r * .42f, .20f + g * .42f, .20f + b * .42f, 1.f));
	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(.16f + r * .40f, .16f + g * .40f, .16f + b * .40f, 1.f));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(.20f + r * .48f, .20f + g * .48f, .20f + b * .48f, 1.f));
	if (ImGui::BeginCombo("##group-preset", selected.empty() ? "No preset" : selected.c_str())) {
		const std::string prefix = group + "/";
		for (const auto &preset : client.getPresetsList()) {
			if (preset.compare(0, prefix.size(), prefix) != 0) continue;
			const std::string presetName = preset.substr(prefix.size());
			if (ImGui::Selectable(presetName.c_str(), presetName == selected)) applyGroupPreset(presetName, group);
		}
		ImGui::EndCombo();
	}
	ImGui::PopStyleColor(5);
	ImGui::PopStyleVar();
	if (!canUseGroup) ImGui::EndDisabled();
	ImGui::PopStyleVar();
	ImGui::PopFont();
	ImGui::PopID();
}

void ofApp::drawParameter(const std::string &name, float groupLabelWidth) {
	RemoteUIParam p = client.getParamForName(name);
	ImGui::PushID(name.c_str());
	const float defaultFontSize = ImGui::GetFontSize() / .8f;
	const float targetRowHeight = rowHeight > 0.f ? rowHeight : defaultFontSize + 2.f;
	ImGui::PushFont(nullptr, ImGui::GetStyle().FontSizeBase * parameterFontScale(rowHeight));
	bool edited = false;
	const float contentWidth = std::max(1.f, ImGui::GetContentRegionAvail().x - 6.f);
	const float labelWidth = std::min(groupLabelWidth, contentWidth);
	const float controlLabelGap = ImGui::GetStyle().ItemSpacing.x + 4.f * parameterFontScale(rowHeight);
	const float controlWidth = std::max(1.f, contentWidth - labelWidth - controlLabelGap);
	const float controlStartX = ImGui::GetCursorScreenPos().x;
	ImGui::SetNextItemWidth(controlWidth);
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4.f, std::max(1.f, (targetRowHeight - ImGui::GetFontSize()) * .5f)));
	ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(.08f, .08f, .08f, .25f));
	ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(.08f, .08f, .08f, .38f));
	ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(.08f, .08f, .08f, .48f));
	const bool coloredSlider = p.a && ((p.type == REMOTEUI_PARAM_FLOAT && p.minFloat < p.maxFloat) ||
		(p.type == REMOTEUI_PARAM_INT && p.minInt < p.maxInt));
	if (coloredSlider) {
		const float r = p.r / 255.f, g = p.g / 255.f, b = p.b / 255.f;
		ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(.10f + .35f * r, .10f + .35f * g, .10f + .35f * b, 1.f));
		ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(.15f + .42f * r, .15f + .42f * g, .15f + .42f * b, 1.f));
		ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(.19f + .45f * r, .19f + .45f * g, .19f + .45f * b, 1.f));
		ImGui::PushStyleColor(ImGuiCol_SliderGrab, ImVec4(.22f + .48f * r, .22f + .48f * g, .22f + .48f * b, 1.f));
		ImGui::PushStyleColor(ImGuiCol_SliderGrabActive, ImVec4(.35f + .42f * r, .35f + .42f * g, .35f + .42f * b, 1.f));
	}
	auto drawFineSlider = [&](auto drawSlider) {
		ImGuiIO &io = ImGui::GetIO();
		const float realMouseX = io.MousePos.x;
		const bool tracking = activeMouseSlider == name && ImGui::IsMouseDown(ImGuiMouseButton_Left) && ImGui::IsMousePosValid();
		if (tracking) {
			if (io.KeyAlt || fineSliderTracking) {
				fineSliderTracking = true;
				virtualSliderMouseX = ofClamp(virtualSliderMouseX + io.MouseDelta.x * (io.KeyAlt ? .05f : 1.f), controlStartX, controlStartX + controlWidth);
				io.MousePos.x = virtualSliderMouseX;
			} else {
				virtualSliderMouseX = realMouseX;
			}
		}
		const bool changed = drawSlider();
		io.MousePos.x = realMouseX;
		if (ImGui::IsItemActivated() && ImGui::IsMouseDown(ImGuiMouseButton_Left) && !io.KeyCtrl) {
			activeMouseSlider = name;
			virtualSliderMouseX = realMouseX;
			fineSliderTracking = false;
		}
		if (ImGui::IsItemDeactivated() && activeMouseSlider == name) {
			activeMouseSlider.clear();
			fineSliderTracking = false;
		}
		if (ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip)) {
			ImGui::SetNextWindowViewport(ImGui::GetMainViewport()->ID);
			ImGui::SetTooltip("Hold Option / Alt while dragging for fine adjustment");
		}
		return changed;
	};
	switch (p.type) {
		case REMOTEUI_PARAM_BOOL: {
			const ImVec2 pos = ImGui::GetCursorScreenPos();
			const float buttonWidth = std::min(controlWidth, std::max(ImGui::GetFrameHeight() * 2.f, ImGui::CalcTextSize("OFF").x + ImGui::GetStyle().FramePadding.x * 2.f));
			ImGui::SetCursorScreenPos(ImVec2(pos.x + std::max(0.f, (controlWidth - buttonWidth) * .5f), pos.y));
			const float r = p.a ? p.r / 255.f : .5f, g = p.a ? p.g / 255.f : .5f, b = p.a ? p.b / 255.f : .5f;
			const float tint = p.boolVal ? .45f : .10f;
			const float base = p.boolVal ? .16f : .18f;
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(base + tint * r, base + tint * g, base + tint * b, 1.f));
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(base + .08f + tint * r, base + .08f + tint * g, base + .08f + tint * b, 1.f));
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(base + .14f + tint * r, base + .14f + tint * g, base + .14f + tint * b, 1.f));
			if (ImGui::Button(p.boolVal ? "ON###value" : "OFF###value", ImVec2(buttonWidth, 0))) { p.boolVal = !p.boolVal; edited = true; }
			ImGui::PopStyleColor(3);
			break;
		}
		case REMOTEUI_PARAM_FLOAT: {
			float value = p.floatVal;
			if (p.minFloat < p.maxFloat) edited = drawFineSlider([&] { return ImGui::SliderFloat("##value", &value, p.minFloat, p.maxFloat, "%.4g"); });
			else edited = ImGui::DragFloat("##value", &value, 0.01f, 0, 0, "%.4g");
			p.floatVal = value; break;
		}
		case REMOTEUI_PARAM_INT: {
			int value = p.intVal;
			if (p.minInt < p.maxInt) edited = drawFineSlider([&] { return ImGui::SliderInt("##value", &value, p.minInt, p.maxInt); });
			else edited = ImGui::InputInt("##value", &value);
			p.intVal = value; break;
		}
		case REMOTEUI_PARAM_ENUM: {
			int index = p.intVal - p.minInt;
			const char *preview = index >= 0 && index < (int)p.enumList.size() ? p.enumList[index].c_str() : "(invalid)";
			if (ImGui::BeginCombo("##value", preview)) {
				for (int i = 0; i < (int)p.enumList.size(); ++i) {
					ImGui::PushID(i);
					if (ImGui::Selectable(p.enumList[i].c_str(), i == index)) { p.intVal = p.minInt + i; edited = true; }
					ImGui::PopID();
				}
				ImGui::EndCombo();
			}
			break;
		}
		case REMOTEUI_PARAM_STRING: {
			char value[4096]{};
			std::strncpy(value, p.stringVal.c_str(), sizeof(value) - 1);
			if (ImGui::InputText("##value", value, sizeof(value), ImGuiInputTextFlags_EnterReturnsTrue)) { p.stringVal = value; edited = true; }
			break;
		}
		case REMOTEUI_PARAM_COLOR: {
			float color[4] = {p.redVal / 255.f, p.greenVal / 255.f, p.blueVal / 255.f, p.alphaVal / 255.f};
			if (ImGui::ColorEdit4("##value", color, ImGuiColorEditFlags_AlphaBar)) {
				p.redVal = (unsigned char)(ofClamp(color[0], 0.f, 1.f) * 255.f + .5f);
				p.greenVal = (unsigned char)(ofClamp(color[1], 0.f, 1.f) * 255.f + .5f);
				p.blueVal = (unsigned char)(ofClamp(color[2], 0.f, 1.f) * 255.f + .5f);
				p.alphaVal = (unsigned char)(ofClamp(color[3], 0.f, 1.f) * 255.f + .5f); edited = true;
			}
			break;
		}
		default: ImGui::TextDisabled("%s", paramTypeName(p.type)); break;
	}
	if (coloredSlider) ImGui::PopStyleColor(5);
	ImGui::PopStyleColor(3);
	ImGui::PopStyleVar();
	ImGui::SameLine(0.f, controlLabelGap);
	if (p.type == REMOTEUI_PARAM_BOOL) {
		const ImVec2 pos = ImGui::GetCursorScreenPos();
		ImGui::SetCursorScreenPos(ImVec2(std::max(pos.x, controlStartX + controlWidth + controlLabelGap), pos.y));
	}
	ImGui::AlignTextToFramePadding();
	const bool blinkBinding = blinkingBoundParams.count(name) != 0;
	const bool temporaryHighlight = blinkBinding || diffParams.count(name);
	const bool highlightLabel = temporaryHighlight || (highlightChangesSinceLoad && changedSinceLoad.count(name));
	float highlightPulse = 0.f;
	if (blinkBinding) {
		highlightPulse = .5f + .5f * std::cos((boundBlinkUntil - ofGetElapsedTimef()) * 3.14159265f * 2.f);
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.f, 1.f, 1.f, 1.f));
	} else if (temporaryHighlight) {
		// A slow white-to-amber pulse keeps the label readable throughout.
		highlightPulse = .5f + .5f * std::sin(ofGetElapsedTimef() * 3.14159265f);
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.f, 1.f - .22f * highlightPulse, 1.f - .6f * highlightPulse, 1.f));
	} else if (highlightLabel) {
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.f, 1.f - .5f * changedHighlightPulse, 1.f - .5f * changedHighlightPulse, 1.f));
	}
	const ImVec2 labelPos = ImGui::GetCursorScreenPos();
	const float halfSpacing = ImGui::GetStyle().ItemSpacing.x * .5f;
	const float labelPadding = 3.f * parameterFontScale(rowHeight);
	const float clippedLabelWidth = std::max(1.f, std::min(labelWidth, ImGui::GetContentRegionAvail().x - 6.f - ImGui::GetStyle().ItemSpacing.x));
	const ImVec2 windowPos = ImGui::GetWindowPos(), windowSize = ImGui::GetWindowSize();
	const float r = p.a ? p.r / 255.f : .5f, g = p.a ? p.g / 255.f : .5f, b = p.a ? p.b / 255.f : .5f;
	ImGui::PushStyleVar(ImGuiStyleVar_SelectableRounding, 5.f);
	const float glow = temporaryHighlight ? .32f * highlightPulse : 0.f;
	ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(.12f + .35f * r + glow, .12f + .35f * g + glow * .30f, .12f + .35f * b, 1.f));
	ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(.16f + .42f * r + glow, .16f + .42f * g + glow * .30f, .16f + .42f * b, 1.f));
	ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(.20f + .48f * r + glow, .20f + .48f * g + glow * .30f, .20f + .48f * b, 1.f));
	ImGui::PushClipRect(ImVec2(labelPos.x - halfSpacing, windowPos.y), ImVec2(labelPos.x + clippedLabelWidth + halfSpacing, windowPos.y + windowSize.y), true);
	const bool labelClicked = ImGui::Selectable("##parameter-label", controllers.learning == name || temporaryHighlight, 0, ImVec2(clippedLabelWidth, ImGui::GetTextLineHeight()));
	const bool labelHovered = ImGui::IsItemHovered();
	if (labelHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) ImGui::OpenPopup("parameter-context");
	const ImVec2 itemMin = ImGui::GetItemRectMin();
	const float textY = itemMin.y + std::floor(ImGui::GetStyle().ItemSpacing.y * .5f);
	const float textLeft = labelPos.x + labelPadding;
	const float textRight = std::max(textLeft, labelPos.x + clippedLabelWidth - labelPadding);
	ImGui::PushClipRect(ImVec2(textLeft, windowPos.y), ImVec2(textRight, windowPos.y + windowSize.y), true);
	ImGui::GetWindowDrawList()->AddText(ImVec2(textLeft, textY), ImGui::GetColorU32(ImGuiCol_Text), name.c_str(), name.c_str() + name.size());
	ImGui::PopClipRect();
	ImGui::PopClipRect();
	ImGui::PopStyleColor(3);
	ImGui::PopStyleVar();
	if (labelClicked) {
		if (p.type == REMOTEUI_PARAM_BOOL || p.type == REMOTEUI_PARAM_FLOAT || p.type == REMOTEUI_PARAM_INT || p.type == REMOTEUI_PARAM_ENUM || p.type == REMOTEUI_PARAM_COLOR) {
			controllers.beginLearn(name);
			bindingParam = name;
			bindingPopupX = labelPos.x;
			bindingPopupY = labelPos.y + ImGui::GetTextLineHeightWithSpacing();
			bindingPopupRequested = true;
		}
	}
	if (highlightLabel) ImGui::PopStyleColor();
	ImGui::PopFont();
	if (ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip) && !ImGui::IsPopupOpen("parameter-context")) {
		ImGui::SetNextWindowViewport(ImGui::GetMainViewport()->ID);
		if (ImGui::BeginTooltip()) {
			ImGui::Text("\"%s\": (%s)",name.c_str(), paramTypeName(p.type));
			if (p.type == REMOTEUI_PARAM_FLOAT && p.minFloat < p.maxFloat)
				ImGui::Text("Range: %.6g to %.6g", p.minFloat, p.maxFloat);
			else if (p.type == REMOTEUI_PARAM_INT && p.minInt < p.maxInt)
				ImGui::Text("Range: %d to %d", p.minInt, p.maxInt);
			ImGui::TextUnformatted("Click to bind a MIDI or joystick control");
			if (!p.description.empty()) {
				ImGui::Separator();
				ImGui::TextUnformatted(p.description.c_str());
			}
			ImGui::EndTooltip();
		}
	}
	if (ImGui::BeginPopup("parameter-context")) {
		const auto baseline = changeBaseline.find(name);
		const bool changed = baseline != changeBaseline.end() && p.type == baseline->second.type && !sameParameterValue(p, baseline->second);
		const float restoreWidth = ImGui::CalcTextSize("Restore Parameter Value").x + ImGui::GetStyle().FramePadding.x * 2.f;
		const ImGuiSelectableFlags restoreFlags = client.isReadyToSend() && changed ? ImGuiSelectableFlags_None : ImGuiSelectableFlags_Disabled;
		if (ImGui::Selectable("Restore Parameter Value", false, restoreFlags, ImVec2(restoreWidth, 0.f))) {
			RemoteUIParam restored = p;
			if (copyParameterValue(restored, baseline->second)) {
				sendEditedParam(name, restored);
				appendLog("Restored connection value for " + name);
			}
		}
		ImGui::SameLine();
		ImGui::TextDisabled("(?)");
		if (ImGui::IsItemHovered()) {
			if (!changed) ImGui::SetTooltip("Restores the value this client received from the server on its most recent connection.\n\nThis parameter has not changed since then.");
			else ImGui::SetTooltip("Restores the value this client received from the server on its most recent connection.");
		}
		ImGui::EndPopup();
	}
	if (edited) sendEditedParam(name, p);
	ImGui::PopID();
}

void ofApp::sendEditedParam(const std::string &name, RemoteUIParam param) {
	if (client.paramExistsForName(name) && !param.isEqualTo(client.getParamForName(name))) {
		groupPresetSelection[param.group].clear();
		selectedPreset.clear();
	}
	client.sendUntrackedParamUpdate(param, name);
	updateChangedParameter(name);
}

void ofApp::drawLogPanel() {
	ImGui::SetNextWindowSize(ImVec2(650, 190), ImGuiCond_FirstUseEver);
	prepareSecondaryWindow();
	if (!ImGui::Begin("Action log", &showLogPanel, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoDocking)) { ImGui::End(); return; }
	if (ImGui::SmallButton("Clear")) logLines.clear();
	ImGui::SameLine(); ImGui::TextDisabled("%zu events", logLines.size());
	ImGui::BeginChild("log-scroll", ImVec2(0, 0), true);
	for (const auto &line : logLines) ImGui::TextUnformatted(line.c_str());
	if (autoScrollLog && ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) ImGui::SetScrollHereY(1.f);
	ImGui::EndChild();
	ImGui::End();
}

void ofApp::drawServerLogPanel() {
	ImGui::SetNextWindowSize(ImVec2(650, 190), ImGuiCond_FirstUseEver);
	prepareSecondaryWindow();
	if (!ImGui::Begin("Server log", &showServerLogPanel, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoDocking)) { ImGui::End(); return; }
	if (ImGui::SmallButton("Clear")) serverLogLines.clear();
	ImGui::SameLine(); ImGui::TextDisabled("%zu lines", serverLogLines.size());
	ImGui::BeginChild("server-log-scroll", ImVec2(0, 0), true);
	for (const auto &line : serverLogLines) ImGui::TextUnformatted(line.c_str());
	if (autoScrollLog && ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) ImGui::SetScrollHereY(1.f);
	ImGui::EndChild();
	ImGui::End();
}

void ofApp::drawSettingsPanel() {
	ImGui::SetNextWindowSize(ImVec2(360, 260), ImGuiCond_FirstUseEver);
	prepareSecondaryWindow();
	if (!ImGui::Begin("Settings", &showSettings, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoDocking)) { ImGui::End(); return; }
	framedCheckbox("Auto-connect to new servers", &autoConnectToNewServers);
	ImGui::Indent();
	framedCheckbox("Only auto-connect to localhost servers", &onlyAutoConnectToLocalHost);
	ImGui::Unindent();
	if (framedCheckbox("Always on top", &alwaysOnTop)) setAlwaysOnTop(alwaysOnTop);
	framedCheckbox("External buttons toggle", &controllers.toggleButtons);
	framedCheckbox("Color knobs control alpha", &controllers.colorControlsAlpha);
	framedCheckbox("Highlight changes after loading presets", &highlightPresetDiff);
	framedCheckbox("Auto-scroll logs", &autoScrollLog);
	float uiScalePercent = parameterFontScale(rowHeight) * 100.f;
	if (ImGui::SliderFloat("Parameter GUI size", &uiScalePercent, 100.f * 10.f / 17.f, 100.f * 46.f / 17.f, "%.0f%%")) rowHeight = 2.f + 17.f * uiScalePercent / 100.f;
	ImGui::SliderFloat("Column width", &columnBaseWidth, 220.f, 500.f, "%.0f px");
	if (ImGui::IsItemHovered()) ImGui::SetTooltip("Smaller values allow more columns. The threshold also follows parameter row height.");
	if (ImGui::ColorEdit3("Window color", windowColor)) applyWindowColor(windowColor);
	if (ImGui::Button("Open ImGui style editor")) showStyleEditor = true;
	ImGui::Spacing();
	ImGui::Spacing();
	if (ImGui::Button("Save preferences")) savePreferences();
	ImGui::End();
}

std::string ofApp::bindingServer() const { return host + ":" + ofToString(port); }

void ofApp::toggleAllGroups() {
	if (!client.isReadyToSend()) return;
	auto &collapsed = collapsedGroupsByServer[bindingServer()];
	std::unordered_set<std::string> groups;
	for (const auto &name : client.getAllParamNamesList()) {
		const auto param = client.getParamForName(name);
		if (param.type == REMOTEUI_PARAM_SPACER) groups.insert(param.group);
	}
	if (groups.empty()) return;
	const bool collapse = std::any_of(groups.begin(), groups.end(), [&](const std::string &group) { return collapsed.count(group) == 0; });
	const float now = ofGetElapsedTimef();
	for (const auto &group : groups) {
		const bool wasCollapsed = collapsed.count(group) != 0;
		if (wasCollapsed == collapse) continue;
		if (collapse) collapsed.insert(group); else collapsed.erase(group);
		groupToggleTimes[group] = now;
	}
	saveCollapsedGroups();
}

void ofApp::blinkBoundParameters(bool midi) {
	blinkingBoundParams.clear();
	for (const auto &binding : controllers.bindings) {
		const bool matchesKind = midi ? binding.kind.compare(0, 5, "MIDI ") == 0 : binding.kind.compare(0, 9, "Joystick ") == 0;
		if (matchesKind && binding.server == bindingServer() && client.paramExistsForName(binding.param)) blinkingBoundParams.insert(binding.param);
	}
	boundBlinkUntil = blinkingBoundParams.empty() ? 0.f : ofGetElapsedTimef() + 4.f;
}

void ofApp::drawBindingsPanel() {
	ImGui::SetNextWindowSize(ImVec2(720, 300), ImGuiCond_FirstUseEver);
	if (focusBindings) { ImGui::SetNextWindowFocus(); focusBindings = false; }
	prepareSecondaryWindow();
	if (!ImGui::Begin("Controller bindings", &showBindings, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoDocking)) { ImGui::End(); return; }
	if (!controllers.learning.empty()) {
		ImGui::TextWrapped("Move a MIDI control or joystick axis, or press a button, to bind %s.", controllers.learning.c_str());
		if (ImGui::Button("Cancel learning")) controllers.learning.clear();
	} else ImGui::TextDisabled("Click a parameter name to learn a control.");
	if (!bindingParam.empty()) framedCheckbox("Show bindings for all parameters and servers", &showAllBindings);
	ImGui::Separator();
	bool any = false;
	ImGui::BeginChild("bindings-list", ImVec2(0, 0));
	const float removeColumnWidth = std::max(ImGui::CalcTextSize("Remove").x, ImGui::CalcTextSize("Action").x) + ImGui::GetStyle().FramePadding.x * 2.f + ImGui::GetStyle().CellPadding.x * 2.f;
	const float tableInnerWidth = std::max(ImGui::GetContentRegionAvail().x, ImGui::GetFontSize() * 36.f + removeColumnWidth);
	if (ImGui::BeginTable("bindings-table", 4, ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollX, ImVec2(0, 0), tableInnerWidth)) {
	ImGui::TableSetupColumn("Parameter", ImGuiTableColumnFlags_WidthStretch, 1.2f);
	ImGui::TableSetupColumn("Controller", ImGuiTableColumnFlags_WidthStretch, 2.0f);
	ImGui::TableSetupColumn("Server", ImGuiTableColumnFlags_WidthStretch, 1.0f);
	ImGui::TableSetupColumn("Action", ImGuiTableColumnFlags_WidthFixed, removeColumnWidth);
	ImGui::TableHeadersRow();
	for (size_t i = 0; i < controllers.bindings.size();) {
		const auto &b = controllers.bindings[i];
		if (!showAllBindings && (b.server != bindingServer() || b.param != bindingParam)) { ++i; continue; }
		any = true; ImGui::PushID((int)i);
		ImGui::TableNextRow();
		ImGui::TableNextColumn();
		if (client.isReadyToSend() && b.server == bindingServer() && client.paramExistsForName(b.param)) {
			const auto param = client.getParamForName(b.param);
			if (param.a) ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, IM_COL32(25 + param.r * 35 / 100, 25 + param.g * 35 / 100, 25 + param.b * 35 / 100, 255));
		}
		ImGui::TextWrapped("%s", b.param.c_str());
		ImGui::TableNextColumn(); ImGui::TextWrapped("%s", ControllerBindings::label(b).c_str());
		ImGui::TableNextColumn(); ImGui::TextWrapped("%s", b.server.c_str());
		ImGui::TableNextColumn(); bool remove = ImGui::SmallButton("Remove");
		ImGui::PopID();
		if (remove) controllers.remove(i); else ++i;
	}
	if (!any) {
		ImGui::TableNextRow();
		ImGui::TableNextColumn();
		ImGui::TextWrapped("%s", showAllBindings ? "No controls are bound yet." : "No controls bound to this parameter.");
	}
	ImGui::EndTable();
	}
	ImGui::EndChild();
	ImGui::End();
}

void ofApp::applyControllerInput(const std::string &name, const ControllerBindings::Input &event) {
	if (!client.isReadyToSend() || !client.paramExistsForName(name)) return;
	RemoteUIParam p = client.getParamForName(name);
	const float value = ofClamp(event.value, 0.f, 1.f);
	if (event.button) {
		if (p.type != REMOTEUI_PARAM_BOOL) return;
		if (controllers.toggleButtons) { if (value == 0) return; p.boolVal = !p.boolVal; }
		else p.boolVal = value > 0.5f;
	} else {
		switch (p.type) {
			case REMOTEUI_PARAM_BOOL: p.boolVal = value > 0.5f; break;
			case REMOTEUI_PARAM_FLOAT: p.floatVal = p.minFloat + (p.maxFloat - p.minFloat) * value; break;
			case REMOTEUI_PARAM_INT: case REMOTEUI_PARAM_ENUM: p.intVal = (int)std::round(p.minInt + (p.maxInt - p.minInt) * value); break;
			case REMOTEUI_PARAM_COLOR: {
				if (controllers.colorControlsAlpha) p.alphaVal = (unsigned char)std::round(value * 255.f);
				else { float h, s, v, r, g, b; ImGui::ColorConvertRGBtoHSV(p.redVal / 255.f, p.greenVal / 255.f, p.blueVal / 255.f, h, s, v); ImGui::ColorConvertHSVtoRGB(value, s, v, r, g, b); p.redVal = (unsigned char)(r * 255); p.greenVal = (unsigned char)(g * 255); p.blueVal = (unsigned char)(b * 255); }
				break;
			}
			default: return;
		}
	}
	sendEditedParam(name, p);
}

void ofApp::rememberCurrentValuesForDiff() {
	previousParams.clear();
	for (const auto &n : client.getAllParamNamesList()) previousParams.emplace(n, client.getParamForName(n));
	diffParams.clear();
}

void ofApp::captureChangeBaseline() {
	changeBaseline.clear();
	for (const auto &name : client.getAllParamNamesList()) {
		const auto param = client.getParamForName(name);
		if (param.type != REMOTEUI_PARAM_SPACER) changeBaseline.emplace(name, param);
	}
	changedSinceLoad.clear();
}

void ofApp::updateChangedParameter(const std::string &name) {
	const auto baseline = changeBaseline.find(name);
	if (baseline != changeBaseline.end() && client.paramExistsForName(name) &&
		!sameParameterValue(client.getParamForName(name), baseline->second)) changedSinceLoad.insert(name);
	else changedSinceLoad.erase(name);
}

void ofApp::applyPreset(const std::string &name) { rememberCurrentValuesForDiff(); selectedPreset = name; groupPresetSelection.clear(); presetReplyPending = true; client.setPreset(name); }
void ofApp::applyGroupPreset(const std::string &name, const std::string &group) { rememberCurrentValuesForDiff(); selectedPreset.clear(); groupPresetSelection[group] = name; presetReplyPending = true; client.setGroupPreset(name, group); }
void ofApp::applyClipboardValues(const std::string &values, bool highlightChanges) {
	std::unordered_map<std::string, RemoteUIParam> before;
	for (const auto &name : client.getAllParamNamesList()) before.emplace(name, client.getParamForName(name));
	client.setValuesFromString(values);
	if (highlightChanges) diffParams.clear();
	for (const auto &entry : before) if (entry.second.type != REMOTEUI_PARAM_SPACER && client.paramExistsForName(entry.first)) {
		const auto current = client.getParamForName(entry.first);
		if (!current.isEqualTo(entry.second)) {
			groupPresetSelection[current.group].clear(); selectedPreset.clear();
			if (highlightChanges && highlightPresetDiff) diffParams.insert(entry.first);
		}
	}
	if (highlightChanges) diffHighlightUntil = diffParams.empty() ? 0.f : ofGetElapsedTimef() + 5.f;
	else appendLog("Applied values from clipboard");
}
void ofApp::selectNeighbor(const Neighbor &n) { host = n.IP; port = n.port; connectTo(host, port); }
bool ofApp::isLocalHost(const std::string &ip) const { return ip == "127.0.0.1" || ip == "localhost" || ip == "::1" || ip.rfind("192.168.", 0) == 0 || ip.rfind("10.", 0) == 0 || ip.rfind("172.16.", 0) == 0; }
void ofApp::setAlwaysOnTop(bool enabled) {
	gui.setFloating(enabled);
}
void ofApp::queuePresetFile(const std::string &path) {
	if (receivedFullSync && client.isReadyToSend()) importPresetFile(path);
	else { pendingPresetFiles.push_back(path); appendLog("Preset waiting for server sync: " + path); }
}
void ofApp::choosePresetFile() {
	auto result = ofSystemLoadDialog("Load RemoteUI preset", false, ofToDataPath("", true));
	if (result.bSuccess) queuePresetFile(result.getPath());
}
void ofApp::importPresetFile(const std::string &path) {
	ofxXmlSettings xml;
	if (!xml.load(path)) { appendLog("Could not open preset " + path); return; }
	std::string values;
	if (xml.pushTag("OFX_REMOTE_UI")) {
		const int version = xml.getValue("OFX_REMOTE_UI_V", -1);
		if (version == 2 && xml.pushTag("OFX_REMOTE_UI_PARAMS")) {
			for (int i = 0; i < xml.getNumTags("P"); ++i) {
				const std::string type = xml.getAttribute("P", "type", "", i);
				const std::string name = xml.getAttribute("P", "name", "", i);
				const bool disabled = xml.getAttribute("P", "disabled", 0, i) != 0;
				if (name.empty() || disabled) continue;
				if (type == "bool") values += name + "=" + (xml.getValue("P", 0, i) ? "1\n" : "0\n");
				else if (type == "float") values += name + "=" + ofToString(xml.getValue("P", 0.0f, i)) + "\n";
				else if (type == "int" || type == "enum") values += name + "=" + ofToString(xml.getValue("P", 0, i)) + "\n";
				else if (type == "string") values += name + "=" + xml.getValue("P", std::string(), i) + "\n";
				else if (type == "color") {
					values += name + "=" + ofToString(xml.getAttribute("P", "c0.red", 0, i)) + " " + ofToString(xml.getAttribute("P", "c1.green", 0, i)) + " " + ofToString(xml.getAttribute("P", "c2.blue", 0, i)) + " " + ofToString(xml.getAttribute("P", "c3.alpha", 255, i)) + "\n";
				}
			}
			xml.popTag();
		}
		xml.popTag();
	}
	if (values.empty()) { appendLog("Preset has no compatible v2 parameter values: " + path); return; }
	applyClipboardValues(values, true);
	appendLog("Loaded preset " + ofFilePath::getFileName(path));
}
void ofApp::saveCollapsedGroups() const {
	ofJson saved = ofJson::object();
	for (const auto &entry : collapsedGroupsByServer) {
		if (entry.second.empty()) continue;
		std::vector<std::string> groups(entry.second.begin(), entry.second.end());
		std::sort(groups.begin(), groups.end());
		saved[entry.first] = groups;
	}
	if (!ofSavePrettyJson(ClientPaths::settingsFile("collapsed-groups.json"), saved))
		ofLogWarning("RemoteUIImGui") << "Could not save collapsed groups";
}

void ofApp::loadCollapsedGroups() {
	const auto path = ClientPaths::existingSettingsFile("collapsed-groups.json");
	if (!ofFile::doesFileExist(path, false)) return;
	try {
		const auto saved = ofLoadJson(path);
		if (!saved.is_object()) throw std::runtime_error("expected an object");
		for (auto it = saved.begin(); it != saved.end(); ++it) {
			if (!it.value().is_array()) continue;
			for (const auto &group : it.value()) if (group.is_string()) collapsedGroupsByServer[it.key()].insert(group.get<std::string>());
		}
	} catch (const std::exception &e) {
		ofLogWarning("RemoteUIImGui") << "Could not load collapsed groups: " << e.what();
	}
}

void ofApp::savePreferences() const {
	std::ofstream out(ClientPaths::settingsFile("remoteui-imgui-client.ini"));
	if (!out) return;
	// Keep the retired filter's slot for compatibility with existing settings.
	out << host << '\n' << port << '\n' << autoDiscovery << '\n' << autoConnectToNewServers << '\n' << onlyAutoConnectToLocalHost << '\n' << continuousRefresh << '\n' << false << '\n' << alwaysOnTop << '\n' << autoScrollLog << '\n' << controllers.toggleButtons << '\n' << controllers.colorControlsAlpha << '\n' << highlightPresetDiff << '\n' << rowHeight << '\n' << windowColor[0] << ' ' << windowColor[1] << ' ' << windowColor[2] << '\n';
	out << columnBaseWidth << '\n';
	out << "highlightChangesSinceLoad=" << (highlightChangesSinceLoad ? 1 : 0) << '\n';
	if (auto window = ofGetWindowPtr()) {
		int x = 0, y = 0, width = 0, height = 0;
		auto *glfwWindow = static_cast<GLFWwindow *>(window->getWindowContext());
		if (glfwWindow) {
			glfwGetWindowPos(glfwWindow, &x, &y);
			glfwGetWindowSize(glfwWindow, &width, &height);
			out << "windowWidth=" << width << '\n' << "windowHeight=" << height << '\n'
				<< "windowX=" << x << '\n' << "windowY=" << y << '\n';
		}
	}
}
void ofApp::loadPreferences() {
	std::ifstream in(ClientPaths::existingSettingsFile("remoteui-imgui-client.ini"));
	if (!in) return;
	std::getline(in, host);
	bool retiredChangedFilter = false;
	in >> port >> autoDiscovery >> autoConnectToNewServers >> onlyAutoConnectToLocalHost >> continuousRefresh >> retiredChangedFilter >> alwaysOnTop >> autoScrollLog;
	in >> controllers.toggleButtons >> controllers.colorControlsAlpha >> highlightPresetDiff >> rowHeight >> windowColor[0] >> windowColor[1] >> windowColor[2];
	float savedColumnWidth;
	if (in >> savedColumnWidth) columnBaseWidth = ofClamp(savedColumnWidth, 220.f, 500.f);
	std::string option;
	while (std::getline(in, option)) {
		if (option == "highlightChangesSinceLoad=1") highlightChangesSinceLoad = true;
		else if (option == "highlightChangesSinceLoad=0") highlightChangesSinceLoad = false;
	}
	if (host.empty()) host = "127.0.0.1";
	std::strncpy(hostBuffer, host.c_str(), sizeof(hostBuffer) - 1);
	setAlwaysOnTop(alwaysOnTop);
}

void ofApp::dragEvent(ofDragInfo dragInfo) {
	for (const auto &path : dragInfo.files) queuePresetFile(path);
}
