//
//  ParamView.h
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


@interface ParamView : UIView {

	IBOutlet UISlider * slider;
	IBOutlet UILabel * paramNameLabel;
	UIView * view;

	RemoteUIParam		param;
	string				name;

}

-(id)initWithParam:(RemoteUIParam)p name:(string)name;
-(IBAction)sliderChanged:(id)sender;
-(void)setup;
-(UIView*) getView;
@end
