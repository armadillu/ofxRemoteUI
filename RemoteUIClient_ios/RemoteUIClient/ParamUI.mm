
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
	//NSLog(@"%@", nib);

	if (nib == nil){
		NSLog(@"nib nil!");
		return nil;
	}
	view = [nib firstObject];
	param = p;
	name = n;

    return self;
}

-(IBAction)sliderChanged:(id)sender{
	UISlider * s= (UISlider*) sender;
	NSLog(@"slider: %f forName: %s", (float)[s value], name.c_str());
	param.floatVal = [s value];
	client->sendUntrackedParamUpdate(param, name);
}

-(void)setup{
	[slider addTarget:self action:@selector(sliderChanged:) forControlEvents:UIControlEventValueChanged];
	[paramNameLabel setText:[NSString stringFromStdString:name]];
	if(param.type == REMOTEUI_PARAM_FLOAT){
		slider.minimumValue = param.minFloat;
		slider.maximumValue = param.maxFloat;
	}
}

-(UIView*) getView;{
	return view;
}
@end