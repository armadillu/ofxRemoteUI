#import "RUMidi.h"


typedef struct {
	UInt8 *bytes;
} RUMidiSysExSendContext;

static void RUMidiSysExCompleteProc(MIDISysexSendRequest *r) {
	if (!r)
		return;
	RUMidiSysExSendContext *c = r->completionRefCon;
	if (c) {
		free(c->bytes);
		free(c);
	}
	free(r);
}


static void RUMidiNotifyProc(const MIDINotification *m, void *ref) {
	if (!m || !ref)
		return;
	RUMidi *x = (RUMidi *)ref;
	switch (m->messageID) {
	case kMIDIMsgSetupChanged:
		[x rebuildDevices];
		[x connectAllSources];
		break;
	case kMIDIMsgObjectAdded: {
		const MIDIObjectAddRemoveNotification *n =
				(const MIDIObjectAddRemoveNotification *)m;
		if (n->childType == kMIDIObjectType_Source)
			[x connectSource:(MIDIEndpointRef)n->child];
		else
			[x rebuildDevices];
		break;
	}
	case kMIDIMsgObjectRemoved:
		[x rebuildDevices];
		break;
	default:
		break;
	}
	id<RUMidiDelegate> d = x.delegate;
	if (d && [d respondsToSelector:@selector(midiSetupChanged:)])
		[d midiSetupChanged:x];
}


static void RUMidiReadProc(const MIDIPacketList *l, void *ref, void *ctx) {
	if (!ref || !l)
		return;
	[(RUMidi *)ref handlePacketList:l fromSource:(MIDIEndpointRef)(uintptr_t)ctx];
}

#pragma mark -
//////////////////////////////////////////////////////////////////////////////////////////////////

@implementation RUMidi
@synthesize delegate = _delegate;

- (id)init {
	if ((self = [super init])) {
		_devices = [[NSMutableArray alloc] init];
		_devicesByEndpoint = [[NSMutableDictionary alloc] init];
		_sysexBuffers = [[NSMutableDictionary alloc] init];
		if (![self start]) {
			[self release];
			return nil;
		}
	}
	return self;
}

- (void)dealloc {
	[self stop];
	[_devices release];
	[_devicesByEndpoint release];
	[_sysexBuffers release];
	[super dealloc];
}

- (BOOL)start {
	if (_client)
		return YES;
	OSStatus s =
			MIDIClientCreate(CFSTR("RUMidi"), RUMidiNotifyProc, self, &_client);
	if (s != noErr)
		return NO;
	s = MIDIInputPortCreate(_client, CFSTR("RUMidi Input"), RUMidiReadProc, self,
													&_inputPort);
	if (s != noErr) {
		[self stop];
		return NO;
	}
	s = MIDIOutputPortCreate(_client, CFSTR("RUMidi Output"), &_outputPort);
	if (s != noErr) {
		[self stop];
		return NO;
	}
	[self rebuildDevices];
	[self connectAllSources];
	return YES;
}

- (void)stop {
	if (_inputPort) {
		MIDIPortDispose(_inputPort);
		_inputPort = 0;
	}
	if (_outputPort) {
		MIDIPortDispose(_outputPort);
		_outputPort = 0;
	}
	if (_client) {
		MIDIClientDispose(_client);
		_client = 0;
	}
	[_devices removeAllObjects];
	[_devicesByEndpoint removeAllObjects];
	[_sysexBuffers removeAllObjects];
}

- (NSArray *)devices {
	return [[_devices copy] autorelease];
}

- (RUMidiDevice *)deviceNamed:(NSString *)name {
	for (RUMidiDevice *d in _devices)
		if ([d.name isEqualToString:name])
			return d;
	return nil;
}

- (void)rebuildDevices {

	[_devices removeAllObjects];
	[_devicesByEndpoint removeAllObjects];
	ItemCount n = MIDIGetNumberOfDestinations();

	for (ItemCount i = 0; i < n; i++) {
		MIDIEndpointRef e = MIDIGetDestination(i);
		if (!e)
			continue;
		CFStringRef r = NULL;
		NSString *name = nil;
		if (MIDIObjectGetStringProperty(e, kMIDIPropertyDisplayName, &r) == noErr && r) {
			name = [NSString stringWithString:(NSString *)r];
			CFRelease(r);
		}
		if (!name)
			name = @"Unknown MIDI Device";
		RUMidiDevice *d =
				[[[RUMidiDevice alloc] initWithName:name endpoint:e midi:self]
						autorelease];
		[_devices addObject:d];
		[_devicesByEndpoint
				setObject:d
				forKey:[NSNumber numberWithUnsignedLong:(unsigned long)e]];
	}
}

- (RUMidiDevice *)deviceForSource:(MIDIEndpointRef)e {
	RUMidiDevice *d = [_devicesByEndpoint
			objectForKey:[NSNumber numberWithUnsignedLong:(unsigned long)e]];
	if (d)
		return d;
	CFStringRef r = NULL;
	NSString *n = nil;
	if (MIDIObjectGetStringProperty(e, kMIDIPropertyDisplayName, &r) == noErr &&
			r) {
		n = [NSString stringWithString:(NSString *)r];
		CFRelease(r);
	}
	if (!n)
		n = @"Unknown MIDI Device";
	return
			[[[RUMidiDevice alloc] initWithName:n endpoint:e midi:self] autorelease];
}

- (void)connectAllSources {
	for (ItemCount i = 0, n = MIDIGetNumberOfSources(); i < n; i++) {
		MIDIEndpointRef s = MIDIGetSource(i);
		if (s)
			[self connectSource:s];
	}
}

- (void)connectSource:(MIDIEndpointRef)s {
	if (!s || !_inputPort)
		return;
	OSStatus st = MIDIPortConnectSource(_inputPort, s, (void *)(uintptr_t)s);
	if (st != noErr)
		NSLog(@"RUMidi: couldn't connect MIDI source %u (%d)", (unsigned)s,
					(int)st);
}

- (BOOL)sendBytes:(const UInt8 *)b
					 length:(NSUInteger)n
			 toEndpoint:(MIDIEndpointRef)e {
	if (!b || !n || !e)
		return NO;
	if (b[0] == 0xF0)
		return [self sendSysExBytes:b length:n toEndpoint:e];
	Byte buf[sizeof(MIDIPacketList) + 64];
	MIDIPacketList *l = (MIDIPacketList *)buf;
	MIDIPacket *p = MIDIPacketListInit(l);
	p = MIDIPacketListAdd(l, sizeof(buf), p, 0, (ByteCount)n, b);
	if (!p)
		return NO;
	return MIDISend(_outputPort, e, l) == noErr;
}

- (BOOL)sendSysExBytes:(const UInt8 *)b
								length:(NSUInteger)n
						toEndpoint:(MIDIEndpointRef)e {
	if (!b || !n || !e)
		return NO;
	RUMidiSysExSendContext *c = calloc(1, sizeof(*c));
	if (!c)
		return NO;
	c->bytes = malloc(n);
	if (!c->bytes) {
		free(c);
		return NO;
	}
	memcpy(c->bytes, b, n);
	MIDISysexSendRequest *r = calloc(1, sizeof(*r));
	if (!r) {
		free(c->bytes);
		free(c);
		return NO;
	}
	r->destination = e;
	r->data = c->bytes;
	r->bytesToSend = (UInt32)n;
	r->complete = false;
	r->completionProc = RUMidiSysExCompleteProc;
	r->completionRefCon = c;
	OSStatus s = MIDISendSysex(r);
	if (s != noErr) {
		free(c->bytes);
		free(c);
		free(r);
		return NO;
	}
	return YES;
}

- (void)handlePacketList:(const MIDIPacketList *)list
							fromSource:(MIDIEndpointRef)s {
	const MIDIPacket *p = &list->packet[0];
	for (UInt32 i = 0; i < list->numPackets; i++) {
		if (p->length && p->data[0] == 0xF0)
			[self handleSysExBytes:p->data length:p->length source:s];
		else {
			RUMidiMessage *m =
					[RUMidiMessage messageWithBytes:p->data length:p->length];
			id<RUMidiDelegate> d = _delegate;
			if (d &&
					[d respondsToSelector:@selector(midi:didReceiveMessage:fromDevice:)])
				[d midi:self didReceiveMessage:m fromDevice:[self deviceForSource:s]];
		}
		p = MIDIPacketNext(p);
	}
}

- (void)handleSysExBytes:(const UInt8 *)b
									length:(NSUInteger)n
									source:(MIDIEndpointRef)s {
	NSNumber *k = [NSNumber numberWithUnsignedLong:(unsigned long)s];
	NSMutableData *buf = [_sysexBuffers objectForKey:k];
	NSUInteger off = 0;
	while (off < n) {
		const UInt8 *c = b + off;
		NSUInteger rem = n - off;
		if (!buf.length) {
			if (c[0] != 0xF0) {
				off++;
				continue;
			}
			buf = [NSMutableData data];
			[_sysexBuffers setObject:buf forKey:k];
		}
		const UInt8 *end = memchr(c, 0xF7, rem);
		if (end) {
			NSUInteger len = (NSUInteger)(end - c) + 1;
			[buf appendBytes:c length:len];
			RUMidiMessage *m =
					[RUMidiMessage messageWithBytes:buf.bytes length:buf.length];
			id<RUMidiDelegate> d = _delegate;
			if (d &&
					[d respondsToSelector:@selector(midi:didReceiveMessage:fromDevice:)])
				[d midi:self didReceiveMessage:m fromDevice:[self deviceForSource:s]];
			[_sysexBuffers removeObjectForKey:k];
			buf = nil;
			off += len;
		} else {
			[buf appendBytes:c length:rem];
			break;
		}
	}
}

@end

#pragma mark -
//////////////////////////////////////////////////////////////////////////////////////////////////

@implementation RUMidiDevice
@synthesize name = _name, endpoint = _endpoint;

- (id)initWithName:(NSString *)name endpoint:(MIDIEndpointRef)e midi:(RUMidi *)m {
	if ((self = [super init])) {
		_name = [name copy];
		_endpoint = e;
		_midi = m;
	}
	return self;
}

- (void)dealloc {
	[_name release];
	[super dealloc];
}

- (BOOL)sendMessage:(RUMidiMessage *)m {
	if (!m || !_midi || !_endpoint)
		return NO;
	NSData *d = m.data;
	return [_midi sendBytes:d.bytes length:d.length toEndpoint:_endpoint];
}

- (BOOL)sendBytes:(const UInt8 *)b length:(NSUInteger)n {
	return _midi && _endpoint ? [_midi sendBytes:b length:n toEndpoint:_endpoint]
														: NO;
}

- (BOOL)sendChannel:(UInt8)c
						 status:(RUMidiMessageType)t
							data1:(UInt8)d1
							data2:(UInt8)d2 {
	return [self sendMessage:[RUMidiMessage messageWithType:t channel:c data1:d1 data2:d2]];
}

- (BOOL)sendChannel:(UInt8)c status:(RUMidiMessageType)t data1:(UInt8)d1 {
	return [self sendChannel:c status:t data1:d1 data2:0];
}

@end
