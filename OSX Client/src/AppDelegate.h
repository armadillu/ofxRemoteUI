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
#import "ExternalDevices.h"
#import "LogWindows.h"

#include "constants.h"

#define MEASURE_PERFORMANCE false

enum RowHeightSize{ SMALL_26 = 0, LARGE_34 = 1, TINY_20 = 2};

struct LayoutConfig{
	NSPoint colsRows;
	int howManyPerCol;
	int maxPerCol;
	float rowW;
};

//declare callback method 
void clientCallback(RemoteUIClientCallBackArg a);

@interface AppDelegate : NSObject <NSApplicationDelegate>{

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
	IBOutlet NSButton *				autoConnectCheckbox;
	IBOutlet NSButton *				onlyAutoConnectLocalCheckbox;
	IBOutlet NSPopUpButton *		rowHeightMenu;
	IBOutlet NSButton *				highlightParamDiffOnPresetLoad;


	IBOutlet ExternalDevices*		externalDevices;
	IBOutlet LogWindows*			logs;

	bool							updateContinuosly;

	NSMutableArray*					currentNeighbors;

	unordered_map<string, ParamUI*>	widgets;
	vector<string>					orderedKeys; // used to keep the order in which the items were added

	unordered_map<string, RemoteUIParam> previousParams;
	vector<string> userPresetSelectionHistory;
	float userChosePresetTimeout; //we use this to keep track of users chosing presets; when user chose a preset, this timer
									//counts N seconds back to zero. If we get a "SERVER_SENT_FULL_PARAMS_UPDATE" callback, we know its
									//bc the user asked for a preset (so we can do the diff thing)

	unordered_map<string, ParamUI*>	spacerGroups; // a subset of the params, only the spacer params
	ofxRemoteUIClient *				client;

	NSTimer *						timer;
	NSTimer *						statusTimer;
	LayoutConfig					lastLayout;

	string							currentGroup;
	string							currentPreset;

	BOOL							launched;
	BOOL							alwaysOnTop;
	BOOL							showNotifications;
																	//otherwise, it is true for as long as its pressed
	BOOL							autoConnectToggle;
	BOOL							onlyAutoConnectToLocalHost;
	BOOL 							highlightDiffOnPresetLoad;
	
	bool							connecting;

	RowHeightSize					rowHeight;
	int								weJustDisconnected;

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

-(IBAction)userWantsRestoreXML:(id)sender;
-(IBAction)userWantsRestoreDefaults:(id)sender;

-(IBAction)pasteSpecial:(id)sender;
-(IBAction)copySpecial:(id)sender;
-(IBAction)applyPrefs:(id)sender;

-(void)updateNeighbors;
-(void)recalcWindowSize;

-(LogWindows*)getLogWindows;
-(ExternalDevices*)getExternalDevices;

-(void)connect;
-(void)autoConnectToNeighbor:(string) host port:(int)p;
-(void)update;

-(void)openLocalPresetFile:(string) file;

-(void)fullParamsUpdate;
-(void)partialParamsUpdate;
-(void)removeParam:(string) paramName;

-(void)hideAllWarnings;

-(void)userChangedParam:(RemoteUIParam)p paramName:(string)name; //this is a delegate method, items will call this on widgetChange

-(void)updateGroupPopup;
-(void)updatePresetsPopup;
-(void)updateGroupPresetMenus;

-(void)clearSelectionPresetMenu;

-(RowHeightSize)getRowHeight;

-(vector<string>)getParamsInGroup:(string)group;
-(vector<string>)getAllGroupsInParams;
-(unordered_map<string, ParamUI*>)getAllGroupSpacerParams;

- (void) filesWereDropped:(NSNotification *) notification;

//growl
-(void)showNotificationWithTitle:(NSString*)title description:(NSString*)desc ID:(NSString*)key priority:(int)p;

//alert
-(NSString *)showAlertWithInput: (NSString *)prompt defaultValue: (NSString *)defaultValue ;

@end
