#pragma once

struct GLFWwindow;
struct ImFont;

// Minimal openFrameworks host for Dear ImGui's upstream GLFW + OpenGL3 backends.
// The client owns its ImGui widgets; this class only manages context/frame/render glue.
class ofxRemoteUIImGui {
public:
	bool setup(GLFWwindow *window = nullptr, bool docking = true, bool multiViewport = true);
	void begin();
	void end();
	void shutdown();
	void setFloating(bool enabled);
	bool isSetup() const { return contextCreated; }
	ImFont *getGroupHeaderFont() const { return groupHeaderFont; }

private:
	GLFWwindow *mainWindow = nullptr;
	ImFont *groupHeaderFont = nullptr;
	bool contextCreated = false;
	bool platformBackendReady = false;
	bool rendererBackendReady = false;
	bool multiViewportEnabled = false;
};
