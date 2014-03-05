
#include "ofxRemoteUI.h"
#import <Foundation/Foundation.h>
#include <string>
#import "ColorView.h"


@interface ParamUI : NSObject <NSTextFieldDelegate>{

	@public
	RemoteUIParam param;
	string paramName;
	NSControl * widget; //
	//int row;//

	IBOutlet ColorView *		ui;
	IBOutlet ColorView *		bg;
	IBOutlet NSSlider *			slider;
	IBOutlet NSTextField *		textView;
	IBOutlet NSButton *			button;
	IBOutlet NSButton *			paramLabel;
	//IBOutlet NSTextField *		paramGroup;
	IBOutlet NSTextField *		sliderVal;
	IBOutlet NSTextField *		sliderMin;
	IBOutlet NSTextField *		sliderMax;
	IBOutlet NSPopUpButton *	enumeratorMenu;
	IBOutlet NSColorWell *		colorWell;
	IBOutlet NSImageView *		warningSign;
	IBOutlet NSTextField *		spacerTitle;
	IBOutlet NSPopUpButton *	groupPresetMenu;
	IBOutlet NSButton *			groupPresetAddButton;
	IBOutlet NSButton *			groupPresetDeleteButton;

	int numberID; // to handle alternating rows in table draw
	bool shouldBeFlashing;

	bool deleting; //we are about to dealloc this param!
	//midi
	bool midiHighlightAnim;
	NSTimer * waitingForMidiTimer;

	//for that group
	string currentPreset;
}

-(void)dealloc;

-(id)initWithParam: (RemoteUIParam)p paramName:(string)name ID:(int)n;
-(void)updateParam:(RemoteUIParam)p;
-(void)updateUI;

-(NSPopUpButton*)getPresetsMenu;
-(void)updatePresetMenuSelectionToCurrent;
-(void)resetSelectedPreset;

-(void)fadeOut;
-(void)fadeIn;
-(void)flashWarning:(NSNumber *)times;
-(void)hideWarning;
-(void)disableChanges;
-(void)enableChanges;
-(void)remapSlider;
-(string)getParamName;

-(NSString*)formatedFloat:(float) f;

//midi
-(void)stopMidiAnim;

-(IBAction)updateFloat:(id)sender;
-(IBAction)updateInt:(id)sender;
-(IBAction)updateBool:(id)sender;
-(IBAction)updateString:(id)sender;

-(IBAction)userPressedAddGroupPreset:(id)sender;
-(IBAction)userPressedDeleteGroupPreset:(id)sender;
-(IBAction)userChoseGroupPreset:(id)sender;

-(IBAction)clickOnLabel:(id)sender;

//textField delegate notifications <NSTextFieldDelegate>
- (void)controlTextDidBeginEditing:(NSNotification *)obj;
- (void)controlTextDidEndEditing:(NSNotification *)obj;
- (void)controlTextDidChange:(NSNotification *)aNotification;

@end
