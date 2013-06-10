# ofxRemoteUI


OF addon allows you to serve any variables you want (bool, float, int, string) on the network, so that you can modify them from away. Uses server client architecture, where your app is the server. It communicates both ways; you can modify your project's variables from a client, but you can also pull your app's variable states from the client, so that you can track values that evolve programatically.

It's OSC based, and it includes a native OSX Client. The Native OSX Client allows param colorization for better clarity, and live param search.

You can also set it to store the current values on quit, so you can keep the values as you had it on your last session. It uses ofxXmlSettings to do so.

It uses Macros + the singleton pattern to make it very easy to share any variable you want to edit remotely, in any class of your project. 

[http://youtu.be/e6mzo8YavoM](http://youtu.be/e6mzo8YavoM)

![OSX Client](http://farm4.staticflickr.com/3830/8752916271_f7acc01712_o.png "OSX Client")

### How to use

Easy! Declare variables in your project, as you normally would:

	class testApp : public ofBaseApp{
		public:
			float x;
			int y;
	}

Then tell the server to share them:

	void setup(){	
	
		OFX_REMOTEUI_SERVER_SETUP(); //start server
		
		OFX_REMOTEUI_SERVER_SHARE_PARAM(x, 0, ofGetWidth()); //Expose vars to the server
		OFX_REMOTEUI_SERVER_SHARE_PARAM(y, 0, ofGetHeight());

		//load values from XML, as they were last saved
		OFX_REMOTEUI_SERVER_LOAD_FROM_XML();
	}
	
	void update(){
		OFX_REMOTEUI_SERVER_UPDATE(0.016666f); //keep it updated
	}
	
	void exit(){
		OFX_REMOTEUI_SERVER_CLOSE();		//stop the server
		OFX_REMOTEUI_SERVER_SAVE_TO_XML();	//save values to XML
	}

And use the supplied OSX Client to view and edit them

PD: to use ofxOsc in your project, which ofxRemoteUI requires, you wil need to add this to you project's header search paths:

    ../../../addons/ofxOsc/libs ../../../addons/ofxOsc/libs/oscpack ../../../addons/ofxOsc/libs/oscpack/src ../../../addons/ofxOsc/libs/oscpack/src/ip ../../../addons/ofxOsc/libs/oscpack/src/ip/posix ../../../addons/ofxOsc/libs/oscpack/src/ip/win32 ../../../addons/ofxOsc/libs/oscpack/src/osc ../../../addons/ofxOsc/src

### TODO

- make a multiplatform client, maybe based on ofxUI?