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

@implementation ColoredNSWindow


NSColor * myWinColor = nil;
NSMutableArray * coloredWindows = nil;


-(void)setColor:(NSColor*)c;{
	[myWinColor release];
	myWinColor = [c retain];
	[self display];
}


-(void)awakeFromNib{

	//store all NSThemeFrame containers for NSWindows who want this (are an instance of ColoredNSWindow)
	if ([self class] == [ColoredNSWindow class]){
		if (coloredWindows == nil) coloredWindows = [[NSMutableArray alloc] init];
		[coloredWindows addObject:[[self contentView] superview]];
	}

	// Get window's frame view class
	id class = [[[self contentView] superview] class];


	// Exchange draw rect
	Method m0 = class_getInstanceMethod([self class], @selector(drawRect:));
	class_addMethod(class, @selector(drawRectOriginal:), method_getImplementation(m0), method_getTypeEncoding(m0));

	Method m1 = class_getInstanceMethod(class, @selector(drawRect:));
	Method m2 = class_getInstanceMethod(class, @selector(drawRectOriginal:));

	method_exchangeImplementations(m1, m2);

	//NSLog(@"awake %@", self);
	if(myWinColor == nil) myWinColor = [[NSColor orangeColor] retain];
}


- (void)drawRect:(NSRect)rect{

	[self drawRectOriginal:rect];

	//if im one of the selected ones, draw custom
	if ( [coloredWindows containsObject:self] ){

		CGContextRef context = [[NSGraphicsContext currentContext] graphicsPort];
		//kCGBlendModeMultiply
		if (floor(NSAppKitVersionNumber) <= 1265) {
			CGContextSetBlendMode(context, kCGBlendModeOverlay);
		}else{
			CGContextSetBlendMode(context, kCGBlendModeNormal);
		}

		[myWinColor set];
		float cornerRadius = 4.5;
		NSBezierPath *path;
		if(rect.size.width >= self.frame.size.width){ //sometimes we get drawn in smaller subregion rects
			path = [NSBezierPath bezierPathWithRoundedRect:rect xRadius:cornerRadius yRadius:cornerRadius];
		}else{
			path = [NSBezierPath bezierPathWithRect:rect];
		}
		[path fill];
	}
}
@end



