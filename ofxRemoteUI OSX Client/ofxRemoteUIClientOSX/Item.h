
#include "ofxRemoteUI.h"
#import <Foundation/Foundation.h>
#include <string>
#import "ColorView.h"

@interface Item : NSObject{

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
	IBOutlet NSTextField *sliderVal;
	IBOutlet NSTextField *sliderMin;
	IBOutlet NSTextField *sliderMax;

	int numberID; // to handle alternating rows in table draw
}

-(void)dealloc;

-(id)initWithParam: (RemoteUIParam)p paramName:(string)name;
-(void)updateParam:(RemoteUIParam)p;
-(void)updateUI;

-(void)fadeOut;
-(void)fadeIn;
-(void)disableChanges;
-(void)enableChanges;
-(void)remapSlider;



-(IBAction)updateFloat:(id)sender;
-(IBAction)updateInt:(id)sender;
-(IBAction)updateBool:(id)sender;
-(IBAction)updateString:(id)sender;
@end
