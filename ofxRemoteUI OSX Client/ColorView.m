//
//  ColorView.m
//  ofxRemoteUIClientOSX
//
//  Created by Oriol Ferrer Mesià on 17/05/13.
//
//

#import "ColorView.h"

@implementation ColorView



-(void)awakeFromNib{

	background = [[NSColor clearColor] retain];
}


-(BOOL)isOpaque{
	return NO;
}


-(void)setBackgroundColor:(NSColor *)aColor{

    if([background isEqual:aColor]) return;
    [background release];
    background = [aColor retain];

    [self setNeedsDisplay:YES];
}

- (void)drawRect:(NSRect)dirtyRect{
//	[background set];
//    NSRectFill([self bounds]);
	[background set];
    NSRectFillUsingOperation(dirtyRect, NSCompositeSourceOver);
}

@end
