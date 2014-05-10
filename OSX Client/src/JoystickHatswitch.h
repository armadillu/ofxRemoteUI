//
//  JoystickHatswitch.h
//  JoystickHIDTest
//
//  Created by John Stringham on 12-05-04.
//  Copyright (c) 2012 We Get Signal. All rights reserved.
//

#import <Foundation/Foundation.h>
#import <IOKit/hid/IOHIDLib.h>
#import "JoystickNotificationDelegate.h"

@interface JoystickHatswitch : NSObject  {
    IOHIDElementRef element;
    Joystick        *owner;
    
    int directions;
    BOOL buttonStates[4];
}

@property(readonly) IOHIDElementRef element;

-(id)initWithElement:(IOHIDElementRef)theElement andOwner:(Joystick *)theOwner;
- (void)checkValue:(int)value andDispatchButtonPressesWithIndexOffset:(int)offset toDelegate:(id<JoystickNotificationDelegate>)delegate;

@end
