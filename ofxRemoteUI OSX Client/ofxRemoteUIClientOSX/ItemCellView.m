//
//  ItemCellView.m
//  ofxRemoteUIClientOSX
//
//  Created by Oriol Ferrer Mesia on 8/29/11.
//  Copyright 2011 uri.cat. All rights reserved.
//

#import "ItemCellView.h"

@implementation ItemCellView

@synthesize textView = _textView;
@synthesize slider = _slider;
@synthesize button = _button;
@synthesize paramLabel = _paramLabel;
@synthesize sliderVal = _sliderVal;
@synthesize sliderMin = _sliderMin;
@synthesize sliderMax = _sliderMax;



- (void)dealloc {
	[_paramLabel release], _paramLabel = nil;
	[_textView release], _textView = nil;
	[_slider release], _slider = nil;
	[_button release], _button = nil;
	[_sliderVal release], _sliderVal = nil;
	[_sliderMin release], _sliderMin = nil;
	[_sliderMax release], _sliderMax = nil;

    [super dealloc];
}

@end
