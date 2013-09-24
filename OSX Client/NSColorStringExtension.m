//
//  NSColorStringExtension.m
//  ofxRemoteUIClientOSX
//
//  Created by Oriol Ferrer Mesià on 24/08/13.
//
//

#import "NSColorStringExtension.h"

//from http://stackoverflow.com/questions/10956777/store-a-nscolor-as-a-string

@implementation NSColor (NSString)

- (NSString*)stringRepresentation{
    CGFloat components[10];

    [self getComponents:components];
    NSMutableString *string = [NSMutableString string];
    for (int i = 0; i < [self numberOfComponents]; i++) {
        [string appendFormat:@"%f ", components[i]];
    }
    [string deleteCharactersInRange:NSMakeRange([string length]-1, 1)]; // trim the trailing space
    return string;
}

+ (NSColor*)colorFromString:(NSString*)string forColorSpace:(NSColorSpace*)colorSpace
{
    CGFloat components[10];    // doubt any color spaces need more than 10 components
    NSArray *componentStrings = [string componentsSeparatedByString:@" "];
    int count = (int)[componentStrings count];
    NSColor *color = nil;
    if (count <= 10) {
        for (int i = 0; i < count; i++) {
            components[i] = [[componentStrings objectAtIndex:i] floatValue];
        }
        color = [NSColor colorWithColorSpace:colorSpace components:components count:count];
    }
    return color;
}

@end