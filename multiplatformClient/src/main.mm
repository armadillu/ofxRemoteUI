#include "ofMain.h"
#include "testApp.h"

//========================================================================
int main( ){

#ifdef TARGET_OF_IOS

	ofAppiOSWindow * iOSWindow = new ofAppiOSWindow();
	if ( [[UIScreen mainScreen] scale] > 1 ){
		//if ( ofxiPhoneGetDeviceType() != OFXIPHONE_DEVICE_IPAD )
		iOSWindow->enableRetina();
	}

	ofSetupOpenGL(iOSWindow, 450, 320, OF_FULLSCREEN);
	ofRunApp(new testApp);

#else

	ofSetupOpenGL(450,600,OF_WINDOW);			// <-------- setup the GL context

	// this kicks off the running of my app
	// can be OF_WINDOW or OF_FULLSCREEN
	// pass in width and height too:
	ofRunApp(new testApp());

#endif

}
