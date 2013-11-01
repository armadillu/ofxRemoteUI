
#include "testApp.h"
#include "ofAppGlutWindow.h"


//--------------------------------------------------------------
int main(){

	ofAppGlutWindow window; // create a window
	#ifdef TARGET_OSX
	window.setGlutDisplayString("rgba double samples>=8 depth");
	#endif
	// set width, height, mode (OF_WINDOW or OF_FULLSCREEN)
	ofSetupOpenGL(&window, 640, 480, OF_WINDOW);
	ofRunApp(new testApp()); // start the app
}
