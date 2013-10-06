
#import "ParamUI.h"
#include "ofxRemoteUI.h"
#include "AppDelegate.h"
#import <QuartzCore/QuartzCore.h>

@implementation ParamUI

-(void)dealloc{
	[ui removeFromSuperview];
	[ui release];
	[super dealloc];
}

-(id)initWithParam: (RemoteUIParam)p paramName:(string)name ID:(int)n{
	self = [super init];
	numberID = n;
	widget = nil;
	param = p;
	paramName = name;
	BOOL didLoad = [NSBundle loadNibNamed:@"View" owner:self];
	if(!didLoad){
		NSLog(@"can0t load Nib for Parameter View!");
		return nil;
	}

	[ui setWantsLayer:NO];
	CALayer *viewLayer = [CALayer layer];
	[ui setLayer:viewLayer];


	CALayer * l = [CALayer layer];
	[l setContents: (id)[[NSImage imageNamed:@"warning@2x"] CGImageForProposedRect:Nil context:[NSGraphicsContext currentContext] hints:nil]];
	[warningSign setLayer:l];
	[warningSign setWantsLayer:YES];
	[warningSign layer].opacity = 0.0f;

	//disable implicit caAnims
	NSMutableDictionary *newActions = [[NSMutableDictionary alloc] initWithObjectsAndKeys:
									   [NSNull null], @"onOrderIn",
									   [NSNull null], @"onOrderOut",
									   [NSNull null], @"sublayers",
									   [NSNull null], @"contents",
									   [NSNull null], @"bounds",
									   nil];
	viewLayer.actions = newActions;
	l.actions = newActions;
	[newActions release];
	return self;
}

-(void)fadeOut{
	[ui setWantsLayer:YES];
	[ui layer].opacity = 0.25;
}


-(void)fadeIn{
	[ui layer].opacity = 1;
	[ui setWantsLayer:NO];
}

-(void)fadeOutSlowly{
	[CATransaction begin];
	[CATransaction setAnimationDuration:5];
	[CATransaction setAnimationTimingFunction:[CAMediaTimingFunction functionWithName:kCAMediaTimingFunctionEaseInEaseOut]];
	[warningSign layer].opacity = 0.0;
	[CATransaction commit];
}

-(void)hideWarning{
	//[CATransaction flush];
	[warningSign layer].opacity = 0.0;
}

-(void)flashWarning:(NSNumber *) times{

	__block int localTimes = (int)[times integerValue];

	float duration = 0.2;
	[CATransaction begin];
	[CATransaction setAnimationDuration: duration];
	[CATransaction setAnimationTimingFunction:[CAMediaTimingFunction functionWithName:kCAMediaTimingFunctionEaseInEaseOut]];
		[CATransaction setCompletionBlock:^{

			[CATransaction begin];
			[CATransaction setAnimationDuration:duration];
			[CATransaction setAnimationTimingFunction:[CAMediaTimingFunction functionWithName:kCAMediaTimingFunctionEaseInEaseOut]];
			[CATransaction setCompletionBlock:^{
				localTimes--;
				//NSLog(@"flash %d times", localTimes);
				if(localTimes > 0){
					[self performSelector:@selector(flashWarning:) withObject:[NSNumber numberWithInt:localTimes] afterDelay:0.2f];
				}else{ //last fadeout is really long
					[CATransaction begin];
					[CATransaction setAnimationDuration: duration];
					[warningSign layer].opacity = 1.0;
					[CATransaction commit];
					[self performSelector:@selector(fadeOutSlowly) withObject:nil afterDelay:5.0f];
					//[self performSelectorOnMainThread:@selector(fadeOutSlowly) withObject:nil waitUntilDone:NO];
				}
			}];
			[warningSign layer].opacity = 0.0;
			[CATransaction commit];
		}];
		[warningSign layer].opacity = 1.0;
	[CATransaction commit];
	//[[NSRunLoop currentRunLoop] performSelector:@selector(commit) target:[CATransaction class] argument:nil order:0 modes:[NSArray arrayWithObjects:NSDefaultRunLoopMode, NSEventTrackingRunLoopMode, nil]];
}


-(void)awakeFromNib{

	// create alternating row look
	if (numberID%2 == 1)
		[bg setBackgroundColor:[NSColor whiteColor]];
	else
		[bg setBackgroundColor:[NSColor colorWithDeviceRed:0.950 green:0.950 blue:1 alpha:1.000]];

	if (param.a > 0 ){
		[bg setBackgroundColor: [NSColor colorWithDeviceRed: param.r/255.
														  green: param.g/255.
														   blue: param.b/255.
														  alpha: param.a/255.]
		 ];
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
		NSSlider * s = (NSSlider*)widget;
		float w = [s frame].size.width;
		int numTicks = w / 7;

		if ([s allowsTickMarkValuesOnly]){ // for int sliders, lets make sure there arent more marks than possible values
			int range = 1 + [s maxValue] - [s minValue];
			if (numTicks > range){
				numTicks = range;
			}
		}
		[s setNumberOfTickMarks: numTicks];
	}
}


-(void)setupUI{

	switch (param.type) {
		case REMOTEUI_PARAM_FLOAT:
			widget = slider;
			[widget setAction:@selector(updateFloat:)];
			[sliderVal setAction:@selector(updateFloatManually:)];
			[sliderVal setTarget:self];
			[slider setMaxValue:param.maxFloat];
			[slider setMinValue:param.minFloat];
			[button removeFromSuperview];
			[textView removeFromSuperview];
			[slider setAllowsTickMarkValuesOnly:false];
			[enumeratorMenu removeFromSuperview];
			[colorWell removeFromSuperview];
			break;

		case REMOTEUI_PARAM_INT:
			widget = slider;
			[slider setMaxValue:param.maxInt];
			[slider setMinValue:param.minInt];
			[sliderVal setAction:@selector(updateIntManually:)];
			[sliderVal setTarget:self];
			[slider setAllowsTickMarkValuesOnly:true];
			[widget setAction:@selector(updateInt:)];
			[button removeFromSuperview];
			[textView removeFromSuperview];
			[enumeratorMenu removeFromSuperview];
			[colorWell removeFromSuperview];
			break;

		case REMOTEUI_PARAM_COLOR:
			widget = colorWell;
			[widget setAction:@selector(updateColor:)];
			[button removeFromSuperview];
			[slider removeFromSuperview];
			[sliderMax removeFromSuperview];
			[sliderMin removeFromSuperview];
			[sliderVal removeFromSuperview];
			[textView removeFromSuperview];
			[enumeratorMenu removeFromSuperview];
			break;

		case REMOTEUI_PARAM_ENUM:{
			widget = enumeratorMenu;
			[widget setAction:@selector(updateEnum:)];
			[button removeFromSuperview];
			[slider removeFromSuperview];
			[textView removeFromSuperview];
			[sliderMax removeFromSuperview];
			[sliderMin removeFromSuperview];
			[sliderVal removeFromSuperview];
			[colorWell removeFromSuperview];
			[enumeratorMenu removeAllItems];
			for(int i = 0; i < param.enumList.size(); i++){
				[enumeratorMenu addItemWithTitle:[NSString stringWithFormat:@"%s", param.enumList[i].c_str()]];
			}
			}break;

		case REMOTEUI_PARAM_BOOL:
			widget = button;
			[button setAction:@selector(updateBool:)];
			[slider removeFromSuperview];
			[textView removeFromSuperview];
			[sliderMax removeFromSuperview];
			[sliderMin removeFromSuperview];
			[sliderVal removeFromSuperview];
			[colorWell removeFromSuperview];
			[enumeratorMenu removeFromSuperview];
			break;

		case REMOTEUI_PARAM_STRING:
			widget = textView;
			[textView setAction:@selector(updateString:)];
			[slider removeFromSuperview];
			[button removeFromSuperview];
			[sliderMax removeFromSuperview];
			[sliderMin removeFromSuperview];
			[sliderVal removeFromSuperview];
			[colorWell removeFromSuperview];
			[enumeratorMenu removeFromSuperview];
			break;

		default:NSLog(@"wtf is this?");
			break;
	}
	paramLabel.stringValue = [self stringFromString:paramName];
	if (param.group!=DEFAULT_PARAM_GROUP){
		paramGroup.stringValue = [self stringFromString:param.group];
	}else{
		paramGroup.stringValue = @"";
	}
	[widget setTarget:self];
}


-(void)updateParam:(RemoteUIParam)p;{
	param = p;
}


-(void)updateUI{
	switch (param.type) {
		case REMOTEUI_PARAM_FLOAT:
			[slider setFloatValue:param.floatVal];
			[sliderVal setStringValue:[self formatedFloat:param.floatVal]];
			[sliderMax setStringValue:[self formatedFloat:param.maxFloat]];
			[sliderMin setStringValue:[self formatedFloat:param.minFloat]];
			break;

		case REMOTEUI_PARAM_INT:
			[slider setIntegerValue:param.intVal];
			[sliderVal setStringValue:[NSString stringWithFormat:@"%d", param.intVal ]];
			[sliderMax setStringValue:[NSString stringWithFormat:@"%d", param.maxInt ]];
			[sliderMin setStringValue:[NSString stringWithFormat:@"%d", param.minInt ]];
			break;

		case REMOTEUI_PARAM_COLOR:{
			NSColor * col = [NSColor colorWithSRGBRed:param.redVal/255. green:param.greenVal/255. blue:param.blueVal/255. alpha:param.alphaVal/255.];
			//col = [col colorUsingColorSpaceName:NSCalibratedRGBColorSpace];
			//CGFloat comp[] = {param.redVal/255., param.greenVal/255., param.blueVal/255., param.alphaVal/255. };
			//NSColor * col = [NSColor colorWithColorSpace:[NSColorSpace sRGBColorSpace] components:comp count:4];
			col = [col colorUsingColorSpaceName:NSCalibratedRGBColorSpace];
			[colorWell setColor:col];
			}break;

		case REMOTEUI_PARAM_ENUM:
			[enumeratorMenu selectItemAtIndex: param.intVal - param.minInt];
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

-(IBAction)updateColor:(id)sender{

	NSColor * col = [sender color];
	//NSLog(@"colorSP: %@", [col colorSpaceName]);
	//col = [col colorUsingColorSpaceName:NSCalibratedRGBColorSpace device:[[NSApp mainWindow] deviceDescription]];
	//NSString*	myColorSpace = [col colorSpaceName];
	col = [col colorUsingColorSpaceName: NSCalibratedRGBColorSpace device:[[NSApp mainWindow] deviceDescription]];
	//[sender setColor:col];
	//NSColor * col2 = [col colorUsingColorSpace:[NSColorSpace sRGBColorSpace] ];
	//[sender performSelector:@selector(setColor:) withObject:col2 afterDelay:1];

	//NSLog(@"colorSP2: %@", [col colorSpaceName]);
	param.redVal = [col redComponent] * 255.0f;
	param.greenVal = [col greenComponent] * 255.0f;
	param.blueVal = [col blueComponent] * 255.0f;
	param.alphaVal = [col alphaComponent] * 255.0f;
	if ([[NSApp delegate] respondsToSelector:@selector(userChangedParam:paramName:)]){
		[[NSApp delegate] userChangedParam: param paramName: paramName];  //blindly send message to App Delegate (TODO!)
	}
}


-(NSString*)formatedFloat:(float) f;{
	NSNumber *num = [NSNumber numberWithFloat:f];
	NSNumberFormatter *formatter = [[NSNumberFormatter alloc] init];
	[formatter setUsesGroupingSeparator:NO];
	[formatter setDecimalSeparator:@"."];
	[formatter setMinimumIntegerDigits:1];
	[formatter setGroupingSeparator:@"."];
	[formatter setMaximumFractionDigits:5];
	NSString *formattedNumber = [formatter stringFromNumber:num];
	[formatter release];
	return formattedNumber;
}

-(IBAction)updateIntManually:(id)sender{
	param.intVal = [sender intValue];
	[slider setIntValue:param.intVal];
	if ([[NSApp delegate] respondsToSelector:@selector(userChangedParam:paramName:)]){
		[[NSApp delegate] userChangedParam: param paramName: paramName];  //blindly send message to App Delegate (TODO!)
	}
}

-(IBAction)updateFloatManually:(id)sender{
	param.floatVal = [sender floatValue];
	[slider setFloatValue:param.floatVal];
	if ([[NSApp delegate] respondsToSelector:@selector(userChangedParam:paramName:)]){
		[[NSApp delegate] userChangedParam: param paramName: paramName];  //blindly send message to App Delegate (TODO!)
	}
	[[NSApp mainWindow] makeFirstResponder:nil];
}

-(IBAction)updateFloat:(id)sender{
	param.floatVal = [sender floatValue];
	[sliderVal setStringValue:[self formatedFloat:param.floatVal]];
	if ([[NSApp delegate] respondsToSelector:@selector(userChangedParam:paramName:)]){
		 [[NSApp delegate] userChangedParam: param paramName: paramName];  //blindly send message to App Delegate (TODO!)
	}
}

-(IBAction)updateEnum:(id)sender{
	int index = (int)[sender indexOfSelectedItem];
	param.intVal = param.minInt + index;
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
