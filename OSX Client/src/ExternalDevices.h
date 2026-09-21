//
//  ExternalDevices.h
//  ofxRemoteUIClientOSX
//
//  Created by Oriol Ferrer Mesià on 19/04/14.
//
//

#import <Foundation/Foundation.h>
#import "JoystickNotificationDelegate.h"
#include "ofxRemoteUIClient.h"
#import "ParamUI.h"
#import "RUMidi.h"
#include "constants.h"

@interface ExternalDevices : NSObject <RUMidiDelegate, JoystickNotificationDelegate>{

	IBOutlet NSTableView			*midiBindingsTable;
	IBOutlet NSButton *				externalButtonsBehaveAsToggleCheckbox;
	IBOutlet NSButton *				knobOnColorAffectsAlpha; //or hue


	//MIDI
	RUMidi *						midi;

	ParamUI							*upcomingDeviceParam;
	map<string, string>				bindingsMap; //table of bindings for midi and joystick

	unordered_map<string, ParamUI*> *			widgets;
	ofxRemoteUIClient *				client;
	BOOL							externalButtonsBehaveAsToggle;	//if true, one press on midi or joystick toggles a bool;
	BOOL 							knobColorAffectsAlpha;

	struct MidiOutCache{
		string deviceName;
		string channel;
		string controlID;
		int channelInt;
		int controlIDInt;
	};

	map<string,MidiOutCache>		midiDevCache;

}

-(void)updateDevicesWithClientValues:(BOOL)onlyColor resetToZero:(BOOL)reset paramName:(const string&)pName;
-(IBAction)flashBoundControllers:(id)sender; //for n seconds

-(void)initWithWidgets:(unordered_map<string, ParamUI*>*) widgets andClient:(ofxRemoteUIClient*) client;
-(void)savePrefs:(id)sender;
-(IBAction)applyPrefs:(id)sender;
-(void)loadPrefs;



-(BOOL)parseDeviceBindingsFromFile:(NSURL*) file;
-(void)saveDeviceBindingsToFile:(NSURL*) path;
-(IBAction)saveDeviceBindings:(id)who;

-(void)updateParamUIOnMainThread:(ParamUI*)item;

//midi
-(void)userClickedOnParamForDeviceBinding:(ParamUI*)param;

//midi delegate

- (void) midiSetupChanged:(RUMidi *)midi;
- (void) midi:(RUMidi *)midi didReceiveMessage:(RUMidiMessage *)message fromDevice:(RUMidiDevice *)device;

//joystick delegates
- (void)joystickAdded:(Joystick *)joystick ;
- (void)joystickAxisChanged:(Joystick *)joystick atAxisIndex:(int)axis;
- (void)joystickButton:(int)buttonIndex state:(BOOL)pressed onJoystick:(Joystick*)joystick;

- (MidiOutCache) cacheForControlURL:(string) url;

- (void) dealloc;
@end
