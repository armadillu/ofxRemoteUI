//
//  ColorView.h
//  ofxRemoteUIClientOSX
//
//  Created by Oriol Ferrer Mesià on 17/05/13.
//
//

#import <Cocoa/Cocoa.h>

@interface ColorView : NSView{

	NSColor *background;
}

-(void)setBackgroundColor:(NSColor *)aColor;

@end
