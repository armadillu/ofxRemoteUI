
#include "ofxRemoteUI.h"
#import <Foundation/Foundation.h>
#include <string>
#import "ColorView.h"


@interface ParamUI : NSObject{

	@public
	RemoteUIParam param;
	string paramName;
	NSControl * widget; //
	//int row;//

	IBOutlet ColorView * ui;
	IBOutlet ColorView * bg;
	IBOutlet NSSlider *slider;
	IBOutlet NSTextField *textView;
	IBOutlet NSButton *button;
	IBOutlet NSTextField *paramLabel;
	IBOutlet NSTextField *paramGroup;
	IBOutlet NSTextField *sliderVal;
	IBOutlet NSTextField *sliderMin;
	IBOutlet NSTextField *sliderMax;
	IBOutlet NSPopUpButton * enumeratorMenu;
	IBOutlet NSColorWell * colorWell;
	IBOutlet NSImageView * warningSign;

	int numberID; // to handle alternating rows in table draw
	bool shouldBeFlashing;
	
}

-(void)dealloc;

-(id)initWithParam: (RemoteUIParam)p paramName:(string)name ID:(int)n;
-(void)updateParam:(RemoteUIParam)p;
-(void)updateUI;

-(void)fadeOut;
-(void)fadeIn;
-(void)flashWarning:(NSNumber *)times;
-(void)hideWarning;
-(void)disableChanges;
-(void)enableChanges;
-(void)remapSlider;

-(NSString*)formatedFloat:(float) f;


-(IBAction)updateFloat:(id)sender;
-(IBAction)updateInt:(id)sender;
-(IBAction)updateBool:(id)sender;
-(IBAction)updateString:(id)sender;
@end
