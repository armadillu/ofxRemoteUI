//
//  AppDelegate.h.m
//  ofxRemoteUIClientOSX
//
//  Created by Oriol Ferrer Mesia on 8/28/11.
//  Copyright 2011 uri.cat. All rights reserved.
//

#import "Item.h"
#import "AppDelegate.h"

//ofxRemoteUIClient callback entry point
void clientCallback(RemoteUICallBackArg a){

	AppDelegate * me = [NSApp delegate];
	switch (a) {
		case PARAMS_UPDATED:
			//NSLog(@"## Callback: PARAMS_UPDATED");
			if(me->needFullParamsUpdate){ //a bit ugly here...
				[me fullParamsUpdate];
				me->needFullParamsUpdate = NO;
			}
			[me partialParamsUpdate];
			[me updateGroupPopup];
			break;

		case PRESETS_UPDATED:{
			//NSLog(@"## Callback: PRESETS_UPDATED");
			vector<string> list = [me getClient]->getPresetsList();
			if ( list.size() > 0 ){
				[me updatePresetsPopup];
			}
			}break;

		case SERVER_DISCONNECTED:{
			//NSLog(@"## Callback: SERVER_DISCONNECTED");
		}break;

		default:
			break;
	}
}

@implementation AppDelegate


-(ofxRemoteUIClient *)getClient;{
	return client;
}

- (void)applicationDidFinishLaunching:(NSNotification *)note {

	launched = FALSE;

	// setup recent connections ///////////////

	//[[addressField cell] setSearchButtonCell:nil];
	[[addressField cell] setCancelButtonCell:nil];
	[[addressField cell] setSendsSearchStringImmediately:NO];
	[[addressField cell] setSendsActionOnEndEditing:NO];
	[addressField setRecentsAutosaveName:@"recentHosts"];

	NSMenu *cellMenu = [[NSMenu alloc] initWithTitle:@"Search Menu"];
	[cellMenu setAutoenablesItems:YES];
    NSMenuItem *item;

    item = [[NSMenuItem alloc] initWithTitle:@"Clear" action:nil keyEquivalent:@""];
    [item setTag:NSSearchFieldClearRecentsMenuItemTag];
	[item setTarget:self];
    [cellMenu insertItem:item atIndex:0];
	[item release];

    item = [NSMenuItem separatorItem];
    [item setTag:NSSearchFieldRecentsTitleMenuItemTag];
	[item setTarget:nil];
    [cellMenu insertItem:item atIndex:1];


    item = [[NSMenuItem alloc] initWithTitle:@"Recent Searches" action:NULL keyEquivalent:@""];
    [item setTag:NSSearchFieldRecentsTitleMenuItemTag];
	[item setTarget:nil];
    [cellMenu insertItem:item atIndex:2];
	[item release];


    item = [[NSMenuItem alloc] initWithTitle:@"Recents" action:NULL keyEquivalent:@""];
    [item setTag:NSSearchFieldRecentsMenuItemTag];
	[item setTarget:nil];
    [cellMenu insertItem:item atIndex:3];
	[item release];

    id searchCell = [addressField cell];
    [searchCell setSearchMenuTemplate:cellMenu];

	///////////////////////////////////////////////

	client = new ofxRemoteUIClient();
	client->setCallback(clientCallback);

	timer = [NSTimer scheduledTimerWithTimeInterval:REFRESH_RATE target:self selector:@selector(update) userInfo:nil repeats:YES];
	statusTimer = [NSTimer scheduledTimerWithTimeInterval:STATUS_REFRESH_RATE target:self selector:@selector(statusUpdate) userInfo:nil repeats:YES];
	updateContinuosly = false;

	//connect to last used server by default
	NSUserDefaults * df = [NSUserDefaults standardUserDefaults];
	//NSLog(@"%@", [df stringForKey:@"lastAddress"]);
	if([df stringForKey:@"lastAddress"]) [addressField setStringValue:[df stringForKey:@"lastAddress"]];
	if([df stringForKey:@"lastPort"]) [portField setStringValue:[df stringForKey:@"lastPort"]];
	lagField.stringValue = @"";
	[self connect];

	//get notified when window is resized
	[[NSNotificationCenter defaultCenter] addObserver:self
											 selector:@selector(windowResized:) name:NSWindowDidResizeNotification
											   object: window];

	[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(windowBecameMain:) name:NSWindowDidBecomeKeyNotification object:window];


	CALayer *viewLayer = [CALayer layer];
	//[viewLayer setBackgroundColor:CGColorCreateGenericRGB(0,0,0,0.1)];
	[listContainer setWantsLayer:YES]; // view's backing store is using a Core Animation Layer
	[listContainer setLayer:viewLayer];
	//disable implicit CAAnims
	NSMutableDictionary *newActions = [[NSMutableDictionary alloc] initWithObjectsAndKeys:
									   [NSNull null], @"onOrderIn",
									   [NSNull null], @"onOrderOut",
									   [NSNull null], @"sublayers",
									   [NSNull null], @"contents",
									   [NSNull null], @"bounds",
									   nil];
	viewLayer.actions = newActions;

	//make windows resize-snap to height of param
	NSRect r = [window frame];
	NSLog(@"%f",	r.size.height );
	float hh = r.size.height - 284;
	int num = hh / ROW_HEIGHT;
	r.size.height = 284 + num * ROW_HEIGHT;
	[window setFrame:r display:NO];
	[window setResizeIncrements:NSMakeSize(1, ROW_HEIGHT)];

	[self updateGroupPopup];
	currentGroup = ""; //empty group means show all params
	[presetsMenu removeAllItems];

	NSLog(@"applicationDidFinishLaunching done!");
	currentPreset = "";
	launched = TRUE;
}


-(void)windowBecameMain:(id)a{
	if(connectButton.state == 0){
		[self connect];
	}
	//[window resignKeyWindow];
}


- (void)applicationDidBecomeActive:(NSNotification *)notification{

	if(connectButton.state == 0 && launched){
		[self connect];
	}
}


- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication *)sender{
    return YES;
}


- (void)windowResized:(NSNotification *)notification;{

	LayoutConfig p = [self calcLayoutParams];
	if ( p.colsRows.x != lastLayout.colsRows.x || p.colsRows.y != lastLayout.colsRows.y ){
		[self layoutWidgetsWithConfig:p];
		//NSLog(@"layoutWidgetsWithConfig");
	}else{
		int off = ((int)[scroll.contentView frame].size.height ) % ((int)(ROW_HEIGHT));
		[listContainer setFrameSize: CGSizeMake( listContainer.frame.size.width, p.maxPerCol * ROW_HEIGHT + off)];
	}
	
	for( map<string,Item*>::iterator ii = widgets.begin(); ii != widgets.end(); ++ii ){
		string key = (*ii).first;
		Item* t = widgets[key];
		[t remapSlider];
	}
}

-(IBAction)pasteSpecial:(id)sender{

	NSPasteboard *pasteboard = [NSPasteboard generalPasteboard];
	NSArray *classes = [[NSArray alloc] initWithObjects:[NSString class], nil];
	NSDictionary *options = [NSDictionary dictionary];
	NSArray *copiedItems = [pasteboard readObjectsForClasses:classes options:options];
	if (copiedItems != nil) {
		client->setValuesFromString([[copiedItems objectAtIndex:0] UTF8String]);
	}
	[self partialParamsUpdate];
	[classes release];
	//[self layoutWidgetsWithConfig: [self calcLayoutParams]];
}

-(IBAction)copySpecial:(id)sender{

	string val = client->getValuesAsString();
    [[NSPasteboard generalPasteboard] declareTypes:[NSArray arrayWithObject:NSStringPboardType] owner:nil];
    [[NSPasteboard generalPasteboard] setString:[NSString stringWithFormat:@"%s", val.c_str()] forType:NSStringPboardType];
}



-(void)fullParamsUpdate{

	[self cleanUpGUIParams];

	vector<string> paramList = client->getAllParamNamesList();
	//vector<string> updatedParamsList = client->getChangedParamsList();

	//NSLog(@"Client holds %d params so far", (int) paramList.size());
	//NSLog(@"Client reports %d params changed since last check", (int)updatedParamsList.size());

	if(paramList.size() > 0 /*&& updatedParamsList.size() > 0*/){

		int c = 0;

		for(int i = 0; i < paramList.size(); i++){

			string paramName = paramList[i];
			RemoteUIParam p = client->getParamForName(paramName);

			map<string,Item*>::iterator it = widgets.find(paramName);
			if ( it == widgets.end() ){	//not found, this is a new param... lets make an UI item for it
				Item * row = [[Item alloc] initWithParam: p paramName: paramName ID: c];
				c++;
				orderedKeys.push_back(paramName);
				widgets[paramName] = row;
			}
		}
		[self layoutWidgetsWithConfig: [self calcLayoutParams]];
	}
}


-(void) partialParamsUpdate{

	vector<string> paramList = client->getAllParamNamesList();

	for(int i = 0; i < paramList.size(); i++){

		string paramName = paramList[i];

		map<string,Item*>::iterator it = widgets.find(paramName);
		if ( it == widgets.end() ){	//not found, this is a new param... lets make an UI item for it
			NSLog(@"uh?");
		}else{
			Item * item = widgets[paramName];
			RemoteUIParam p = client->getParamForName(paramName);
			[item updateParam:p];
			[item updateUI];
		}
	}
//	for( map<string,Item*>::iterator ii = widgets.begin(); ii != widgets.end(); ++ii ){
//		string key = (*ii).first;
//		Item* t = widgets[key];
//		[t disableChanges];
//	}

}


-(void)adjustScrollView{

	vector<string> paramsInGroup = [self getParamsInGroup:currentGroup];
	int totalH = ROW_HEIGHT * (paramsInGroup.size() );
	[listContainer setFrameSize: CGSizeMake( listContainer.frame.size.width, totalH)];
}


-(LayoutConfig)calcLayoutParams{

	LayoutConfig p;
	float scrollW = listContainer.frame.size.width;
	float scrollH = scroll.frame.size.height;

	vector<string> paramsInGroup = [self getParamsInGroup:currentGroup];

	int numParams = paramsInGroup.size();
	int howManyPerCol = floor( scrollH / ROW_HEIGHT );
	int colIndex = 0;
	int numUsedColumns = ceil((float)numParams / (float)howManyPerCol);
	float rowW = scrollW / numUsedColumns;
	bool didTweak = false;
	while (rowW < ROW_WIDTH && numUsedColumns > 1) {
		numUsedColumns--;
		rowW = scrollW / numUsedColumns;
		didTweak = true;
	}

	if(didTweak){
		howManyPerCol = numParams / (float)numUsedColumns;
	}

	int h = 0;
	int maxPerCol = 0;
	int howManyThisCol = 0;

	for(int i = 0; i < numParams; i++){
		h += ROW_HEIGHT;
		howManyThisCol++;
		if (howManyThisCol >= howManyPerCol ){
			colIndex++;
			if (maxPerCol < howManyThisCol) maxPerCol = howManyThisCol;
			howManyThisCol = 0;
			h = 0;
		}
	}
	if (maxPerCol == 0){
		maxPerCol = scrollH / ROW_HEIGHT;
	}

	p.colsRows.x = colIndex;
	p.colsRows.y = maxPerCol;
	p.rowW = rowW;
	p.howManyPerCol = howManyPerCol;
	p.maxPerCol = maxPerCol;
	return p;
}

-(vector<string>)getParamsInGroup:(string)group{

	if ( group.length() == 0){
		return orderedKeys;
	}
	vector<string>paramsInGroup;

	int numParams = orderedKeys.size();

	for(int i = 0; i < numParams; i++){
		string key = orderedKeys[i];
		Item * item = widgets[key];
		RemoteUIParam p = item->param;
		if (p.group == group){
			paramsInGroup.push_back(key);
		}
	}
	return paramsInGroup;
}


-(vector<string>)getAllGroupsInParams{

	vector<string> v; //all groups

	int numParams = orderedKeys.size();

	for(int i = 0; i < numParams; i++){
		string key = orderedKeys[i];
		Item * item = widgets[key];
		RemoteUIParam p = item->param;
		if (std::find(v.begin(), v.end(), p.group) == v.end()){
			v.push_back(p.group);
		}
	}
	return v;
}

-(void)layoutWidgetsWithConfig:(LayoutConfig) p{

	//remove all views, start over
	NSArray * subviews = [listContainer subviews];
	for( int i = [subviews count]-1 ; i >= 0 ; i-- ){
		[[subviews objectAtIndex:i] removeFromSuperview];
		//[[subviews objectAtIndex:i] release];
	}
	[self adjustScrollView];


	vector<string> paramsInGroup = [self getParamsInGroup:currentGroup];
	int numParams = paramsInGroup.size();

	int h = 0;
	int howManyThisCol = 0;
	int colIndex = 0;

	for(int i = 0; i < numParams; i++){
		string key = paramsInGroup[i];
		Item * item = widgets[key];
		NSRect r = item->ui.frame;

		item->ui.frame = NSMakeRect( colIndex * p.rowW, (numParams - 1) * ROW_HEIGHT - h , p.rowW, r.size.height);
		[listContainer addSubview: item->ui];
		h += r.size.height;
		howManyThisCol++;
		if (howManyThisCol >= p.howManyPerCol ){ // next column
			colIndex++;
			howManyThisCol = 0;
			h = 0;
		}
		[item updateUI];
		[item remapSlider];
	}
	int off = ((int)[scroll.contentView frame].size.height ) % ((int)(ROW_HEIGHT));

	for (int i = 1; i < colIndex + 1; i++) {
		NSBox * box;
		box = [[NSBox alloc] initWithFrame: NSMakeRect( i *  p.rowW, -ROW_HEIGHT, 1, 3 * ROW_HEIGHT + ROW_HEIGHT * numParams )];
		[box setAutoresizingMask: NSViewHeightSizable | NSViewMinXMargin | NSViewMaxXMargin | NSViewMaxYMargin | NSViewMinYMargin ];
		[listContainer addSubview:box];
	}

	for (int i = 0; i < p.maxPerCol ; i++) {
		NSBox * box;
		box = [[NSBox alloc] initWithFrame: NSMakeRect( -10,(numParams - 1) * ROW_HEIGHT - ROW_HEIGHT * i, scroll.frame.size.width + 20, 1)];
		[box setAutoresizingMask: NSViewMinYMargin | NSViewWidthSizable ];
		[listContainer addSubview:box];
	}

	lastLayout = p;
	[listContainer setFrameSize: CGSizeMake( listContainer.frame.size.width, p.maxPerCol * ROW_HEIGHT + off)];
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
	client->requestCompleteUpdate(); //both params and presets
}


-(IBAction)userChoseGroup:(id)sender{
	int index = [sender indexOfSelectedItem];
	NSString * gr = [[sender itemAtIndex:index] title];
	if ( [gr isEqualToString:ALL_PARAMS_GROUP]){
		currentGroup = "";
	}else{
		currentGroup = [gr UTF8String];
	}

	NSLog(@"user chose group: _%s_",currentGroup.c_str());
	[self layoutWidgetsWithConfig: [self calcLayoutParams]];
}


-(IBAction)userChosePreset:(id)sender{
	int index = [sender indexOfSelectedItem];
	if (index == 0) {
		return; //empty preset does nothing
		currentPreset = "";
	}
	NSString * preset = [[sender itemAtIndex:index] title];
	string prest = [preset UTF8String];
	client->setPreset(prest);
	currentPreset = prest;
	//NSLog(@"user chose preset: %@", preset );
}


-(IBAction)userAddPreset:(id)sender{
	NSString * newPreset = [self showAlertWithInput:@"Add a new Preset" defaultValue:@"myPreset"];
	//NSLog(@"user add preset: %@", newPreset);
	if(newPreset != nil){
		currentPreset = [newPreset UTF8String];
		client->savePresetWithName([newPreset UTF8String]);
	}
}


-(IBAction)userDeletePreset:(id)sender{
	int index = [presetsMenu indexOfSelectedItem];
	if (index == 0) return; //empty preset does nothing, cant be deleted

	NSString * preset = [[presetsMenu itemAtIndex:index] title];
	//NSLog(@"user delete preset: %@", preset );
	client->deletePreset([preset UTF8String]);
	currentPreset = "";
}


-(void)updateGroupPopup{
	
	NSMutableArray *menuItemNameArray = [NSMutableArray arrayWithCapacity:4];
	[menuItemNameArray addObject: ALL_PARAMS_GROUP ];
	vector<string> allGroupNames = [self getAllGroupsInParams];
	for(int i = 0 ; i < allGroupNames.size(); i++){
		[menuItemNameArray addObject: [NSString stringWithFormat:@"%s",allGroupNames[i].c_str()] ];
	}
    [groupsMenu removeAllItems];
    [groupsMenu addItemsWithTitles: menuItemNameArray];
}


-(void)updatePresetsPopup{

	NSMutableArray *menuItemNameArray = [NSMutableArray arrayWithCapacity:4];
	vector<string> presetsList = client->getPresetsList();
	[menuItemNameArray addObject: DIRTY_PRESET_NAME ];
	for(int i = 0 ; i < presetsList.size(); i++){
		if ( presetsList[i] != OFX_REMOTEUI_NO_PRESETS ){
			[menuItemNameArray addObject: [NSString stringWithFormat:@"%s",presetsList[i].c_str()] ];
		}
	}
    [presetsMenu removeAllItems];
    [presetsMenu addItemsWithTitles: menuItemNameArray];
	NSString* selPres = [NSString stringWithFormat:@"%s", currentPreset.c_str()];
	if ([selPres length] > 0){
		if ([menuItemNameArray containsObject:selPres]) {
			[presetsMenu selectItemWithTitle:selPres];
		}
	}else{
		[presetsMenu selectItemAtIndex:0];
	}
}


-(IBAction)filterType:(id)sender{
	NSString * filter = [sender stringValue];

	for( map<string,Item*>::iterator ii = widgets.begin(); ii != widgets.end(); ++ii ){
		string key = (*ii).first;
		Item* t = widgets[key];
		NSString * paramName = [NSString stringWithFormat:@"%s", t->paramName.c_str()];
		if ([paramName rangeOfString:filter options:NSCaseInsensitiveSearch].location != NSNotFound || [filter length] == 0){
			[t fadeIn];
		}else{
			[t fadeOut];
		}
	}
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
	//NSLog(@"pressedConnect");
	[self connect];
}


-(void) connect{
	
	//NSLog(@"connect!");
	NSUserDefaults * df = [NSUserDefaults standardUserDefaults];
	[df setObject: addressField.stringValue forKey:@"lastAddress"];
	[df setObject: portField.stringValue forKey:@"lastPort"];

	if ([[connectButton title] isEqualToString:@"Connect"]){ //we are not connected, let's connect
		//NSLog(@"connecting");
		[addressField setEnabled:false];
		[portField setEnabled:false];
		connectButton.title = @"Disconnect";
		connectButton.state = 1;
		NSLog(@"ofxRemoteUIClientOSX Connecting to %@", addressField.stringValue);
		int port = [portField.stringValue intValue];
		if (port < OFXREMOTEUI_PORT - 1) {
			port = OFXREMOTEUI_PORT - 1;
			portField.stringValue = [NSString stringWithFormat:@"%d", OFXREMOTEUI_PORT - 1];
		}
		client->setup([addressField.stringValue UTF8String], port);
		[updateFromServerButton setEnabled: true];
		[updateContinuouslyCheckbox setEnabled: true];
		[statusImage setImage:nil];
		//first load of vars
		[self pressedSync:nil];
		[self performSelector:@selector(pressedSync:) withObject:nil afterDelay:REFRESH_RATE];
		[progress startAnimation:self];
		lagField.stringValue = @"";
		needFullParamsUpdate = YES;

	}else{ // let's disconnect
		//NSLog(@"disconnecting");
		[presetsMenu removeAllItems];
		[groupsMenu removeAllItems];
		[addressField setEnabled:true];
		[portField setEnabled:true];
		connectButton.state = 0;
		connectButton.title = @"Connect";
		[updateFromServerButton setEnabled: false];
		[updateContinuouslyCheckbox setEnabled:false];
		[statusImage setImage:[NSImage imageNamed:@"offline"]];
		[progress stopAnimation:self];
		lagField.stringValue = @"";
		[self cleanUpGUIParams];
		client->disconnect();
	}
}


-(void)cleanUpGUIParams{
	for( map<string,Item*>::iterator ii = widgets.begin(); ii != widgets.end(); ++ii ){
		string key = (*ii).first;
		Item* t = widgets[key];
		[t release];
	}
	widgets.clear();
	orderedKeys.clear();

	//also remove the spacer bars. Dont ask me why, but dynamic array walking crashes! :?
	//that why this ghetto walk is here
	NSArray * subviews = [listContainer subviews];
	for( int i = [subviews count]-1 ; i >= 0 ; i-- ){
		[[subviews objectAtIndex:i] removeFromSuperview];
		//[[subviews objectAtIndex:i] release];
	}
}


-(void)statusUpdate{

	if (connectButton.state == 1){
		float lag = client->connectionLag();
		//printf("lag: %f\n", lag);
		if (lag > CONNECTION_TIMEOUT || lag < 0.0f){
			[self connect]; //force disconnect if lag is too large
			[progress stopAnimation:self];
			[statusImage setImage:[NSImage imageNamed:@"offline"]];
		}else{
			if (lag > 0.0f){
				lagField.stringValue = [NSString stringWithFormat:@"%.1fms", lag];
				[progress stopAnimation:self];
				[statusImage setImage:[NSImage imageNamed:@"connected"]];
			}
		}
	}
}


-(void)update{

	if ( connectButton.state == 1 ){ // if connected

		client->update(REFRESH_RATE);


		if(updateContinuosly){
			client->requestCompleteUpdate();
		}

		if(!client->isReadyToSend()){	//if the other side disconnected, or error
			NSLog(@"disconnect cos its NOT isReadyToSend");
			[self connect]; //this disconnects if we were connectd
		}
	}
}


//UI callback, we will get notified with this when user changes something in UI
-(void)userChangedParam:(RemoteUIParam)p paramName:(string)name{
	//NSLog(@"usr changed param! %s", name.c_str());
	if( connectButton.state == 1 ){
		[presetsMenu selectItemAtIndex:0]; //when user thouches anything, leave the current preset
		currentPreset = "";
		//printf("client sending: "); p.print();
		client->sendParamUpdate(p, name);
	}
}




-(NSString *)showAlertWithInput: (NSString *)prompt defaultValue: (NSString *)defaultValue {
	NSAlert *alert = [NSAlert alertWithMessageText: prompt
									 defaultButton:@"Add Preset"
								   alternateButton:@"Cancel"
									   otherButton:nil
						 informativeTextWithFormat:@"Type a new for the new preset"];

	NSTextField *input = [[NSTextField alloc] initWithFrame:NSMakeRect(0, 0, 170, 24)];
	[input setStringValue:defaultValue];
	[input autorelease];
	[alert layout];
	[alert setAlertStyle:NSInformationalAlertStyle];
	[alert setAccessoryView:input];
	[alert setIcon:nil];
	//	NSRect fr = [[alert accessoryView] frame];
	//	fr.origin.y += 10;
	//	[[alert accessoryView] setFrame: fr];
	NSInteger button = [alert runModal];
	if (button == NSAlertDefaultReturn) {
		[input validateEditing];
		return [input stringValue];
	} else if (button == NSAlertAlternateReturn) {
		return nil;
	} else {
		//NSAssert1(NO, @"Invalid input dialog button %d", button);
		return nil;
	}
}

@end
