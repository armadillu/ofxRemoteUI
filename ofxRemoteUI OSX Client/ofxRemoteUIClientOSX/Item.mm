
#import "Item.h"
#include "ofxRemoteUI.h"


@implementation Item


-(id)initWithParam: (RemoteUIParam)p paramName:(string)name{

	param = p;
	paramName = name;
	return self;
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


-(void)setCellView:(ItemCellView*)v{

	cellView = v;

	switch (param.type) {
		case REMOTEUI_PARAM_FLOAT:
			widget = v.slider;
			[widget setAction:@selector(updateFloat:)];
			[v.slider setMaxValue:param.maxFloat];
			[v.slider setMinValue:param.minFloat];
			[v.button removeFromSuperview];
			[v.textView removeFromSuperview];
			[v.slider setAllowsTickMarkValuesOnly:false];
			break;

		case REMOTEUI_PARAM_INT:
			widget = v.slider;
			[v.slider setMaxValue:param.maxInt];
			[v.slider setMinValue:param.minInt];
			[v.slider setAllowsTickMarkValuesOnly:true];
			[widget setAction:@selector(updateInt:)];
			[v.button removeFromSuperview];
			[v.textView removeFromSuperview];
			break;

		case REMOTEUI_PARAM_BOOL:
			widget = v.button;
			[v.button setAction:@selector(updateBool:)];
			[v.slider removeFromSuperview];
			[v.textView removeFromSuperview];
			[v.sliderMax removeFromSuperview];
			[v.sliderMin removeFromSuperview];
			[v.sliderVal removeFromSuperview];
			break;

		case REMOTEUI_PARAM_STRING:
			widget = v.textView;
			[v.textView setAction:@selector(updateString:)];
			[v.slider removeFromSuperview];
			[v.button removeFromSuperview];
			[v.sliderMax removeFromSuperview];
			[v.sliderMin removeFromSuperview];
			[v.sliderVal removeFromSuperview];
			break;

		default:
			break;
	}

	v.paramLabel.stringValue = [self stringFromString:paramName];
	[widget setTarget:self];
}

-(void)updateValues:(RemoteUIParam)p;{
	param = p;
}

-(void)updateUI{
	switch (param.type) {
		case REMOTEUI_PARAM_FLOAT:
			[cellView.slider setFloatValue:param.floatVal];
			[cellView.sliderVal setStringValue:[NSString stringWithFormat:@"%.2f", param.floatVal ]];
			[cellView.sliderMax setStringValue:[NSString stringWithFormat:@"%.1f", param.maxFloat ]];
			[cellView.sliderMin setStringValue:[NSString stringWithFormat:@"%.1f", param.minFloat ]];
			break;

		case REMOTEUI_PARAM_INT:
			[cellView.slider setIntegerValue:param.intVal];
			[cellView.sliderVal setStringValue:[NSString stringWithFormat:@"%d", param.intVal ]];
			[cellView.sliderMax setStringValue:[NSString stringWithFormat:@"%d", param.maxInt ]];
			[cellView.sliderMin setStringValue:[NSString stringWithFormat:@"%d", param.minInt ]];
			break;

		case REMOTEUI_PARAM_BOOL:
			[cellView.button setState:param.boolVal];
			cellView.button.title = param.boolVal ? @"ON" : @"OFF";
			break;

		case REMOTEUI_PARAM_STRING:
			[cellView.textView setStringValue: [self stringFromString: param.stringVal]];
			break;

		default:
			break;
	}
}


-(IBAction)updateFloat:(id)sender{
	param.floatVal = [sender floatValue];
	[cellView.sliderVal setStringValue:[NSString stringWithFormat:@"%.2f", param.floatVal ]];
	if ([[NSApp delegate] respondsToSelector:@selector(userChangedParam:paramName:)]){
		 [[NSApp delegate] userChangedParam: param paramName: paramName];  //blindly send message to App Delegate (TODO!)
	}
}

-(IBAction)updateInt:(id)sender{
	param.intVal = [sender intValue];
	[cellView.sliderVal setStringValue:[NSString stringWithFormat:@"%d", param.intVal ]];
	if ([[NSApp delegate] respondsToSelector:@selector(userChangedParam:paramName:)]){
		[[NSApp delegate] userChangedParam: param paramName: paramName];  //blindly send message to App Delegate (TODO!)
	}
}

-(IBAction)updateBool:(id)sender{
	printf("%d\n", [sender intValue]);
	param.boolVal = [sender intValue];
	cellView.button.title = param.boolVal ? @"ON" : @"OFF";
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
