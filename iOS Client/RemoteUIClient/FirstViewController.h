//
//  FirstViewController.h
//  RemoteUIClient
//
//  Created by Oriol Ferrer Mesià on 11/05/14.
//  Copyright (c) 2014 Oriol Ferrer Mesià. All rights reserved.
//

#import <UIKit/UIKit.h>
#import "ParamUI.h"
#include "ofxRemoteUIClient.h"


#define REFRESH_RATE			1.0f/15.0f
#define TOOLBAR_H				44
#define CONNECT_EMOJI			@"🌍"
#define PRESET_EMOJI			@"📖"
#define SAVE_EMOJI				@"Save"
#define ADD_PRESET_EMOJI		@"Add📖"

@interface FirstViewController : UICollectionViewController <UIActionSheetDelegate>{

	@public


	ofxRemoteUIClient *				client;
	NSTimer *						timer;

	map<string, ParamUI*>			widgets;

	NSMutableArray *				paramViews;
	NSMutableArray *				currentNeighbors;
	vector<string>					presets;

	bool							needFullParamsUpdate;
	BOOL							connected;

	//toolbar
	UIToolbar *						toolbar;
	UIBarButtonItem *				connectB;
	UIBarButtonItem *				presetsButton;
	UIBarButtonItem *				saveButton;
	UIBarButtonItem *				addPresetsButton;


	//current or upcoming connection
	NSString *						address;
	NSString *						port;

	//actionSheets
	UIActionSheet *					connectSheet;
	UIActionSheet *					presetsSheet;
}

-(ofxRemoteUIClient *)getClient;
-(void)fullParamsUpdate;
-(void)partialParamsUpdate;
-(void)cleanUpGUIParams;

-(void)updateNeighbors;
-(void)updatePresets;

-(IBAction)pressedConnectButton;
-(IBAction)pressedPresetsButton;
-(IBAction)pressedSaveButton;
-(IBAction)pressedAddPresetButton;

-(void)connect;
-(void)disconnect;

- (void)actionSheet:(UIActionSheet *)actionSheet clickedButtonAtIndex:(NSInteger)buttonIndex;

@end
