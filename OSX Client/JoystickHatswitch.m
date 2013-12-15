//
//  JoystickHatswitch.m
//  JoystickHIDTest
//
//  Created by John Stringham on 12-05-04.
//  Copyright (c) 2012 We Get Signal. All rights reserved.
//

#import "JoystickHatswitch.h"

@implementation JoystickHatswitch

@synthesize element;

-(id)initWithElement:(IOHIDElementRef)theElement andOwner:(Joystick *)theOwner {
    
    self = [super init];
    
    if (!self)
        return nil;
    
    int usage = IOHIDElementGetUsage(theElement);
    
    if (usage != kHIDUsage_GD_Hatswitch)
        NSLog(@"WARN: Invalid element given to JoystickHatswitch object");
    
    element = theElement;
    owner = theOwner;
    
    int logicalMax = (int)IOHIDElementGetLogicalMax(theElement);
    int logicalMin = (int)IOHIDElementGetLogicalMin(theElement);
    
    directions = logicalMax-logicalMin;
    
    NSLog(@"HatSwitch found. lMax: %d , lMin: %d",logicalMax,logicalMin);
    
    return self;
}

- (void)checkValue:(int)value andDispatchButtonPressesWithIndexOffset:(int)offset toDelegate:(id<JoystickNotificationDelegate>)delegate {
    
    BOOL newButtonStates[4] = {NO,NO,NO,NO};
    // UP, RIGHT, DOWN, LEFT.
    
    if (directions > 4) {
        newButtonStates[0] = (value == 0 || value == 1 || value == 7);
        newButtonStates[1] = (value == 1 || value == 2 || value == 3);
        newButtonStates[2] = (value == 3 || value == 4 || value == 5);
        newButtonStates[3] = (value == 5 || value == 6 || value == 7);
    } else {
        newButtonStates[0] = (value == 0);
        newButtonStates[1] = (value == 1);
        newButtonStates[2] = (value == 2);
        newButtonStates[3] = (value == 3);
    }
    
    int i;
    
    for (i=0; i<4; ++i) {
        if (newButtonStates[i] != buttonStates[i]) {
            // dispatch a button change event
            if (newButtonStates[i])
                [delegate joystickButtonPushed:offset+i onJoystick:owner];
            else
                [delegate joystickButtonReleased:offset+i onJoystick:owner];
        }
    
        buttonStates[i] = newButtonStates[i];
    }
}

@end
