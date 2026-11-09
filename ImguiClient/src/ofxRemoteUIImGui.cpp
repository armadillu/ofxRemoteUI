		#include "ofxRemoteUIImGui.h"

#include "ofMain.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "ClientPaths.h"
#include <cstring>

#ifndef GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_NONE
#endif
#include <GLFW/glfw3.h>

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
	// Use the same Rubik font and crisp rasterizer settings as ParticlesLife.
	// The renderer bakes the font at each viewport's framebuffer density.
	const ofBuffer fontData = ofBufferFromFile("fonts/Rubik-Regular.ttf", true);
	if (fontData.size() > 100) {
		ImFontConfig fontConfig;
		fontConfig.OversampleH = fontConfig.OversampleV = 1;
		fontConfig.PixelSnapH = true;
		void *fontBytes = IM_ALLOC(fontData.size());
		std::memcpy(fontBytes, fontData.getData(), fontData.size());
		io.Fonts->AddFontFromMemoryTTF(fontBytes, static_cast<int>(fontData.size()), 17.f, &fontConfig);
	} else {
		ofLogWarning("ofxRemoteUIImGui") << "Rubik font could not be read; using the embedded vector font";
		io.Fonts->AddFontDefaultVector();
	}
	const ofBuffer boldFontData = ofBufferFromFile("fonts/Rubik-Bold.ttf", true);
	if (boldFontData.size() > 100) {
		ImFontConfig boldConfig;
		boldConfig.OversampleH = boldConfig.OversampleV = 1;
		boldConfig.PixelSnapH = true;
		void *fontBytes = IM_ALLOC(boldFontData.size());
		std::memcpy(fontBytes, boldFontData.getData(), boldFontData.size());
		groupHeaderFont = io.Fonts->AddFontFromMemoryTTF(fontBytes, static_cast<int>(boldFontData.size()), 17.f, &boldConfig);
	}
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
	style.GrabMinSize = 16.f;
	style.WindowBorderSize = 0.f;
	style.ChildBorderSize = 0.f;
	style.PopupBorderSize = 1.f;
	style.FrameBorderSize = 0.f;
	style.WindowRounding = multiViewport ? 0.0f : 4.0f;
	style.Colors[ImGuiCol_WindowBg].w = 1.0f;
	style.FrameRounding = 3.0f;
	style.GrabRounding = 3.0f;

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
