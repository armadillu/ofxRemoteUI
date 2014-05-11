//
//  NSStringAdditions.m
//  RemoteUIClient
//
//  Created by Oriol Ferrer Mesià on 11/05/14.
//  Copyright (c) 2014 Oriol Ferrer Mesià. All rights reserved.
//

#import "NSStringAdditions.h"

@implementation NSString (StdStringAdditions)

+(NSString*) stringFromStdString:(std::string)s{
	return [NSString stringWithUTF8String:s.c_str()];
}


@end
