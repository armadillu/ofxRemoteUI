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
#pragma mark - CALLBACKS
void clientCallback(RemoteUIClientCallBackArg a){

	AppDelegate * me = [NSApp delegate];
	LogWindows * logs = [me getLogWindows];
	NSString * remoteIP = [NSString stringWithFormat:@"%s", a.host.c_str()];
	
	switch (a.action) {

		case SERVER_CONNECTED:{
			[me showNotificationWithTitle:@"Connected to Server" description:remoteIP ID:@"ConnectedToServer" priority:-1];
		}break;

		case SERVER_DELETED_PRESET:{
			[me showNotificationWithTitle:@"Server Deleted Preset OK" description:[NSString stringWithFormat:@"%@ deleted preset named '%s'", remoteIP, a.msg.c_str()] ID:@"ServerDeletedPreset" priority:1];
		}break;

		case SERVER_SAVED_PRESET:{
			[me showNotificationWithTitle:@"Server Saved Preset OK" description:[NSString stringWithFormat:@"%@ saved preset named '%s'", remoteIP, a.msg.c_str()] ID:@"ServerSavedPreset" priority:1];
		}break;

		case SERVER_DID_SET_PRESET:{
			[me hideAllWarnings];
			[me showNotificationWithTitle:@"Server Did Set Preset OK" description:[NSString stringWithFormat:@"%@ did set preset named '%s'", remoteIP, a.msg.c_str()] ID:@"ServerDidSetPreset" priority:-1];
		}break;

		case SERVER_SAVED_GROUP_PRESET:{
			[me showNotificationWithTitle:@"Server Saved Group Preset OK" description:[NSString stringWithFormat:@"%@ saved group preset named '%s'", remoteIP, a.msg.c_str()] ID:@"ServerSavedPreset" priority:1];
		}break;

		case SERVER_DID_SET_GROUP_PRESET:{
			[me hideAllWarnings];
			[me showNotificationWithTitle:@"Server Did Set Group Preset OK" description:[NSString stringWithFormat:@"%@ did set group preset named '%s'", remoteIP, a.msg.c_str()] ID:@"ServerDidSetPreset" priority:-1];
		}break;

		case SERVER_DELETED_GROUP_PRESET:{
			[me showNotificationWithTitle:@"Server Deleted Group Preset OK" description:[NSString stringWithFormat:@"%@ deleted group preset named '%s'", remoteIP, a.msg.c_str()] ID:@"ServerDeletedPreset" priority:1];
		}break;

		case SERVER_SENT_FULL_PARAMS_UPDATE:
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
			vector<string> presetsList = [me getClient]->getPresetsList();
			if ( presetsList.size() > 0 ){
				[me updatePresetsPopup];
				[me updateGroupPresetMenus];
			}
			for(int i = 0; i < a.paramList.size(); i++){ //notify the missing params
				ParamUI* t = me->widgets[ a.paramList[i] ];
				[t flashWarning:[NSNumber numberWithInt:NUM_FLASH_WARNING]];
			}
			}break;

		case SERVER_DISCONNECTED:{
			//NSLog(@"## Callback: SERVER_DISCONNECTED");
			[me connect];
			me->client->disconnect();
			[me showNotificationWithTitle:@"Server Exited, Disconnected!" description:remoteIP ID:@"ServerDisconnected" priority:-1];
			[me updateGroupPopup];
			[me updatePresetsPopup];
			[me updateGroupPresetMenus];
		}break;

		case SERVER_CONFIRMED_SAVE:{
			NSString * s = [NSString stringWithFormat:@"%@ - Default XML now holds the current param values", remoteIP];
			[me showNotificationWithTitle:@"Server Saved OK" description:s ID:@"CurrentParamsSavedToDefaultXML" priority:1];
		}break;

		case SERVER_DID_RESET_TO_XML:{
			NSString * s = [NSString stringWithFormat:@"%@ - Params are reset to Server-Launch XML values", remoteIP];
			[me showNotificationWithTitle:@"Server Did Reset To XML OK" description:s ID:@"ServerDidResetToXML" priority:0];
		}break;

		case SERVER_DID_RESET_TO_DEFAULTS:{
			NSString * s = [NSString stringWithFormat:@"%@ - Params are reset to its Share-Time values (Source Code Defaults)", remoteIP];
			[me showNotificationWithTitle:@"Server Did Reset To Default OK" description:s ID:@"ServerDidResetToDefault" priority:0];
		}break;

		case SERVER_REPORTS_MISSING_PARAMS_IN_PRESET:{
			//printf("SERVER_REPORTS_MISSING_PARAMS_IN_PRESET\n");
			for(int i = 0; i < a.paramList.size(); i++){
				ParamUI* t = me->widgets[ a.paramList[i] ];
				[t flashWarning:[NSNumber numberWithInt:NUM_FLASH_WARNING]];
			}
		}

		case NEIGHBORS_UPDATED:{
			[me updateNeighbors];
		}break;

		case SERVER_SENT_LOG_LINE:{
			NSString * date = [[NSDate date] descriptionWithCalendarFormat:@"%H:%M:%S" timeZone:nil locale:nil];
			NSString * logLine = [NSString stringWithFormat:@"%@ >> %s\n", date,  a.msg.c_str() ];
			[logs performSelectorOnMainThread:@selector(appendToServerLog:) withObject:logLine
							  waitUntilDone:NO];
		}break;

		case NEIGHBOR_JUST_LAUNCHED_SERVER:
			[me autoConnectToNeighbor:a.host port:a.port];
			break;
		default:
			break;
	}

	if( a.action != SERVER_DISCONNECTED ){ //server disconnection is logged from connect button press
		[logs log:a];
	}
}

@implementation AppDelegate

-(LogWindows*) getLogWindows{
	return logs;
}

-(ExternalDevices*)getExternalDevices;{
	return externalDevices;
}

- (BOOL)application:(NSApplication *)sender openFile:(NSString *)fileName{
	return [externalDevices parseDeviceBindingsFromFile: [NSURL fileURLWithPath:fileName]];
}




#pragma mark applescript


-(void)openAccessibilitySystemPrefs{
	NSAppleScript* appleScript = [[NSAppleScript alloc] initWithContentsOfURL:[[NSBundle mainBundle] URLForResource:@"openAccessibility" withExtension:@"scpt" ] error:nil];
	bool c = [appleScript compileAndReturnError:nil];
	NSDictionary *errDict = nil;
	bool r = [appleScript executeAndReturnError:&errDict];
	if(!r){
		NSLog(@"openAccessibilitySystemPrefs: %d %d >> %@", c, r, errDict);
	}
	[appleScript release];
}

-(void)restartXcodeApp{
	NSAppleScript* appleScript = [[NSAppleScript alloc] initWithContentsOfURL:[[NSBundle mainBundle] URLForResource:@"RestartXcodeApp" withExtension:@"scptd" ] error:nil];
	bool c = [appleScript compileAndReturnError:nil];
	NSDictionary *errDict = nil;
	bool r = [appleScript executeAndReturnError:&errDict];
	if( r == false){
		[self openAccessibilitySystemPrefs];
		NSLog(@"restartXcodeApp: %d %d >> %@", c, r, errDict);
	}
	[appleScript release];
}

-(IBAction)restartXcodeApp:(id)sender{
	client->saveCurrentStateToDefaultXML();
	//[self performSelector:@selector(stopXcodeApp) withObject:Nil afterDelay:0.33];
	[self performSelector:@selector(restartXcodeApp) withObject:Nil afterDelay:0.33];
}


-(void)clearSelectionPresetMenu{
	[presetsMenu selectItemAtIndex:0];
	currentPreset = "";
}

-(IBAction)userChoseNeighbor:(id)sender{

	int index = (int)[sender indexOfSelectedItem];
	if ([currentNeighbors count] > 0){
		NSString * server_port = [currentNeighbors objectAtIndex:index];
		NSArray * info = [server_port componentsSeparatedByString:@":"];
		[portField setStringValue:[info objectAtIndex:1]];
		[addressField setStringValue:[info objectAtIndex:0]];
		if([[connectButton title] isEqualToString:CONNECT_STRING]){ //connect to that dude if not connected
			[self connect];
		}else{
			[self connect];
			[self update];
			[self performSelector:@selector(connect) withObject:Nil afterDelay:0.0];
		}
	}
}


-(void)updateNeighbors{
	vector<Neighbor> ns = client->getNeighbors();
	[neigbhorsMenu removeAllItems];
	NSMutableArray *arr = [NSMutableArray arrayWithCapacity:1];
	[currentNeighbors removeAllObjects];
	for(int i = 0; i < ns.size(); i++){
		[currentNeighbors addObject:[NSString stringWithFormat:@"%s:%d",  ns[i].IP.c_str(), ns[i].port]];
		[arr addObject:[NSString stringWithFormat:@"%s @ %s (%s:%d)", ns[i].binary.c_str(), ns[i].name.c_str(), ns[i].IP.c_str(), ns[i].port]];
	}
	NSString * text = [NSString stringWithFormat:@"(%d)",(int)[arr count] ];
	[neigbhorsMenu addItemsWithTitles: arr];
	[neigbhorsField setStringValue: text];
}


-(RowHeightSize)getRowHeight{
	return rowHeight;
}

-(ofxRemoteUIClient *)getClient;{
	return client;
}

#pragma mark - LAUNCH

- (void)applicationDidFinishLaunching:(NSNotification *)note {

	weJustDisconnected = 0;
	rowHeight = LARGE_34;
	launched = FALSE;
	connecting = FALSE;
	currentNeighbors = [[NSMutableArray alloc] initWithCapacity:1];

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

	[progress setUsesThreadedAnimation: YES];

	///////////////////////////////////////////////

	client = new ofxRemoteUIClient();
	client->setCallback(clientCallback);
	client->setVerbose(false);

	timer = [NSTimer scheduledTimerWithTimeInterval:REFRESH_RATE target:self selector:@selector(update) userInfo:nil repeats:YES];
	statusTimer = [NSTimer scheduledTimerWithTimeInterval:STATUS_REFRESH_RATE target:self selector:@selector(statusUpdate) userInfo:nil repeats:YES];
	updateContinuosly = false;

	//connect to last used server by default
	NSUserDefaults * df = [NSUserDefaults standardUserDefaults];
	//NSLog(@"%@", [df stringForKey:@"lastAddress"]);
	if([df stringForKey:@"lastAddress"]) [addressField setStringValue:[df stringForKey:@"lastAddress"]];
	if([df stringForKey:@"lastPort"]) [portField setStringValue:[df stringForKey:@"lastPort"]];
	//lagField.stringValue = @"";
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

	[self updateGroupPopup];
	currentGroup = ""; //empty group means show all params
	[presetsMenu removeAllItems];

	//NSLog(@"applicationDidFinishLaunching done!");
	currentPreset = "";
	//[scroll setScrollerStyle:NSScrollerStyleOverlay];
	//[scroll setScrollerKnobStyle:NSScrollerKnobStyleDefault];

	[externalDevices parseDeviceBindingsFromFile:[NSURL fileURLWithPath:[DEFAULT_BINDINGS_FOLDER stringByAppendingString:DEFAULT_BINDINGS_FILE]]];//load last used midi bindings

	[self loadPrefs];
	[self recalcWindowSize];

	[window setAllowsToolTipsWhenApplicationIsInactive:YES];


	[externalDevices initWithWidgets:&widgets andClient:client];
	launched = TRUE;

	NSLog(@"Launched ofxRemoteUI version %@", GIT_COMMIT_NUMBER);
}


-(void)windowBecameMain:(id)a{
	if(connectButton.state == 0 && client->isSetup()){
		[self connect];
	}
	//[window resignKeyWindow];
}


- (void)applicationDidBecomeActive:(NSNotification *)notification{

	if(connectButton.state == 0 && launched && client->isSetup()){
		[self connect];
	}
}

- (void)applicationWillTerminate:(NSNotification *)notification;{
	[self savePrefs:self];
	//save current bindings setup, to keep it across launches
	NSFileManager * fm = [NSFileManager defaultManager];
	[fm createDirectoryAtPath:DEFAULT_BINDINGS_FOLDER withIntermediateDirectories:YES attributes:Nil error:nil];
	NSString * fullPath = [DEFAULT_BINDINGS_FOLDER stringByAppendingString:DEFAULT_BINDINGS_FILE];
	[externalDevices saveDeviceBindingsToFile: [NSURL fileURLWithPath:fullPath]];
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
		int off = ((int)[scroll.contentView frame].size.height + 1 ) % ((int)(ROW_HEIGHT));
		int totalCols = p.maxPerCol;
		if (totalCols < p.howManyPerCol) totalCols = p.howManyPerCol;
		[listContainer setFrameSize: NSMakeSize( listContainer.frame.size.width, totalCols * ROW_HEIGHT + off - 1)];
		//NSLog(@"NO CONFIG");
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
	//NSLog(@"numParams: %d", numParams);
	//NSLog(@"scrollH: %f / %f)", scrollH, (scrollH + 1)/ ROW_HEIGHT);
	int howManyPerCol = ( (scrollH + 1)/ ROW_HEIGHT );
	//NSLog(@"howManyPerCol: %d", howManyPerCol);
	int colIndex = 0;
	int numUsedColumns = ceil((float)numParams / (float)(howManyPerCol));
	//NSLog(@"numUsedColumns: %d", numUsedColumns);
	float rowW = scrollW / numUsedColumns;
	bool didTweak = false;
	while (rowW < ROW_WIDTH && numUsedColumns > 1) {
		numUsedColumns--;
		rowW = scrollW / numUsedColumns;
		didTweak = true;
	}

	if(didTweak){
		howManyPerCol = ceil(numParams / (float)numUsedColumns);
		//NSLog(@" -- TWEAK -- numUsedColumns: %d", numUsedColumns);
		//NSLog(@" -- TWEAK -- howManyPerCol: %d", howManyPerCol);
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
		maxPerCol = (scrollH / ROW_HEIGHT);
	}

	p.colsRows.x = colIndex;
	p.colsRows.y = maxPerCol;
	p.rowW = rowW;
	p.howManyPerCol = howManyPerCol;
	p.maxPerCol = maxPerCol;
//	NSLog(@"colsRows.y: %d   colsRows.x: %d", (int)p.colsRows.y , (int)p.colsRows.x);
//	NSLog(@"howManyPerCol: %d   maxPerCol: %d", (int)p.howManyPerCol , (int)p.maxPerCol);
//	NSLog(@"#######################");
//	NSLog(@" ");

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
	int maxInACol = 0;

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
		if(howManyThisCol > maxInACol) maxInACol = howManyThisCol;
	}
	int off = ((int)[scroll.contentView frame].size.height + 1) % ((int)(ROW_HEIGHT));

	// draw grid ///////////////////////////////////////////

	for (int i = 1; i < colIndex + 1; i++) {
		NSBox * box = [[NSBox alloc] initWithFrame: NSMakeRect( i *  p.rowW, -ROW_HEIGHT, 1, 3 * ROW_HEIGHT + ROW_HEIGHT * numParams )];
		[box setAutoresizingMask: NSViewHeightSizable | NSViewMinXMargin | NSViewMaxXMargin | NSViewMaxYMargin | NSViewMinYMargin ];
		[listContainer addSubview:box];
	}

	for (int i = 0; i < p.maxPerCol ; i++) {
		NSBox * box = [[NSBox alloc] initWithFrame: NSMakeRect( -10,(numParams - 1) * ROW_HEIGHT - ROW_HEIGHT * i, scroll.frame.size.width + 20, 1)];
		[box setAutoresizingMask: NSViewMinYMargin | NSViewWidthSizable ];
		[listContainer addSubview:box];
	}

	lastLayout = p;
	int totalCols = p.maxPerCol;
	if (totalCols < p.howManyPerCol) totalCols = p.howManyPerCol;
	//NSLog(@"colIndex %d", colIndex);
	//NSLog(@"totalCols %d", totalCols);
	[listContainer setFrameSize: NSMakeSize( listContainer.frame.size.width, totalCols * ROW_HEIGHT + off - 1) ];
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
	string prest = [[[sender itemAtIndex:index] title] UTF8String];
	client->setPreset(prest);
	currentPreset = prest;

	//set all group presets to dirty
	for(int i = 0; i < orderedKeys.size(); i++){
		ParamUI* t = widgets[ orderedKeys[i] ];
		if(t->param.type == REMOTEUI_PARAM_SPACER){ //spacers hold presets
			[t resetSelectedPreset];
		}
	}
}

-(IBAction)nextPreset:(id)sender;{
	int n = (int)[presetsMenu numberOfItems];
	if (n > 0){
		int sel = (int)[presetsMenu indexOfSelectedItem];
		sel ++;
		if(sel >= n ) sel = 0;
		[presetsMenu selectItemAtIndex:sel];
		[self userChosePreset:presetsMenu];
	}
}

-(IBAction)previousPreset:(id)sender;{

	int n = (int)[presetsMenu numberOfItems];
	if (n > 0){
		int sel = (int)[presetsMenu indexOfSelectedItem];
		sel --;
		if(sel < 0 ) sel = n - 1;
		[presetsMenu selectItemAtIndex:sel];
		[self userChosePreset:presetsMenu];
	}
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
		bool isGroupPreset = presetsList[i].find_first_of("/") != std::string::npos;
		if ( presetsList[i] != OFXREMOTEUI_NO_PRESETS && !isGroupPreset){
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


-(map<string, ParamUI*>)getAllGroupSpacerParams{
	map<string, ParamUI*> groups;
	for(int i = 0; i < orderedKeys.size(); i++){
		ParamUI* t = widgets[ orderedKeys[i] ];
		if(t->param.type == REMOTEUI_PARAM_SPACER){
			groups[t->param.stringVal] = t;
		}
	}
	return groups;
}


-(void)updateGroupPresetMenus{

	spacerGroups = [self getAllGroupSpacerParams];

	//walk all group spacer ParamUIs, empty and add dirty option to its menu
	for( map<string,ParamUI*>::iterator ii = spacerGroups.begin(); ii != spacerGroups.end(); ++ii ){
		string key = (*ii).first;
		ParamUI* t = spacerGroups[key];
		[[t getPresetsMenu] removeAllItems];
		[[t getPresetsMenu] addItemWithTitle:DIRTY_PRESET_NAME];
	}

	//get all presets (global and local to group)
	vector<string> presetsList = client->getPresetsList();

	//walk all presets, for al local ones, add options to menu as needed
	for(int i = 0 ; i < presetsList.size(); i++){
		bool isGroupPreset = presetsList[i].find_first_of("/") != std::string::npos;
		if(isGroupPreset){
			vector<string>sides;
			split(sides, presetsList[i], '/'); //find out group name and preset name
			string groupName = sides[0];
			string presetName = sides[1];
			NSPopUpButton * popup = [spacerGroups[groupName] getPresetsMenu];
			[popup addItemWithTitle:[NSString stringWithFormat:@"%s",presetName.c_str()]];
			[spacerGroups[groupName] updatePresetMenuSelectionToCurrent];
		}
	}
}


-(void)hideAllWarnings{
	for( map<string,ParamUI*>::iterator ii = widgets.begin(); ii != widgets.end(); ++ii ){
		string key = (*ii).first;
		ParamUI* t = widgets[key];
		[t hideWarning];
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
	[self connect];
}


-(void) autoConnectToNeighbor:(string)host_ port:(int)port_{
	if(autoConnectToggle){
		if ([[connectButton title] isEqualToString:CONNECT_STRING] || connecting){ //we are not connected, let's connect to this newly launched neighbor!
			NSString * host = [NSString stringWithFormat:@"%s", host_.c_str()];
			NSString * port = [NSString stringWithFormat:@"%d", port_];
			[addressField setStringValue: host];
			[portField setStringValue: port];
			[self connect];
		}
	}
}

-(void) connect{

	NSUserDefaults * df = [NSUserDefaults standardUserDefaults];
	[df setObject: addressField.stringValue forKey:@"lastAddress"];
	[df setObject: portField.stringValue forKey:@"lastPort"];

	NSString * date = [[NSDate date] descriptionWithCalendarFormat:@"%H:%M:%S" timeZone:nil locale:nil];


	if ([[connectButton title] isEqualToString:CONNECT_STRING]){ //we are not connected, let's connect

		int port = [portField.stringValue intValue];
		bool OK = client->setup([addressField.stringValue UTF8String], port);
		if (!OK){//most likely no network inerfaces available!
			NSLog(@"Can't Setup ofxRemoteUI Client! Most likely no network interfaces available!");
			[self showNotificationWithTitle:@"Cant Setup ofxRemoteUI Client!"
								description:@"No Network Interface available?"
										 ID:@"CantSetupClient"
								   priority:2];
			return;
		}

		[addressField setEnabled:false];
		[portField setEnabled:false];
		connectButton.title = DISCONNECT_STRING;
		connectButton.state = 1;
		printf("ofxRemoteUIClientOSX Connecting to %s\n", [addressField.stringValue UTF8String] );
		[updateFromServerButton setEnabled: true];
		[updateContinuouslyCheckbox setEnabled: true];
		[statusImage setImage:nil];
		//first load of vars
		[self pressedSync:nil];
		[self performSelector:@selector(pressedSync:) withObject:nil afterDelay:REFRESH_RATE];
		[progress startAnimation:self];
		connecting = TRUE;
		needFullParamsUpdate = YES;
		client->connect();
		[logs appendToServerLog:[NSString stringWithFormat:@"%@ >> ## CLIENT CONNECTED ###################\n", date]];

	}else{ // let's disconnect

		RemoteUIClientCallBackArg arg;
		arg.action = SERVER_DISCONNECTED;
		arg.host = [addressField.stringValue UTF8String];
		[logs log:arg];
		arg.host = "offline";
		[presetsMenu removeAllItems];
		[groupsMenu removeAllItems];
		[addressField setEnabled:true];
		[portField setEnabled:true];
		[updateFromServerButton setEnabled: false];
		[updateContinuouslyCheckbox setEnabled:false];
		if ([statusImage image] != [NSImage imageNamed:@"offline"])
			[statusImage setImage:[NSImage imageNamed:@"offline"]];
		[progress stopAnimation:self];
		connecting = FALSE;
		[self cleanUpGUIParams];
		client->disconnect();
		connectButton.state = 0;
		connectButton.title = CONNECT_STRING;
		[self layoutWidgetsWithConfig: [self calcLayoutParams]]; //update scrollbar
		[logs appendToServerLog:[NSString stringWithFormat:@"%@ >> ## CLIENT DISCONNECTED ###################\n", date]];
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

		if (lag > OFXREMOTEUI_CONNECTION_TIMEOUT || lag < 0.0f){
			[self connect]; //force disconnect if lag is too large
			[progress stopAnimation:self];
			connecting = FALSE;
			if ([statusImage image] != [NSImage imageNamed:@"offline"])
				[statusImage setImage:[NSImage imageNamed:@"offline"]];
		}else{
			if (lag > 0.0f){
				//lagField.stringValue = [NSString stringWithFormat:@"%.0fms", 1000 * lag];
				[progress stopAnimation:self];
				connecting = FALSE;
				if ([statusImage image] != [NSImage imageNamed:@"connected"]){
					[statusImage setImage:[NSImage imageNamed:@"connected"]];
				}
			}
		}
	}
}

-(void)loadPrefs{

	NSUserDefaults * d = [NSUserDefaults standardUserDefaults];

	//set some defaults of ours
	[[NSUserDefaults standardUserDefaults] setObject: [NSNumber numberWithInt: 1]
											  forKey: @"NSInitialToolTipDelay"];
	[[NSUserDefaults standardUserDefaults] setObject: [NSNumber numberWithBool:NO]
											  forKey: @"ApplePersistenceIgnoreState"];

	if([d objectForKey: @"ApplePersistenceIgnoreState"] == nil)
		[d setBool: YES forKey:@"ApplePersistenceIgnoreState"];

	int onTop = (int)[d integerForKey:@"alwaysOnTop"] ;
	if (onTop > 0) [window setLevel:NSScreenSaverWindowLevel];
	else [window setLevel:NSNormalWindowLevel];
	[alwaysOnTopCheckbox setState:onTop];

	showNotifications = (int)[d integerForKey:@"showNotifications"];
	[showNotificationsCheckbox setState:showNotifications];
	
	NSString * winColor = [d stringForKey:@"windowColor"];
	NSColor * col;
	if (winColor == nil) {
		col = [NSColor colorWithCalibratedWhite:1 alpha:0.5 ];
	}else{
		col = [NSColor colorFromString:winColor forColorSpace:[NSColorSpace deviceRGBColorSpace]];
	}

	[colorWell setColor:col];
	[window setColor:col];

	autoConnectToggle = (int)[d integerForKey:@"autoConnectToJustLaunchedApps"];
	[autoConnectCheckbox setState: autoConnectToggle];

	rowHeight = (RowHeightSize)[d integerForKey:@"rowHeightSize"];
	[rowHeightMenu selectItemWithTag:(int)rowHeight];
	[externalDevices loadPrefs];
	[self recalcWindowSize];

}


-(IBAction)applyPrefs:(id)sender{

	if(sender == rowHeightMenu){
		int sel = (int)[rowHeightMenu indexOfSelectedItem];
		switch (sel) {
			case 0: rowHeight = TINY_20; break;
			case 1: rowHeight = SMALL_26; break;
			case 2: rowHeight = LARGE_34; break;
			default: NSLog(@"wait, what?"); break;
		}
		//reconnect to see results
		if ([[connectButton title] isEqualToString:DISCONNECT_STRING]){
			[self connect];
			[self performSelector:@selector(connect) withObject:Nil afterDelay:0.3];
		}
		[self recalcWindowSize];
	}
	int onTop = (int)[alwaysOnTopCheckbox state];
	if (onTop > 0) [window setLevel:NSScreenSaverWindowLevel];
	else [window setLevel:NSNormalWindowLevel];

	showNotifications = (int)[showNotificationsCheckbox state];
	[window setColor:[colorWell color]];
	autoConnectToggle = [autoConnectCheckbox state];
	[externalDevices applyPrefs:self];
}


-(void)savePrefs:(id)sender{
	NSUserDefaults * d = [NSUserDefaults standardUserDefaults];
	[d setValue:[[colorWell color] stringRepresentation] forKey:@"windowColor"];
	[d setInteger: ([window level] == NSScreenSaverWindowLevel) ? 1 : 0   forKey:@"alwaysOnTop"];
	[d setInteger: showNotifications  forKey:@"showNotifications"];
	[d setInteger: autoConnectToggle forKey:@"autoConnectToJustLaunchedApps"];
	[d setInteger: (int)rowHeight forKey:@"rowHeightSize"];
	[externalDevices savePrefs:self];
	[d synchronize];
}


bool resizeWindowUpDown = false; //if you keep changing paramUI size, with this we keep track if win should grow or shrink, so that it alternates

-(void)recalcWindowSize{
	[window setResizeIncrements:NSMakeSize(1, 1)];
	float listH = [window frame].size.height - MAIN_WINDOW_NON_LIST_H;
	float newWinH = MAIN_WINDOW_NON_LIST_H + listH + (resizeWindowUpDown ?  ROW_HEIGHT - fmodf(listH, ROW_HEIGHT) : -fmodf(listH, ROW_HEIGHT)) ;
	NSRect frame = [window frame];
	frame.size.height = newWinH;
	[window setFrame:frame display:YES];
	[window setResizeIncrements:NSMakeSize(1, ROW_HEIGHT)];
	resizeWindowUpDown ^= true;
}


-(void)update{

	if (client->isSetup()){
		client->updateAutoDiscovery(REFRESH_RATE);
		client->update(REFRESH_RATE);

		if ( connectButton.state == 1 ){ // if connected

			if(updateContinuosly){
				client->requestCompleteUpdate();
			}

			if(!client->isReadyToSend() && weJustDisconnected <= 0){	//if the other side disconnected, or error
				//NSLog(@"disconnect cos its NOT isReadyToSend");
				//[self connect]; //this disconnects if we were connectd
				weJustDisconnected = 10;
			}
		}
		weJustDisconnected--;
		if(weJustDisconnected <= 0){
			weJustDisconnected = 0;
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

		if (spacerGroups.count(p.group) == 1){ //if the group of the param is there
			ParamUI* pp = spacerGroups[p.group];
			if (!pp->deleting){
				[pp resetSelectedPreset];
			}
		}
	}
}


-(NSString *)showAlertWithInput: (NSString *)prompt defaultValue: (NSString *)defaultValue {

	//set level to Normal to avoid blocking new preset alert window
	NSInteger level = [window level];
	[window setLevel:NSNormalWindowLevel];
	[[NSApplication sharedApplication] activateIgnoringOtherApps:YES];

	NSAlert *alert = [NSAlert alertWithMessageText: prompt
									 defaultButton:@"Add Preset"
								   alternateButton:@"Cancel"
									   otherButton:nil
						 informativeTextWithFormat:@"Type a name for the new preset"];
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

	[window setLevel:level];//restore window level
	
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

-(void)showNotificationWithTitle:(NSString*)title description:(NSString*)desc ID:(NSString*)key priority:(int)p{
	if(showNotifications || p >= 2){
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

}

@end
