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

@interface FirstViewController : UICollectionViewController <UIActionSheetDelegate>{

	@public


	ofxRemoteUIClient *				client;
	NSTimer *						timer;

	map<string, ParamUI*>			widgets;
	vector<string>					orderedKeys; // used to keep the order in which the items were added

	NSMutableArray	*				paramViews;
	NSMutableArray*					currentNeighbors;

	bool							needFullParamsUpdate;
	BOOL							connected;

	UIToolbar *						toolbar;
	UIBarButtonItem *				connectB;

	//current or upcoming connection
	NSString *						address;
	NSString *						port;
}

-(ofxRemoteUIClient *)getClient;
-(void)fullParamsUpdate;
-(void) partialParamsUpdate;
-(void)cleanUpGUIParams;

-(void)updateNeighbors;

-(IBAction)pressedConnectButton;
-(void)connect;

// Called when a button is clicked. The view will be automatically dismissed after this call returns
- (void)actionSheet:(UIActionSheet *)actionSheet clickedButtonAtIndex:(NSInteger)buttonIndex;

// Called when we cancel a view (eg. the user clicks the Home button). This is not called when the user clicks the cancel button.
// If not defined in the delegate, we simulate a click in the cancel button
- (void)actionSheetCancel:(UIActionSheet *)actionSheet;
@end
