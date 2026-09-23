#import "RUMidiMessage.h"
#import <CoreMIDI/CoreMIDI.h>
#import <Foundation/Foundation.h>

@class RUMidi, RUMidiDevice;

@protocol RUMidiDelegate <NSObject>
@optional
- (void)midi:(RUMidi *)midi didReceiveMessage:(RUMidiMessage *)message fromDevice:(RUMidiDevice *)device;
- (void)midiSetupChanged:(RUMidi *)midi;
@end

//////////////////////////////////////////////////////////////////////////////////////////////////

@interface RUMidi : NSObject {
	MIDIClientRef _client;
	MIDIPortRef _inputPort;
	MIDIPortRef _outputPort;
	NSMutableArray *_devices;
	NSMutableDictionary *_devicesByEndpoint;
	NSMutableDictionary *_sysexBuffers;
	id<RUMidiDelegate> _delegate;
}

- (BOOL)sendBytes:(const UInt8 *)bytes length:(NSUInteger)length toEndpoint:(MIDIEndpointRef)endpoint;
- (BOOL)sendSysExBytes:(const UInt8 *)bytes length:(NSUInteger)length toEndpoint:(MIDIEndpointRef)endpoint;

- (void)connectAllSources;
- (void)connectSource:(MIDIEndpointRef)source;

- (void)handlePacketList:(const MIDIPacketList *)list fromSource:(MIDIEndpointRef)source;
- (void)handleSysExBytes:(const UInt8 *)bytes length:(NSUInteger)length source:(MIDIEndpointRef)source;
- (RUMidiDevice *)deviceForSource:(MIDIEndpointRef)source;

- (void)rebuildDevices;


@property(nonatomic, assign) id<RUMidiDelegate> delegate;
- (BOOL)start;
- (void)stop;
- (NSArray *)devices;
- (RUMidiDevice *)deviceNamed:(NSString *)name;
@end

//////////////////////////////////////////////////////////////////////////////////////////////////

@interface RUMidiDevice : NSObject {
	NSString *_name;
	MIDIEndpointRef _endpoint;
	RUMidi *_midi;
}
@property(nonatomic, readonly) NSString *name;
@property(nonatomic, readonly) MIDIEndpointRef endpoint;

- (BOOL)sendMessage:(RUMidiMessage *)message;
- (BOOL)sendBytes:(const UInt8 *)bytes length:(NSUInteger)length;
- (BOOL)sendChannel:(UInt8)channel
			 status:(RUMidiMessageType)status
			  data1:(UInt8)data1
			  data2:(UInt8)data2;
- (BOOL)sendChannel:(UInt8)channel
			 status:(RUMidiMessageType)status
			  data1:(UInt8)data1;
@end

