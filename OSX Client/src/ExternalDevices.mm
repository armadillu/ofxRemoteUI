//
//  ExternalDevices.m
//  ofxRemoteUIClientOSX
//
//  Created by Oriol Ferrer Mesià on 19/04/14.
//
//

#import "ExternalDevices.h"
#import "Joystick.h"
#import "JoystickManager.h"

float valMap(float value, float inputMin, float inputMax, float outputMin, float outputMax);
float valMap(float value, float inputMin, float inputMax, float outputMin, float outputMax){
	return ((value - inputMin) / (inputMax - inputMin) * (outputMax - outputMin) + outputMin);
}

float convertHueToMidiFigtherHue(float hue);
float convertHueToMidiFigtherHue(float hue){
	hue = 1 - hue;
	hue += 0.66;
	if(hue > 1.0){
		hue = hue - 1.0;
	}
	return hue;
}


@implementation ExternalDevices

-(void)initWithWidgets:(unordered_map<string, ParamUI*> *) widgets_ andClient:(ofxRemoteUIClient*) client_{

	widgets = widgets_;
	client = client_;

	//midi
	midi = [[RUMidi alloc] init];
	midi.delegate = self;
	[midi start];

	upcomingDeviceParam = nil;

	//joystick
    [[JoystickManager sharedInstance] setJoystickAddedDelegate:self];
}


-(void)savePrefs:(id)sender{
	NSUserDefaults * d = [NSUserDefaults standardUserDefaults];
	[d setInteger: externalButtonsBehaveAsToggle  forKey:@"externalButtonsBehaveAsToggle"];
	[d setInteger: knobColorAffectsAlpha  forKey:@"knobColorAffectsAlpha"];
}


-(void)updateParamUIOnMainThread:(ParamUI*)item{
	[item updateUI];
}


-(void)loadPrefs{
	NSUserDefaults * d = [NSUserDefaults standardUserDefaults];
	externalButtonsBehaveAsToggle = (int)[d integerForKey:@"externalButtonsBehaveAsToggle"];
	[externalButtonsBehaveAsToggleCheckbox setState:externalButtonsBehaveAsToggle];

	knobColorAffectsAlpha = (int)[d integerForKey:@"knobColorAffectsAlpha"];
	[knobOnColorAffectsAlpha setState:knobColorAffectsAlpha];

}


-(IBAction)applyPrefs:(id)sender{
	externalButtonsBehaveAsToggle = (int)[externalButtonsBehaveAsToggleCheckbox state];
	knobColorAffectsAlpha = (int)[knobOnColorAffectsAlpha state];
}


#pragma mark - MIDI
-(void)userClickedOnParamForDeviceBinding:(ParamUI*)param{

	if( upcomingDeviceParam == nil ){ //no current param waiting to be set
		upcomingDeviceParam = param;
		//[window setTitle:@"ofxRemoteUI (Waiting for External Device Input)"];
	}else{ //already have one waiting param, wtf is the user doing?
		[upcomingDeviceParam stopMidiAnim];
		if (upcomingDeviceParam == param){ //user cliked on blinking param, most likely wants to cancel
			upcomingDeviceParam = nil;
			//[window setTitle:@"ofxRemoteUI"];
		}else{ //usr clicked on another param, lets make that one the selected one
			upcomingDeviceParam = param;
		}
	}
}


//midi setup delegate
- (void) midiSetupChanged:(RUMidi *)midi{

	NSLog(@"setupChanged: Midi Device connected/disconnected!");
	[self updateDevicesWithClientValues:FALSE resetToZero:FALSE paramName:""];

	for (RUMidiDevice *device in [midi devices]) {
		NSLog(@"MIDI device: %@", [device name]);
	}
}


-(IBAction)updateDevicesWithClientValues:(BOOL)onlyColor resetToZero:(BOOL)reset paramName:(const string&)pName{

	RUMidiDevice *midiFighter = [midi deviceNamed:@"Midi Fighter Twister"];
	static UInt8 msgBytes[6];

	if(midiFighter){

		//hide all colors that are not used
		if(reset && pName == ""){
			for(int i = 0; i < 16 * 4; i++){
				msgBytes[0]=0xB0;
				msgBytes[1]=(unsigned char)(i); //knob id
				msgBytes[2]=(unsigned char)(17 + 0); //value brightness to zero
				RUMidiMessage * msg = [RUMidiMessage messageWithBytes:msgBytes length:3 channel:2];
				[midiFighter sendMessage:msg];
				//[msg release];
			}
		}

		map<string, string>::iterator it = bindingsMap.begin();
		while(it != bindingsMap.end()){ //all bindings

			string paramName = it->second;
			if (pName.size() == 0 || pName == paramName){
				string devNameAndAddress = it->first; //looks like "[controlID # channel] @ deviceName]"

				MidiOutCache midiOutConfig = [self cacheForControlURL:devNameAndAddress];

				if(client->paramExistsForName(paramName)){

					RemoteUIParam p = client->getParamForName(paramName);
					unsigned char value = 0; //have to map to [0..127]
					BOOL send = true;
					if (onlyColor && p.type != REMOTEUI_PARAM_COLOR){
						send = FALSE;
					}
					int channelOffset = 0;
					if(send){
						if(!reset){
							switch(p.type){
								case REMOTEUI_PARAM_BOOL:
									if(p.boolVal) value = 127/2 + 3;
									else value = 127/2 - 3; break;
								case REMOTEUI_PARAM_FLOAT:
									value = valMap(p.floatVal, p.minFloat, p.maxFloat, 0, 127);
									break;
								case REMOTEUI_PARAM_ENUM:
								case REMOTEUI_PARAM_INT:
									value = valMap(p.intVal, p.minInt, p.maxInt, 0, 127);
									break;
								case REMOTEUI_PARAM_COLOR:{
									NSColor * c = [NSColor colorWithSRGBRed:p.redVal/255.0f green:p.greenVal/255.0f blue:p.blueVal/255.0f alpha:p.alphaVal/255.0f];
									float hue = [c hueComponent];
									//the midifigter has a weird color mapping, 0 is blue, 1 is red;
									//usually hue is 0 is red, 1 is red. so we apply offset to the hue we get
									//midiOutConfig.channelInt ++; //this is a hack for the MidiFighterTwister! TODO make UI to enable this!!
									channelOffset = 1;
									hue = convertHueToMidiFigtherHue(hue);
									value = hue * 127;
								}break;
								default:
									break;//ignore other types
							}
						}
						//send out on the same channel we got the message in + 1
						msgBytes[0]=0xB0;
						msgBytes[1]=midiOutConfig.controlIDInt; //knob id
						msgBytes[2]=value; //value
						RUMidiMessage * msg = [RUMidiMessage messageWithBytes:msgBytes length:3 channel:channelOffset];
						[midiFighter sendMessage:msg];

						//NSLog(@"sending %s update", paramName.c_str());
						//set param highlight color to match midiFigtherTwister color //TODO make UI to toggle this behavior!
						if(p.type != REMOTEUI_PARAM_COLOR){
							NSColor * paramColor = [NSColor colorWithSRGBRed:p.r/255.0f green:p.g/255.0f blue:p.b/255.0f alpha:1.0];

							float hue = convertHueToMidiFigtherHue([paramColor hueComponent]);
							//param hue
							msgBytes[0]=0xB0;
							msgBytes[1]=midiOutConfig.controlIDInt; //knob id
							msgBytes[2]=hue * 127; //value
							RUMidiMessage * msg = [RUMidiMessage messageWithBytes:msgBytes length:3 channel:1];
							[midiFighter sendMessage:msg];

							//param alpha
							float a = p.a / 96.0f; //remote ui sends (a == 96 || a = 55)
							if (a > 1.0) a = 1.0f;
							if (a < 0.8) a = 0.8;

							msgBytes[0]=0xB0;
							msgBytes[1]=midiOutConfig.controlIDInt; //knob id
							msgBytes[2]=17 + 30 * a; //value
							RUMidiMessage * msg2 = [RUMidiMessage messageWithBytes:msgBytes length:3 channel:2];
							[midiFighter sendMessage:msg2];

						}else{ //if color param, re-set the brightness
							msgBytes[0]=0xB0;
							msgBytes[1]=midiOutConfig.controlIDInt; //knob id
							msgBytes[2]=17 + 30; //value
							RUMidiMessage * msg = [RUMidiMessage messageWithBytes:msgBytes length:3 channel:2];
							[midiFighter sendMessage:msg];

						}
					}
				}
			}
			++it;
		}
	}
}

- (MidiOutCache) cacheForControlURL:(const string &) devNameAndAddress{

	MidiOutCache midiOutConfig;
	map<string,MidiOutCache>::iterator cacheIt = midiDevCache.find(devNameAndAddress);

	if(cacheIt == midiDevCache.end()){ //not in cache, lets cache it
		std::size_t found = devNameAndAddress.find_first_of("@");
		if( found > 0){
			midiOutConfig.deviceName = devNameAndAddress.substr(found + 1, devNameAndAddress.size() - found - 1);
			int i;
			for(i = 1; i < devNameAndAddress.size(); i++){
				if (devNameAndAddress[i] == ':') break;
			}
			for(i = i + 1; i < devNameAndAddress.size(); i++){ //reading into "[ cc :nn#xx]
				if (devNameAndAddress[i] == '#') break;
				midiOutConfig.controlID += devNameAndAddress[i];
			}
			for(i = i + 1; i < devNameAndAddress.size(); i++){
				if (devNameAndAddress[i] == ']') break;
				midiOutConfig.channel += devNameAndAddress[i];
			}
		}

		for(int i = 0; i < midiOutConfig.deviceName.size(); i++){
			if(midiOutConfig.deviceName[i] == '_') midiOutConfig.deviceName[i] = ' '; //replace dashes for spaces , undo what we did before
		}
		midiOutConfig.channelInt = atoi(midiOutConfig.channel.c_str());
		midiOutConfig.controlIDInt = atoi(midiOutConfig.controlID.c_str());
		//NSLog(@"%s : %d %d", devNameAndAddress.c_str(), midiOutConfig.controlIDInt, midiOutConfig.channelInt);
		midiDevCache[devNameAndAddress] = midiOutConfig;
	}else{
		midiOutConfig = cacheIt->second;
	}
	return midiOutConfig;
}


-(IBAction)flashBoundControllers:(id)sender{

	RUMidiDevice *midiFighter = [midi deviceNamed:@"Midi Fighter Twister"];

	if(midiFighter){

		map<string, string>::iterator it = bindingsMap.begin();
		while(it != bindingsMap.end()){ //all bindings

			string devNameAndAddress = it->first; //looks like "[controlID # channel] @ deviceName]"
			string paramName = it->second;
			MidiOutCache midiOutConfig = [self cacheForControlURL:devNameAndAddress];

			if(midiFighter){
				if(client->paramExistsForName(paramName)){

					ParamUI * item = widgets->at(paramName);
					[item flashBackground:[NSNumber numberWithInt:NUM_BOUND_FLASH]];

					static UInt8 msgBytes[6];

					msgBytes[0]=0xB0;
					msgBytes[1]=midiOutConfig.controlIDInt; //knob id
					msgBytes[2]=6; //value
					RUMidiMessage * msg = [RUMidiMessage messageWithBytes:msgBytes length:3 channel:2];
					[midiFighter sendMessage:msg];

					//disable animation after 2 seconds
					dispatch_time_t popTime = dispatch_time(DISPATCH_TIME_NOW, 1.5 * NSEC_PER_SEC);
					dispatch_after(popTime, dispatch_get_main_queue(), ^(void){

						msgBytes[0]=0xB0;
						msgBytes[1]=midiOutConfig.controlIDInt; //knob id
						msgBytes[2]=47; //value
						RUMidiMessage * msg = [RUMidiMessage messageWithBytes:msgBytes length:3 channel:2];
						[midiFighter sendMessage:msg];
					});
				}
			}
			++it;
		}
	}
}

#pragma mark MIDI_RX

- (void)midi:(RUMidi *)midi didReceiveMessage:(RUMidiMessage *)message fromDevice:(RUMidiDevice *)device {

	if(!client->isReadyToSend()) return;
	

	vector<string> updatedParamsThisLoop;

	RUMidiMessageType b = [message type];
	//only some msg types matter , noteOn and noteOff
	bool slider = ( b >= 0xb0 && b <= 0xbF );
	bool noteOff = ( b >= 0x80 && b <= 0x8F );
	bool noteOn = ( b >= 0x90 && b <= 0x9F );
	bool isMidiFighterHighRes = false;
	int channel;
	int knobID;
	float value;

	if([message isSysEx]){ //look for special midifighter twister firmware with 14bit encoded knob data through sysEx https://github.com/armadillu/Midi_Fighter_Twister_Open_Source
		NSArray * sysData = [message sysexArray];
		if([sysData count] == 10){
			int manufact1 = [[sysData objectAtIndex:2] intValue];
			int manufact2 = [[sysData objectAtIndex:3] intValue];
			if(manufact1 == 1 && manufact2 == 121){ //dj techtools
				int command = [[sysData objectAtIndex:4] intValue];
				if(command == 6){ //high res knob data see https://github.com/armadillu/Midi_Fighter_Twister_Open_Source
					isMidiFighterHighRes = true;
					channel = [[sysData objectAtIndex:5] intValue];
					knobID = [[sysData objectAtIndex:6] intValue];
					unsigned char knobDataMSB = [[sysData objectAtIndex:7] intValue];
					unsigned char knobDataLSB = [[sysData objectAtIndex:8] intValue];

					//we get 2 x 7bit values in 2 x 8bit chars to represent 14bit value with a max val of 16383
					unsigned int knobVal = ((knobDataMSB & 0x7f) << 7 ) | (knobDataLSB & 0x7f);
					value = knobVal / float(16383);
				}
			}
		}
	}

	if( slider || noteOff || noteOn || isMidiFighterHighRes ) {

		NSString * dn = [device name];
		//if(dn == nil){continue;} //ignore virtual devices?

		//NSLog(@"%@ %f", [msgPtr description], [msgPtr doubleValue]);
		if(!isMidiFighterHighRes){
			channel = [message channel];
			knobID = [message data1];
			value = [message doubleValue];
		}
		//NSLog(@"_%@_ _%@_ _%@_", [n name], [n deviceName], [n fullName]);
		string type = "??";
		if(noteOff || noteOn) type = "note";
		if(slider || isMidiFighterHighRes) type = "cc";

		string desc = [[dn stringByReplacingOccurrencesOfString:@" " withString:@"_"] UTF8String];
		string channelStr = [[NSString stringWithFormat:@"[%s:%d#%d]", type.c_str(), knobID, channel] UTF8String];
		string controllerUniqueAddress = channelStr + "@" + desc;

		//NSLog(@"## %s ###############################################", controllerUniqueAddress.c_str());

		if( upcomingDeviceParam == nil ){ //we are not setting a midi binding

			map<string,string>::iterator ii = bindingsMap.find(controllerUniqueAddress);
			if ( ii != bindingsMap.end() ){ //found a param linked to that controller
				string paramName = bindingsMap[controllerUniqueAddress];
				unordered_map<string,ParamUI*>::iterator it = widgets->find(paramName);
				if ( it == widgets->end() ){	//not found! wtf?
					//NSLog(@"uh? midi binding pointing to an unexisting param!");
				}else{
					updatedParamsThisLoop.push_back(paramName);
					ParamUI * item = widgets->at(paramName);
					RemoteUIParam p = client->getParamForName(paramName);

					if(slider || isMidiFighterHighRes){ //control type midi msg (slider)
						switch(p.type){
							case REMOTEUI_PARAM_BOOL:
								p.boolVal = value > 0.5f;
								break;
							case REMOTEUI_PARAM_FLOAT:
								p.floatVal = p.minFloat + (p.maxFloat - p.minFloat) * value;
								break;
							case REMOTEUI_PARAM_ENUM:
							case REMOTEUI_PARAM_INT:{
								p.intVal = round(p.minInt + (p.maxInt - p.minInt) * value);
								}break;
							case REMOTEUI_PARAM_COLOR:{
								if(knobColorAffectsAlpha){
									p.alphaVal = value * 255;

								}else{
									NSColor * c = [NSColor colorWithSRGBRed:p.redVal/255.0f green:p.greenVal/255.0f blue:p.blueVal/255.0f alpha:p.alphaVal/255.0f];
									NSColor * c2 = [NSColor colorWithHue:value saturation:[c saturationComponent] brightness:[c brightnessComponent] alpha:[c alphaComponent]];
									p.redVal = [c2 redComponent] * 255.0f;
									p.greenVal = [c2 greenComponent] * 255.0f;
									p.blueVal = [c2 blueComponent] * 255.0f;
									p.alphaVal = [c2 alphaComponent] * 255.0f;
								}
							}break;
							default:
								break;//ignore other types
						}
					}else{ //must be noteOn or noteOff midi msg
						if(p.type == REMOTEUI_PARAM_BOOL){
							if(externalButtonsBehaveAsToggle){
								if(noteOn){ //ignore onRelease (noteOff) event if we toggle
									p.boolVal = !p.boolVal;
								}
							}else{
								p.boolVal = noteOn;
							}
						}
					}
					client->sendUntrackedParamUpdate(p, paramName); //send over network
					[item updateParam:p];
					//this is called form second thread, we need to update UI from main thread
					[self performSelectorOnMainThread:@selector(updateParamUIOnMainThread:) withObject:item waitUntilDone:NO];
				}
			}
		}else{ // we are setting a midi binding

			if (
				( upcomingDeviceParam->param.type == REMOTEUI_PARAM_BOOL && (noteOn || noteOff ) ) //piano keys only for bools
				||
				slider//slider midi msg for any valid param
				||
				isMidiFighterHighRes
				){
				const string & paramN = [upcomingDeviceParam getParamName];
				bindingsMap[controllerUniqueAddress] = paramN;
				[midiBindingsTable performSelectorOnMainThread:@selector(reloadData) withObject:nil waitUntilDone:NO];
				[upcomingDeviceParam stopMidiAnim];
				upcomingDeviceParam = nil;
				//[window setTitle:@"ofxRemoteUI"];
			}

			//save to default bindings
			NSFileManager * fm = [NSFileManager defaultManager];
			[fm createDirectoryAtPath:DEFAULT_BINDINGS_FOLDER withIntermediateDirectories:YES attributes:Nil error:nil];
			NSString * fullPath = [DEFAULT_BINDINGS_FOLDER stringByAppendingString:DEFAULT_BINDINGS_FILE];
			[self saveDeviceBindingsToFile: [NSURL fileURLWithPath:fullPath]];
			[midiBindingsTable performSelectorOnMainThread:@selector(reloadData) withObject:nil waitUntilDone:NO];
		}
	}

	for(auto & pn : updatedParamsThisLoop){
		[self updateDevicesWithClientValues:false resetToZero:FALSE paramName:pn];
	}
}


-(IBAction)saveDeviceBindings:(id)who{

	NSSavePanel *panel = [NSSavePanel savePanel];
	[panel setExtensionHidden:YES];
	[panel setAllowedFileTypes:[NSArray arrayWithObjects:@"ctrlrBind", nil]];
	[panel setAllowedFileTypes:@[@"ctrlrBind"]];
	[panel setCanSelectHiddenExtension:NO];

	NSInteger ret = [panel runModal];
	if (ret == NSFileHandlingPanelOKButton) {
		[self saveDeviceBindingsToFile:[panel URL]];
	}
}

-(void)saveDeviceBindingsToFile:(NSURL*)path;{
	//fill in a NSDict to save as plist
	NSMutableDictionary *dict = [[NSMutableDictionary alloc] initWithCapacity:5];
	for(int i = 0; i < bindingsMap.size(); i++){
		map<string,string>::iterator ii = bindingsMap.begin();
		std::advance(ii, i);
		NSString* midiAddress = [NSString stringWithUTF8String: (*ii).first.c_str()];
		NSString* paramName = [NSString stringWithUTF8String: (*ii).second.c_str()];
		[dict setObject:paramName forKey:midiAddress];
	}

	if([dict count] > 0){
		[dict writeToURL:path atomically:YES];
	}
	[dict release];
}

-(IBAction)loadMidiBindings:(id)who;{

	NSOpenPanel *openPanel = [NSOpenPanel openPanel];
	[openPanel setDirectoryURL:[NSURL fileURLWithPath:[@"~" stringByExpandingTildeInPath]]];
	[openPanel setCanChooseDirectories:YES];
	[openPanel setTitle:@"Locate your bindingsMap file"];
	[openPanel beginWithCompletionHandler:^(NSInteger result) {
		if(result == NSFileHandlingPanelOKButton) {
			[self parseDeviceBindingsFromFile: [[openPanel URLs] objectAtIndex:0]];
		}
	}];
}

-(IBAction)clearMidiBindings:(id)sender;{
	bindingsMap.clear();
	[midiBindingsTable reloadData];
}


-(IBAction)deleteSelectedMidiBinding:(id)sender;{
	int sel = (int)[midiBindingsTable selectedRow];
	if (sel >= 0 && sel < bindingsMap.size()){
		map<string,string>::iterator ii = bindingsMap.begin();
		std::advance(ii, sel);
		bindingsMap.erase(ii);
		[midiBindingsTable reloadData];
	}else{
		NSBeep();
	}
}


-(BOOL)parseDeviceBindingsFromFile:(NSURL*) file{
	NSDictionary * d = [NSDictionary dictionaryWithContentsOfURL:file];
	if(d){
		NSArray * keys = [d allKeys];
		bindingsMap.clear();
		for( id key in keys ){
			bindingsMap[[key UTF8String]] = [[d objectForKey:key] UTF8String];
		}
		[midiBindingsTable reloadData];
		return YES;
	}
	return NO;
}

- (NSInteger) numberOfRowsInTableView:(NSTableView *)tv	{
	return bindingsMap.size();
}

//populate bindingsMap table
- (id) tableView:(NSTableView *)tv objectValueForTableColumn:(NSTableColumn *)tc row:(NSInteger)row	{
	if(row >= bindingsMap.size()) return nil;
	map<string,string>::iterator ii = bindingsMap.begin();
	std::advance(ii, row);

	if([[tc identifier] isEqualToString:@"Device @"]){
		return [NSString stringWithUTF8String: (*ii).first.c_str()];
	}else{
		return [NSString stringWithUTF8String: (*ii).second.c_str()];
	}
}

//allow user to type in a custom param name in the midi bindings table
- (void) tableView:(NSTableView *)tv setObjectValue:(id)v forTableColumn:(NSTableColumn *)tc row:(NSInteger)row	{

	if(row >= bindingsMap.size()) return;
	if([[tc identifier] isEqualToString:@"Device @"]) return; //dont allow editing midi @'s

	NSString * p = v;
	if ([p length] == 0){ //user did input an empty param name, shortcut for deleting this row
		map<string,string>::iterator ii = bindingsMap.begin();
		std::advance(ii, row);
		bindingsMap.erase( ii );
		[midiBindingsTable reloadData];
		return;
	}
	if(![[tc identifier] isEqualToString:@"Device @"]){ //we can set param names by hand if we really want to.
		map<string,string>::iterator ii = bindingsMap.begin();
		std::advance(ii, row);
		string paramName = [p UTF8String];
		unordered_map<string,ParamUI*>::iterator it = widgets->find(paramName);
		if(it != widgets->end()){
			bindingsMap[(*ii).first] = paramName;
		}
	}
}

///////// END MIDI ////////////////////////////


#pragma mark joystick


- (void)joystickAdded:(Joystick *)joystick {
    [joystick registerForNotications:self];
    NSLog(@"Added Joystick: \"%@ (%@)\"", [joystick productName], [joystick manufacturerName] );

}

- (void)joystickAxisChanged:(Joystick *)joystick atAxisIndex:(int)axisIndex{

	string desc = [[[joystick productName] stringByReplacingOccurrencesOfString:@" " withString:@"_"] UTF8String];

	char numstr[21]; // enough to hold all numbers up to 64-bits
	sprintf(numstr, "%d", axisIndex);
	string controllerAddress = "[axis_" + (string)numstr + "]@" + desc;

	if( upcomingDeviceParam == nil ){ //we are not setting a midi binding

		map<string,string>::iterator ii = bindingsMap.find(controllerAddress);
		if ( ii != bindingsMap.end() ){ //found a param linked to that controller

			float value = [joystick getRelativeValueOfAxesIndex:axisIndex];
			const string & paramName = bindingsMap[controllerAddress];
			unordered_map<string,ParamUI*>::iterator it = widgets->find(paramName);
			if ( it == widgets->end() ){	//not found! wtf?
				NSLog(@"uh? joystick binding pointing to an unexisting param!");
			}else{
				ParamUI * item = widgets->at(paramName);
				RemoteUIParam p = client->getParamForName(paramName);
				switch(p.type){
					case REMOTEUI_PARAM_FLOAT:
						p.floatVal = p.minFloat + (p.maxFloat - p.minFloat) * value;
						break;
					case REMOTEUI_PARAM_ENUM:
					case REMOTEUI_PARAM_INT:
						p.intVal = p.minInt + (p.maxInt - p.minInt) * value;
						break;
					case REMOTEUI_PARAM_COLOR:{
						NSColor * c = [NSColor colorWithSRGBRed:p.redVal/255.0f green:p.greenVal/255.0f blue:p.blueVal/255.0f alpha:p.alphaVal/255.0f];
						float sat = [c saturationComponent];
						float bri = [c brightnessComponent];
						float a = [c alphaComponent];
						NSColor * c2 = [NSColor colorWithDeviceHue:value saturation:sat brightness:bri alpha:a];
						p.redVal = [c2 redComponent] * 255.0f;
						p.greenVal = [c2 greenComponent] * 255.0f;
						p.blueVal = [c2 blueComponent] * 255.0f;
						p.alphaVal = [c2 alphaComponent] * 255.0f;
					}break;
					default:
						break;//ignore other types
				}
				client->sendUntrackedParamUpdate(p, paramName); //send over network
				[item updateParam:p];
				//this is called form second thread, we need to update UI from main thread
				[self performSelectorOnMainThread:@selector(updateParamUIOnMainThread:) withObject:item waitUntilDone:NO];
			}
		}
	}else{
		if (upcomingDeviceParam->param.type == REMOTEUI_PARAM_FLOAT ||
			upcomingDeviceParam->param.type == REMOTEUI_PARAM_INT ||
			upcomingDeviceParam->param.type == REMOTEUI_PARAM_COLOR ||
			upcomingDeviceParam->param.type == REMOTEUI_PARAM_ENUM
			){
			float value = [joystick getRelativeValueOfAxesIndex:axisIndex];

			if(value > 0.9 || value < 0.1){ //only accept values where the user is really pushing
				const string & paramN = [upcomingDeviceParam getParamName];
				bindingsMap[controllerAddress] = paramN;
				//[midiBindingsTable reloadData];
				[midiBindingsTable performSelectorOnMainThread:@selector(reloadData) withObject:nil waitUntilDone:NO];
				[upcomingDeviceParam stopMidiAnim];
				upcomingDeviceParam = nil;
				//[window setTitle:@"ofxRemoteUI"];
			}
		}
	}
}

- (void)joystickButton:(int)buttonIndex state:(BOOL)pressed onJoystick:(Joystick*)joystick{

	string desc = [[[joystick productName] stringByReplacingOccurrencesOfString:@" " withString:@"_"] UTF8String];
	char numstr[21]; // enough to hold all numbers up to 64-bits
	sprintf(numstr, "%d", buttonIndex);
	string controllerAddress = "[btn_" + (string)numstr + "]@" + desc;

	if( upcomingDeviceParam == nil ){ //we are not setting a midi binding

		map<string,string>::iterator ii = bindingsMap.find(controllerAddress);
		if ( ii != bindingsMap.end() ){ //found a param linked to that controller

			const string & paramName = bindingsMap[controllerAddress];
			unordered_map<string,ParamUI*>::iterator it = widgets->find(paramName);
			if ( it == widgets->end() ){	//not found! wtf?
				NSLog(@"uh? joystick binding pointing to an unexisting param!");
			}else{
				ParamUI * item = widgets->at(paramName);
				RemoteUIParam p = client->getParamForName(paramName);
				if(p.type ==  REMOTEUI_PARAM_BOOL){
					if(externalButtonsBehaveAsToggle){
						if(pressed){ //ignore onRelease event if we toggle
							p.boolVal = !p.boolVal;
						}
					}else{
						p.boolVal = pressed;
					}
					client->sendUntrackedParamUpdate(p, paramName); //send over network
					[item updateParam:p];
					//this is called form second thread, we need to update UI from main thread
					[self performSelectorOnMainThread:@selector(updateParamUIOnMainThread:) withObject:item waitUntilDone:NO];
				}
			}
		}

	}else{
		if (upcomingDeviceParam->param.type == REMOTEUI_PARAM_BOOL){
			const string & paramN = [upcomingDeviceParam getParamName];
			bindingsMap[controllerAddress] = paramN;
			//[midiBindingsTable reloadData];
			[midiBindingsTable performSelectorOnMainThread:@selector(reloadData) withObject:nil waitUntilDone:NO];
			[upcomingDeviceParam stopMidiAnim];
			upcomingDeviceParam = nil;
			//[window setTitle:@"ofxRemoteUI"];
		}
	}
}

- (void)dealloc{
	[midi stop];
	[midi release];
	[super dealloc];
}

@end
