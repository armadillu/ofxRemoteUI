//
//  AppDelegate.h.m
//  ofxRemoteUIClientOSX
//
//  Created by Oriol Ferrer Mesia on 8/28/11.
//  Copyright 2011 uri.cat. All rights reserved.
//

#import "Item.h"
#import "ItemCellView.h"
#import "AppDelegate.h"


@implementation AppDelegate

- (void)applicationDidFinishLaunching:(NSNotification *)note {

	[self setup];
	timer = [NSTimer scheduledTimerWithTimeInterval:REFRESH_RATE target:self selector:@selector(update) userInfo:nil repeats:YES];
	statusTimer = [NSTimer scheduledTimerWithTimeInterval:STATUS_REFRESH_RATE target:self selector:@selector(statusUpdate) userInfo:nil repeats:YES];
	updateContinuosly = false;

	//connect to last used server by default
	NSUserDefaults * df = [NSUserDefaults standardUserDefaults];
	//NSLog(@"%@", [df stringForKey:@"lastAddress"]);
	if([df stringForKey:@"lastAddress"]) [addressField setStringValue:[df stringForKey:@"lastAddress"]];
	if([df stringForKey:@"lastPort"]) [portField setStringValue:[df stringForKey:@"lastPort"]];
	lagField.stringValue = @"";
	[self performSelector:@selector(connect) withObject:nil afterDelay:0.0];

	//get notified when window is resized
	[[NSNotificationCenter defaultCenter] addObserver:self
											 selector:@selector(windowResized:) name:NSWindowDidResizeNotification
											   object: window];
}


- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication *)sender{
    return YES;
}


- (void)windowResized:(NSNotification *)notification;{

	for( map<string,Item*>::iterator ii = widgets.begin(); ii != widgets.end(); ++ii ){
		string key = (*ii).first;
		Item* t = widgets[key];
		[t remapSlider];
	}

}

-(NSString*)stringFromString:(string) s{
	return  [NSString stringWithCString:s.c_str() encoding:[NSString defaultCStringEncoding]];
}


-(void)syncLocalParamsToClientParams{ 

	vector<string> paramList = client.getAllParamNamesList();
	vector<string> updatedParamsList = client.getChangedParamsList();

	//NSLog(@"Client holds %d params so far", (int) paramList.size());
	//NSLog(@"Client reports %d params changed since last check", (int)updatedParamsList.size());

	//TODO remove params that are on the local DB that are not anymore in the server
	//	for (id key in [widgets allKeys]) { // see what's on the UI now
	//
	//		NSLog(@"%@ - %@",key,[widgets objectForKey:key]);
	//
	//		string paramName = string([key UTF8String]);
	//		if (std::find(paramList.begin(), paramList.end(), paramName) == paramList.end()){ // if this UI element is NOT the list, we r fine
	//
	//		}
	//	}

	for(int i = 0; i < paramList.size(); i++){

		string paramName = paramList[i];
		RemoteUIParam p = client.getParamForName(paramName);
		//printf("%s >> ",paramName.c_str());
		//p.print();

		map<string,Item*>::iterator it = widgets.find(paramName);
		if ( it == widgets.end() ){	//not found, this is a new param... lets make an UI item for it
			Item * row = [[Item alloc] initWithParam: p paramName: paramName];
			
			keyOrder.push_back(paramName);
			widgets[paramName] = row;
		}else{
			[widgets[paramName] updateValues:p];
			//if param has been changed, update the UI
			if(find(updatedParamsList.begin(), updatedParamsList.end(), paramName) != updatedParamsList.end()){ // found in list
				[widgets[paramName] updateUI];
				//printf("updating UI for %s\n", paramName.c_str());
			}
		}
	}
	//[tableView reloadData];
}


-(void)disableAllWidgets{
	for( map<string,Item*>::iterator ii = widgets.begin(); ii != widgets.end(); ++ii ){
		string key = (*ii).first;
		Item* t = widgets[key];
		[t disableChanges];
	}
}


-(void)enableAllWidgets{
	for( map<string,Item*>::iterator ii = widgets.begin(); ii != widgets.end(); ++ii ){
		string key = (*ii).first;
		Item* t = widgets[key];
		[t enableChanges];
	}
}


-(IBAction)pressedSync:(id)sender;{
	client.requestCompleteUpdate();
	//delay a bit the screen update so taht we have gathered the values
	[self performSelector:@selector(syncLocalParamsToClientParams) withObject:nil afterDelay: 3 * REFRESH_RATE];
	[tableView performSelector:@selector(reloadData) withObject:nil afterDelay: 3 * REFRESH_RATE];
}


-(IBAction)pressedContinuously:(NSButton *)sender;{

	if(connectButton.state == 1){
		if ([sender state]) {
			[self disableAllWidgets];
			updateContinuosly = true;
			[updateFromServerButton setEnabled: false];
		}else{
			updateContinuosly = false;
			[updateFromServerButton setEnabled: true];
			[self enableAllWidgets];
		}
	}
}


-(IBAction)pressedConnect:(id)sender{
	[self connect];
}


-(void) connect{
	//NSLog(@"connect!");
	NSUserDefaults * df = [NSUserDefaults standardUserDefaults];
	[df setObject: addressField.stringValue forKey:@"lastAddress"];
	[df setObject: portField.stringValue forKey:@"lastPort"];

	if ([[connectButton title] isEqualToString:@"Connect"]){ //we are not connected
		[addressField setEnabled:false];
		[portField setEnabled:false];
		connectButton.title = @"Disconnect";
		connectButton.state = 1;
		NSLog(@"ofxRemoteUIClientOSX Connecting to %@", addressField.stringValue);
		client.setup([addressField.stringValue UTF8String], [portField.stringValue intValue]);
		[updateFromServerButton setEnabled: true];
		[updateContinuouslyCheckbox setEnabled: true];
		[statusImage setImage:nil];
		//first load of vars
		[self performSelector:@selector(pressedSync:) withObject:nil afterDelay:0.5];
		[progress startAnimation:self];
		lagField.stringValue = @"";

	}else{
		[addressField setEnabled:true];
		[portField setEnabled:true];
		connectButton.state = 0;
		connectButton.title = @"Connect";
		[updateFromServerButton setEnabled: false];
		[updateContinuouslyCheckbox setEnabled:false];
		widgets.clear();
		[tableView reloadData];
		[statusImage setImage:[NSImage imageNamed:@"offline.png"]];
		[progress stopAnimation:self];
		lagField.stringValue = @"";
	}
}



- (void)windowDidResize:(NSNotification *)notification{

}



-(void)setup{
	//client.setup("127.0.0.1", 0.1);
}


-(void)statusUpdate{

	if (connectButton.state == 1){
		float lag = client.connectionLag();
		//printf("lag: %f\n", lag);
		if (lag > CONNECTION_TIMEOUT || lag < 0.0f){
			[self connect]; //force disconnect if lag is too large
			[progress stopAnimation:self];
			[statusImage setImage:[NSImage imageNamed:@"offline"]];
		}else{
			if (lag > 0.0f){
				lagField.stringValue = [NSString stringWithFormat:@"%.1fms", lag];
				[progress stopAnimation:self];
				[statusImage setImage:[NSImage imageNamed:@"connected.png"]];
			}
		}
	}
}


-(void)update{

	if (connectButton.state == 1 ){

		client.update(REFRESH_RATE);

		if(updateContinuosly){
			client.requestCompleteUpdate();
			[self syncLocalParamsToClientParams];
			[tableView reloadData];
		}
	}
}


//UI callback, we will get notified with this when user changes something in UI
-(void)userChangedParam:(RemoteUIParam)p paramName:(string)name{
	//NSLog(@"usr changed param! %s", name.c_str());
	if( connectButton.state == 1 ){
		//printf("client sending: "); p.print();
		client.sendParamUpdate(p, name);
	}
}


- (NSInteger)numberOfRowsInTableView:(NSTableView *)tableView {
	return widgets.size();
}


- (NSView *)tableView:(NSTableView *)myTableView viewForTableColumn:(NSTableColumn *)tableColumn row:(NSInteger)row {

	if (row <= keyOrder.size() && row >= 0){
		Item * item = widgets[ keyOrder[row] ];
		//NSLog(@"viewForTableColumn %@", item);
		ItemCellView *result = [myTableView makeViewWithIdentifier:tableColumn.identifier owner:self];
		[item setCellView:result];
		[item performSelector:@selector(remapSlider) withObject:nil afterDelay:0.01];
		[item updateUI];

		//NSLog(@"item: %@", item);
		//result.detailTextField.stringValue = item.itemKind;
		return result;
	}else{
		return nil;
	}
}


- (BOOL)tableView:(NSTableView *)tableView shouldSelectRow:(NSInteger)row{
	return NO;
}

- (BOOL)tableView:(NSTableView *)tableView shouldSelectTableColumn:(NSTableColumn *)tableColumn{
	return NO;
}

- (BOOL)selectionShouldChangeInTableView:(NSTableView *)tableView;{
	return NO;
}


@end
