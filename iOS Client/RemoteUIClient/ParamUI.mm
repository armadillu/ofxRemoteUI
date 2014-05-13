
#import "ParamUI.h"




///////////////////////////////////////////////////////////////////////////////

@implementation ParamUI

-(void)dealloc{
	NSLog(@"deallpc %s", name.c_str());
}

-(id)initWithParam:(RemoteUIParam)p name:(string)n ID:(int)ID client:(ofxRemoteUI*) c{

	self = [super init];
	client = c;
	numberID = ID;
	NSArray * nib = [[NSBundle mainBundle] loadNibNamed:@"ParamView_ipad" owner:self options:nil];
	if (nib == nil){
		NSLog(@"nib nil!");
		return nil;
	}
	view = [nib objectAtIndex:0];
	param = p;
	name = n;
	hasBeenSetup = false;
    return self;
}

-(BOOL)hasBeenSetup{

	return hasBeenSetup;
}

-(void)setup{

	UIColor * col = [UIColor colorWithRed: param.r/255.
									green: param.g/255.
									 blue: param.b/255.
									alpha: param.a/255.];

	[rightView setBackgroundColor: col];
	[leftView setBackgroundColor: col];

	//[slider addTarget:self action:@selector(sliderChanged:) forControlEvents:UIControlEventValueChanged];
	[paramLabel setText:[NSString stringFromStdString:name]];


	switch (param.type) {
		case REMOTEUI_PARAM_FLOAT:
			widget = slider;
			[widget addTarget:self action:@selector(sliderChanged:) forControlEvents:UIControlEventValueChanged];
			slider.minimumValue = param.minFloat;
			slider.maximumValue = param.maxFloat;
			[button removeFromSuperview];
			[textView removeFromSuperview];
			[spacerTitle removeFromSuperview];
			break;

		case REMOTEUI_PARAM_INT:
			widget = slider;
			[widget addTarget:self action:@selector(sliderChanged:) forControlEvents:UIControlEventValueChanged];
			slider.minimumValue = param.minInt;
			slider.maximumValue = param.maxInt;
			[button removeFromSuperview];
			[textView removeFromSuperview];
			[spacerTitle removeFromSuperview];
			break;

		case REMOTEUI_PARAM_COLOR:
			for(id v in [leftView subviews]){
				[v removeFromSuperview];
			}
			[spacerTitle removeFromSuperview];
//			widget = colorWell;
//			[widget setAction:@selector(updateColor:)];
//			[button removeFromSuperviewWithoutNeedingDisplay];
//			[slider removeFromSuperviewWithoutNeedingDisplay];
//			[sliderMax removeFromSuperviewWithoutNeedingDisplay];
//			[sliderMin removeFromSuperviewWithoutNeedingDisplay];
//			[sliderVal removeFromSuperviewWithoutNeedingDisplay];
//			[textView removeFromSuperviewWithoutNeedingDisplay];
//			[enumeratorMenu removeFromSuperviewWithoutNeedingDisplay];
//			[spacerTitle removeFromSuperviewWithoutNeedingDisplay];
//			[groupPresetMenu removeFromSuperviewWithoutNeedingDisplay];
//			[groupPresetAddButton removeFromSuperviewWithoutNeedingDisplay];
//			[groupPresetDeleteButton removeFromSuperviewWithoutNeedingDisplay];
			break;

		case REMOTEUI_PARAM_ENUM:{
			for(id v in [leftView subviews]){
				[v removeFromSuperview];
			}
			[spacerTitle removeFromSuperview];
//			widget = enumeratorMenu;
//			[widget setAction:@selector(updateEnum:)];
//			[button removeFromSuperviewWithoutNeedingDisplay];
//			[slider removeFromSuperviewWithoutNeedingDisplay];
//			[textView removeFromSuperviewWithoutNeedingDisplay];
//			[sliderMax removeFromSuperviewWithoutNeedingDisplay];
//			[sliderMin removeFromSuperviewWithoutNeedingDisplay];
//			[sliderVal removeFromSuperviewWithoutNeedingDisplay];
//			[colorWell removeFromSuperviewWithoutNeedingDisplay];
//			[spacerTitle removeFromSuperviewWithoutNeedingDisplay];
//			[groupPresetMenu removeFromSuperviewWithoutNeedingDisplay];
//			[groupPresetAddButton removeFromSuperviewWithoutNeedingDisplay];
//			[groupPresetDeleteButton removeFromSuperviewWithoutNeedingDisplay];
//			[enumeratorMenu removeAllItems];
//			for(int i = 0; i < param.enumList.size(); i++){
//				[enumeratorMenu addItemWithTitle:[NSString stringWithFormat:@"%s", param.enumList[i].c_str()]];
//			}
		}break;

		case REMOTEUI_PARAM_BOOL:
			widget = button;
			[widget addTarget:self action:@selector(switchChanged:) forControlEvents:UIControlEventValueChanged];
			[slider removeFromSuperview];
			[textView removeFromSuperview];
			[sliderMax removeFromSuperview];
			[sliderMin removeFromSuperview];
			[sliderVal removeFromSuperview];
			//[colorWell removeFromSuperview];
			//[enumeratorMenu removeFromSuperview];
			[spacerTitle removeFromSuperview];
			break;

		case REMOTEUI_PARAM_STRING:
			widget = textView;
			textView.delegate = self;

			[slider removeFromSuperview];
			[button removeFromSuperview];
			[sliderMax removeFromSuperview];
			[sliderMin removeFromSuperview];
			[sliderVal removeFromSuperview];
//			[colorWell removeFromSuperview];
//			[enumeratorMenu removeFromSuperview];
			[spacerTitle removeFromSuperview];
			break;

		case REMOTEUI_PARAM_SPACER:
			for(id v in [leftView subviews]){
				[v removeFromSuperview];
			}
			[paramLabel removeFromSuperview];
			spacerTitle.text = [NSString stringFromStdString: param.stringVal];
			break;

		default:NSLog(@"wtf is this?");
			break;
	}
	[self updateUI];
	hasBeenSetup = true;
}

- (BOOL)textFieldShouldReturn:(UITextField *)textField {
	param.stringVal = [textField.text UTF8String];
	[textField resignFirstResponder];
	client->sendUntrackedParamUpdate(param, name);

    return NO;
}

-(IBAction)switchChanged:(id)sender;{
	UISwitch * s = (UISwitch*) sender;
	param.boolVal = s.on;
	client->sendUntrackedParamUpdate(param, name);
}



-(IBAction)sliderChanged:(id)sender{
	UISlider * s= (UISlider*) sender;
	if(param.type == REMOTEUI_PARAM_FLOAT){
		param.floatVal = [s value];
		sliderVal.text = [self formatedFloat:param.floatVal];
	}
	if(param.type == REMOTEUI_PARAM_INT){
		param.intVal = [s value];
		sliderVal.text = [NSString stringWithFormat:@"%d", param.intVal ];
	}
	client->sendUntrackedParamUpdate(param, name);
}

-(void)updateParam:(RemoteUIParam)p;{
	param = p;
}


-(void)updateUI{

	switch (param.type) {
		case REMOTEUI_PARAM_FLOAT:
			sliderVal.text = [self formatedFloat:param.floatVal];
			sliderMax.text = [self formatedFloat:param.maxFloat];
			sliderMin.text = [self formatedFloat:param.minFloat];
			slider.value = param.floatVal;
			break;

		case REMOTEUI_PARAM_INT:
			sliderVal.text = [NSString stringWithFormat:@"%d", param.intVal ];
			sliderMax.text = [NSString stringWithFormat:@"%d", param.maxInt ];
			sliderMin.text = [NSString stringWithFormat:@"%d", param.minInt ];
			slider.value = param.intVal;
			break;

		case REMOTEUI_PARAM_COLOR:{
//			NSColor * col = [NSColor colorWithSRGBRed:param.redVal/255. green:param.greenVal/255. blue:param.blueVal/255. alpha:param.alphaVal/255.];
//			//col = [col colorUsingColorSpaceName:NSCalibratedRGBColorSpace];
//			//CGFloat comp[] = {param.redVal/255., param.greenVal/255., param.blueVal/255., param.alphaVal/255. };
//			//NSColor * col = [NSColor colorWithColorSpace:[NSColorSpace sRGBColorSpace] components:comp count:4];
//			col = [col colorUsingColorSpaceName:NSCalibratedRGBColorSpace];
//			[colorWell setColor:col];
		}break;

		case REMOTEUI_PARAM_ENUM:
			//[enumeratorMenu selectItemAtIndex: param.intVal - param.minInt];
			break;

		case REMOTEUI_PARAM_BOOL:
			button.on = param.boolVal;
			//button.title = param.boolVal ? @"ON" : @"OFF";
			break;

		case REMOTEUI_PARAM_STRING:
			textView.text = [NSString stringFromStdString: param.stringVal];
			//[textView setStringValue: [NSString stringWithFormat:@"%@", [NSDate date]]];
			break;
		case REMOTEUI_PARAM_SPACER:
			break;
		default:
			NSLog(@"updateUI wtf");
			break;
	}
}

-(NSString*)formatedFloat:(float) f;{
	NSNumber *num = [NSNumber numberWithFloat:f];
	NSNumberFormatter *formatter = [[NSNumberFormatter alloc] init];
	[formatter setUsesGroupingSeparator:NO];
	[formatter setDecimalSeparator:@"."];
	[formatter setMinimumIntegerDigits:1];
	[formatter setGroupingSeparator:@"."];
	[formatter setMaximumFractionDigits:2];
	NSString *formattedNumber = [formatter stringFromNumber:num];
	return formattedNumber;
}

-(UIView*) getView;{
	return view;
}
@end