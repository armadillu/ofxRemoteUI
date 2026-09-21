#import "RUMidiMessage.h"

@implementation RUMidiMessage

@synthesize type = _type, channel = _channel, data1 = _data1, data2 = _data2, data = _data;

- (id)init {
	if ((self = [super init])) {
		_type = RUMidiMessageUnknown;
	}
	return self;
}

- (void)dealloc {
	[_data release];
	[super dealloc];
}

+ (instancetype)messageWithBytes:(const UInt8 *)bytes
						  length:(NSUInteger)length {
	if (!bytes || !length)
		return nil;
	RUMidiMessage *m = [[[self alloc] init] autorelease];
	m->_data = [[NSData alloc] initWithBytes:bytes length:length];
	UInt8 s = bytes[0];
	m->_type = ((s & 0xF0) == 0xF0) ? RUMidiMessageSystem
	: (RUMidiMessageType)(s & 0xF0);
	m->_channel = ((s & 0xF0) == 0xF0) ? 0 : (s & 0x0F);
	m->_data1 = length > 1 ? bytes[1] : 0;
	m->_data2 = length > 2 ? bytes[2] : 0;
	return m;
}

+ (instancetype)messageWithBytes:(const UInt8 *)bytes
						  length:(NSUInteger)length
						 channel:(UInt8)channel {
	if (!bytes || !length)
		return nil;
	NSMutableData *d = [NSMutableData dataWithBytes:bytes length:length];
	UInt8 *b = d.mutableBytes;
	if ((b[0] & 0xF0) != 0xF0)
		b[0] = (b[0] & 0xF0) | (channel & 0x0F);
	return [self messageWithBytes:d.bytes length:d.length];
}

+ (instancetype)messageWithType:(RUMidiMessageType)type
						channel:(UInt8)channel
						  data1:(UInt8)data1
						  data2:(UInt8)data2 {
	if (type < 0x80 || type > 0xE0)
		return nil;
	UInt8 b[3];
	b[0] = (type & 0xF0) | (channel & 0x0F);
	b[1] = data1;
	NSUInteger n = 3;
	if (type == RUMidiMessageProgramChange ||
		type == RUMidiMessageChannelPressure)
		n = 2;
	else
		b[2] = data2;
	return [self messageWithBytes:b length:n];
}

- (BOOL)isSysEx {
	return _type == RUMidiMessageSystem && _data.length &&
	((const UInt8 *)_data.bytes)[0] == 0xF0;
}

- (NSArray *)sysexArray {
	if (!self.isSysEx)
		return nil;
	NSMutableArray *a = [NSMutableArray arrayWithCapacity:_data.length];
	const UInt8 *b = _data.bytes;
	for (NSUInteger i = 0; i < _data.length; i++)
		[a addObject:[NSNumber numberWithUnsignedChar:b[i]]];
	return a;
}

- (double)doubleValue {
	switch (_type) {
		case RUMidiMessageNoteOff:
		case RUMidiMessageNoteOn:
		case RUMidiMessagePolyPressure:
		case RUMidiMessageControlChange:
			return _data2 / 127.0;
		case RUMidiMessageProgramChange:
		case RUMidiMessageChannelPressure:
			return _data1 / 127.0;
		case RUMidiMessagePitchBend:
			return ((double)((UInt16)_data1 | ((UInt16)_data2 << 7))) / 16383.0;
		default:
			return 0.0;
	}
}

@end
