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
#define REFRESH_RATE 1.0f/15.0f

@interface AppDelegate : NSObject <NSApplicationDelegate, NSTableViewDataSource, NSTableViewDelegate>{

	IBOutlet NSWindow *window;
	IBOutlet NSTableView *tableView;
	IBOutlet NSButton *updateFromServerButton;
	IBOutlet NSButton *updateContinuouslyCheckbox;
	IBOutlet NSButton *connectButton;
	IBOutlet NSTextField *addressField;

	bool updateContinuosly;

	map<string, Item*> widgets;

	ofxRemoteUIClient client;
	NSTimer * timer;

}


-(IBAction)pressedSync:(id)sender;
-(IBAction)pressedContinuously:(id)sender;
-(IBAction)pressedConnect:(id)sender;
-(void) connect;

-(void)setup;
-(void)update;

-(void)userChangedParam:(RemoteUIParam)p paramName:(string)name; //this is a delegate method, items will call this on widgetChange

@end
