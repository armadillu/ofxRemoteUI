//
//  AppDelegate.h.h
//  ofxRemoteUIClientOSX
//
//  Created by Oriol Ferrer Mesia on 8/28/11.
//  Copyright 2011 uri.cat. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#include "ofxRemoteUIClient.h"
#import "Item.h"
#import "MyScrollView.h"

#define REFRESH_RATE			1.0f/15.0f
#define STATUS_REFRESH_RATE		0.333f
#define ROW_HEIGHT				34.0f
#define ROW_WIDTH				280.0f
#define ALL_PARAMS_GROUP		@"*All Params"

struct LayoutConfig{
	NSPoint colsRows;
	int howManyPerCol;
	int maxPerCol;
	float rowW;
};


@interface AppDelegate : NSObject <NSApplicationDelegate>{

	IBOutlet NSWindow *window;
	IBOutlet NSButton *updateFromServerButton;
	IBOutlet NSButton *updateContinuouslyCheckbox;
	IBOutlet NSButton *connectButton;
	IBOutlet NSSearchField *addressField; //NSSearchField
	IBOutlet NSTextField *portField;
	IBOutlet NSImageView *statusImage;
	IBOutlet NSProgressIndicator *progress;
	IBOutlet NSTextField *lagField;
	IBOutlet NSView *listContainer;
	IBOutlet MyScrollView * scroll;
	IBOutlet NSPopUpButton * groups;
	

	bool updateContinuosly;

	map<string, Item*> widgets;
	vector<string> orderedKeys; // used to keep the order in which the items were added

	ofxRemoteUIClient * client;
	NSTimer * timer;
	NSTimer * statusTimer;
	LayoutConfig lastLayout;

	string currentGroup;
	BOOL waitingForResults;

	BOOL launched;
}


-(IBAction)pressedSync:(id)sender;
-(IBAction)pressedContinuously:(id)sender;
-(IBAction)pressedConnect:(id)sender;
-(IBAction)filterType:(id)sender;
-(IBAction)userChoseGroup:(id)sender;

-(IBAction)pasteSpecial:(id)sender;
-(IBAction)copySpecial:(id)sender;

-(void)connect;

-(void)update;

-(BOOL)fullParamsUpdate;
-(void)partialParamsUpdate;

-(void)userChangedParam:(RemoteUIParam)p paramName:(string)name; //this is a delegate method, items will call this on widgetChange

-(vector<string>)getParamsInGroup:(string)group;
-(vector<string>)getAllGroupsInParams;

@end
