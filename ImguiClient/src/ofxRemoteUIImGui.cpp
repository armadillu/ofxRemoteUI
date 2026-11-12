		#include "ofxRemoteUIImGui.h"

#include "ofMain.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "ClientPaths.h"
#include <cstring>
#include <cmath>

#ifndef GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_NONE
#endif
#include <GLFW/glfw3.h>

namespace {
// GLFW can report inconsistent display geometry while monitors are unplugged.
// Sanitize it after the backend refresh and before ImGui checks the data.
void sanitizeMonitorBounds(GLFWwindow *window) {
	auto &monitors = ImGui::GetPlatformIO().Monitors;
	if (monitors.empty()) {
		int x = 0, y = 0, width = 0, height = 0;
		glfwGetWindowPos(window, &x, &y);
		glfwGetWindowSize(window, &width, &height);
		ImGuiPlatformMonitor fallback;
		fallback.MainPos = fallback.WorkPos = ImVec2(static_cast<float>(x), static_cast<float>(y));
		fallback.MainSize = fallback.WorkSize = ImVec2(static_cast<float>(width > 0 ? width : 1), static_cast<float>(height > 0 ? height : 1));
		fallback.DpiScale = 1.f;
		monitors.push_back(fallback);
	}
	for (ImGuiPlatformMonitor &monitor : monitors) {
		const ImVec2 &pos = monitor.MainPos, &size = monitor.MainSize;
		const ImVec2 &work = monitor.WorkPos, &workSize = monitor.WorkSize;
		if (!(std::isfinite(work.x) && std::isfinite(work.y) &&
			workSize.x > 0.f && workSize.y > 0.f &&
			work.x >= pos.x && work.y >= pos.y &&
			work.x + workSize.x <= pos.x + size.x && work.y + workSize.y <= pos.y + size.y)) {
			monitor.WorkPos = monitor.MainPos;
			monitor.WorkSize = monitor.MainSize;
		}
		if (!(monitor.DpiScale > 0.f && monitor.DpiScale < 99.f)) monitor.DpiScale = 1.f;
	}
}
}

bool ofxRemoteUIImGui::setup(GLFWwindow *window, bool docking, bool multiViewport) {
	if (contextCreated) return true;
	mainWindow = window ? window : static_cast<GLFWwindow *>(ofGetWindowPtr()->getWindowContext());
	if (!mainWindow) return false;

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	contextCreated = true;

	ImGuiIO &io = ImGui::GetIO();
	#ifdef TARGET_OSX
	static const std::string imguiSettingsPath = ClientPaths::settingsFile("imgui.ini");
	io.IniFilename = imguiSettingsPath.c_str();
	#endif
	// ImGui bakes each weight at the viewport's framebuffer density.
	auto loadFont = [&](const std::string &path) -> ImFont * {
		const ofBuffer data = ofBufferFromFile(path, true);
		if (data.size() <= 100) return nullptr;
		ImFontConfig config;
		config.OversampleH = config.OversampleV = 1;
		config.PixelSnapH = true;
		void *bytes = IM_ALLOC(data.size());
		std::memcpy(bytes, data.getData(), data.size());
		return io.Fonts->AddFontFromMemoryTTF(bytes, static_cast<int>(data.size()), 17.f, &config);
	};
	ImFont *regularFont = loadFont("fonts/Rubik-Regular.ttf");
	if (!regularFont) {
		ofLogWarning("ofxRemoteUIImGui") << "Rubik font could not be read; using the embedded vector font";
		regularFont = io.Fonts->AddFontDefaultVector();
	}
	io.FontDefault = regularFont;
	groupHeaderFont = loadFont("fonts/Rubik-Bold.ttf");
	if (!groupHeaderFont) groupHeaderFont = regularFont;
	if (docking) io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
	if (multiViewport) io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
	multiViewportEnabled = multiViewport;

	ImGui::StyleColorsDark();
	ImGuiStyle &style = ImGui::GetStyle();
	style.WindowPadding = ImVec2(6.f, 5.f);
	style.FontScaleMain = .8f;
	style.FramePadding = ImVec2(12.f, 3.f);
	style.ItemSpacing = ImVec2(2.f, 2.f);
	style.ItemInnerSpacing = ImVec2(6.f, 4.f);
	style.TouchExtraPadding = ImVec2(0.f, 0.f);
	style.IndentSpacing = 12.f;
	style.GrabMinSize = 10.f;
	style.ScrollbarSize = 10.f;
	style.ColorMarkerSize = 1.f;
	style.WindowBorderSize = 0.f;
	style.ChildBorderSize = 0.f;
	style.PopupBorderSize = 0.f;
	style.PopupRounding = 6.f;
	style.FrameBorderSize = 0.f;
	style.Colors[ImGuiCol_Border] = ImVec4(0.f, 0.f, 0.f, .433f);
	style.WindowRounding = multiViewport ? 0.0f : 4.0f;
	style.Colors[ImGuiCol_WindowBg].w = 1.0f;
	style.FrameRounding = 3.0f;
	style.GrabRounding = 3.0f;
	style.AntiAliasedLines = false;
	style.AntiAliasedLineEnds = false;
	style.AntiAliasedFill = false;
	#ifndef IMGUI_DISABLE_OBSOLETE_FUNCTIONS
	style.AntiAliasedLinesUseTex = false;
	#endif

	platformBackendReady = ImGui_ImplGlfw_InitForOpenGL(mainWindow, true);
	if (!platformBackendReady) {
		shutdown();
		return false;
	}

	// nullptr lets the upstream backend choose the GLSL version for the active GL context.
	rendererBackendReady = ImGui_ImplOpenGL3_Init(nullptr);
	if (!rendererBackendReady) {
		shutdown();
		return false;
	}
	return true;
}

void ofxRemoteUIImGui::begin() {
	if (!contextCreated || !platformBackendReady || !rendererBackendReady) return;
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	sanitizeMonitorBounds(mainWindow);
	ImGui::NewFrame();
}

void ofxRemoteUIImGui::end() {
	if (!contextCreated || !platformBackendReady || !rendererBackendReady) return;
	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

	if (multiViewportEnabled && (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable)) {
		GLFWwindow *currentContext = glfwGetCurrentContext();
		ImGui::UpdatePlatformWindows();
		ImGui::RenderPlatformWindowsDefault();
		glfwMakeContextCurrent(currentContext ? currentContext : mainWindow);
	}
}

void ofxRemoteUIImGui::shutdown() {
	if (rendererBackendReady) {
		ImGui_ImplOpenGL3_Shutdown();
		rendererBackendReady = false;
	}
	if (platformBackendReady) {
		ImGui_ImplGlfw_Shutdown();
		platformBackendReady = false;
	}
	if (contextCreated) {
		ImGui::DestroyContext();
		contextCreated = false;
	}
	groupHeaderFont = nullptr;
	mainWindow = nullptr;
}

void ofxRemoteUIImGui::setFloating(bool enabled) {
	if (mainWindow) glfwSetWindowAttrib(mainWindow, GLFW_FLOATING, enabled ? GLFW_TRUE : GLFW_FALSE);
}
