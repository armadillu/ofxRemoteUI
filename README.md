# ofxRemoteUI


OF addon serves any variables you want (bool, float, int, string) on the network, so that you can mod from away. Includes native OSX Client. OSC based.

Uses Macros + the singleton pattern to make it very easy to share any variable you want to edit remotely during app runtime. 

[http://www.youtube.com/watch?v=kp1rajoN0o4](http://www.youtube.com/watch?v=kp1rajoN0o4)
  


### How to use

Easy! Declare variables in your project, as you normally would:

	class testApp : public ofBaseApp{
		public:
			float x;
			float y;
	}

Then tell the server to share them:

	void setup(){
	
		OFX_REMOTEUI_SERVER_SETUP(); //start server

		//expose vars to the server
		OFX_REMOTEUI_SERVER_SHARE_PARAM(x, 0, ofGetWidth());
		OFX_REMOTEUI_SERVER_SHARE_PARAM(y, 0, ofGetHeight());
	}
	
	void update(){
		OFX_REMOTEUI_SERVER_UPDATE(0.016666f);
	}
	

And use the supplied OSX Client to view and edit them

PD: to use ofxOsc in your project, which ofxRemoteUI requires, you wil need to add this to you project's header search paths:

    ../../../addons/ofxOsc/libs ../../../addons/ofxOsc/libs/oscpack ../../../addons/ofxOsc/libs/oscpack/src ../../../addons/ofxOsc/libs/oscpack/src/ip ../../../addons/ofxOsc/libs/oscpack/src/ip/posix ../../../addons/ofxOsc/libs/oscpack/src/ip/win32 ../../../addons/ofxOsc/libs/oscpack/src/osc ../../../addons/ofxOsc/src

### TODO

- make a multiplatform client, maybe based on ofxUI?