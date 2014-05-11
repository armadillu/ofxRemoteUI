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


@interface FirstViewController : UICollectionViewController{

	@public


	ofxRemoteUIClient *				client;
	NSTimer *						timer;

	map<string, ParamUI*>			widgets;
	vector<string>					orderedKeys; // used to keep the order in which the items were added

	NSMutableArray	*				paramViews;


	bool							needFullParamsUpdate;
	BOOL							connected;
}

-(ofxRemoteUIClient *)getClient;
-(void)fullParamsUpdate;
-(void) partialParamsUpdate;
-(void)cleanUpGUIParams;

-(void)connect;
@end
