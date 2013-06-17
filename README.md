# ofxRemoteUI


OF addon allows you to serve any variables you want (bool, float, int, enum, string) on the network, so that you can modify them remotely. Uses server client architecture, where your app is the server. It communicates both ways; you can modify your project's variables from the client, but you can also pull your app's variable values from the client; this way you can track values that evolve programatically.

I know there's tons of very good UI's already, but one thing that bothers me about most of them is how by using them, your are affecting your framerates quite a lot. That's the main reason that made me build a ofxRemoteUI. Being quite comfy in OSX development, I chose to make a full-featured native OSX client; although clients for other platforms could be developed as well, using the underlying classes.

It's OSC based, and it includes a native OSX Client. The Native OSX Client allows param colorization for better clarity, and live param filtering. It also supports grouping the params into categories, to filter them by category in the OSX Client.

It can also be set to store the current values when quitting the app (or whenever its convenient), so that you can carry on where you left off last time you used it. It uses ofxXmlSettings to store the settings. 

You can also create and delete presets. Presets are stored with your OF app, inside a "ofxRemoteUIPresets" folder, inside your data folder. Whenever you like the current config, you can make a preset to keep it around. You can also delete presets.

It uses Macros + the singleton pattern to make it very easy to share any variable you want to edit remotely, in any class of your project. 

Also, the OSX client allows to copy all params as plain text. You can also paste them back after editing them! Thx to @kritzikratzi for this!

--Update!-- @kritzikratzi made a Processing version of this! https://github.com/kritzikratzi/remoteUI-java

[demo video](http://youtu.be/QVyAiopGK4U)

![OSX Client](http://farm4.staticflickr.com/3721/9062611508_e86273db52_o.png "OSX Client")

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
		
		OFX_REMOTEUI_SERVER_SHARE_PARAM(x, 0, ofGetWidth()); //Expose vars to the server, and set a valid range
		OFX_REMOTEUI_SERVER_SHARE_PARAM(y, 0, ofGetHeight());

		OFX_REMOTEUI_SERVER_LOAD_FROM_XML(); //load values from XML, as they were last saved
	}
	
	void update(){
		OFX_REMOTEUI_SERVER_UPDATE(0.016666f); //keep it updated
	}
	
	void exit(){
		OFX_REMOTEUI_SERVER_CLOSE();		//stop the server
		OFX_REMOTEUI_SERVER_SAVE_TO_XML();	//save values to XML
	}

And use the supplied OSX Client to view and edit them.

Note: right now, enums need to be consecutive, so that each enum item is ++1 the previous one.

PD: to use ofxOsc in your project, which ofxRemoteUI requires, you wil need to add this to you project's header search paths:

    ../../../addons/ofxOsc/libs ../../../addons/ofxOsc/libs/oscpack ../../../addons/ofxOsc/libs/oscpack/src ../../../addons/ofxOsc/libs/oscpack/src/ip ../../../addons/ofxOsc/libs/oscpack/src/ip/posix ../../../addons/ofxOsc/libs/oscpack/src/ip/win32 ../../../addons/ofxOsc/libs/oscpack/src/osc ../../../addons/ofxOsc/src

### TODO

- make a multiplatform client, maybe based on ofxUI?
- add loadable/saveable presets?
