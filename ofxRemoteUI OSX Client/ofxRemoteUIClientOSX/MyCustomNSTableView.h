//
//  MyCustomNSTableView.h
//  ofxRemoteUIClientOSX
//
//  Created by Oriol Ferrer Mesià on 11/01/13.
//
//

#import <Foundation/Foundation.h>

@interface MyCustomNSTableView : NSTableView

- (void)mouseDown:(NSEvent *)theEvent ;

@end


@interface NSColor (ColorChangingFun)
	+(NSColor*)_blueAlternatingRowColor;
@end