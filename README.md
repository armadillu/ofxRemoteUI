# ofxRemoteUI


OF addon allows you to serve any variables you want (bool, float, int, enum, string, ofColor) on the network, so that you can modify them remotely. Uses server client architecture, where your app is the server. It communicates both ways; you can modify your project's variables from the client, but you can also pull your app's variable values from the client; this way you can track values that evolve programatically. 

Watch the [demo video](http://www.youtube.com/watch?v=EHS3bd0beKQ).

![OSX Client](http://farm8.staticflickr.com/7418/10126260445_1eddc4c357_o.png "OSX Client")

---

## Features

* Edit & Track variables remotely thorugh UDP/OSC (bool, int, float, string, Enum, ofColor)
* Native OSX interface
* Allows to save/load your variable states across app launches
* Allows creation of Presets, variable states that you can switch from and to quickly.
* Easily create parameter Groups, and quickly access through keyboard shortcuts in the supplied OSX Client.
* Colorize your variables for easy reading in the OSX client.
* Restore paramters to previous launch ones or to Default values
* ofxRemoteUI Can be used outside OF in any C++ project, and in Processing thx to @kritzikratzi
* Easy to use macros hide complexity away.



## Compatibility
Works in OpenFrameworks, but also in plain C++ projects.

There's also a feature limited version of the Server for Processing, made by @kritzikratzi! See <a href="http://superduper.org/processing/remoteUI">here</a>.

## Why?

I know there's tons of very good UI's already, but one thing that bothers me about most of them is how by using them, your are affecting your framerates quite a lot. That's the main reason that made me build ofxRemoteUI. Being quite comfy in OSX development, I chose to make a full-featured native OSX client; although clients for other platforms could be developed as well, using the underlying ofxRemoteUIClient class.

## Details

It's OSC based, and it includes a native OSX Client. The Native OSX Client allows param colorization for better clarity, and live param filtering. It also supports the grouping of params into categories, for easy access. There's automatic keyboard shortcuts to do so.

It can also be set to store the current values when quitting the app (or whenever its convenient), so that you can carry on where you left off last time you used it. It does so by saving a file called "ofxRemoteUISettings.xml" in your data folder. It uses ofxXmlSettings to store everything. 

You can also create and delete presets, which are parameter states for your app. Presets are stored with your OF app, inside an "ofxRemoteUIPresets" folder, in your data folder. This makes it easy to check in your presets with your soruce code. Whenever you like the current config, you can make a preset to keep it around. You can also delete presets.

ofxRemoteUI uses Macros + the singleton pattern to make it very easy to share any variable you decide to edit remotely, from any class of your project.

The OSX client also allows to copy all the current params as plain text. You can also paste them back after editing them! Thx to @kritzikratzi for this idea!

**"Restore to initial XML Values"** sets alls params to whatever values they had at server app launch.  
**"Restore to Default Values"** sets alls params to whatever values the shared variable had before sharing it with OFX_REMOTEUI_SERVER_SHARE_PARAM().

To use it outisde of OpenFrameworks, you can see how the example-noOF is setup.   

-----


## How To Use

	float x;
	int y;
	ofColor color;

	void setup(){	
	
		OFX_REMOTEUI_SERVER_SETUP(); //start server
		
		//Expose x and y vars to the server, providing a valid slider range
		OFX_REMOTEUI_SERVER_SHARE_PARAM(x, 0, ofGetWidth()); 
		OFX_REMOTEUI_SERVER_SHARE_PARAM(y, 0, ofGetHeight());
		
		//share the color var
		OFX_REMOTEUI_SERVER_SHARE_COLOR_PARAM(color);
		
		//load values from XML, as they were last saved (if they were)
		OFX_REMOTEUI_SERVER_LOAD_FROM_XML(); 
	}
	
	void update(){
		OFX_REMOTEUI_SERVER_UPDATE(0.016666f); //keep the server updated
	}
	
Use the supplied OSX Client to view and edit your shared parameters.


## Notes

Enums must be consecutive so that each enum item is +1 the previous one for them to work.

When loading a preset, it might be that the preset doesnt specify values for all your current params. If so, the params whose values havent been modified by the preset will show a small warning sign for a few seconds.


## To Do

- make a basic multiplatform client, maybe based on ofxUI?