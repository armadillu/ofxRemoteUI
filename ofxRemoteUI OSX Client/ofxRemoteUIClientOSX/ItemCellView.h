//
//  ItemCellView.h
//  ofxRemoteUIClientOSX
//
//  Created by Oriol Ferrer Mesia on 8/29/11.
//  Copyright 2011 uri.cat. All rights reserved.
//

@interface ItemCellView : NSTableCellView

@property (nonatomic, retain) IBOutlet NSSlider *slider;
@property (nonatomic, retain) IBOutlet NSTextField *textView;
@property (nonatomic, retain) IBOutlet NSButton *button;
@property (nonatomic, retain) IBOutlet NSTextField *paramLabel;
@property (nonatomic, retain) IBOutlet NSTextField *sliderVal;
@property (nonatomic, retain) IBOutlet NSTextField *sliderMin;
@property (nonatomic, retain) IBOutlet NSTextField *sliderMax;

@end
