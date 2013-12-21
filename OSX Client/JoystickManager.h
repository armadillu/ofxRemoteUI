//
//  JoystickManager.h
//  JoystickHIDTest
//
//  Created by John Stringham on 12-05-01.
//  Copyright 2012 We Get Signal. All rights reserved.
//

#import <Foundation/Foundation.h>
#import <IOKit/hid/IOHIDLib.h>
#import "Joystick.h"

void gamepadWasRemoved(void* inContext, IOReturn inResult, void* inSender, IOHIDDeviceRef device);
void gamepadWasAdded(void* inContext, IOReturn inResult, void* inSender, IOHIDDeviceRef device);
void gamepadAction(void* inContext, IOReturn inResult, void* inSender, IOHIDValueRef value);

@interface JoystickManager : NSObject {
    id      joystickAddedDelegate;
    
@private
    IOHIDManagerRef hidManager;
    
    NSMutableDictionary  *joysticks;
    
    int                 joystickIDIndex;
}

@property(assign) id joystickAddedDelegate;

+ (JoystickManager *)sharedInstance;
- (unsigned long)connectedJoysticks;
- (void)registerNewJoystick:(Joystick *)joystick;

- (int)deviceIDByReference:(IOHIDDeviceRef)deviceRef;
- (Joystick *)joystickByID:(int)joystickID;

@end
