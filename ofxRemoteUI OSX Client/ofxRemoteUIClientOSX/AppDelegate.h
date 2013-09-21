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
#import <Growl/Growl.h>

#define REFRESH_RATE			1.0f/15.0f
#define STATUS_REFRESH_RATE		0.333f
#define ROW_HEIGHT				34.0f
#define ROW_WIDTH				280.0f
#define ALL_PARAMS_GROUP		@"*All Params"
#define DIRTY_PRESET_NAME		@"*No Preset"

struct LayoutConfig{
	NSPoint colsRows;
	int howManyPerCol;
	int maxPerCol;
	float rowW;
};

//declare callback method 
void clientCallback(RemoteUIClientCallBackArg a);

@interface AppDelegate : NSObject <NSApplicationDelegate>{

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
	IBOutlet NSMenu *				groupsMenuBar;

	IBOutlet NSColorWell *			colorWell;
	IBOutlet NSButton *				alwaysOnTopCheckbox;

	IBOutlet NSTextView *			logView;

	bool							updateContinuosly;

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

@public

	bool							needFullParamsUpdate;
}

-(ofxRemoteUIClient *)getClient;

-(IBAction)pressedSync:(id)sender;
-(IBAction)pressedContinuously:(id)sender;
-(IBAction)pressedConnect:(id)sender;
-(IBAction)filterType:(id)sender;
-(IBAction)userChoseGroup:(id)sender;
-(IBAction)userChosePreset:(id)sender;
-(IBAction)userAddPreset:(id)sender;
-(IBAction)userDeletePreset:(id)sender;

-(IBAction)userPressedSave:(id)sender;

-(IBAction)userWantsRestoreXML:(id)sender;
-(IBAction)userWantsRestoreDefaults:(id)sender;

-(IBAction)pasteSpecial:(id)sender;
-(IBAction)copySpecial:(id)sender;
-(IBAction)applyPrefs:(id)sender;

-(void)log:(RemoteUIClientCallBackArg) arg;
-(IBAction)clearLog:(id)sender;

-(void)connect;
-(void)update;

-(void)fullParamsUpdate;
-(void)partialParamsUpdate;

-(void)userChangedParam:(RemoteUIParam)p paramName:(string)name; //this is a delegate method, items will call this on widgetChange

-(void)updateGroupPopup;
-(void)updatePresetsPopup;

-(vector<string>)getParamsInGroup:(string)group;
-(vector<string>)getAllGroupsInParams;

//growl
-(void)showNotificationWithTitle:(NSString*)title description:(NSString*)desc ID:(NSString*)key priority:(int)p;

@end
