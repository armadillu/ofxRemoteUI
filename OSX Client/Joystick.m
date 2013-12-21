//
//  Joystick.m
//  JoystickHIDTest
//
//  Created by John Stringham on 12-05-01.
//  Copyright 2012 We Get Signal. All rights reserved.
//

#import "Joystick.h"
#import "JoystickHatswitch.h"

@implementation Joystick

@synthesize device;

- (id)initWithDevice:(IOHIDDeviceRef)theDevice
{
    self = [super init];
    if (self) {
        device = theDevice;
        
        delegates = [[NSMutableArray alloc] initWithCapacity:0];
        
        elements = (NSArray *)IOHIDDeviceCopyMatchingElements(theDevice, NULL, kIOHIDOptionsTypeNone);
        
        NSMutableArray *tempButtons = [NSMutableArray array];
        NSMutableArray *tempAxes = [NSMutableArray array];
        NSMutableArray *tempHats = [NSMutableArray array];
        
        int i;
        for (i=0; i<elements.count; ++i) {
            IOHIDElementRef thisElement = (IOHIDElementRef)[elements objectAtIndex:i];
            
            int elementType = IOHIDElementGetType(thisElement);
            int elementUsage = IOHIDElementGetUsage(thisElement);
            
            if (elementUsage == kHIDUsage_GD_Hatswitch) {
                JoystickHatswitch *hatSwitch = [[JoystickHatswitch alloc] initWithElement:thisElement andOwner:self];
                [tempHats addObject:hatSwitch];
                [hatSwitch release];
            } else if (elementType == kIOHIDElementTypeInput_Axis || elementType == kIOHIDElementTypeInput_Misc) {
                [tempAxes addObject:thisElement];
            } else {
                [tempButtons addObject:thisElement];
            }
        }
        buttons = [[NSArray arrayWithArray:tempButtons] retain];
        axes = [[NSArray arrayWithArray:tempAxes] retain];
        hats = [[NSArray arrayWithArray:tempHats] retain];
        
        //NSLog(@"New device address: %p from %p",device,theDevice);
        NSLog(@"Joystick: found %lu buttons, %lu axes and %lu hats",tempButtons.count,tempAxes.count,tempHats.count);
        // For more detailed info there are Usage tables
        // eg: kHIDUsage_GD_X
        // declared in IOHIDUsageTables.h
        // could use to determine major axes

		/*
         Get the vendor ID (which is a 32-bit integer)
		 */
		CFNumberRef vendor, product;
		CFStringRef manufacturer;
		CFStringRef productName_;
		char string_buffer[1024];

		if ((vendor = (CFNumberRef)IOHIDDeviceGetProperty(device, CFSTR(kIOHIDVendorIDKey))) != NULL)
			CFNumberGetValue(vendor, kCFNumberSInt32Type, &vendorID);
		NSLog(@"Joystick: vendorID: %04lX ", vendorID);

		/*
         Get the product ID (which is a 32-bit integer)
		 */
		if ((product = (CFNumberRef)IOHIDDeviceGetProperty(device, CFSTR(kIOHIDProductIDKey))) != NULL)
			CFNumberGetValue((CFNumberRef)product, kCFNumberSInt32Type, &productID);
		NSLog(@"Joystick: productID: %04lX\n", productID);

		/*
         Get the manufacturer name (which is a string)
		 */
		if ((manufacturer = (CFStringRef)IOHIDDeviceGetProperty(device, CFSTR(kIOHIDManufacturerKey)))!= NULL)
		{
			CFStringGetCString(manufacturer, string_buffer, sizeof(string_buffer), kCFStringEncodingUTF8);
			NSLog(@"Joystick: Manufacturer: %s\n", string_buffer);
			manufacturerName = [[NSString stringWithUTF8String:string_buffer] retain];
		}

		/*
         Get the product name (which is a string)
		 */
		if ((productName_ = (CFStringRef)IOHIDDeviceGetProperty(device, CFSTR(kIOHIDProductKey))) != NULL)
		{
			CFStringGetCString(productName_, string_buffer, sizeof(string_buffer), kCFStringEncodingUTF8);
			NSLog(@"Joystick: Product Name: %s", string_buffer);
			productName = [[NSString stringWithUTF8String:string_buffer] retain];
		}

    }

    return self;
}

- (void)elementReportedChange:(IOHIDElementRef)theElement {

    int elementType = IOHIDElementGetType(theElement);
    IOHIDValueRef pValue;
    IOHIDDeviceGetValue(device, theElement, &pValue);
    
    int elementUsage = IOHIDElementGetUsage(theElement);
    int value = IOHIDValueGetIntegerValue(pValue);
    int i;
    
    if (elementUsage == kHIDUsage_GD_Hatswitch) {
        
        // determine a unique offset. index is buttons.count
        // so all dpads will report buttons.count+(hats.indexOfObject(hatObject)*5)
        // 8 ways are interpreted as UP DOWN LEFT RIGHT so this is fine.
        int offset = (int)[buttons count];
        JoystickHatswitch *hatswitch;
        for (i=0; i<hats.count; ++i) {
            hatswitch = [hats objectAtIndex:i];
            
            if ([hatswitch element] == theElement) {
                offset += i*5;
                break;
            }
        }
        
        for (i=0; i<delegates.count; ++i) {
            id <JoystickNotificationDelegate> delegate = [delegates objectAtIndex:i];
            [hatswitch checkValue:value andDispatchButtonPressesWithIndexOffset:offset toDelegate:delegate];
        }
        
        return;
    }
    
    
    if (elementType != kIOHIDElementTypeInput_Axis && elementType != kIOHIDElementTypeInput_Misc) {
        
        
        for (i=0; i<delegates.count; ++i) {
            id <JoystickNotificationDelegate> delegate = [delegates objectAtIndex:i];
			[delegate joystickButton:[self getElementIndex:theElement] state:(BOOL)(value==1) onJoystick:self];
        }
        
        //NSLog(@"Non-axis reported value of %d",value);
        return;
    }

	if (elementType != kIOHIDElementTypeInput_Axis){

		int axisIndex = [self getElementIndex:theElement];
		for (i=0; i<delegates.count; ++i) {
			id <JoystickNotificationDelegate> delegate = [delegates objectAtIndex:i];
			[delegate joystickAxisChanged:self atAxisIndex: axisIndex];
		}
		return;
	}

    //NSLog(@"Axis reported value of %d",value);
    
//    for (i=0; i<delegates.count; ++i) {
//        id <JoystickNotificationDelegate> delegate = [delegates objectAtIndex:i];
//        
//        //[delegate joystickStateChanged:self];
//    }
}

- (void)registerForNotications:(id <JoystickNotificationDelegate>)delegate {
    [delegates addObject:delegate];
}

- (void)deregister:(id<JoystickNotificationDelegate>)delegate {
    [delegates removeObject:delegate];
}

- (int)getElementIndex:(IOHIDElementRef)theElement {
    int elementType = IOHIDElementGetType(theElement);
    
    NSArray *searchArray;
    NSString *returnString = @"";
    
    if (elementType == kIOHIDElementTypeInput_Button) {
        searchArray = buttons;
        returnString = @"Button";
    } else {
        searchArray = axes;
        returnString = @"Axis";
    }
    
    int i;
    
    for (i=0; i<searchArray.count; ++i) {
        if ([searchArray objectAtIndex:i] == theElement)
            return i;
            //  returnString = [NSString stringWithFormat:@"%@_%d",returnString,i];
    }
    
    return -1;
}

- (double)getRelativeValueOfAxesIndex:(int)index {
    IOHIDElementRef theElement = (IOHIDElementRef)[axes objectAtIndex:index];
    
    double value;
    double min = IOHIDElementGetLogicalMin(theElement);
    double max = IOHIDElementGetLogicalMax(theElement);

	//NSLog(@"%f %f", min, max);
    IOHIDValueRef pValue;
    IOHIDDeviceGetValue(device, theElement, &pValue);
    
    value = ((double)IOHIDValueGetIntegerValue(pValue)-min) * (1/(max-min));
    
    return value;
}

- (unsigned int)numButtons {
    return (unsigned int)[buttons count];
}

- (unsigned int)numAxes {
    return (unsigned int)[axes count];
}

- (unsigned int)numHats {
    return (unsigned int)[hats count];
}


- (long)vendorID{
	return vendorID;
}

- (long)productID{
	return productID;
}

- (NSString*)productName{
	return productName;
}

- (NSString*)manufacturerName{
	return manufacturerName;
}


- (void)dealloc
{
    [delegates release];
    
    [axes release];
    [hats release];
    [buttons release];
    
    [elements release];
    
    [super dealloc];
}

@end
