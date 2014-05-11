
#import "ParamView.h"




///////////////////////////////////////////////////////////////////////////////

@implementation ParamView

- (id) initWithParam:(RemoteUIParam)p name:(string)n{

	self = [super init];

	NSArray * nib = [[NSBundle mainBundle] loadNibNamed:@"ParamView_ipad" owner:self options:nil];
	NSLog(@"%@", nib);

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
}

-(void)setup{
	[slider addTarget:self action:@selector(sliderChanged:) forControlEvents:UIControlEventValueChanged];
	[paramNameLabel setText:[NSString stringFromStdString:name]];
}

-(UIView*) getView;{
	return view;
}
@end