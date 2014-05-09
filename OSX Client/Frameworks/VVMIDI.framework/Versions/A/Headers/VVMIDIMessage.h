
#import <Cocoa/Cocoa.h>
#import <CoreMIDI/CoreMIDI.h>



@interface VVMIDIMessage : NSObject <NSCopying> {
	Byte			type;
	Byte			channel;
	Byte			data1;		//	usually controller/note #
	Byte			data2;		//	usually controller/note value.  if 14-bit, the MSB ("coarse")
	Byte			data3;		//	usually -1.  if 14-bit, the LSB ("fine").
	//	array of NSNumbers, or nil if this isn't a sysex message
	//	DOES NOT CONTAIN SYSEX START/STOP STATUS BYTES (0xF0 / 0xF7)
	NSMutableArray	*sysexArray;
}

+ (id) createWithType:(Byte)t channel:(Byte)c;
+ (id) createFromVals:(Byte)t :(Byte)c :(Byte)d1 :(Byte)d2;
+ (id) createFromVals:(Byte)t :(Byte)c :(Byte)d1 :(Byte)d2 :(Byte)d3;
+ (id) createWithSysexArray:(NSMutableArray *)s;
- (id) initWithType:(Byte)t channel:(Byte)c;
- (id) initFromVals:(Byte)t :(Byte)c :(Byte)d1 :(Byte)d2;
- (id) initFromVals:(Byte)t :(Byte)c :(Byte)d1 :(Byte)d2 :(Byte)d3;
- (id) initWithSysexArray:(NSMutableArray *)s;

- (NSString *) description;
- (NSString *) lengthyDescription;

- (Byte) type;
- (void) setType:(Byte)newType;
- (Byte) channel;
- (void) setData1:(Byte)newData;
- (Byte) data1;
- (void) setData2:(Byte)newData;
- (Byte) data2;
- (void) setData3:(Byte)newData;
- (Byte) data3;
- (NSMutableArray *) sysexArray;
- (double) doubleValue;

@end
