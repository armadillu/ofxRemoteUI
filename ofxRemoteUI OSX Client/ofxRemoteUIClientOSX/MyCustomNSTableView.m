//
//  MyCustomNSTableView.m
//  ofxRemoteUIClientOSX
//
//  Created by Oriol Ferrer Mesià on 11/01/13.
//
//

#import "MyCustomNSTableView.h"

@implementation MyCustomNSTableView

//this is to allow focus on the textfield when inside a table

- (void)mouseDown:(NSEvent *)theEvent {
	[super mouseDown:theEvent];
	NSPoint point = [self convertPoint:theEvent.locationInWindow fromView:nil];
	NSView *theView = [self hitTest:point];
	if ([theView isKindOfClass:[NSTextField class]]) {
		//NSLog(@"%@",[(NSTextField *)theView stringValue]);
		NSTextField * field = (NSTextField *)theView;
		//[[field window] setInitialFirstResponder: [textViewContainer textView]];
		[[field window] makeFirstResponder: field];
	}
}
@end
