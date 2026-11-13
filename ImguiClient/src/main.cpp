#include "ofMain.h"
#include "ofApp.h"
#include "ofAppGLFWWindow.h"
#include "NativeMacFileOpen.h"
#include "ClientPaths.h"
#include <GLFW/glfw3.h>

#include <fstream>
#include <string>

namespace {
struct SavedWindowGeometry {
	int width = 1280;
	int height = 800;
	int x = 0;
	int y = 0;
	bool hasPosition = false;
};

SavedWindowGeometry loadWindowGeometry() {
	SavedWindowGeometry geometry;
	std::ifstream in(ClientPaths::existingSettingsFile("remoteui-imgui-client.ini"));
	std::string line;
	while (std::getline(in, line)) {
		const auto separator = line.find('=');
		if (separator == std::string::npos) continue;
		const std::string key = line.substr(0, separator);
		try {
			const int value = std::stoi(line.substr(separator + 1));
			if (key == "windowWidth") geometry.width = ofClamp(value, 320, 16384);
			else if (key == "windowHeight") geometry.height = ofClamp(value, 240, 16384);
			else if (key == "windowX") { geometry.x = value; geometry.hasPosition = true; }
			else if (key == "windowY") { geometry.y = value; geometry.hasPosition = true; }
		} catch (...) {
			// Ignore invalid geometry entries and use the default for that field.
		}
	}
	return geometry;
}
}

int main() {
	#ifdef TARGET_OSX
	const auto bundledData = ofFilePath::getCurrentExeDirFS() / "../Resources/data";
	if (ofDirectory::doesDirectoryExist(bundledData.string(), false)) ofSetDataPathRoot(bundledData);
	NativeMacFileOpen::setup();
	#endif
	const SavedWindowGeometry geometry = loadWindowGeometry();
	ofGLWindowSettings settings;
	settings.setSize(geometry.width, geometry.height);
	settings.glVersionMajor = 4;
	settings.glVersionMinor = 1;
	settings.windowMode = OF_WINDOW;
	settings.title = "ofxRemoteUI ImGui Client";
	auto window = ofCreateWindow(settings);
	#ifdef TARGET_OSX
	NativeMacFileOpen::setup();
	// OF's window shape is specified in framebuffer pixels, so compensate for
	// Retina to keep the requested dimensions in logical screen coordinates.
	if (auto glfwWindow = std::dynamic_pointer_cast<ofAppGLFWWindow>(window)) {
		const int scale = glfwWindow->getPixelScreenCoordScale();
		glfwWindow->setWindowShape(geometry.width * scale, geometry.height * scale);
	}
	#endif
	if (geometry.hasPosition) {
		if (auto glfwWindow = std::dynamic_pointer_cast<ofAppGLFWWindow>(window)) {
			glfwSetWindowPos(static_cast<GLFWwindow *>(glfwWindow->getWindowContext()), geometry.x, geometry.y);
		}
	}
	ofRunApp(std::make_shared<ofApp>());
}
