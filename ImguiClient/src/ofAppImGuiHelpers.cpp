#include "ofAppImGuiHelpers.h"
#include "imgui.h"
#include <algorithm>
#include <cctype>
#include <cmath>

namespace ofAppImGuiHelpers {

void pushPopupWindowPadding() {
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(12.f, 12.f));
}

float presetNameInputWidth() {
	return 200.f;
}

float boundBlinkDurationSeconds() {
	return 4.f;
}

float presetDiffHighlightDurationSeconds() {
	return 5.f;
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
	//ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.f);
	const bool changed = ImGui::Checkbox(label, value);
	//ImGui::PopStyleVar();
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
	style.Colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.f, 0.f, 0.f, .9f);
	style.Colors[ImGuiCol_Text] = lightWindow ? ImVec4(.06f, .06f, .06f, 1.f) : ImVec4(.94f, .94f, .94f, 1.f);
	style.Colors[ImGuiCol_TextDisabled] = lightWindow ? ImVec4(.30f, .30f, .30f, 1.f) : ImVec4(.68f, .68f, .68f, 1.f);
	style.Colors[ImGuiCol_PopupBg] = tone(.1f, 1.f);
	style.Colors[ImGuiCol_FrameBg] = tone(.23f);
	style.Colors[ImGuiCol_FrameBgHovered] = tone(.34f);
	style.Colors[ImGuiCol_FrameBgActive] = tone(.40f);
	style.Colors[ImGuiCol_TitleBg] = tone(.20f);
	style.Colors[ImGuiCol_TitleBgActive] = tone(.10f);
	style.Colors[ImGuiCol_TitleBgCollapsed] = tone(.15f);
	style.Colors[ImGuiCol_MenuBarBg] = tone(.20f);
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
