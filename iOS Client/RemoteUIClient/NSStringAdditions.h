//
//  NSStringAdditions.h
//  RemoteUIClient
//
//  Created by Oriol Ferrer Mesià on 11/05/14.
//  Copyright (c) 2014 Oriol Ferrer Mesià. All rights reserved.
//

#import <Foundation/Foundation.h>
#include <iostream>

@interface NSString (StdStringAdditions)

+(NSString*) stringFromStdString:(std::string)s;

@end
