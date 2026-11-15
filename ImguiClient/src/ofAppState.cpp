#include "ofApp.h"
#include "ofAppImGuiHelpers.h"
#include <GLFW/glfw3.h>
#include "imgui.h"
#include "ofxXmlSettings.h"
#include "ClientPaths.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <unordered_map>

using namespace ofAppImGuiHelpers;
std::string ofApp::bindingServer() const { return host + ":" + ofToString(port); }
void ofApp::updateLocalHostAddress() {
	std::string subnetMask;
	localHostAddress = client.getMyIP("", subnetMask);
}

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
	boundBlinkUntil = blinkingBoundParams.empty() ? 0.f : ofGetElapsedTimef() + boundBlinkDurationSeconds();
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
	if (highlightChanges) diffHighlightUntil = diffParams.empty() ? 0.f : ofGetElapsedTimef() + presetDiffHighlightDurationSeconds();
	else appendLog("Applied values from clipboard");
}
void ofApp::selectNeighbor(const Neighbor &n) { host = n.IP; port = n.port; connectTo(host, port); }
bool ofApp::isLocalHost(const std::string &ip) const {
	if (ip == "localhost" || ip == "::1" || ip.compare(0, 4, "127.") == 0) return true;
	return !localHostAddress.empty() && ip == localHostAddress;
}
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
void ofApp::chooseControllerBindingsFile() {
	auto result = ofSystemLoadDialog("Import controller bindings");
	if (result.bSuccess) importControllerBindings(result.getPath());
}
void ofApp::importControllerBindings(const std::string &path) {
	std::string error;
	if (!controllers.importFile(path, error)) {
		appendLog("Could not import controller bindings: " + error);
		return;
	}
	appendLog("Imported " + ofToString(controllers.bindings.size()) + " controller binding(s) from " + ofFilePath::getFileName(path));
}
void ofApp::exportControllerBindingsFile() {
	auto result = ofSystemSaveDialog("controller-bindings.ctrlBind", "Export controller bindings for all servers");
	if (!result.bSuccess) return;
	std::string path = result.getPath();
	if (ofToLower(ofFilePath::getFileExt(path)) != "ctrlbind") path += ".ctrlBind";
	std::string error;
	size_t exportedCount = 0;
	if (!controllers.exportFile(path, bindingServerFilter, bindingSearchBuffer, exportedCount, error)) {
		appendLog("Could not export controller bindings: " + error);
		return;
	}
	appendLog("Exported " + ofToString(exportedCount) + " controller binding(s) to " + ofFilePath::getFileName(path));
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
		ofLogWarning("ofxRemoteUIClient") << "Could not save collapsed groups";
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
		ofLogWarning("ofxRemoteUIClient") << "Could not load collapsed groups: " << e.what();
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
	for (const auto &path : dragInfo.files) {
		if (ofToLower(ofFilePath::getFileExt(path)) == "ctrlbind") importControllerBindings(path);
		else queuePresetFile(path);
	}
}
