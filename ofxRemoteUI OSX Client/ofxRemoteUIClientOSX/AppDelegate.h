//
//  AppDelegate.h.h
//  ofxRemoteUIClientOSX
//
//  Created by Oriol Ferrer Mesia on 8/28/11.
//  Copyright 2011 uri.cat. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#include "ofxRemoteUI.h"
#import "Item.h"
#define REFRESH_RATE			1.0f/15.0f
#define STATUS_REFRESH_RATE		1.0

@interface AppDelegate : NSObject <NSApplicationDelegate, NSTableViewDataSource, NSTableViewDelegate>{

	IBOutlet NSWindow *window;
	IBOutlet NSTableView *tableView;
	IBOutlet NSButton *updateFromServerButton;
	IBOutlet NSButton *updateContinuouslyCheckbox;
	IBOutlet NSButton *connectButton;
	IBOutlet NSTextField *addressField;
	IBOutlet NSTextField *portField;
	IBOutlet NSImageView *statusImage;
	IBOutlet NSProgressIndicator *progress;
	IBOutlet NSTextField *lagField;
	bool updateContinuosly;

	map<string, Item*> widgets;
	vector<string> keyOrder; // used to keep the order in which the items were added

	ofxRemoteUIClient client;
	NSTimer * timer;
	NSTimer * statusTimer;
}


-(IBAction)pressedSync:(id)sender;
-(IBAction)pressedContinuously:(id)sender;
-(IBAction)pressedConnect:(id)sender;
-(void) connect;

-(void)setup;
-(void)update;

-(void)userChangedParam:(RemoteUIParam)p paramName:(string)name; //this is a delegate method, items will call this on widgetChange

@end
