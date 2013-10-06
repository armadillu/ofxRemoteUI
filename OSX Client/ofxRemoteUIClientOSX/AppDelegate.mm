//
//  AppDelegate.h.m
//  ofxRemoteUIClientOSX
//
//  Created by Oriol Ferrer Mesia on 8/28/11.
//  Copyright 2011 uri.cat. All rights reserved.
//

#import "ParamUI.h"
#import "AppDelegate.h"
#import "NSColorStringExtension.h"

//ofxRemoteUIClient callback entry point
void clientCallback(RemoteUIClientCallBackArg a){

	AppDelegate * me = [NSApp delegate];
	NSString * remoteIP = [NSString stringWithFormat:@"%s", a.host.c_str()];
	
	switch (a.action) {

		case SERVER_CONNECTED:{
			[me showNotificationWithTitle:@"Connected to Server" description:remoteIP ID:@"ConnectedToServer" priority:-1];
		}break;

		case SERVER_DELETED_PRESET:{
			[me showNotificationWithTitle:@"Server Deleted Preset OK" description:[NSString stringWithFormat:@"%@ deleted preset named '%s'", remoteIP, a.msg.c_str()] ID:@"ServerDeletedPreset" priority:2];
		}break;

		case SERVER_SAVED_PRESET:{
			[me showNotificationWithTitle:@"Server Saved Preset OK" description:[NSString stringWithFormat:@"%@ saved preset named '%s'", remoteIP, a.msg.c_str()] ID:@"ServerSavedPreset" priority:2];
		}break;

		case SERVER_DID_SET_PRESET:{
			[me showNotificationWithTitle:@"Server Did Set Preset OK" description:[NSString stringWithFormat:@"%@ did set preset named '%s'", remoteIP, a.msg.c_str()] ID:@"ServerDidSetPreset" priority:-1];
		}break;

		case SERVER_REQUESTED_ALL_PARAMS_UPDATE:
			//NSLog(@"## Callback: PARAMS_UPDATED");
			if(me->needFullParamsUpdate){ //a bit ugly here...
				[me fullParamsUpdate];
				me->needFullParamsUpdate = NO;
			}
			[me partialParamsUpdate];
			[me updateGroupPopup];
			break;

		case SERVER_PRESETS_LIST_UPDATED:{
			//NSLog(@"## Callback: PRESETS_UPDATED");
			vector<string> list = [me getClient]->getPresetsList();
			if ( list.size() > 0 ){
				[me updatePresetsPopup];
			}
			}break;

		case SERVER_DISCONNECTED:{
			//NSLog(@"## Callback: SERVER_DISCONNECTED");
			[me connect];
			[me showNotificationWithTitle:@"Server Exited, Disconnected!" description:remoteIP ID:@"ServerDisconnected" priority:-1];
			[me updateGroupPopup];
			[me updatePresetsPopup];
		}break;

		case SERVER_CONFIRMED_SAVE:{
			NSString * s = [NSString stringWithFormat:@"%@ - Default XML now holds the current param values", remoteIP];
			[me showNotificationWithTitle:@"Server Saved OK" description:s ID:@"CurrentParamsSavedToDefaultXML" priority:2];
		}break;

		case SERVER_DID_RESET_TO_XML:{
			NSString * s = [NSString stringWithFormat:@"%@ - Params are reset to Server-Launch XML values", remoteIP];
			[me showNotificationWithTitle:@"Server Did Reset To XML OK" description:s ID:@"ServerDidResetToXML" priority:1];
		}break;

		case SERVER_DID_RESET_TO_DEFAULTS:{
			NSString * s = [NSString stringWithFormat:@"%@ - Params are reset to its Share-Time values (Source Code Defaults)", remoteIP];
			[me showNotificationWithTitle:@"Server Did Reset To Default OK" description:s ID:@"ServerDidResetToDefault" priority:1];
		}break;

		case SERVER_REPORTS_MISSING_PARAMS_IN_PRESET:{
			printf("SERVER_REPORTS_MISSING_PARAMS_IN_PRESET\n");
			for(int i = 0; i < a.paramList.size(); i++){
				ParamUI* t = me->widgets[ a.paramList[i] ];
				[t flash:[NSNumber numberWithInt:5]];
			}
		}
			break;
		default:
			break;
	}

	[me log:a];
}

@implementation AppDelegate

-(void)log:(RemoteUIClientCallBackArg) arg{

	if ( arg.action == SERVER_REQUESTED_ALL_PARAMS_UPDATE || arg.action == SERVER_PRESETS_LIST_UPDATED) return; //this stuff is not worth logging
	
	NSString * action = @"";
	switch (arg.action) {
		case SERVER_CONNECTED: action = @"Connected To Server!";  break;
		case SERVER_DISCONNECTED: action = @"Server Disconnected!"; break;
		case SERVER_REQUESTED_ALL_PARAMS_UPDATE: action = @"Server Requested all Params Update!"; break;
		case SERVER_PRESETS_LIST_UPDATED: action = @"Server Presets lists updated!"; break;
		case SERVER_DELETED_PRESET: action = [NSString stringWithFormat:@"Server Deleted Preset named '%s'", arg.msg.c_str()]; break;
		case SERVER_SAVED_PRESET:  action = [NSString stringWithFormat:@"Server Saved Preset named '%s'", arg.msg.c_str()]; break;
		case SERVER_DID_SET_PRESET: action = [NSString stringWithFormat:@"Server did set Preset named '%s'", arg.msg.c_str()]; break;
		case SERVER_CONFIRMED_SAVE: action = @"Server Did Save to Default XML"; break;
		case SERVER_DID_RESET_TO_XML: action = @"Server Did Reset Params to Server-Launch Default XML"; break;
		case SERVER_DID_RESET_TO_DEFAULTS: action = @"Server Did Reset Params to Share-Time (Source Code)"; break;
		default:break;
	}

	NSString * date = [[NSDate date] descriptionWithCalendarFormat:@"%H:%M:%S" timeZone:nil locale:nil];
	NSString * logLine = [NSString stringWithFormat:@"[%@@%s] %@\n", date, arg.host.c_str(), action ];

	[[logView textStorage] beginEditing];
    [[[logView textStorage] mutableString] appendString:logLine];
    [[logView textStorage] endEditing];
}


-(IBAction)clearLog:(id)sender;{
	[[logView textStorage] beginEditing];
    [[[logView textStorage] mutableString] setString:@""];
    [[logView textStorage] endEditing];
}

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
	//client->setVerbose(true);

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


	[[NSColorPanel sharedColorPanel] setShowsAlpha: YES];

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
	[newActions release];

	//make windows resize-snap to height of param
	NSRect r = [window frame];
	float hh = r.size.height - 284;
	int num = hh / ROW_HEIGHT;
	r.size.height = 284 + num * ROW_HEIGHT;
	[window setFrame:r display:NO];
	[window setResizeIncrements:NSMakeSize(1, ROW_HEIGHT)];

	[self updateGroupPopup];
	currentGroup = ""; //empty group means show all params
	[presetsMenu removeAllItems];

	//NSLog(@"applicationDidFinishLaunching done!");
	currentPreset = "";
	//[scroll setScrollerStyle:NSScrollerStyleOverlay];
	//[scroll setScrollerKnobStyle:NSScrollerKnobStyleDefault];

	[self loadPrefs];
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

- (void)applicationWillTerminate:(NSNotification *)notification;{
	[self savePrefs:self];
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
		[listContainer setFrameSize: NSMakeSize( listContainer.frame.size.width, p.maxPerCol * ROW_HEIGHT + off)];
	}
	
	for( map<string,ParamUI*>::iterator ii = widgets.begin(); ii != widgets.end(); ++ii ){
		string key = (*ii).first;
		ParamUI* t = widgets[key];
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

			map<string,ParamUI*>::iterator it = widgets.find(paramName);
			if ( it == widgets.end() ){	//not found, this is a new param... lets make an UI item for it
				ParamUI * row = [[ParamUI alloc] initWithParam: p paramName: paramName ID: c];
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

		map<string,ParamUI*>::iterator it = widgets.find(paramName);
		if ( it == widgets.end() ){	//not found, this is a new param... lets make an UI item for it
			NSLog(@"uh?");
		}else{
			ParamUI * item = widgets[paramName];
			RemoteUIParam p = client->getParamForName(paramName);
			[item updateParam:p];
			[item updateUI];
		}
	}
//	for( map<string,ParamUI*>::iterator ii = widgets.begin(); ii != widgets.end(); ++ii ){
//		string key = (*ii).first;
//		ParamUI* t = widgets[key];
//		[t disableChanges];
//	}

}


-(void)adjustScrollView{

	vector<string> paramsInGroup = [self getParamsInGroup:currentGroup];
	int totalH = ROW_HEIGHT * ((int)paramsInGroup.size() );
	[listContainer setFrameSize: NSMakeSize( [scroll documentVisibleRect].size.width , totalH)];
}


-(LayoutConfig)calcLayoutParams{

	LayoutConfig p;
	float scrollW = [scroll documentVisibleRect].size.width;
	float scrollH = scroll.frame.size.height;

	vector<string> paramsInGroup = [self getParamsInGroup:currentGroup];

	int numParams = (int)paramsInGroup.size();
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
	NSLog(@"colsRows.y: %d   colsRows.x: %d", (int)p.colsRows.y , (int)p.colsRows.x);
	NSLog(@"howManyPerCol: %d   maxPerCol: %d", (int)p.howManyPerCol , (int)p.maxPerCol);
	NSLog(@"#######################");

	return p;
}

-(vector<string>)getParamsInGroup:(string)group{

	if ( group.length() == 0){
		return orderedKeys;
	}
	vector<string>paramsInGroup;

	int numParams = (int)orderedKeys.size();

	for(int i = 0; i < numParams; i++){
		string key = orderedKeys[i];
		ParamUI * item = widgets[key];
		RemoteUIParam p = item->param;
		if (p.group == group){
			paramsInGroup.push_back(key);
		}
	}
	return paramsInGroup;
}


-(vector<string>)getAllGroupsInParams{

	vector<string> v; //all groups

	int numParams = (int)orderedKeys.size();

	for(int i = 0; i < numParams; i++){
		string key = orderedKeys[i];
		ParamUI * item = widgets[key];
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
	for( int i = (int)[subviews count] - 1 ; i >= 0 ; i-- ){
		if( [[subviews objectAtIndex:i] isKindOfClass:[NSBox class]]){
			[[subviews objectAtIndex:i] release]; // release NSBox we allocated before
		}
		[[subviews objectAtIndex:i] removeFromSuperview];
	}
	[self adjustScrollView];


	vector<string> paramsInGroup = [self getParamsInGroup:currentGroup];
	int numParams = (int)paramsInGroup.size();

	int h = 0;
	int howManyThisCol = 0;
	int colIndex = 0;

	for(int i = 0; i < numParams; i++){
		string key = paramsInGroup[i];
		ParamUI * item = widgets[key];
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

	// draw grid ///////////////////////////////////////////

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
	int extra;
	[listContainer setFrameSize: NSMakeSize( listContainer.frame.size.width, p.maxPerCol * ROW_HEIGHT + off)];
}


-(void)disableAllWidgets{
	for( map<string,ParamUI*>::iterator ii = widgets.begin(); ii != widgets.end(); ++ii ){
		string key = (*ii).first;
		ParamUI* t = widgets[key];
		[t disableChanges];
	}
}


-(void)enableAllWidgets{
	for( map<string,ParamUI*>::iterator ii = widgets.begin(); ii != widgets.end(); ++ii ){
		string key = (*ii).first;
		ParamUI* t = widgets[key];
		[t enableChanges];
	}
}


-(IBAction)userWantsRestoreXML:(id)sender;{
	client->restoreAllParamsToInitialXML();
}


-(IBAction)userWantsRestoreDefaults:(id)sender;{
	client->restoreAllParamsToDefaultValues();
}

-(IBAction)userPressedSave:(id)sender;{
	client->saveCurrentStateToDefaultXML();
}

-(IBAction)pressedSync:(id)sender;{
	client->requestCompleteUpdate(); //both params and presets
}


-(IBAction)userChoseGroup:(id)sender{

	NSString * groupName ;
	if( [sender class] == [NSPopUpButton class] ){
		int index = (int)[sender indexOfSelectedItem];
		groupName = [[sender itemAtIndex:index] title];
	}else{
		groupName = [sender title];
		[groupsMenu selectItemWithTitle:groupName];
	}

	if ( [groupName isEqualToString:ALL_PARAMS_GROUP]){
		currentGroup = "";
	}else{
		currentGroup = [groupName UTF8String];
	}
	//NSLog(@"user chose group: _%s_",currentGroup.c_str());
	[self layoutWidgetsWithConfig: [self calcLayoutParams]];
}


-(IBAction)userChosePreset:(id)sender{
	int index = (int)[sender indexOfSelectedItem];
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
	int index = (int)[presetsMenu indexOfSelectedItem];
	if (index == 0) {
		NSBeep();
		return; //empty preset does nothing, cant be deleted
	}

	NSString * preset = [[presetsMenu itemAtIndex:index] title];
	//NSLog(@"user delete preset: %@", preset );
	client->deletePreset([preset UTF8String]);
	currentPreset = "";
}


-(void)updateGroupPopup{
	
	NSMutableArray *menuItemNameArray = [NSMutableArray arrayWithCapacity:4];
	[menuItemNameArray addObject: ALL_PARAMS_GROUP ];
	vector<string> allGroupNames = [self getAllGroupsInParams];
	if (allGroupNames.size() == 1 ) allGroupNames.clear(); //if only default group found, dont show it
	for(int i = 0 ; i < allGroupNames.size(); i++){
		[menuItemNameArray addObject: [NSString stringWithFormat:@"%s",allGroupNames[i].c_str()] ];
	}
    [groupsMenu removeAllItems];
    [groupsMenu addItemsWithTitles: menuItemNameArray];

	//menubar groups
	[groupsMenuBar removeAllItems];
	NSMenuItem * m = [groupsMenuBar addItemWithTitle:[menuItemNameArray objectAtIndex:0]
							 action:@selector(userChoseGroup:)
					  keyEquivalent: @"0"
	 ];
	[m setAction:@selector(userChoseGroup:)];
	[m setTag:0];

	for(int i = 0 ; i < allGroupNames.size(); i++){
		m = [groupsMenuBar addItemWithTitle:[menuItemNameArray objectAtIndex:i+1]
								 action:@selector(userChoseGroup:)
						  keyEquivalent: i <= 9 ? [NSString stringWithFormat:@"%d", i+1] : @""
		 ];

		[m setAction:@selector(userChoseGroup:)];
		[m setTarget: self];
		[m setEnabled:YES];
		[m setTag:i+1];
	}

	if(currentGroup.size() > 0){
		[groupsMenu selectItemWithTitle:[NSString stringWithFormat:@"%s",currentGroup.c_str()] ];
	}else{
		[groupsMenu selectItemAtIndex:0];
	}
}

-(BOOL)validateMenuItem:(NSMenuItem*) item{
	return YES;
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

	for( map<string,ParamUI*>::iterator ii = widgets.begin(); ii != widgets.end(); ++ii ){
		string key = (*ii).first;
		ParamUI* t = widgets[key];
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
		printf("ofxRemoteUIClientOSX Connecting to %s\n", [addressField.stringValue UTF8String] );
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
		RemoteUIClientCallBackArg arg;
		arg.action = SERVER_DISCONNECTED;
		arg.host = "offline";
		[self log:arg];
		[presetsMenu removeAllItems];
		[groupsMenu removeAllItems];
		[addressField setEnabled:true];
		[portField setEnabled:true];
		[updateFromServerButton setEnabled: false];
		[updateContinuouslyCheckbox setEnabled:false];
		[statusImage setImage:[NSImage imageNamed:@"offline"]];
		[progress stopAnimation:self];
		lagField.stringValue = @"";
		[self cleanUpGUIParams];
		client->disconnect();
		connectButton.state = 0;
		connectButton.title = @"Connect";
	}
}


-(void)cleanUpGUIParams{
	for( map<string,ParamUI*>::iterator ii = widgets.begin(); ii != widgets.end(); ++ii ){
		string key = (*ii).first;
		ParamUI* t = widgets[key];
		[t release];
	}
	widgets.clear();
	orderedKeys.clear();

	//also remove the spacer bars. Dont ask me why, but dynamic array walking crashes! :?
	//that why this ghetto walk is here
	NSArray * subviews = [listContainer subviews];
	for( int i = (int)[subviews count]-1 ; i >= 0 ; i-- ){
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

-(void)loadPrefs{

	if([[NSUserDefaults standardUserDefaults] objectForKey: @"ApplePersistenceIgnoreState"] == nil)
		[[NSUserDefaults standardUserDefaults] setBool: YES forKey:@"ApplePersistenceIgnoreState"];

	NSUserDefaults * d = [NSUserDefaults standardUserDefaults];
	int onTop = (int)[d integerForKey:@"alwaysOnTop"] ;
	if (onTop > 0) [window setLevel:NSScreenSaverWindowLevel];
	else [window setLevel:NSNormalWindowLevel];
	[alwaysOnTopCheckbox setState:onTop];
	
	NSString * winColor = [d stringForKey:@"windowColor"];
	NSColor * col;
	if (winColor == nil) {
		col = [NSColor colorWithCalibratedWhite:1 alpha:0.5 ];
	}else{
		col = [NSColor colorFromString:winColor forColorSpace:[NSColorSpace deviceRGBColorSpace]];
	}

	[colorWell setColor:col];
	[window setColor:col];
}

-(IBAction)applyPrefs:(id)sender{

	int onTop = (int)[alwaysOnTopCheckbox state];
	if (onTop > 0) [window setLevel:NSScreenSaverWindowLevel];
	else [window setLevel:NSNormalWindowLevel];

	[window setColor:[colorWell color]];
}


-(void)savePrefs:(id)sender{
	NSUserDefaults * d = [NSUserDefaults standardUserDefaults];
	[d setValue:[[colorWell color] stringRepresentation] forKey:@"windowColor"];
	[d setInteger: ([window level] == NSScreenSaverWindowLevel) ? 1 : 0   forKey:@"alwaysOnTop"];
	[d synchronize];
}


-(void)update{

	if ( connectButton.state == 1 ){ // if connected

		client->update(REFRESH_RATE);


		if(updateContinuosly){
			client->requestCompleteUpdate();
		}

		if(!client->isReadyToSend()){	//if the other side disconnected, or error
			//NSLog(@"disconnect cos its NOT isReadyToSend");
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
		client->sendUntrackedParamUpdate(p, name);
	}
}




-(NSString *)showAlertWithInput: (NSString *)prompt defaultValue: (NSString *)defaultValue {

	//set level to Normal to avoid blocking new preset alert window
	int level = (int)[window level];
	[window setLevel:NSNormalWindowLevel];
	[[NSApplication sharedApplication] activateIgnoringOtherApps:YES];

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
	//restore window level
	[window setLevel:level];
}

-(void)showNotificationWithTitle:(NSString*)title description:(NSString*)desc ID:(NSString*)key priority:(int)p{
	if ([GrowlApplicationBridge isGrowlRunning]){
		[GrowlApplicationBridge notifyWithTitle:title
									description:desc
							   notificationName:key
									   iconData:nil
									   priority:p
									   isSticky:NO
								   clickContext:nil];
	}

}

@end
