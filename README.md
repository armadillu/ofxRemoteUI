# ofxRemoteUI


OpenFrameworks addon that allows you to serve c++ variables/parameters (bool, float, int, enum, string, ofColor) on the network, so that you can modify them remotely. It uses server client architecture, where your app is the server. It communicates both ways; you can modify your project's variables from the client, but you can also pull your app's variable values from the client; this way you can track values that evolve programatically. It runs on OSC.

You can save and then load "presets", which allow you to quickly change values for a lot of your parameters quickly. You can also make "group presets" to change only the values of a subset of your parameters.

Watch a quick [Intro Video](http://youtu.be/F18f67d_WjU).

![MultiPlatform Client](https://farm4.staticflickr.com/3926/14983323502_4019a37a8f_o_d.png "List of Clients") In Order of appearance, OSX Client, Built In Client, iOS Client (wip), multiplatform client (based on ofxUI)


---

## Features

* Edit & Track variables remotely through UDP/OSC (bool, int, float, string, Enum, ofColor).
* Allows to save/load your variable states across app launches.
* MIDI and Joystick controller bindings. Bind any parameter to any MIDI controller knob/slider/note or HID Joystick. (OSX Client)
* Parameter values are saved in your app's data folder, in xml format.
* Allows creation/deletion of Presets, variable states that you can switch from and to quickly.
* Presets can be created globally (saving all parameter values at once), or for a subset of params (param group).
* Easily create Parameter Groups, and access them through keyboard shortcuts (OSX Client).
* Parameter Groups are automatically colorized to easily identify them.
* Realtime Filter your params by name to find things quickly (OSX Client).
* Automatic discovery of servers in the network; easily control multiple apps from one Client interface.
* Press "tab" on your OF app to see/edit your params from within your OF app. Allows you to do quick client-less edits; and also load and save Global and Group Presets.
* Event notifications in both OSX client (through growl) and on your OF app.
* You can always restore your parameters to the "previous launch" state, or to the default values.
* Log remotely - ofxRemoteUIServer allows you to log messages to you client with RUI_LOG(); which accepts printf-like formatted writing.
* ofxRemoteUI can be used outside OF in any C++ project, and in Processing thx to [@kritzikratzi](http://github.com/kirtzikratzi)
* Easy to use C++ Macros hide complexity away, very easy to plug into any existing project.
* Support for native ofParameters of compatible types (int, float, bool, string ofColor, ofVec). Allows you to edit native OF ofParamater types from RemoteUI. See "example-ofParameter".

##Available Clients

* Native OSX client, feature complete (this should be your first choice)
* Alternative mutliplatform (iOS/win) client built on top of ofxUI (less features & less robust, stale)
* Native iOS client (WIP - OSC is not very reliable over WIFI).
* built in client (inside the OF app) for basic edits, saving, resetting, and global and group preset loading and saving.


## Compatibility

Works in OpenFrameworks, but also in plain C++ projects.
There a Cinder Block I created of as "proof of concept" but it's not even published. Ask me if you are interested.

There's also a feature limited version of the server for Processing, made by [@kritzikratzi](http://github.com/kirtzikratzi)! See <a href="http://superduper.org/processing/remoteUI">here</a>.

## Motivations

I know there's tons of very good UI's already, but one thing that bothers me about most of them is how by using them, your are affecting your framerates quite a lot. That's the main reason that made me build ofxRemoteUI. Being quite comfy in OSX development, I chose to make a full-featured native OSX client; although clients for other platforms could be developed as well, using the underlying ofxRemoteUIClient class.

## Details

It's OSC based, and it includes a native OSX Client. The Native OSX Client allows param colorization for better clarity, and live param filtering. It also supports the grouping of params into categories, for easy access. There's automatic keyboard shortcuts to do so.

It can also be set to store the current values when quitting the app (or whenever its convenient), so that you can carry on where you left off last time you used it. It does so by saving a file called "ofxRemoteUISettings.xml" in your data folder. It uses ofxXmlSettings to store everything. 

You can also create and delete presets, which are parameter states for your app. Presets are stored with your OF app, inside an "ofxRemoteUIPresets" folder, in your data folder. This makes it easy to check in your presets with your source code. Whenever you like the current config, you can make a preset to keep it around. You can also delete presets.

ofxRemoteUI uses Macros + the singleton pattern to make it very easy to share any variable you decide to edit remotely, from any class of your project.

The OSX client also allows to copy all the current params as plain text. You can also paste them back after editing them! Thx to @kritzikratzi for this idea!

To use it outside of OpenFrameworks, you can see how the example-noOF is setup.   

-----


## How To Use

the most basic setup only requires a few calls.

	float x;
	int y;
	ofColor color;

	void setup(){	
	
		RUI_SETUP(); //start server
		
		//Expose x and y vars to the server, providing a valid slider range
		RUI_SHARE_PARAM(x, 0, ofGetWidth()); 
		RUI_SHARE_PARAM(y, 0, ofGetHeight());
		
		//share the color param
		RUI_SHARE_COLOR_PARAM(color);
		
		//load values from XML, as they were last saved (if they were)
		RUI_LOAD_FROM_XML(); 
	}
	
Then, use any of the client options to view and edit your shared parameters.
Look into the server example to see more features. It is fairly documented.


## Controller Bindings
The ofxRemoteUI OSX client allows to bind any midi control / joystick axis / button to any of your params. Make sure your device is connected before you launch the app. To bind a device to a parameter, do the following:

1. Click on a param name on the main Window. It will start blinking.
2. Rotate/Slide/Press your external device control (joystick, gamepad button, midi slider, knob, etc).
3. Done! You can now control that param from your external controller

You can Save/Load/Edit/Clear your midi bindings from the "MIDI Bindings" window. 

MIDI Sliders/Knobs/etc can be bound to floats, ints, enums, bools and colors. For ints, floats and enums, the mapping is obvious; for bools, the lower half of a slider/knob sets the param to false, the upper half to true. For colors, the slider shifts the hue of the color parameter.

Bools can also be bound to "piano keys"; params being set to true for as long as a key is held down.

Bindings are saved when the app is quit. You can also save any particular device binding configuration into a ".midiBind" file. You can also double-click any .midiBind file form the finder to load your previously saved bindings. There is a "bindings" window that allows you to delete particular bindings. You can see what parameters are currently bound by choosing "File->Blink Bound Midi Controls".

## Random Notes

**"Restore to initial XML Values"** sets alls params to whatever values they had at server app launch.  
**"Restore to Default Values"** sets alls params to whatever values the shared variable had before sharing it with RUI_SHARE_PARAM().

Enums must be consecutive so that each enum item is +1 the previous one for them to work. This is usually the default c++ behavior if you don't set specific values when defining your enums. Break this rule and you will get crashes.

When loading a preset, it might be that the preset doesn't specify values for all your current params (because it was created when that param didn't exist). If so, the params whose values haven't been modified by the preset will show a small warning sign for a few seconds, so that you are aware.

Automatic discovery relies on each server advertising itself (its hostname, app name and port) on port 25748 over OSC.

There is a setting in the OSX client that allows for it to automatically connect to a server app when it launches in the local network, or only on your local computer. Look into the OSX Client preferences window.

RUI_SETUP() assigns a random port the first time the app is launched, and it uses that same port on successive launches. You can also manually specify a port by supplying it RUI_SETUP(10000);

ofxRemoteUIServer listens for the keyDown event, and if "tab" is pressed, it displays a built-in client with some basic features. You can interact with the built-in client using arrow keys, return, and some other keystrokes depending on the context. Read the on-screen help at the bottom.

The built-in UI can be set to draw in any scale (useful for retina screens) by using:
```
RUI_GET_INSTANCE()->setBuiltInUiScale(scale);
```

You can set the built-in UI to be drawn using [ofxFontStash](https://github.com/armadillu/ofxFontStash) by adding it to your project, and defining USE_OFX_FONTSTASH in your project's PreProcessor Macros. This allows you to use any font to draw the built-in client ui by calling, instead of the default OF bitmap font.
```
RUI_GET_INSTANCE()->drawUiWithFontStash("myFont.ttf");
```

##XML FILE FORMAT

Commits before the Git Tag "LastCommitWithXMLv1" use the original file format for XML files. Commits after that tag, will automatically save in the new format (v2), but will also parse files with the old format. You can also save files in the old format by pressing "E" (for export) from the built in client, either globally or on a per-group basis.

The new file format is proper XML with a root node, it's more human readable, it keeps params listed in the same order they are added in so its easier to read changes on versioning systems, and it has comments showing the group they belong to.

![g](https://farm8.staticflickr.com/7358/16344314099_97e5a68275_o_d.png)

## LICENSE and ATTRIBUTIONS

ofxRemoteUI is made available under the [MIT](http://opensource.org/licenses/MIT) license.

The OSX Client uses the [vvMidi](https://github.com/mrRay/vvopensource) frameworks to handle MIDI devices more easily. VVMidi uses a [LGPL](https://github.com/mrRay/vvopensource/blob/master/lgpl-3.0.txt) license. 

The OSX Client's HID capabilities come mostly from [@jotapeh](https://github.com/jotapeh/MacJoystickHIDTest).

ofxRemoteUI bundles ofxXmlSettings and ofxOSC from [OpenFrameworks](http://openframeworks.cc) to allow non-OF C++ projects to use ofxRemoteUI.