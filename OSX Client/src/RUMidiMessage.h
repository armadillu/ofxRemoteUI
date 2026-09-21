#import <CoreMIDI/CoreMIDI.h>
#import <Foundation/Foundation.h>

typedef NS_ENUM(NSInteger, RUMidiMessageType) {
	RUMidiMessageUnknown = 0,
	RUMidiMessageNoteOff = 0x80,
	RUMidiMessageNoteOn = 0x90,
	RUMidiMessagePolyPressure = 0xA0,
	RUMidiMessageControlChange = 0xB0,
	RUMidiMessageProgramChange = 0xC0,
	RUMidiMessageChannelPressure = 0xD0,
	RUMidiMessagePitchBend = 0xE0,
	RUMidiMessageSystem = 0xF0
};

@interface RUMidiMessage : NSObject {
	RUMidiMessageType _type;
	UInt8 _channel;
	UInt8 _data1;
	UInt8 _data2;
	NSData *_data;
}

@property(nonatomic, readonly) RUMidiMessageType type;
@property(nonatomic, readonly) UInt8 channel;
@property(nonatomic, readonly) UInt8 data1;
@property(nonatomic, readonly) UInt8 data2;
@property(nonatomic, readonly) NSData *data;
@property(nonatomic, readonly) BOOL isSysEx;

+ (instancetype)messageWithBytes:(const UInt8 *)bytes length:(NSUInteger)length;
+ (instancetype)messageWithBytes:(const UInt8 *)bytes
						  length:(NSUInteger)length
						 channel:(UInt8)channel;
+ (instancetype)messageWithType:(RUMidiMessageType)type
						channel:(UInt8)channel
						  data1:(UInt8)data1
						  data2:(UInt8)data2;
- (double)doubleValue;
- (NSArray *)sysexArray;

@end
