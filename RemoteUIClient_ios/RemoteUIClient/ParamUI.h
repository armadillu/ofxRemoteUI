//
//  ParamUI.h
//  RemoteUIClient
//
//  Created by Oriol Ferrer Mesià on 11/05/14.
//  Copyright (c) 2014 Oriol Ferrer Mesià. All rights reserved.
//

#import <Foundation/Foundation.h>
#include "ofxRemoteUI.h"
#include "RemoteParam.h"
#import "NSStringAdditions.h"
using namespace std;


/////////////////////////////////////////////////////////////////


@interface ParamUI : UIView {

	IBOutlet UISlider * slider;
	IBOutlet UILabel * paramNameLabel;
	UIView *			view;
	ofxRemoteUI*		client;

	RemoteUIParam		param;
	string				name;
	int					numberID;

}

-(id)initWithParam:(RemoteUIParam)p name:(string)name ID:(int)ID client:(ofxRemoteUI*) client;
-(IBAction)sliderChanged:(id)sender;
-(void)setup;
-(UIView*) getView;
@end
