//
//  AppDelegate.h.h
//  ofxRemoteUIClientOSX
//
//  Created by Oriol Ferrer Mesia on 8/28/11.
//  Copyright 2011 uri.cat. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#include "ofxRemoteUIClient.h"
#import "ParamUI.h"
#import "MyScrollView.h"
#import "ColoredNSWindow.h"
#import "GitCommitNumber.h"
#import <Growl/Growl.h>
#import <VVMIDI/VVMIDI.h>

#define REFRESH_RATE			1.0f/15.0f
#define STATUS_REFRESH_RATE		0.333f
#define ROW_HEIGHT				34.0f
#define ROW_WIDTH				260.0f
#define ALL_PARAMS_GROUP		@"*All Params"
#define DIRTY_PRESET_NAME		@"*No Preset"
#define NUM_FLASH_WARNING		5

struct LayoutConfig{
	NSPoint colsRows;
	int howManyPerCol;
	int maxPerCol;
	float rowW;
};

//declare callback method 
void clientCallback(RemoteUIClientCallBackArg a);

@interface AppDelegate : NSObject <NSApplicationDelegate, VVMIDIDelegateProtocol>{

@public

	IBOutlet ColoredNSWindow *		window;
	IBOutlet NSButton *				updateFromServerButton;
	IBOutlet NSButton *				updateContinuouslyCheckbox;
	IBOutlet NSButton *				connectButton;
	IBOutlet NSSearchField *		addressField; //NSSearchField
	IBOutlet NSTextField *			portField;
	IBOutlet NSImageView *			statusImage;
	IBOutlet NSProgressIndicator *	progress;
	IBOutlet NSTextField *			lagField;
	IBOutlet NSView *				listContainer;
	IBOutlet MyScrollView *			scroll;
	IBOutlet NSPopUpButton *		groupsMenu;
	IBOutlet NSPopUpButton *		presetsMenu;
	IBOutlet NSPopUpButton *		neigbhorsMenu;
	IBOutlet NSTextField *			neigbhorsField;
	IBOutlet NSMenu *				groupsMenuBar;

	IBOutlet NSColorWell *			colorWell;
	IBOutlet NSButton *				alwaysOnTopCheckbox;
	IBOutlet NSButton *				showNotificationsCheckbox;

	IBOutlet NSTextView *			logView;

	bool							updateContinuosly;

	NSMutableArray*					currentNeighbors;

	map<string, ParamUI*>			widgets;
	vector<string>					orderedKeys; // used to keep the order in which the items were added

	ofxRemoteUIClient *				client;

	NSTimer *						timer;
	NSTimer *						statusTimer;
	LayoutConfig					lastLayout;

	string							currentGroup;
	string							currentPreset;

	BOOL							launched;
	BOOL							alwaysOnTop;
	BOOL							showNotifications;

	bool							needFullParamsUpdate;


	//MIDI
	VVMIDIManager					*midiManager;
	ParamUI							*upcomingMidiParam;
	map<string, string>				midiBindings;
	IBOutlet NSTableView			*midiBindingsTable;
}

-(ofxRemoteUIClient *)getClient;

-(IBAction)pressedSync:(id)sender;
-(IBAction)pressedContinuously:(id)sender;
-(IBAction)pressedConnect:(id)sender;
-(IBAction)filterType:(id)sender;
-(IBAction)userAddPreset:(id)sender;
-(IBAction)userDeletePreset:(id)sender;

-(IBAction)userChoseGroup:(id)sender;
-(IBAction)userChosePreset:(id)sender;
-(IBAction)userChoseNeighbor:(id)sender;

-(IBAction)restartXcodeApp:(id)sender;
-(void)restartXcodeApp;
-(void)openAccessibilitySystemPrefs;

-(IBAction)userPressedSave:(id)sender;

-(IBAction)saveMidiBindings:(id)who;
-(IBAction)loadMidiBindings:(id)who;
-(IBAction)clearMidiBindings:(id)sender;
-(IBAction)deleteSelectedMidiBinding:(id)sender;

-(IBAction)userWantsRestoreXML:(id)sender;
-(IBAction)userWantsRestoreDefaults:(id)sender;

-(IBAction)pasteSpecial:(id)sender;
-(IBAction)copySpecial:(id)sender;
-(IBAction)applyPrefs:(id)sender;

-(void)log:(RemoteUIClientCallBackArg) arg;
-(IBAction)clearLog:(id)sender;
-(void)updateNeighbors;

-(void)connect;
-(void)update;

-(void)fullParamsUpdate;
-(void)partialParamsUpdate;

-(void)userChangedParam:(RemoteUIParam)p paramName:(string)name; //this is a delegate method, items will call this on widgetChange

-(void)updateGroupPopup;
-(void)updatePresetsPopup;

-(vector<string>)getParamsInGroup:(string)group;
-(vector<string>)getAllGroupsInParams;

//midi
-(void)userClickedOnParamForMidiBinding:(ParamUI*)param;

//growl
-(void)showNotificationWithTitle:(NSString*)title description:(NSString*)desc ID:(NSString*)key priority:(int)p;

@end
