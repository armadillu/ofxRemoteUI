# ofxRemoteUI


OF addon allows you to serve any variables you want (bool, float, int, enum, string, ofColor) on the network, so that you can modify them remotely. Uses server client architecture, where your app is the server. It communicates both ways; you can modify your project's variables from the client, but you can also pull your app's variable values from the client; this way you can track values that evolve programatically. 

Watch the [demo video](http://www.youtube.com/watch?v=EHS3bd0beKQ).

![OSX Client](http://farm4.staticflickr.com/3760/9167922622_dc7f64f4e1_o.png "OSX Client")

---

## Features

* Edit & Track variables remotely thorugh UDP/OSC (bool, int, float, string, Enum, ofColor)
* Native OSX interface
* Allows to save/load variables
* Allows creation of Presets
* Can be used outisde OF too
* Easily create parameter Groups, and quick access through keyboard shortcuts
* Colorize your variables for easy reading
* Restore paramters to previous launch ones or to Default values
* Can be used outside OF, and in Processing thx to @kritzikratzi
* Easy to use macros



## Compatibility
Works in OpenFrameworks, but also in plain C++ projects.

There's also a feature limited version of the Server for Processing, made by @kritzikratzi! See <a href="http://superduper.org/processing/remoteUI">here</a>.

## Why?

I know there's tons of very good UI's already, but one thing that bothers me about most of them is how by using them, your are affecting your framerates quite a lot. That's the main reason that made me build ofxRemoteUI. Being quite comfy in OSX development, I chose to make a full-featured native OSX client; although clients for other platforms could be developed as well, using the underlying ofxRemoteUIClient class.

## Details

It's OSC based, and it includes a native OSX Client. The Native OSX Client allows param colorization for better clarity, and live param filtering. It also supports grouping the params into categories, to filter them by category in the OSX Client.

It can also be set to store the current values when quitting the app (or whenever its convenient), so that you can carry on where you left off last time you used it. It uses ofxXmlSettings to store the settings. 

You can also create and delete presets. Presets are stored with your OF app, inside a "ofxRemoteUIPresets" folder, inside your data folder. Whenever you like the current config, you can make a preset to keep it around. You can also delete presets.

It uses Macros + the singleton pattern to make it very easy to share any variable you want to edit remotely, in any class of your project. 

Also, the OSX client allows to copy all params as plain text. You can also paste them back after editing them! Thx to @kritzikratzi for this!

**"Restore to initial XML Values"** sets alls params to whatever values they had at server app launch.  
**"Restore to Default Values"** sets alls params to whatever values the shared variable had before sharing it with OFX_REMOTEUI_SERVER_SHARE_PARAM().

To use it outisde of OpenFrameworks, you can see how the noOF_Example is setup.   


-----


## How To Use

	float x;
	int y;
	ofColor color;

	void setup(){	
	
		OFX_REMOTEUI_SERVER_SETUP(); //start server
		
		OFX_REMOTEUI_SERVER_SHARE_PARAM(x, 0, ofGetWidth());  //Expose vars to the server, set a range
		OFX_REMOTEUI_SERVER_SHARE_PARAM(y, 0, ofGetHeight());
		OFX_REMOTEUI_SERVER_SHARE_COLOR_PARAM(color);         //share our color var

		OFX_REMOTEUI_SERVER_LOAD_FROM_XML(); //load values from XML, as they were last saved
	}
	
	void update(){
		OFX_REMOTEUI_SERVER_UPDATE(0.016666f); //keep the server updated
	}
	
	void exit(){
		OFX_REMOTEUI_SERVER_CLOSE();		//stop the server
		OFX_REMOTEUI_SERVER_SAVE_TO_XML();	//save values to XML
	}

And use the supplied OSX Client to view and edit them.


## Notes

Enums need to be consecutive, so that each enum item is +1 the previous one.


## To Do

- make a multiplatform client, maybe based on ofxUI?
