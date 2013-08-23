//
//  ColoredNSWindow.m
//  ofxRemoteUIClientOSX
//
//  Created by Oriol Ferrer Mesià on 15/08/13.
//
//

#import "ColoredNSWindow.h"
#import <objc/runtime.h>

@interface ColoredNSWindow(ShutUpXcode)
- (float)roundedCornerRadius;
- (void)drawRectOriginal:(NSRect)rect;
- (NSWindow*)window;
@end


// mostly from http://parmanoir.com/Custom_NSThemeFrame

NSColor * myColor;

@implementation ColoredNSWindow


-(void)awakeFromNib{

	//NSLog(@"awake!");

	// Get window's frame view class
	id class = [[[self contentView] superview] class];
	//NSLog(@"class=%@", class);


	// Exchange draw rect
	Method m0 = class_getInstanceMethod([self class], @selector(drawRect:));
	class_addMethod(class, @selector(drawRectOriginal:), method_getImplementation(m0), method_getTypeEncoding(m0));

	Method m1 = class_getInstanceMethod(class, @selector(drawRect:));
	Method m2 = class_getInstanceMethod(class, @selector(drawRectOriginal:));

	method_exchangeImplementations(m1, m2);

	//choose a random color
	struct timeval tv;
	gettimeofday(&tv, 0);
	long int n = (tv.tv_sec ^ tv.tv_usec) ^ getpid();
	srand(n);

	float r = rand()%1000 / 1000.;
	float g = rand()%1000 / 1000.;
	float b = rand()%1000 / 1000.;
	myColor = [[NSColor colorWithCalibratedRed:r green:g blue:b alpha:0.500] retain];
}

- (void)drawRect:(NSRect)rect{

	// Call original drawing method
	[self drawRectOriginal:rect];


	//
	// Draw a background color on top of everything
	//
	CGContextRef context = [[NSGraphicsContext currentContext] graphicsPort];
	CGContextSetBlendMode(context, kCGBlendModeColor);

	[myColor set];
	[[NSBezierPath bezierPathWithRect:rect] fill];
}
@end



