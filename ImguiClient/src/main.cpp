#include "ofMain.h"
#include "ofApp.h"
#include "ofAppGLFWWindow.h"
#include "NativeMacFileOpen.h"

int main() {
	#ifdef TARGET_OSX
	const auto bundledData = ofFilePath::getCurrentExeDirFS() / "../Resources/data";
	if (ofDirectory::doesDirectoryExist(bundledData.string(), false)) ofSetDataPathRoot(bundledData);
	NativeMacFileOpen::setup();
	#endif
	ofGLWindowSettings settings;
	settings.setSize(1280, 800);
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
		glfwWindow->setWindowShape(1280 * scale, 800 * scale);
	}
	#endif
	ofRunApp(std::make_shared<ofApp>());
}
