
#import "Item.h"
#include "ofxRemoteUI.h"


@implementation Item

-(void)dealloc{

	//NSLog(@"dealloc Item %s ", paramName.c_str());
	[ui removeFromSuperview];
	[ui release];
	[super dealloc];
}

-(id)initWithParam: (RemoteUIParam)p paramName:(string)name ID:(int)n{
	numberID = n;
	widget = nil;
	param = p;
	paramName = name;
	BOOL didLoad = [NSBundle loadNibNamed:@"View" owner:self];
	return self;
}


-(void)awakeFromNib{
	//NSLog(@"Item awakeFromNib %s", paramName.c_str());
	if (numberID%2 == 1)
		[ui setBackgroundColor:[NSColor whiteColor]];
	else
		[ui setBackgroundColor:[NSColor colorWithCalibratedRed:0.914 green:0.945 blue:0.998 alpha:1.000]];

	if (param.a > 0 ){
		[bg setBackgroundColor: [NSColor colorWithDeviceRed: param.r/255.
													  green: param.g/255.
													   blue: param.b/255.
													  alpha: param.a/255.]
		 ];
		[ui setBackgroundColor:[NSColor whiteColor]];
	}
	[self setupUI];

}


-(void)disableChanges;{
	[widget setEnabled:false];
}


-(void)enableChanges;{
	[widget setEnabled:true];
}


-(void)remapSlider;{
	
	if ([widget isKindOfClass: [NSSlider class]]){
		float w = [widget frame].size.width;
		int numTicks = w / 7;

		if ([widget allowsTickMarkValuesOnly]){ // for int sliders, lets make sure there arent more marks than possible values
			int range = 1 + [widget maxValue] - [widget minValue];
			if (numTicks > range){
				numTicks = range;
			}
		}
		[widget setNumberOfTickMarks: numTicks];
	}
}


-(void)setupUI{

	switch (param.type) {
		case REMOTEUI_PARAM_FLOAT:
			widget = slider;
			[widget setAction:@selector(updateFloat:)];
			[slider setMaxValue:param.maxFloat];
			[slider setMinValue:param.minFloat];
			[button removeFromSuperview];
			[textView removeFromSuperview];
			[slider setAllowsTickMarkValuesOnly:false];
			break;

		case REMOTEUI_PARAM_INT:
			widget = slider;
			[slider setMaxValue:param.maxInt];
			[slider setMinValue:param.minInt];
			[slider setAllowsTickMarkValuesOnly:true];
			[widget setAction:@selector(updateInt:)];
			[button removeFromSuperview];
			[textView removeFromSuperview];
			break;

		case REMOTEUI_PARAM_BOOL:
			widget = button;
			[button setAction:@selector(updateBool:)];
			[slider removeFromSuperview];
			[textView removeFromSuperview];
			[sliderMax removeFromSuperview];
			[sliderMin removeFromSuperview];
			[sliderVal removeFromSuperview];
			break;

		case REMOTEUI_PARAM_STRING:
			widget = textView;
			[textView setAction:@selector(updateString:)];
			[slider removeFromSuperview];
			[button removeFromSuperview];
			[sliderMax removeFromSuperview];
			[sliderMin removeFromSuperview];
			[sliderVal removeFromSuperview];
			break;

		default:NSLog(@"wtf is this?");
			break;
	}
	paramLabel.stringValue = [self stringFromString:paramName];
	[widget setTarget:self];
}


-(void)updateParam:(RemoteUIParam)p;{
	param = p;
}


-(void)updateUI{
	switch (param.type) {
		case REMOTEUI_PARAM_FLOAT:
			[slider setFloatValue:param.floatVal];
			[sliderVal setStringValue:[NSString stringWithFormat:@"%.2f", param.floatVal ]];
			[sliderMax setStringValue:[NSString stringWithFormat:@"%.1f", param.maxFloat ]];
			[sliderMin setStringValue:[NSString stringWithFormat:@"%.1f", param.minFloat ]];
			break;

		case REMOTEUI_PARAM_INT:
			[slider setIntegerValue:param.intVal];
			[sliderVal setStringValue:[NSString stringWithFormat:@"%d", param.intVal ]];
			[sliderMax setStringValue:[NSString stringWithFormat:@"%d", param.maxInt ]];
			[sliderMin setStringValue:[NSString stringWithFormat:@"%d", param.minInt ]];
			break;

		case REMOTEUI_PARAM_BOOL:
			[button setState:param.boolVal];
			button.title = param.boolVal ? @"ON" : @"OFF";
			break;

		case REMOTEUI_PARAM_STRING:
			[textView setStringValue: [self stringFromString: param.stringVal]];
			//[textView setStringValue: [NSString stringWithFormat:@"%@", [NSDate date]]];
			break;

		default:
			NSLog(@"updateUI wtf");
			break;
	}
}


-(IBAction)updateFloat:(id)sender{
	param.floatVal = [sender floatValue];
	[sliderVal setStringValue:[NSString stringWithFormat:@"%.2f", param.floatVal ]];
	if ([[NSApp delegate] respondsToSelector:@selector(userChangedParam:paramName:)]){
		 [[NSApp delegate] userChangedParam: param paramName: paramName];  //blindly send message to App Delegate (TODO!)
	}
}

-(IBAction)updateInt:(id)sender{
	param.intVal = [sender intValue];
	[sliderVal setStringValue:[NSString stringWithFormat:@"%d", param.intVal ]];
	if ([[NSApp delegate] respondsToSelector:@selector(userChangedParam:paramName:)]){
		[[NSApp delegate] userChangedParam: param paramName: paramName];  //blindly send message to App Delegate (TODO!)
	}
}

-(IBAction)updateBool:(id)sender{
	//printf("%d\n", [sender intValue]);
	param.boolVal = [sender intValue];
	button.title = param.boolVal ? @"ON" : @"OFF";
	if ([[NSApp delegate] respondsToSelector:@selector(userChangedParam:paramName:)]){
		[[NSApp delegate] userChangedParam: param paramName: paramName];  //blindly send message to App Delegate (TODO!)
	}
}

-(IBAction)updateString:(id)sender{
	param.stringVal = string([[sender stringValue] UTF8String]);
	if ([[NSApp delegate] respondsToSelector:@selector(userChangedParam:paramName:)]){
		[[NSApp delegate] userChangedParam: param paramName: paramName];  //blindly send message to App Delegate (TODO!)
	}
}

-(NSString*)stringFromString:(string) s{
	return  [NSString stringWithCString:s.c_str() encoding:[NSString defaultCStringEncoding]];
}

@end
