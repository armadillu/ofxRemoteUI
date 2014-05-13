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


@interface ParamUI : UIView <UITextFieldDelegate> {

	IBOutlet UISlider * slider;
	IBOutlet UILabel *	paramLabel;

	IBOutlet UIView *	rightView;
	IBOutlet UIView *	leftView;


	IBOutlet UITextField *		textView;
	IBOutlet UISwitch *			button;
	IBOutlet UILabel *			sliderVal;
	IBOutlet UILabel *			sliderMin;
	IBOutlet UILabel *			sliderMax;
	//IBOutlet NSPopUpButton *	enumeratorMenu;
	//IBOutlet NSColorWell *		colorWell;
	IBOutlet UILabel *		spacerTitle;

	UIControl *			widget; //wildcard

	UIView *			view;
	ofxRemoteUI*		client;

	RemoteUIParam		param;
	string				name;
	int					numberID;
	BOOL				hasBeenSetup;
}

-(id)initWithParam:(RemoteUIParam)p name:(string)name ID:(int)ID client:(ofxRemoteUI*) client;

-(void)updateUI;
-(void)updateParam:(RemoteUIParam)p;

-(IBAction)sliderChanged:(id)sender;
-(IBAction)switchChanged:(id)sender;
-(IBAction)textChanged:(id)sender;

-(void)setup;
-(UIView*) getView;

-(IBAction)updateFloat:(id)sender;
-(IBAction)updateInt:(id)sender;
-(IBAction)updateBool:(id)sender;
-(IBAction)updateString:(id)sender;
-(BOOL)hasBeenSetup;

@end
