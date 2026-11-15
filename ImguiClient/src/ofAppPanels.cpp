#include "ofApp.h"
#include "imgui.h"
#include "ofAppImGuiHelpers.h"

#include <algorithm>

using namespace ofAppImGuiHelpers;

void ofApp::drawLogPanel() {
	ImGui::SetNextWindowSize(ImVec2(650, 190), ImGuiCond_FirstUseEver);
	prepareSecondaryWindow();
	if (!ImGui::Begin("Action log", &showLogPanel, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoDocking)) { ImGui::End(); ImGui::PopStyleVar(); return; }
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
	if (!ImGui::Begin("Server log", &showServerLogPanel, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoDocking)) { ImGui::End(); ImGui::PopStyleVar(); return; }
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
	if (!ImGui::Begin("Settings", &showSettings, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoDocking)) { ImGui::End(); ImGui::PopStyleVar(); return; }
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
	if (ImGui::SliderFloat("UI Scale", &uiScalePercent, 100.f * 10.f / 17.f, 100.f * 46.f / 17.f, "%.0f%%")) rowHeight = 2.f + 17.f * uiScalePercent / 100.f;
	ImGui::SliderFloat("Column width", &columnBaseWidth, 220.f, 500.f, "%.0f px");
	if (ImGui::IsItemHovered()) ImGui::SetTooltip("Smaller values allow more columns. The threshold also follows parameter row height.");
	if (ImGui::ColorEdit3("Window color", windowColor)) applyWindowColor(windowColor);
	ImGui::Spacing();
	ImGui::Spacing();
	const float saveButtonWidth = ImGui::CalcTextSize("Save preferences").x + ImGui::GetStyle().FramePadding.x * 2.f;
	ImGui::SetCursorPosX(ImGui::GetWindowContentRegionMax().x - saveButtonWidth);
	if (ImGui::Button("Save preferences", ImVec2(saveButtonWidth, 0))) savePreferences();
	ImGui::End();
}

void ofApp::drawBindingsPanel() {
	ImGui::SetNextWindowSize(ImVec2(720, 300), ImGuiCond_FirstUseEver);
	if (focusBindings) { ImGui::SetNextWindowFocus(); focusBindings = false; }
	prepareSecondaryWindow();
	if (!ImGui::Begin("Controller bindings", &showBindings, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoDocking)) { ImGui::End(); ImGui::PopStyleVar(); return; }
	std::vector<std::string> servers;
	for (const auto &binding : controllers.bindings) servers.push_back(binding.server);
	if (client.isReadyToSend()) servers.push_back(bindingServer());
	if (!bindingServerFilter.empty()) servers.push_back(bindingServerFilter);
	std::sort(servers.begin(), servers.end());
	servers.erase(std::unique(servers.begin(), servers.end()), servers.end());
	const char *serverFilterLabel = bindingServerFilter.empty() ? "All servers" : bindingServerFilter.c_str();
	if (ImGui::BeginTable("binding-filters", 2, ImGuiTableFlags_SizingStretchSame)) {
		ImGui::TableNextRow();
		ImGui::TableNextColumn();
		ImGui::AlignTextToFramePadding();
		ImGui::TextUnformatted("Filter By Server:");
		ImGui::SameLine();
		ImGui::SetNextItemWidth(-1.f);
		if (ImGui::BeginCombo("##server-filter", serverFilterLabel)) {
			if (ImGui::Selectable("All servers", bindingServerFilter.empty())) bindingServerFilter.clear();
			for (const auto &server : servers) {
				if (server.empty()) continue;
				if (ImGui::Selectable(server.c_str(), bindingServerFilter == server)) bindingServerFilter = server;
			}
			ImGui::EndCombo();
		}
		ImGui::TableNextColumn();
		ImGui::AlignTextToFramePadding();
		ImGui::TextUnformatted("Filter By Param:");
		ImGui::SameLine();
		const float clearButtonWidth = ImGui::CalcTextSize("Clear").x + ImGui::GetStyle().FramePadding.x * 2.f;
		const float searchWidth = std::max(1.f, ImGui::GetContentRegionAvail().x - clearButtonWidth - ImGui::GetStyle().ItemSpacing.x);
		ImGui::SetNextItemWidth(searchWidth);
		ImGui::InputTextWithHint("##binding-param-filter", "Search parameter names", bindingSearchBuffer, sizeof(bindingSearchBuffer));
		ImGui::SameLine();
		if (ImGui::Button("Clear", ImVec2(clearButtonWidth, 0))) bindingSearchBuffer[0] = '\0';
		ImGui::EndTable();
	}
	ImGui::Separator();
	std::vector<size_t> visibleBindingIndices;
	visibleBindingIndices.reserve(controllers.bindings.size());
	for (size_t i = 0; i < controllers.bindings.size(); ++i) {
		if (ControllerBindings::matchesFilters(controllers.bindings[i], bindingServerFilter, bindingSearchBuffer)) visibleBindingIndices.push_back(i);
	}
	const bool any = !visibleBindingIndices.empty();
	const float buttonRowHeight = ImGui::GetFrameHeight();
	const float footerSpace = buttonRowHeight + ImGui::GetStyle().ItemSpacing.y * 2.f + 1.f;
	ImGui::BeginChild("bindings-list", ImVec2(0, -footerSpace));
	const float removeColumnWidth = std::max(ImGui::CalcTextSize("Remove").x, ImGui::CalcTextSize("Action").x) + ImGui::GetStyle().FramePadding.x * 2.f + ImGui::GetStyle().CellPadding.x * 2.f;
	const float invertColumnWidth = ImGui::CalcTextSize("Invert").x + ImGui::GetStyle().FramePadding.x * 2.f + ImGui::GetStyle().CellPadding.x * 2.f;
	const float tableInnerWidth = std::max(ImGui::GetContentRegionAvail().x, ImGui::GetFontSize() * 36.f + removeColumnWidth);
	if (ImGui::BeginTable("bindings-table", 5, ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollX, ImVec2(0, 0), tableInnerWidth)) {
	ImGui::TableSetupColumn("Parameter", ImGuiTableColumnFlags_WidthStretch, 1.2f);
	ImGui::TableSetupColumn("Controller", ImGuiTableColumnFlags_WidthStretch, 2.0f);
	ImGui::TableSetupColumn("Server", ImGuiTableColumnFlags_WidthStretch, 1.0f);
	ImGui::TableSetupColumn("Invert", ImGuiTableColumnFlags_WidthFixed, invertColumnWidth);
	ImGui::TableSetupColumn("Action", ImGuiTableColumnFlags_WidthFixed, removeColumnWidth);
	ImGui::TableHeadersRow();
		size_t pendingRemoval = controllers.bindings.size();
		ImGuiListClipper clipper;
		clipper.Begin((int)visibleBindingIndices.size());
		while (clipper.Step()) for (int row = clipper.DisplayStart; row < clipper.DisplayEnd; ++row) {
		const size_t i = visibleBindingIndices[(size_t)row];
		const auto &b = controllers.bindings[i];
		ImGui::PushID((int)i);
		ImGui::TableNextRow();
		if (controllers.wasRecentlyActivated(b)) {
			const ImVec4 activeRowColor(
				windowColor[0] + (1.f - windowColor[0]) * .28f,
				windowColor[1] + (1.f - windowColor[1]) * .28f,
				windowColor[2] + (1.f - windowColor[2]) * .28f,
				1.f);
			ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, ImGui::ColorConvertFloat4ToU32(activeRowColor));
		}
		ImGui::TableNextColumn();
		if (client.isReadyToSend() && b.server == bindingServer() && client.paramExistsForName(b.param)) {
			const auto param = client.getParamForName(b.param);
			if (param.a) ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, IM_COL32(25 + param.r * 35 / 100, 25 + param.g * 35 / 100, 25 + param.b * 35 / 100, 255));
		}
		ImGui::TextUnformatted(b.param.c_str());
		ImGui::TableNextColumn();
		const std::string controllerLabel = ControllerBindings::label(b);
		ImGui::TextUnformatted(controllerLabel.c_str());
		ImGui::TableNextColumn(); ImGui::TextUnformatted(b.server.c_str());
		ImGui::TableNextColumn();
		const bool analogRange = b.kind == "MIDI CC" || b.kind == "Joystick Axis";
		bool invertRange = b.invertRange;
		ImGui::BeginDisabled(!analogRange);
		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + std::max(0.f, (ImGui::GetContentRegionAvail().x - ImGui::GetFrameHeight()) * .5f));
		if (ImGui::Checkbox("##invert-range", &invertRange) && analogRange) controllers.setRangeInverted(i, invertRange);
		ImGui::EndDisabled();
		ImGui::TableNextColumn(); bool remove = ImGui::SmallButton("Remove");
		ImGui::PopID();
		if (remove) pendingRemoval = i;
	}
	if (!any) {
		ImGui::TableNextRow();
		ImGui::TableNextColumn();
		ImGui::TextWrapped("%s", (!bindingServerFilter.empty() || bindingSearchBuffer[0] != '\0') ? "No bindings match these filters." : "No controls are bound yet.");
	}
		ImGui::EndTable();
		if (pendingRemoval < controllers.bindings.size()) controllers.remove(pendingRemoval);
	}
	ImGui::EndChild();
	ImGui::Separator();
	const float removeAllWidth = ImGui::CalcTextSize("Remove All Bindings").x + ImGui::GetStyle().FramePadding.x * 2.f;
	const float importWidth = ImGui::CalcTextSize("Import Bindings...").x + ImGui::GetStyle().FramePadding.x * 2.f;
	const float exportWidth = ImGui::CalcTextSize("Export Bindings...").x + ImGui::GetStyle().FramePadding.x * 2.f;
	const float buttonsWidth = removeAllWidth + ImGui::GetStyle().ItemSpacing.x * 2.f + importWidth + exportWidth;
	ImGui::SetCursorPosX(ImGui::GetWindowContentRegionMax().x - buttonsWidth);
	ImGui::BeginDisabled(controllers.bindings.empty());
	if (ImGui::Button("Remove All Bindings", ImVec2(removeAllWidth, 0))) ImGui::OpenPopup("Confirm remove all bindings");
	ImGui::EndDisabled();
	pushPopupWindowPadding();
	if (ImGui::BeginPopupModal("Confirm remove all bindings", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
		ImGui::TextUnformatted("Remove all controller bindings from every server?");
		ImGui::Spacing();
		if (ImGui::Button("Cancel")) ImGui::CloseCurrentPopup();
		ImGui::SameLine();
		if (ImGui::Button("Remove All")) {
			controllers.removeAll();
			appendLog("Removed all controller bindings");
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
	}
	ImGui::PopStyleVar();
	ImGui::SameLine();
	if (ImGui::Button("Import Bindings...", ImVec2(importWidth, 0))) chooseControllerBindingsFile();
	ImGui::SameLine();
	if (ImGui::Button("Export Bindings...", ImVec2(exportWidth, 0))) exportControllerBindingsFile();
	ImGui::End();

}

