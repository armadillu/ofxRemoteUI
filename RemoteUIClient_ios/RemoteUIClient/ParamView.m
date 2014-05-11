
#import "ParamView.h"



@interface ParamViewContainer : NSObject {

}

@property (nonatomic, retain) IBOutlet ParamView *progressView;

@end

@implementation ParamViewContainer

@synthesize progressView = progressView;

- (void) dealloc {
	//[progressView release];
    //[super dealloc];
}

@end

///////////////////////////////////////////////////////////////////////////////

@implementation ParamView

+ (ParamView *) newParamView {
    ParamViewContainer *container = [[ParamViewContainer alloc] init];
    [[NSBundle mainBundle] loadNibNamed:@"ParamView" owner:container options:nil];

    ParamView *progressView = container.progressView;
	//[container release];
    return progressView;
}

@end