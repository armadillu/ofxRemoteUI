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
#import "JoystickNotificationDelegate.h"

#define REFRESH_RATE			1.0f/15.0f
#define STATUS_REFRESH_RATE		0.333f
#define ROW_HEIGHT				(rowHeight == LARGE_34 ? 34.0f : 26.0f) 
#define ROW_WIDTH				260.0f
#define ALL_PARAMS_GROUP		@"*All Params"
#define DIRTY_PRESET_NAME		@"*No Preset"
#define NUM_FLASH_WARNING		5
#define MAIN_WINDOW_NON_LIST_H (82 + 66)

#define DEFAULT_BINDINGS_FOLDER ([NSString stringWithFormat:@"%@/Library/Application Support/ofxRemoteUIClient/",NSHomeDirectory()])
#define DEFAULT_BINDINGS_FILE (@"lastUsedBindings.ctrlrBind")

#define CONNECT_STRING		@"Connect"
#define DISCONNECT_STRING	@"Disconnect"

enum RowHeightSize{ SMALL_26 = 0, LARGE_34 = 1};

struct LayoutConfig{
	NSPoint colsRows;
	int howManyPerCol;
	int maxPerCol;
	float rowW;
};

//declare callback method 
void clientCallback(RemoteUIClientCallBackArg a);

@interface AppDelegate : NSObject <NSApplicationDelegate, VVMIDIDelegateProtocol, JoystickNotificationDelegate>{

@public

	IBOutlet ColoredNSWindow *		window;
	IBOutlet NSButton *				updateFromServerButton;
	IBOutlet NSButton *				updateContinuouslyCheckbox;
	IBOutlet NSButton *				connectButton;
	IBOutlet NSSearchField *		addressField; //NSSearchField
	IBOutlet NSTextField *			portField;
	IBOutlet NSImageView *			statusImage;
	IBOutlet NSProgressIndicator *	progress;
//	IBOutlet NSTextField *			lagField;
	IBOutlet NSView *				listContainer;
	IBOutlet MyScrollView *			scroll;
	IBOutlet NSPopUpButton *		groupsMenu;
	IBOutlet NSPopUpButton *		presetsMenu;
	IBOutlet NSPopUpButton *		neigbhorsMenu;
	IBOutlet NSTextField *			neigbhorsField;
	IBOutlet NSMenu *				groupsMenuBar;

	//prefs
	IBOutlet NSColorWell *			colorWell;
	IBOutlet NSButton *				alwaysOnTopCheckbox;
	IBOutlet NSButton *				showNotificationsCheckbox;
	IBOutlet NSButton *				externalButtonsBehaveAsToggleCheckbox;
	IBOutlet NSButton *				autoConnectCheckbox;
	IBOutlet NSPopUpButton *		rowHeightMenu;

	IBOutlet NSTextView *			logView;
	IBOutlet NSTextView *			serverLogView;

	bool							updateContinuosly;

	NSMutableArray*					currentNeighbors;

	map<string, ParamUI*>			widgets;
	vector<string>					orderedKeys; // used to keep the order in which the items were added

	map<string, ParamUI*>			spacerGroups; // a subset of the params, only the spacer params
	ofxRemoteUIClient *				client;

	NSTimer *						timer;
	NSTimer *						statusTimer;
	LayoutConfig					lastLayout;

	string							currentGroup;
	string							currentPreset;

	BOOL							launched;
	BOOL							alwaysOnTop;
	BOOL							showNotifications;
	BOOL							externalButtonsBehaveAsToggle;	//if true, one press on midi or joystick toggles a bool;
																	//otherwise, it is true for as long as its pressed
	BOOL							autoConnectToggle;

	bool							needFullParamsUpdate;

	bool							connecting;

	//MIDI
	VVMIDIManager					*midiManager;
	ParamUI							*upcomingMidiParam;
	map<string, string>				bindingsMap; //table of bindings for midi and joystick
	IBOutlet NSTableView			*midiBindingsTable;

	RowHeightSize					rowHeight;
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
-(IBAction)nextPreset:(id)sender;
-(IBAction)previousPreset:(id)sender;

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
-(IBAction)clearServerLog:(id)sender;
-(void)appendToServerLog:(NSString*)line ;
-(void)appendToLog:(NSString*) line;
-(void)updateNeighbors;
-(void)recalcWindowSize;

-(void)saveMidiBindingsToFile:(NSURL*)path;

-(void)connect;
-(void)autoConnectToNeighbor:(string) host port:(int)p;
-(void)update;

-(void)fullParamsUpdate;
-(void)partialParamsUpdate;

-(void)hideAllWarnings;

-(void)userChangedParam:(RemoteUIParam)p paramName:(string)name; //this is a delegate method, items will call this on widgetChange

-(void)updateGroupPopup;
-(void)updatePresetsPopup;
-(void)updateGroupPresetMenus;

-(void)clearSelectionPresetMenu;

-(RowHeightSize)getRowHeight;

-(vector<string>)getParamsInGroup:(string)group;
-(vector<string>)getAllGroupsInParams;
-(map<string, ParamUI*>)getAllGroupSpacerParams;

//midi
-(void)userClickedOnParamForMidiBinding:(ParamUI*)param;

//midi delegate
- (void) setupChanged;
- (void) receivedMIDI:(NSArray *)a fromNode:(VVMIDINode *)n;

//joystick delegates
- (void)joystickAdded:(Joystick *)joystick ;
- (void)joystickAxisChanged:(Joystick *)joystick atAxisIndex:(int)axis;
- (void)joystickButton:(int)buttonIndex state:(BOOL)pressed onJoystick:(Joystick*)joystick;


//growl
-(void)showNotificationWithTitle:(NSString*)title description:(NSString*)desc ID:(NSString*)key priority:(int)p;

//alert
-(NSString *)showAlertWithInput: (NSString *)prompt defaultValue: (NSString *)defaultValue ;
@end
