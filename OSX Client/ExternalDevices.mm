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


@implementation ExternalDevices


-(void)initWithWidgets:(map<string, ParamUI*> *) widgets_ andClient:(ofxRemoteUIClient*) client_{

	widgets = widgets_;
	client = client_;

	//midi
	midiManager = [[VVMIDIManager alloc] init];
	[midiManager setDelegate:self];
	upcomingMidiParam = nil;

	//joystick
    [[JoystickManager sharedInstance] setJoystickAddedDelegate:self];
}


-(void)savePrefs:(id)sender{
	NSUserDefaults * d = [NSUserDefaults standardUserDefaults];
	[d setInteger: externalButtonsBehaveAsToggle  forKey:@"externalButtonsBehaveAsToggle"];
}


-(void)loadPrefs{
	NSUserDefaults * d = [NSUserDefaults standardUserDefaults];

	externalButtonsBehaveAsToggle = (int)[d integerForKey:@"externalButtonsBehaveAsToggle"];
	[externalButtonsBehaveAsToggleCheckbox setState:externalButtonsBehaveAsToggle];
}

-(IBAction)applyPrefs:(id)sender{

	externalButtonsBehaveAsToggle = (int)[externalButtonsBehaveAsToggleCheckbox state];
}


#pragma mark - MIDI
-(void)userClickedOnParamForMidiBinding:(ParamUI*)param{

	if( upcomingMidiParam == nil ){ //no current param waiting to be set
		upcomingMidiParam = param;
		//[window setTitle:@"ofxRemoteUI (Waiting for External Device Input)"];
	}else{ //already have one waiting param, wtf is the user doing?
		[upcomingMidiParam stopMidiAnim];
		if (upcomingMidiParam == param){ //user cliked on blinking param, most likely wants to cancel
			upcomingMidiParam = nil;
			//[window setTitle:@"ofxRemoteUI"];
		}else{ //usr clicked on another param, lets make that one the selected one
			upcomingMidiParam = param;
		}
	}
}

//midi setup delegate
- (void) setupChanged{
}

- (void) receivedMIDI:(NSArray *)a fromNode:(VVMIDINode *)n	{

	NSEnumerator		*it = [a objectEnumerator];
	VVMIDIMessage		*msgPtr;

	while (msgPtr = [it nextObject]){
		Byte b = [msgPtr type];
		//only slider type , noteOn and noteOff
		bool slider = ( b >= 0xb0 && b <= 0xbF );
		bool noteOff = ( b >= 0x80 && b <= 0x8F );
		bool noteOn = ( b >= 0x90 && b <= 0x9F );

		if( slider || noteOff || noteOn ) {

			//NSLog(@"%@ %f", [msgPtr description], [msgPtr doubleValue]);
			string desc = [[[n deviceName] stringByReplacingOccurrencesOfString:@" " withString:@"_"] UTF8String];
			string channel = [ [NSString stringWithFormat:@"[%d#%d]", [msgPtr data1], [msgPtr channel]] UTF8String];
			string controllerUniqueAddress = channel + "@" + desc;

			if( upcomingMidiParam == nil ){ //we are not setting a midi binding

				map<string,string>::iterator ii = bindingsMap.find(controllerUniqueAddress);
				if ( ii != bindingsMap.end() ){ //found a param linked to that controller
					string paramName = bindingsMap[controllerUniqueAddress];
					map<string,ParamUI*>::iterator it = widgets->find(paramName);
					if ( it == widgets->end() ){	//not found! wtf?
						NSLog(@"uh? midi binding pointing to an unexisting param!");
					}else{
						ParamUI * item = widgets->at(paramName);
						RemoteUIParam p = client->getParamForName(paramName);
						if(slider){ //control type midi msg (slider)
							switch(p.type){
								case REMOTEUI_PARAM_BOOL:
									p.boolVal = [msgPtr doubleValue] > 0.5;
									break;
								case REMOTEUI_PARAM_FLOAT:
									p.floatVal = p.minFloat + (p.maxFloat - p.minFloat) * [msgPtr doubleValue];
									break;
								case REMOTEUI_PARAM_ENUM:
								case REMOTEUI_PARAM_INT:
									p.intVal = p.minInt + (p.maxInt - p.minInt) * [msgPtr doubleValue];
									break;
								case REMOTEUI_PARAM_COLOR:{
									NSColor * c = [NSColor colorWithDeviceRed:p.redVal/255. green:p.greenVal/255. blue:p.blueVal/255. alpha:p.alphaVal/255.];
									float sat = [c saturationComponent];
									float bri = [c brightnessComponent];
									float a = [c alphaComponent];
									NSColor * c2 = [NSColor colorWithDeviceHue:[msgPtr doubleValue] saturation:sat brightness:bri alpha:a];
									p.redVal = [c2 redComponent] * 255.f;
									p.greenVal = [c2 greenComponent] * 255.f;
									p.blueVal = [c2 blueComponent] * 255.f;
									p.alphaVal = [c2 alphaComponent] * 255.f;
								}break;
								default:
									break;//ignore other types
							}
						}else{ //must be noteOn or noteOff midi msg
							if(p.type == REMOTEUI_PARAM_BOOL){
								if(externalButtonsBehaveAsToggle){
									if(noteOn){ //ignore onRelease event if we toggle
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
					( upcomingMidiParam->param.type == REMOTEUI_PARAM_BOOL && (noteOn || noteOff ) ) //piano keys only for bools
					||
					slider //slider midi msg for any valid param
					){
					string paramN = [upcomingMidiParam getParamName];
					bindingsMap[controllerUniqueAddress] = paramN;
					[midiBindingsTable reloadData];
					[upcomingMidiParam stopMidiAnim];
					upcomingMidiParam = nil;
					//[window setTitle:@"ofxRemoteUI"];
				}
			}
		}
	}
}


-(IBAction)saveMidiBindings:(id)who{

	NSSavePanel *panel = [NSSavePanel savePanel];
	[panel setExtensionHidden:YES];
	[panel setAllowedFileTypes:[NSArray arrayWithObjects:@"ctrlrBind", nil]];
	[panel setAllowedFileTypes:@[@"ctrlrBind"]];
	[panel setCanSelectHiddenExtension:NO];

	NSInteger ret = [panel runModal];
	if (ret == NSFileHandlingPanelOKButton) {
		[self saveMidiBindingsToFile:[panel URL]];
	}
}

-(void)saveMidiBindingsToFile:(NSURL*)path;{
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
			[self parseMidiBindingsFromFile: [[openPanel URLs] objectAtIndex:0]];
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


-(BOOL)parseMidiBindingsFromFile:(NSURL*) file{
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
		return [NSString stringWithFormat:@"%s",(*ii).first.c_str()];
	}else{
		return [NSString stringWithFormat:@"%s",(*ii).second.c_str()];
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
		map<string,ParamUI*>::iterator it = widgets->find(paramName);
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
	string controllerAddress = "Axis_" + (string)numstr + "@" + desc;

	if( upcomingMidiParam == nil ){ //we are not setting a midi binding

		map<string,string>::iterator ii = bindingsMap.find(controllerAddress);
		if ( ii != bindingsMap.end() ){ //found a param linked to that controller

			float value = [joystick getRelativeValueOfAxesIndex:axisIndex];
			string paramName = bindingsMap[controllerAddress];
			map<string,ParamUI*>::iterator it = widgets->find(paramName);
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
						NSColor * c = [NSColor colorWithDeviceRed:p.redVal/255. green:p.greenVal/255. blue:p.blueVal/255. alpha:p.alphaVal/255.];
						float sat = [c saturationComponent];
						float bri = [c brightnessComponent];
						float a = [c alphaComponent];
						NSColor * c2 = [NSColor colorWithDeviceHue:value saturation:sat brightness:bri alpha:a];
						p.redVal = [c2 redComponent] * 255.f;
						p.greenVal = [c2 greenComponent] * 255.f;
						p.blueVal = [c2 blueComponent] * 255.f;
						p.alphaVal = [c2 alphaComponent] * 255.f;
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
		if (	upcomingMidiParam->param.type == REMOTEUI_PARAM_FLOAT ||
			upcomingMidiParam->param.type == REMOTEUI_PARAM_INT ||
			upcomingMidiParam->param.type == REMOTEUI_PARAM_COLOR ||
			upcomingMidiParam->param.type == REMOTEUI_PARAM_ENUM
			){
			string paramN = [upcomingMidiParam getParamName];
			bindingsMap[controllerAddress] = paramN;
			[midiBindingsTable reloadData];
			[upcomingMidiParam stopMidiAnim];
			upcomingMidiParam = nil;
			//[window setTitle:@"ofxRemoteUI"];
		}
	}
}

- (void)joystickButton:(int)buttonIndex state:(BOOL)pressed onJoystick:(Joystick*)joystick{

	string desc = [[[joystick productName] stringByReplacingOccurrencesOfString:@" " withString:@"_"] UTF8String];
	char numstr[21]; // enough to hold all numbers up to 64-bits
	sprintf(numstr, "%d", buttonIndex);
	string controllerAddress = "Button_" + (string)numstr + "@" + desc;

	if( upcomingMidiParam == nil ){ //we are not setting a midi binding

		map<string,string>::iterator ii = bindingsMap.find(controllerAddress);
		if ( ii != bindingsMap.end() ){ //found a param linked to that controller

			string paramName = bindingsMap[controllerAddress];
			map<string,ParamUI*>::iterator it = widgets->find(paramName);
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
		if (upcomingMidiParam->param.type == REMOTEUI_PARAM_BOOL){
			string paramN = [upcomingMidiParam getParamName];
			bindingsMap[controllerAddress] = paramN;
			[midiBindingsTable reloadData];
			[upcomingMidiParam stopMidiAnim];
			upcomingMidiParam = nil;
			//[window setTitle:@"ofxRemoteUI"];
		}
	}
}


@end
