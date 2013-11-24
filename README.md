# ofxRemoteUI


OF addon allows you to serve any variables you want (bool, float, int, enum, string, ofColor) on the network, so that you can modify them remotely. Uses server client architecture, where your app is the server. It communicates both ways; you can modify your project's variables from the client, but you can also pull your app's variable values from the client; this way you can track values that evolve programatically. 

Watch a quick [Intro Video](http://youtu.be/F18f67d_WjU).

![OSX Client](http://farm4.staticflickr.com/3680/10821138976_d11125455d_o.png "OSX Client")

---

## Features

* Edit & Track variables remotely through UDP/OSC (bool, int, float, string, Enum, ofColor).
* Native OSX interface.
* Alternative mutliplatform client built on top of ofxUI (with less features for now)
* Allows to save/load your variable states across app launches.
* Parameter values are saved in your app's data folder, in xml format.
* Allows creation/deletion of Presets, variable states that you can switch from and to quickly.
* Easily create Parameter Groups, and access them through keyboard shortcuts from the OSX Client.
* Colorize your variables to visually group them in the OSX client.
* Automatic discovery of servers in the network; easily control multiple apps from the OSX Client.
* Press "tab" on server app to quickly see all your params on screen
* Notifications in both OSX client (through growl) and on your OF app.
* Restore parameters to the "previous launch" state or to the default values.
* ofxRemoteUI Can be used outside OF in any C++ project, and in Processing thx to [@kritzikratzi](http://github.com/kirtzikratzi)
* Easy to use macros hide complexity away, very easy to plug into any existing project.



## Compatibility
Works in OpenFrameworks, but also in plain C++ projects.

There's also a feature limited version of the Server for Processing, made by [@kritzikratzi](http://github.com/kirtzikratzi)! See <a href="http://superduper.org/processing/remoteUI">here</a>.

## Why?

I know there's tons of very good UI's already, but one thing that bothers me about most of them is how by using them, your are affecting your framerates quite a lot. That's the main reason that made me build ofxRemoteUI. Being quite comfy in OSX development, I chose to make a full-featured native OSX client; although clients for other platforms could be developed as well, using the underlying ofxRemoteUIClient class.

## Details

It's OSC based, and it includes a native OSX Client. The Native OSX Client allows param colorization for better clarity, and live param filtering. It also supports the grouping of params into categories, for easy access. There's automatic keyboard shortcuts to do so.

It can also be set to store the current values when quitting the app (or whenever its convenient), so that you can carry on where you left off last time you used it. It does so by saving a file called "ofxRemoteUISettings.xml" in your data folder. It uses ofxXmlSettings to store everything. 

You can also create and delete presets, which are parameter states for your app. Presets are stored with your OF app, inside an "ofxRemoteUIPresets" folder, in your data folder. This makes it easy to check in your presets with your source code. Whenever you like the current config, you can make a preset to keep it around. You can also delete presets.

ofxRemoteUI uses Macros + the singleton pattern to make it very easy to share any variable you decide to edit remotely, from any class of your project.

The OSX client also allows to copy all the current params as plain text. You can also paste them back after editing them! Thx to @kritzikratzi for this idea!

**"Restore to initial XML Values"** sets alls params to whatever values they had at server app launch.  
**"Restore to Default Values"** sets alls params to whatever values the shared variable had before sharing it with OFX_REMOTEUI_SERVER_SHARE_PARAM().

To use it outside of OpenFrameworks, you can see how the example-noOF is setup.   

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
	
Use the supplied OSX Client to view and edit your shared parameters.


## Notes

Enums must be consecutive so that each enum item is +1 the previous one for them to work.

When loading a preset, it might be that the preset doesn't specify values for all your current params. If so, the params whose values haven't been modified by the preset will show a small warning sign for a few seconds.

Automatic discovery relies on each server advertising itself (its hostname, app name and port) on port 25748 over OSC.

OFX_REMOTEUI_SERVER_SETUP() assigns a random port the first time the app is launched, and it uses that same port on successive launches. You can also manually specify a port by supplying it OFX_REMOTEUI_SERVER_SETUP(10000);

ofxRemoteUIServer listens for the keyDown event, and if "tab" is pressed, it displays all your parameters on screen.


## WIP

- A basic multi-platform client (based in OF, so can target win/linux/ios/androd) based on ofxUI.