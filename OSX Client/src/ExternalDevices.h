//
//  ExternalDevices.h
//  ofxRemoteUIClientOSX
//
//  Created by Oriol Ferrer Mesià on 19/04/14.
//
//

#import <Foundation/Foundation.h>
#import <VVMIDI/VVMIDI.h>
#import "JoystickNotificationDelegate.h"
#include "ofxRemoteUIClient.h"
#import "ParamUI.h"
#include "constants.h"

@interface ExternalDevices : NSObject <VVMIDIDelegateProtocol, JoystickNotificationDelegate>{

	IBOutlet NSTableView			*midiBindingsTable;
	IBOutlet NSButton *				externalButtonsBehaveAsToggleCheckbox;

	//MIDI
	VVMIDIManager					*midiManager;
	ParamUI							*upcomingDeviceParam;
	map<string, string>				bindingsMap; //table of bindings for midi and joystick

	unordered_map<string, ParamUI*> *			widgets;
	ofxRemoteUIClient *				client;
	BOOL							externalButtonsBehaveAsToggle;	//if true, one press on midi or joystick toggles a bool;

	struct MidiOutCache{
		string deviceName;
		string channel;
		string controlID;
		int channelInt;
		int controlIDInt;
	};

	map<string,MidiOutCache>		midiDevCache;

}

-(void)updateDevicesWithClientValues:(BOOL)onlyColor resetToZero:(BOOL)reset paramName:(string)pName;
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
- (void) setupChanged;
- (void) receivedMIDI:(NSArray *)a fromNode:(VVMIDINode *)n;

//joystick delegates
- (void)joystickAdded:(Joystick *)joystick ;
- (void)joystickAxisChanged:(Joystick *)joystick atAxisIndex:(int)axis;
- (void)joystickButton:(int)buttonIndex state:(BOOL)pressed onJoystick:(Joystick*)joystick;

- (MidiOutCache) cacheForControlURL:(string) url;

@end
