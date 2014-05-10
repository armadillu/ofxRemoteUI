//
//  Joystick.h
//  JoystickHIDTest
//
//  Created by John Stringham on 12-05-01.
//  Copyright 2012 We Get Signal. All rights reserved.
//

#import <Foundation/Foundation.h>
#import <IOKit/hid/IOHIDLib.h>
#import "JoystickNotificationDelegate.h"

@interface Joystick : NSObject {
    IOHIDDeviceRef  device;

    
@private

    NSArray  *elements;
    
    NSArray *axes;
    NSArray *buttons;
    NSArray *hats;
    
    NSMutableArray *delegates;
	long vendorID;
	long productID;
	NSString * productName;
	NSString * manufacturerName;
}

@property(readwrite) IOHIDDeviceRef device;

@property(readonly) unsigned int numButtons;
@property(readonly) unsigned int numAxes;
@property(readonly) unsigned int numHats;

- (id)initWithDevice:(IOHIDDeviceRef)theDevice;
- (int)getElementIndex:(IOHIDElementRef)theElement;

- (double)getRelativeValueOfAxesIndex:(int)index;

- (void)elementReportedChange:(IOHIDElementRef)theElement;
- (void)registerForNotications:(id <JoystickNotificationDelegate>)delegate;
- (void)deregister:(id<JoystickNotificationDelegate>)delegate;

- (long)vendorID;
- (long)productID;
- (NSString*)productName;
- (NSString*)manufacturerName;

@end
