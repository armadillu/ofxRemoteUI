#include "ofApp.h"
#include "imgui.h"
#include "ofAppImGuiHelpers.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstring>
#include <ctime>
#include <type_traits>

using namespace ofAppImGuiHelpers;

static float sliderGrabHeight(float frameHeight) {
	// ImGui insets horizontal slider grabs by two pixels on each edge.
	return std::max(1.f, frameHeight - 4.f);
}

template <typename DrawControl>
static bool drawControlAtHeight(float height, float y, DrawControl drawControl) {
	ImGui::SetCursorScreenPos(ImVec2(ImGui::GetCursorScreenPos().x, y));
	const float fontScale = std::min(1.f, std::floor(height) / ImGui::GetFontSize());
	ImGui::PushFont(nullptr, ImGui::GetStyle().FontSizeBase * fontScale);
	ImGui::PushStyleVarY(ImGuiStyleVar_FramePadding, std::max(0.f, (height - ImGui::GetFontSize()) * .5f));
	const bool activated = drawControl();
	ImGui::PopStyleVar();
	ImGui::PopFont();
	return activated;
}

static bool drawColorEditor(float color[4], float width) {
	const auto &style = ImGui::GetStyle();
	const float height = ImGui::GetFrameHeight();
	const float spacing = style.ItemInnerSpacing.x;
	const float swatchWidth = std::clamp(width - 4.f * (1.f + spacing), 1.f, 2.f * height);
	const auto options = ImGui::GetIO().ConfigColorEditFlags;
	bool changed = false;

	auto drawChannels = [&] {
		ImGui::SetNextItemWidth(std::max(1.f, width - swatchWidth - spacing));
		changed |= ImGui::ColorEdit4("##value", color, ImGuiColorEditFlags_NoSmallPreview);
	};
	auto drawSwatch = [&] {
		const auto previewFlags = options & (ImGuiColorEditFlags_AlphaOpaque | ImGuiColorEditFlags_AlphaNoBg |
			ImGuiColorEditFlags_AlphaPreviewHalf | ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoDragDrop);
		const bool clicked = ImGui::ColorButton("##color-swatch", ImVec4(color[0], color[1], color[2], color[3]), previewFlags, ImVec2(swatchWidth, height));
		const ImGuiID originalIds[] = {
			ImGui::GetID("##original-red"), ImGui::GetID("##original-green"),
			ImGui::GetID("##original-blue"), ImGui::GetID("##original-alpha")
		};
		ImGuiStorage *storage = ImGui::GetStateStorage();
		if (clicked) {
			for (int i = 0; i < 4; ++i) storage->SetFloat(originalIds[i], color[i]);
			ImGui::OpenPopup("##color-picker");
			ImGui::SetNextWindowPos(ImVec2(ImGui::GetItemRectMin().x, ImGui::GetItemRectMax().y + style.ItemSpacing.y));
		}
		if (!(previewFlags & ImGuiColorEditFlags_NoDragDrop) && ImGui::BeginDragDropTarget()) {
			const ImGuiPayload *payload = ImGui::AcceptDragDropPayload(IMGUI_PAYLOAD_TYPE_COLOR_4F);
			if (!payload) payload = ImGui::AcceptDragDropPayload(IMGUI_PAYLOAD_TYPE_COLOR_3F);
			if (payload) {
				std::memcpy(color, payload->Data, payload->DataSize);
				changed = true;
			}
			ImGui::EndDragDropTarget();
		}
		if (ImGui::BeginPopup("##color-picker")) {
			float original[4];
			for (int i = 0; i < 4; ++i) original[i] = storage->GetFloat(originalIds[i], color[i]);
			ImGui::SetNextItemWidth(height * 12.f);
			changed |= ImGui::ColorPicker4("##picker", color, ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_NoLabel |
				ImGuiColorEditFlags_AlphaPreviewHalf | ImGuiColorEditFlags_DisplayRGB | ImGuiColorEditFlags_DisplayHSV | ImGuiColorEditFlags_DisplayHex, original);
			ImGui::EndPopup();
		}
	};

	if (style.ColorButtonPosition == ImGuiDir_Left) {
		drawSwatch();
		ImGui::SameLine(0.f, spacing);
		drawChannels();
	} else {
		drawChannels();
		ImGui::SameLine(0.f, spacing);
		drawSwatch();
	}
	return changed;
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
	const float topSectionGap = 10.f * topScale;
	const float topSectionWidth = std::max(1.f, (ImGui::GetContentRegionAvail().x - topSectionGap) * .5f);
	const float topSectionHeight = ImGui::GetTextLineHeight() + ImGui::GetFrameHeight() + ImGui::GetStyle().ItemSpacing.y + ImGui::GetStyle().WindowPadding.y * 2.f;
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
	if (ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip | ImGuiHoveredFlags_AllowWhenDisabled)) ImGui::SetTooltip("Save current values as a new global preset");
	if (ImGui::BeginPopup("save-global-preset")) {
		ImGui::TextUnformatted("Save global preset");
		ImGui::SetNextItemWidth(presetNameInputWidth() * topScale);
		if (ImGui::IsWindowAppearing()) ImGui::SetKeyboardFocusHere();
		const bool submit = ImGui::InputText("##global-preset-name", presetNameBuffer, sizeof(presetNameBuffer), ImGuiInputTextFlags_EnterReturnsTrue);
		if ((ImGui::Button("Save") || submit) && presetNameBuffer[0]) { client.savePresetWithName(presetNameBuffer); ImGui::CloseCurrentPopup(); }
		ImGui::SameLine(); if (ImGui::Button("Cancel")) ImGui::CloseCurrentPopup();
		ImGui::EndPopup();
	}
	ImGui::SameLine();
	if (selectedPreset.empty()) ImGui::BeginDisabled();
	if (ImGui::Button("-##global")) client.deletePreset(selectedPreset);
	if (ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip | ImGuiHoveredFlags_AllowWhenDisabled)) ImGui::SetTooltip("Delete the selected global preset");
	if (selectedPreset.empty()) ImGui::EndDisabled();
	if (!client.isReadyToSend()) ImGui::EndDisabled();
	ImGui::EndChild();
	ImGui::SameLine(0.f, topSectionGap);
	ImGui::BeginChild("group-filter-controls", ImVec2(0, topSectionHeight), ImGuiChildFlags_AlwaysUseWindowPadding);
	ImGui::TextUnformatted("Filter Parameters");
	const float clearWidth = ImGui::CalcTextSize("Clear").x + ImGui::GetStyle().FramePadding.x * 2.f;
	ImGui::SetNextItemWidth(std::max(1.f, ImGui::GetContentRegionAvail().x - clearWidth - ImGui::GetStyle().ItemSpacing.x));
	ImGui::InputTextWithHint("##filter", "type here...", filterBuffer, sizeof(filterBuffer));
	ImGui::SameLine();
	if (ImGui::Button("Clear")) filterBuffer[0] = '\0';
	ImGui::EndChild();
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
	const float footerWidth = std::max(1.f, ImGui::GetContentRegionAvail().x - ImGui::GetStyle().WindowPadding.x * 2.f);
	const float footerSpacing = ImGui::GetStyle().ItemSpacing.x;
	auto footerButtonWidth = [](const char *label) { return ImGui::CalcTextSize(label).x + ImGui::GetStyle().FramePadding.x * 2.f; };
	const float hostWidth = std::max(1.f, std::floor(std::min(110.f * footerScale, footerWidth - ImGui::CalcTextSize("Host:").x - footerSpacing)));
	const float portWidth = std::max(1.f, std::floor(std::min(60.f * footerScale, footerWidth - ImGui::CalcTextSize("Port:").x - footerSpacing)));
	const float hostPortGap = 10.f * footerScale;
	const std::string pendingText = pendingPresetFiles.empty() ? "" : ofToString(pendingPresetFiles.size()) + " preset file(s) waiting for sync";
	const auto neighbors = client.getNeighbors();
	const char *serverLabel = "Servers:";
	std::string serverPreview = "(" + ofToString(neighbors.size()) + ")";
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
	const float nonStopLabelWidth = ImGui::CalcTextSize("Non-Stop").x;
	const float syncControlsWidth = dividerWidth + footerSpacing + footerButtonWidth("Sync") + syncContinuousGap + ImGui::GetFrameHeight() + ImGui::GetStyle().ItemInnerSpacing.x + nonStopLabelWidth;
	const std::vector<float> connectionWidths = {
		ImGui::CalcTextSize("Host:").x + footerSpacing + hostWidth,
		hostPortGap + ImGui::CalcTextSize("Port:").x + footerSpacing + portWidth,
		ImGui::GetFrameHeight() + footerSpacing + connectButtonWidth,
		pendingText.empty() ? 0.f : ImGui::CalcTextSize(pendingText.c_str()).x
	};
	const float connectionRowWidth = connectionWidths[0] + connectionWidths[1] + connectionWidths[2] + footerSpacing * 2.f;
	const float serverLabelWidth = ImGui::CalcTextSize(serverLabel).x + footerSpacing;
	const float minDropdownWidth = ImGui::GetFrameHeight() + footerButtonWidth("(0)");
	const float serverDropdownWidth = std::floor(std::clamp(connectionRowWidth - serverLabelWidth - footerSpacing - syncControlsWidth,
		std::min(minDropdownWidth, std::max(1.f, footerWidth - serverLabelWidth)), std::max(1.f, footerWidth - serverLabelWidth)));
	const std::vector<float> actionWidths = {serverLabelWidth + serverDropdownWidth, syncControlsWidth};
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
	auto connectionLines = footerLineWidths(connectionWidths);
	auto actionLines = footerLineWidths(actionWidths);
	if (connectionRowWidth <= footerWidth && actionLines.size() == 1) {
		// Center both rows together so their first labels share a left edge.
		connectionLines.front() = actionLines.front() = std::max(connectionLines.front(), actionLines.front());
	}
	const int footerLines = static_cast<int>(connectionLines.size() + actionLines.size());
	const float footerHeight = ImGui::GetFrameHeight() * footerLines + ImGui::GetStyle().ItemSpacing.y * (footerLines - 1) + ImGui::GetStyle().WindowPadding.y * 2.f + footerRowGap;
	ImGui::PopFont();

	
	const float parameterListGap = 6.f;
	ImGui::SetCursorPosY(ImGui::GetCursorPosY() + parameterListGap);
	ImGui::PushStyleColor(ImGuiCol_ScrollbarBg, ImVec4(0.f, 0.f, 0.f, 0.f));
	if (ImGui::BeginChild("param-list", ImVec2(0, -footerHeight - ImGui::GetStyle().ItemSpacing.y - parameterListGap), ImGuiChildFlags_AlwaysUseWindowPadding)) {
		if (!client.isReadyToSend()) {
			ImGui::PushFont(nullptr, ImGui::GetStyle().FontSizeBase * topScale);
			ImGui::TextDisabled("Connect to an ofxRemoteUI Server to edit its parameters.");
			ImGui::PopFont();
		} else if (!receivedFullSync) {
			ImGui::PushFont(nullptr, ImGui::GetStyle().FontSizeBase * topScale);
			ImGui::TextDisabled("Waiting for server parameter data...");
			ImGui::PopFont();
		} else if (names.empty()) {
			ImGui::PushFont(nullptr, ImGui::GetStyle().FontSizeBase * topScale);
			ImGui::TextDisabled("Sync completed with no parameters. Check the server and try Sync.");
			ImGui::PopFont();
		}
		ImGui::PushStyleVarY(ImGuiStyleVar_ItemSpacing, 2.0f);
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
		const float parameterIndent = ImGui::GetStyle().IndentSpacing * .5f;
		const float groupBottomPadding = 4.f;
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
			bool drawContent = hasContent;
			bool animationChild = false;
			if (animating) {
				const float progress = ofClamp((groupAnimationNow - animation->second) / groupAnimationDuration, 0.f, 1.f);
				constexpr float curveStrength = 1.25f;
				const float eased = .5f + .5f * std::tanh((2.f * progress - 1.f) * curveStrength) / std::tanh(curveStrength);
				const float fraction = collapsing ? 1.f - eased : eased;
				const auto height = groupContentHeights.find(group);
				const float rowSpacing = ImGui::GetStyle().ItemSpacing.y;
				const float estimatedHeight = groupBottomPadding + (blocks[i].size() - firstRow) * (ImGui::GetFrameHeight() + rowSpacing);
				const float fullHeight = height != groupContentHeights.end() ? height->second : estimatedHeight;
				const float animatedHeight = std::floor(fullHeight * fraction);
				// ImGui clamps child windows to four pixels. Keep moving through the last pixels.
				animationChild = animatedHeight >= 4.f;
				if (animationChild) {
					drawContent = ImGui::BeginChild("group-animation", ImVec2(0.f, animatedHeight), ImGuiChildFlags_None,
						ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
				} else {
					drawContent = false;
					if (animatedHeight > 0.f) ImGui::Dummy(ImVec2(0.f, animatedHeight));
				}
			}
			if (drawContent) {
				const float contentStartY = ImGui::GetCursorPosY();
				if (parameterIndent > 0.f) ImGui::Indent(parameterIndent);
				for (size_t row = firstRow; row < blocks[i].size(); ++row) {
					const auto &name = blocks[i][row];
					ImGui::PushID(static_cast<int>(row));
					if (continuousRefresh) ImGui::BeginDisabled();
					if (animating) ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
					drawParameter(name);
					if (animating) ImGui::PopItemFlag();
					if (continuousRefresh) ImGui::EndDisabled();
					ImGui::PopID();
				}
				if (parameterIndent > 0.f) ImGui::Unindent(parameterIndent);
				ImGui::Dummy(ImVec2(0.f, groupBottomPadding));
				// EndChild adds the trailing item spacing when it advances the parent cursor.
				const float measuredHeight = ImGui::GetCursorPosY() - contentStartY - ImGui::GetStyle().ItemSpacing.y;
				if (measuredHeight > groupBottomPadding) groupContentHeights[group] = measuredHeight;
			}
			if (animationChild) ImGui::EndChild();
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
		ImGui::PopStyleVar();
	}
	ImGui::EndChild();
	ImGui::PopStyleColor();
	if (bindingPopupRequested) {
		const ImGuiViewport *viewport = ImGui::GetMainViewport();
		ImGui::SetNextWindowPos(ImVec2(viewport->WorkPos.x + viewport->WorkSize.x * .5f,
			viewport->WorkPos.y + viewport->WorkSize.y * .5f), ImGuiCond_Appearing, ImVec2(.5f, .5f));
		ImGui::OpenPopup("Param <-> Controller Binding...");
		bindingPopupRequested = false;
		bindingPopupActive = true;
	}
	if (bindingPopupActive) {
		const bool listeningForBinding = !controllers.learning.empty();
		const ImVec4 popupBg = ImGui::GetStyle().Colors[ImGuiCol_PopupBg];
		if (listeningForBinding) {
			const float pulse = .5f + .5f * std::sin(ofGetElapsedTimef() * 2.f * 3.14159265f);
			const float redBlend = .15f * pulse;
			ImGui::PushStyleColor(ImGuiCol_PopupBg, ImVec4(
				popupBg.x * (1.f - redBlend) + redBlend,
				popupBg.y * (1.f - redBlend),
				popupBg.z * (1.f - redBlend),
				popupBg.w));
		}
		if (ImGui::BeginPopupModal("Param <-> Controller Binding...", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
			if (controllers.learning.empty()) {
				ImGui::CloseCurrentPopup();
				bindingPopupActive = false;
			} else {
				ImGui::Text("Waiting for controller input to connect to: \"%s\"",controllers.learning.c_str());
				ImGui::Dummy(ImVec2(0.f, ImGui::GetTextLineHeight() * .2f));
				ImGui::Separator();
				ImGui::Dummy(ImVec2(0.f, ImGui::GetTextLineHeight() * .2f));
				ImGui::TextUnformatted("Move/Press any MIDI or joystick control\nto bind it to this parameter.");
				ImGui::Dummy(ImVec2(0.f, ImGui::GetTextLineHeight() * .5f));
				const float cancelWidth = ImGui::CalcTextSize("Cancel Bind").x + ImGui::GetStyle().FramePadding.x * 2.f;
				ImGui::SetCursorPosX(std::max(ImGui::GetCursorPosX(), ImGui::GetWindowContentRegionMax().x - cancelWidth));
				if (ImGui::Button("Cancel Bind")) {
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
		if (listeningForBinding) ImGui::PopStyleColor();
	}
	ImGui::SetCursorPosY(ImGui::GetCursorPosY() + parameterListGap);
	ImGui::PushFont(nullptr, ImGui::GetStyle().FontSizeBase * footerScale);
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
	ImGui::AlignTextToFramePadding();
	ImGui::TextUnformatted(serverLabel);
	ImGui::SameLine();
	ImGui::SetNextItemWidth(serverDropdownWidth);
	const bool serverDropdownOpen = ImGui::BeginCombo("##servers", serverPreview.c_str());
		if (ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip)) {
			ImGui::SetNextWindowViewport(ImGui::GetMainViewport()->ID);
			ImGui::SetTooltip("Connect ofxRemoteUIServers running on this lan");
		}
	if (serverDropdownOpen) {
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
	ImGui::PopFont();
	ImGui::End();
}

void ofApp::drawGroupHeader(const std::string &name, const RemoteUIParam &param) {
	ImGui::PushID(name.c_str());
	ImGui::PushFont(gui.getGroupHeaderFont(), ImGui::GetStyle().FontSizeBase * parameterFontScale(rowHeight));
	const auto &style = ImGui::GetStyle();
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
	const float arrowSize = fontSize * .70f;
	const float arrowX = pos.x + style.FramePadding.x, arrowY = pos.y + (height - arrowSize) * .5f;
	const float titleX = arrowX + arrowSize + style.ItemInnerSpacing.x;
	ImDrawList *headerDrawList = ImGui::GetWindowDrawList();
	headerDrawList->PushClipRect(pos, ImVec2(pos.x + titleWidth, pos.y + height), true);
	const ImU32 textColor = ImGui::GetColorU32(ImGuiCol_Text);
	if (collapsedGroups.count(group)) {
		headerDrawList->AddTriangleFilled(ImVec2(arrowX, arrowY), ImVec2(arrowX + arrowSize, arrowY + arrowSize * .5f), ImVec2(arrowX, arrowY + arrowSize), textColor);
	} else {
		headerDrawList->AddTriangleFilled(ImVec2(arrowX, arrowY), ImVec2(arrowX + arrowSize, arrowY), ImVec2(arrowX + arrowSize * .5f, arrowY + arrowSize), textColor);
	}
	headerDrawList->AddText(ImVec2(titleX, pos.y + style.FramePadding.y), textColor, param.stringVal.c_str());
	headerDrawList->PopClipRect();
	const float titleTextWidth = std::max(0.f, pos.x + titleWidth - titleX);
	if (ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip) && ImGui::CalcTextSize(param.stringVal.c_str()).x > titleTextWidth) {
		ImGui::SetNextWindowViewport(ImGui::GetMainViewport()->ID);
		ImGui::SetTooltip("%s", param.stringVal.c_str());
	}
	ImGui::PopStyleColor(3);
	ImGui::PopFont();
	ImGui::PushFont(nullptr, ImGui::GetStyle().FontSizeBase * parameterFontScale(rowHeight));
	ImGui::SameLine();
	const bool canUseGroup = client.isReadyToSend() && !group.empty();
	const float presetHeight = sliderGrabHeight(ImGui::GetFrameHeight());
	const float presetY = pos.y + (height - presetHeight) * .5f;
	const float presetSignWidth = std::max(ImGui::CalcTextSize("+").x, ImGui::CalcTextSize("-").x) + 2.f * style.FramePadding.x;
	const float presetButtonWidth = std::min(std::max({24.f, fontSize * 1.5f, presetSignWidth}), std::max(1.f, (controlsWidth - 40.f - ImGui::GetStyle().ItemSpacing.x * 2.f) * .5f));
	const float buttonR = param.r / 255.f, buttonG = param.g / 255.f, buttonB = param.b / 255.f;
	auto presetButton = [&](const char *label) {
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(.08f + buttonR * .35f, .08f + buttonG * .35f, .08f + buttonB * .35f, 1.f));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(.16f + buttonR * .45f, .16f + buttonG * .45f, .16f + buttonB * .45f, 1.f));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(.22f + buttonR * .5f, .22f + buttonG * .5f, .22f + buttonB * .5f, 1.f));
		const bool pressed = drawControlAtHeight(presetHeight, presetY, [&] { return ImGui::Button(label, ImVec2(presetButtonWidth, presetHeight)); });
		ImGui::PopStyleColor(3);
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
		ImGui::SetNextItemWidth(presetNameInputWidth());
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
	ImGui::SetNextItemWidth(std::max(1.f, ImGui::GetContentRegionAvail().x - 1.5f * parameterFontScale(rowHeight)));
	const float r = param.r / 255.f, g = param.g / 255.f, b = param.b / 255.f;
	ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(.12f + r * .32f, .12f + g * .32f, .12f + b * .32f, 1.f));
	ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(.16f + r * .38f, .16f + g * .38f, .16f + b * .38f, 1.f));
	ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(.20f + r * .42f, .20f + g * .42f, .20f + b * .42f, 1.f));
	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(.16f + r * .40f, .16f + g * .40f, .16f + b * .40f, 1.f));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(.20f + r * .48f, .20f + g * .48f, .20f + b * .48f, 1.f));
	if (drawControlAtHeight(presetHeight, presetY, [&] { return ImGui::BeginCombo("##group-preset", selected.empty() ? "No preset" : selected.c_str()); })) {
		const std::string prefix = group + "/";
		for (const auto &preset : client.getPresetsList()) {
			if (preset.compare(0, prefix.size(), prefix) != 0) continue;
			const std::string presetName = preset.substr(prefix.size());
			if (ImGui::Selectable(presetName.c_str(), presetName == selected)) applyGroupPreset(presetName, group);
		}
		ImGui::EndCombo();
	}
	ImGui::PopStyleColor(5);
	if (!canUseGroup) ImGui::EndDisabled();
	ImGui::PopFont();
	ImGui::PopID();
}

void ofApp::drawParameter(const std::string &name) {
	auto & style = ImGui::GetStyle();
	RemoteUIParam p = client.getParamForName(name);
	ImGui::PushID(name.c_str());
	ImGui::PushFont(nullptr, ImGui::GetStyle().FontSizeBase * parameterFontScale(rowHeight));
	bool edited = false;
	const float contentWidth = std::max(1.f, ImGui::GetContentRegionAvail().x - 6.f);
	const float controlLabelGap = std::min(contentWidth - 1.f, style.ItemSpacing.x + 4.f * parameterFontScale(rowHeight));
	const float splitWidth = contentWidth - controlLabelGap;
	const float labelWidth = splitWidth * .4f;
	const float controlWidth = splitWidth * .6f;
	const float controlStartX = ImGui::GetCursorScreenPos().x;
	const float rectRounding = style.FrameRounding;
	ImGui::SetNextItemWidth(controlWidth);
	// RUI_NEW_COLOR() changes the background alpha of subsequent parameters.
	if (p.a) {
		const ImVec2 rowMin = ImGui::GetCursorScreenPos();
		const ImVec2 rowMax(rowMin.x + contentWidth, rowMin.y + ImGui::GetFrameHeight());
		ImGui::GetWindowDrawList()->AddRectFilled(rowMin, rowMax, IM_COL32(p.r, p.g, p.b, p.a), rectRounding);
	}
	ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(.08f, .08f, .08f, .25f));
	ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(.08f, .08f, .08f, .38f));
	ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(.08f, .08f, .08f, .48f));
	const bool coloredSlider = p.a && ((p.type == REMOTEUI_PARAM_FLOAT && p.minFloat < p.maxFloat) ||
		(p.type == REMOTEUI_PARAM_INT && p.minInt < p.maxInt));
	if (coloredSlider) {
		const float r = p.r / 255.f, g = p.g / 255.f, b = p.b / 255.f;
		ImGui::PushStyleColor(ImGuiCol_SliderGrab, ImVec4(.22f + .48f * r, .22f + .48f * g, .22f + .48f * b, 1.f));
		ImGui::PushStyleColor(ImGuiCol_SliderGrabActive, ImVec4(.35f + .42f * r, .35f + .42f * g, .35f + .42f * b, 1.f));
	}
	auto drawNumericSlider = [&](auto &value, auto min, auto max, auto drawSlider) {
		ImGuiIO &io = ImGui::GetIO();
		const float realMouseX = io.MousePos.x;
		const auto previousValue = value;
		const bool tracking = activeMouseSlider == name && ImGui::IsMouseDown(ImGuiMouseButton_Left) && ImGui::IsMousePosValid();
		const ImVec2 sliderPos = ImGui::GetCursorScreenPos();
		const float sliderWidth = ImGui::CalcItemWidth();
		const bool startRelative = io.KeyShift && !io.KeyCtrl && ImGui::IsMouseClicked(ImGuiMouseButton_Left) &&
			ImGui::IsWindowHovered() && ImGui::IsMouseHoveringRect(sliderPos, ImVec2(sliderPos.x + sliderWidth, sliderPos.y + ImGui::GetFrameHeight()));
		if (startRelative) {
			// Start the native slider drag at the grab center instead of the click position.
			constexpr float grabInset = 2.f;
			const float trackWidth = std::max(0.f, sliderWidth - 2.f * grabInset);
			const double range = static_cast<double>(max) - min;
			float grabWidth = style.GrabMinSize;
			if constexpr (std::is_integral_v<decltype(min)>) {
				grabWidth = std::max(grabWidth, static_cast<float>(trackWidth / (range + 1.0)));
			}
			grabWidth = std::min(grabWidth, trackWidth);
			const float fraction = static_cast<float>(std::clamp((static_cast<double>(value) - min) / range, 0.0, 1.0));
			io.MousePos.x = sliderPos.x + grabInset + grabWidth * .5f + (trackWidth - grabWidth) * fraction;
		} else if (tracking) {
			if (io.KeyAlt || relativeSliderTracking) {
				relativeSliderTracking = true;
				virtualSliderMouseX = ofClamp(virtualSliderMouseX + io.MouseDelta.x * (io.KeyAlt ? .01f : 1.f), controlStartX, controlStartX + controlWidth);
				io.MousePos.x = virtualSliderMouseX;
			} else {
				virtualSliderMouseX = realMouseX;
			}
		}
		const bool changed = drawSlider();
		const float sliderMouseX = io.MousePos.x;
		io.MousePos.x = realMouseX;
		const bool activated = ImGui::IsItemActivated() && ImGui::IsMouseDown(ImGuiMouseButton_Left) && !io.KeyCtrl;
		if (activated) {
			activeMouseSlider = name;
			virtualSliderMouseX = sliderMouseX;
			relativeSliderTracking = startRelative;
		}
		if (ImGui::IsItemDeactivated() && activeMouseSlider == name) {
			activeMouseSlider.clear();
			relativeSliderTracking = false;
		}
		if (ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip)) {
			ImGui::SetNextWindowViewport(ImGui::GetMainViewport()->ID);
			ImGui::SetTooltip("Shift-drag anywhere on the slider to adjust without jumping.\nHold Option / Alt while dragging for fine adjustment.");
		}
		if ((activated && startRelative) || (tracking && relativeSliderTracking && io.MouseDelta.x == 0.f && ImGui::IsItemActive())) {
			value = previousValue;
			return false;
		}
		return changed;
	};
	switch (p.type) {
		case REMOTEUI_PARAM_BOOL: {
			const ImVec2 pos = ImGui::GetCursorScreenPos();
			const float frameHeight = ImGui::GetFrameHeight();
			const float horizontalPadding = style.FramePadding.x;
			const float buttonWidth = std::min(controlWidth, std::max(frameHeight * 2.f, ImGui::CalcTextSize("OFF").x + horizontalPadding * 2.f));
			const float buttonHeight = sliderGrabHeight(frameHeight);
			ImGui::Dummy(ImVec2(0.f, frameHeight));
			ImGui::SameLine(0.f, 0.f);
			ImGui::SetCursorScreenPos(ImVec2(pos.x + (controlWidth - buttonWidth) * .5f, pos.y + (frameHeight - buttonHeight) * .5f));
			const float r = p.a ? p.r / 255.f : .5f, g = p.a ? p.g / 255.f : .5f, b = p.a ? p.b / 255.f : .5f;
			const float tint = p.boolVal ? .45f : .10f;
			const float base = p.boolVal ? .16f : .18f;
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(base + tint * r, base + tint * g, base + tint * b, 1.f));
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(base + .08f + tint * r, base + .08f + tint * g, base + .08f + tint * b, 1.f));
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(base + .14f + tint * r, base + .14f + tint * g, base + .14f + tint * b, 1.f));
			ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, std::max(1.f, style.FrameBorderSize));
			ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(p.r / 655.f, p.g / 655.f, p.b / 655.f, .5f));
			if (ImGui::Button("###value", ImVec2(buttonWidth, buttonHeight))) { p.boolVal = !p.boolVal; edited = true; }
			ImGui::PopStyleColor(4);
			ImGui::PopStyleVar();
			if (ImGui::IsItemVisible()) {
				const char *label = p.boolVal ? "ON" : "OFF";
				ImFont *font = ImGui::GetFont();
				const float labelSize = std::min(ImGui::GetFontSize(), buttonHeight);
				const float textWidth = font->CalcTextSizeA(labelSize, FLT_MAX, 0.f, label).x;
				const ImVec2 min = ImGui::GetItemRectMin(), max = ImGui::GetItemRectMax();
				const ImVec2 textPos(min.x + (buttonWidth - textWidth) * .5f, min.y + (buttonHeight - labelSize) * .5f);
				ImDrawList *drawList = ImGui::GetWindowDrawList();
				drawList->PushClipRect(min, max, true);
				drawList->AddText(font, labelSize, textPos, ImGui::GetColorU32(ImGuiCol_Text), label);
				drawList->PopClipRect();
			}
			break;
		}
		case REMOTEUI_PARAM_FLOAT: {
			float value = p.floatVal;
			if (p.minFloat < p.maxFloat) edited = drawNumericSlider(value, p.minFloat, p.maxFloat, [&] { return ImGui::SliderFloat("##value", &value, p.minFloat, p.maxFloat, "%.4g"); });
			else edited = ImGui::DragFloat("##value", &value, 0.01f, 0, 0, "%.4g");
			p.floatVal = value; break;
		}
		case REMOTEUI_PARAM_INT: {
			int value = p.intVal;
			if (p.minInt < p.maxInt) edited = drawNumericSlider(value, p.minInt, p.maxInt, [&] { return ImGui::SliderInt("##value", &value, p.minInt, p.maxInt); });
			else edited = ImGui::InputInt("##value", &value);
			p.intVal = value; break;
		}
		case REMOTEUI_PARAM_ENUM: {
			int index = p.intVal - p.minInt;
			const char *preview = index >= 0 && index < (int)p.enumList.size() ? p.enumList[index].c_str() : "(invalid)";
			const float frameHeight = ImGui::GetFrameHeight();
			const float comboHeight = sliderGrabHeight(frameHeight);
			const float comboInset = (frameHeight - comboHeight) * .5f;
			const float comboY = ImGui::GetCursorScreenPos().y + comboInset;
			const float leftInset = std::min(comboInset, std::max(0.f, controlWidth - 1.f));
			ImGui::Dummy(ImVec2(0.f, frameHeight));
			ImGui::SameLine(0.f, 0.f);
			ImGui::SetCursorPosX(ImGui::GetCursorPosX() + leftInset);
			ImGui::SetNextItemWidth(controlWidth - leftInset);
			if (drawControlAtHeight(comboHeight, comboY, [&] { return ImGui::BeginCombo("##value", preview); })) {
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
			ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, std::max(1.f, style.FrameBorderSize));
			ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(p.r / 755.f, p.g / 755.f, p.b / 755.f, .5f));
			if (ImGui::InputText("##value", value, sizeof(value), ImGuiInputTextFlags_EnterReturnsTrue)) { p.stringVal = value; edited = true; }
			ImGui::PopStyleColor();
			ImGui::PopStyleVar();
			break;
		}
		case REMOTEUI_PARAM_COLOR: {
			float color[4] = {p.redVal / 255.f, p.greenVal / 255.f, p.blueVal / 255.f, p.alphaVal / 255.f};
			const ImVec2 rowPos = ImGui::GetCursorScreenPos();
			const float frameHeight = ImGui::GetFrameHeight();
			const float colorHeight = sliderGrabHeight(frameHeight);
			const float inset = (frameHeight - colorHeight) * .5f;
			const float leftInset = std::min(inset, std::max(0.f, controlWidth - 1.f));
			ImGui::SetCursorPosX(ImGui::GetCursorPosX() + leftInset);
			if (drawControlAtHeight(colorHeight, rowPos.y + inset, [&] { return drawColorEditor(color, controlWidth - leftInset); })) {
				p.redVal = (unsigned char)(ofClamp(color[0], 0.f, 1.f) * 255.f + .5f);
				p.greenVal = (unsigned char)(ofClamp(color[1], 0.f, 1.f) * 255.f + .5f);
				p.blueVal = (unsigned char)(ofClamp(color[2], 0.f, 1.f) * 255.f + .5f);
				p.alphaVal = (unsigned char)(ofClamp(color[3], 0.f, 1.f) * 255.f + .5f); edited = true;
			}
			// Lay out the color controls on their own line before reserving the full row.
			ImGui::SetCursorScreenPos(rowPos);
			ImGui::Dummy(ImVec2(controlWidth, frameHeight));
			break;
		}
		default: ImGui::TextDisabled("%s", paramTypeName(p.type)); break;
	}
	if (coloredSlider) ImGui::PopStyleColor(2);
	ImGui::PopStyleColor(3);
	ImGui::SameLine(0.f, controlLabelGap);
	if (p.type == REMOTEUI_PARAM_BOOL) {
		const ImVec2 pos = ImGui::GetCursorScreenPos();
		ImGui::SetCursorScreenPos(ImVec2(std::max(pos.x, controlStartX + controlWidth + controlLabelGap), pos.y));
	}
	ImGui::AlignTextToFramePadding();
	const bool blinkBinding = blinkingBoundParams.count(name) != 0;
	const bool temporaryHighlight = blinkBinding || diffParams.count(name);
	const bool highlightLabel = temporaryHighlight || (highlightChangesSinceLoad && changedSinceLoad.count(name));
	const float r = p.a ? p.r / 255.f : .5f, g = p.a ? p.g / 255.f : .5f, b = p.a ? p.b / 255.f : .5f;
	float highlightPulse = 0.f;
	if (blinkBinding) {
		highlightPulse = .5f + .5f * std::sin((boundBlinkUntil - ofGetElapsedTimef()) * 3.14159265f * 2.f);
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.f, 1.f, 1.f, 1.f));
	} else if (temporaryHighlight) { //shows a param is different than what it was
		// Pulse between white and a light tint of the parameter color.
		highlightPulse = .5f + .5f * std::sin(3.0f * ofGetElapsedTimef() * 3.14159265f);
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.f - .3f * (1.f - r) * highlightPulse, 1.f - .3f * (1.f - g) * highlightPulse, 1.f - .3f * (1.f - b) * highlightPulse, 1.f));
	} else if (highlightLabel) { //shows a param is dirty
		float d = 0.6f + .4f * changedHighlightPulse;
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, d, d, 1.0f));
	}
	const ImVec2 labelPos = ImGui::GetCursorScreenPos();
	const float halfSpacing = style.ItemSpacing.x * .5f;
	const float labelPadding = 3.f * parameterFontScale(rowHeight);
	const float clippedLabelWidth = std::max(1.f, std::min(labelWidth, ImGui::GetContentRegionAvail().x - 6.f - style.ItemSpacing.x));
	const ImVec2 windowPos = ImGui::GetWindowPos(), windowSize = ImGui::GetWindowSize();
	ImGui::PushStyleVar(ImGuiStyleVar_SelectableRounding, 5.f);
	const float glow = temporaryHighlight ? .25f * highlightPulse : 0.f;
	ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(.12f + .4f * r + glow * r, .12f + .4f * g + glow * g, .12f + .4f * b + glow * b, 1.f));
	ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(.16f + .62f * r + glow * r, .16f + .62f * g + glow * g, .16f + .62f * b + glow * b, 1.f));
	ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(.20f + .48f * r + glow * r, .20f + .48f * g + glow * g, .20f + .48f * b + glow * b, 1.f));
	ImGui::PushClipRect(ImVec2(labelPos.x - halfSpacing, windowPos.y), ImVec2(labelPos.x + clippedLabelWidth + halfSpacing, windowPos.y + windowSize.y), true);
	const bool labelClicked = ImGui::Selectable("##parameter-label", controllers.learning == name || temporaryHighlight, 0, ImVec2(clippedLabelWidth, ImGui::GetTextLineHeight()));
	const bool labelHovered = ImGui::IsItemHovered();
	const ImVec2 itemMin = ImGui::GetItemRectMin();
	const float textY = itemMin.y + std::floor(style.ItemSpacing.y * .5f);
	const bool watched = client.isParamWatched(name);
	const float textLeft = labelPos.x + labelPadding;
	const float textRight = std::max(textLeft, labelPos.x + clippedLabelWidth - labelPadding);
	const bool missingFromPreset = missingPresetParams.count(name) != 0;
	const float warningSize = ImGui::GetFontSize();
	const float labelTextRight = missingFromPreset ? std::max(textLeft, textRight - warningSize - labelPadding) : textRight;
	ImGui::PushClipRect(ImVec2(textLeft, windowPos.y), ImVec2(labelTextRight, windowPos.y + windowSize.y), true);
	ImDrawList *drawList = ImGui::GetWindowDrawList();
	const ImU32 textColor = ImGui::GetColorU32(ImGuiCol_Text);
	drawList->AddText(ImVec2(textLeft, textY), textColor, name.c_str());
	ImGui::PopClipRect();
	if (missingFromPreset && std::sin(ofGetElapsedTimef() * 3.14159265f * 4.f) >= 0.f) {
		const float x = textRight - warningSize, y = textY;
		const ImU32 yellow = IM_COL32(255, 195, 32, 255), dark = IM_COL32(35, 30, 15, 255);
		drawList->AddTriangleFilled(ImVec2(x + warningSize * .5f, y), ImVec2(x + warningSize, y + warningSize), ImVec2(x, y + warningSize), yellow);
		drawList->AddLine(ImVec2(x + warningSize * .5f, y + warningSize * .32f), ImVec2(x + warningSize * .5f, y + warningSize * .65f), dark, std::max(1.f, warningSize * .1f));
		drawList->AddCircleFilled(ImVec2(x + warningSize * .5f, y + warningSize * .79f), std::max(1.f, warningSize * .06f), dark);
	}
	ImGui::PopClipRect();
	if (watched) { //draw blinking dot
		const float radius = ImGui::GetFontSize() * .15f;
		const ImVec2 center(labelPos.x - radius, textY + ImGui::GetFontSize() * .5f);
		const float pulse = .5f + .5f * std::sin(ofGetElapsedTimef() * 10.f);
		const ImU32 starColor = ImGui::GetColorU32(ImVec4(.35f + .55f * r, .35f + .55f * g, .35f + .55f * b, pulse));
		drawList->AddCircleFilled(center, radius, starColor, 10);
	}
	ImGui::PopStyleColor(3);
	ImGui::PopStyleVar();
	const bool canBind = p.type == REMOTEUI_PARAM_BOOL || p.type == REMOTEUI_PARAM_FLOAT || p.type == REMOTEUI_PARAM_INT || p.type == REMOTEUI_PARAM_ENUM || p.type == REMOTEUI_PARAM_COLOR;
	if (labelClicked && canBind) {
		controllers.beginLearn(name);
		bindingPopupRequested = true;
	}
	if (highlightLabel) ImGui::PopStyleColor();
	ImGui::PopFont();
	ImGui::PushStyleVar(ImGuiStyleVar_PopupRounding, 8.f);
	ImGui::PushStyleVar(ImGuiStyleVar_PopupBorderSize, 1.f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10.f, 8.f));
	ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.f, 0.f, 0.f, 1.f));
	if (ImGui::BeginPopupContextItem("parameter-context")) {

		const auto baseline = changeBaseline.find(name);
		const bool changed = baseline != changeBaseline.end() && p.type == baseline->second.type && !sameParameterValue(p, baseline->second);

		if (ImGui::MenuItem("Restore parameter value", nullptr, false, client.isReadyToSend() && changed)) {
			RemoteUIParam restored = p;
			if (copyParameterValue(restored, baseline->second)) {
				sendEditedParam(name, restored);
				appendLog("Restored connection value for \"" + name + "\"");
			}
		}

		if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
			if (!changed) ImGui::SetTooltip("Restores the value to what this client received from the server on its most recent connection.\n\nThis parameter has not changed since then.");
			else ImGui::SetTooltip("Restores the value to what this client received from the server on its most recent connection.");
		}

		if (ImGui::MenuItem("Watch parameter on Server", nullptr, watched, client.isReadyToSend()))
			client.setParamWatched(name, !watched);

		if (p.type == REMOTEUI_PARAM_COLOR && ImGui::MenuItem("Copy as RGBA uint8")) {
			const std::string rgba = std::to_string(p.redVal) + ", " + std::to_string(p.greenVal) + "," +
				std::to_string(p.blueVal) + "" + std::to_string(p.alphaVal);
			ImGui::SetClipboardText(rgba.c_str());
		}

		if (p.type == REMOTEUI_PARAM_COLOR && ImGui::MenuItem("Copy as RGBA hex")) {
			char hex[10];
			std::snprintf(hex, sizeof(hex), "#%02X%02X%02X%02X", p.redVal, p.greenVal, p.blueVal, p.alphaVal);
			ImGui::SetClipboardText(hex);
		}

		if (p.type == REMOTEUI_PARAM_COLOR && ImGui::MenuItem("Copy as RGBA float")) {
			const std::string rgba = std::to_string(p.redVal/255.0f) + "," + std::to_string(p.greenVal/255.0f) + "," +
				std::to_string(p.blueVal/255.0f) + ", " + std::to_string(p.alphaVal/255.0f);
			ImGui::SetClipboardText(rgba.c_str());
		}

		ImGui::EndPopup();
	}
	ImGui::PopStyleColor();
	ImGui::PopStyleVar(3);
	if (labelHovered && !ImGui::IsPopupOpen("parameter-context")) {
		ImGui::SetNextWindowViewport(ImGui::GetMainViewport()->ID);

		if (ImGui::BeginTooltip()) {

			ImGui::Text("Param: \"%s\"",name.c_str());
			ImGui::Text("Type: %s", paramTypeName(p.type));
			if (missingFromPreset) ImGui::TextUnformatted("This parameter was not defined in the loaded preset.");

			if (p.type == REMOTEUI_PARAM_FLOAT && p.minFloat < p.maxFloat)
				ImGui::Text("Range: [%.6g .. %.6g]", p.minFloat, p.maxFloat);
			else if ((p.type == REMOTEUI_PARAM_INT || p.type == REMOTEUI_PARAM_ENUM) && p.minInt < p.maxInt )
				ImGui::Text("Range: [%d .. %d]", p.minInt, p.maxInt);
			else if (p.type == REMOTEUI_PARAM_COLOR){
				static char hexBuffer[10];
				std::snprintf(hexBuffer, sizeof(hexBuffer), "#%02X%02X%02X%02X", p.redVal, p.greenVal, p.blueVal, p.alphaVal);
				ImGui::Text("RGBA hex: %s", hexBuffer);
			}

			if (changedSinceLoad.count(name) && p.lastModifiedUnix > 0) {
				const std::time_t modified = static_cast<std::time_t>(p.lastModifiedUnix);
				if (const std::tm *localTime = std::localtime(&modified)) {
					char formattedTime[32] = {};
					if (std::strftime(formattedTime, sizeof(formattedTime), "%H:%M %d %b", localTime))
						ImGui::Text("Last Modified: %s", formattedTime);
				}
			}
			const auto baseline = changeBaseline.find(name);
			const bool changed = baseline != changeBaseline.end() && p.type == baseline->second.type && !sameParameterValue(p, baseline->second);
			if(changed){
				string tt = "Original Value: " + baseline->second.getValueAsString();
				ImGui::TextUnformatted(tt.c_str());
			}

			if (!p.description.empty()) {
				ImGui::Text("Description: %s", p.description.c_str());
			}

			if (canBind) {
				ImGui::Dummy(ImVec2(0.f, ImGui::GetTextLineHeight() * .3f));
				ImGui::Separator();
				ImGui::Dummy(ImVec2(0.f, ImGui::GetTextLineHeight() * .2f));
				ImGui::TextUnformatted("[ Click to bind a MIDI or joystick control ]");
			}

			ImGui::EndTooltip();
		}
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
