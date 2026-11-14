#include "ofApp.h"
#include "imgui.h"
#include "ofAppImGuiHelpers.h"

#include <algorithm>

using namespace ofAppImGuiHelpers;

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

