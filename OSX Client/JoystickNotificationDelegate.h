//
//  JoystickNotificationDelegate.h
//  JoystickHIDTest
//
//  Created by John Stringham on 12-05-01.
//  Copyright 2012 We Get Signal. All rights reserved.
//

#import <Foundation/Foundation.h>

@class Joystick;

@protocol JoystickNotificationDelegate

- (void)joystickAxisChanged:(Joystick *)joystick atAxisIndex:(int)axis;
- (void)joystickButton:(int)buttonIndex state:(BOOL)pressed onJoystick:(Joystick*)joystick;


@end
