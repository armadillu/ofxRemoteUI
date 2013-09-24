//
//  NSColorStringExtension.h
//  ofxRemoteUIClientOSX
//
//  Created by Oriol Ferrer Mesià on 24/08/13.
//
//

#import <Foundation/Foundation.h>

//from http://stackoverflow.com/questions/10956777/store-a-nscolor-as-a-string

@interface NSColor (NSString)
- (NSString*)stringRepresentation;
+ (NSColor*)colorFromString:(NSString*)string forColorSpace:(NSColorSpace*)colorSpace;
@end



