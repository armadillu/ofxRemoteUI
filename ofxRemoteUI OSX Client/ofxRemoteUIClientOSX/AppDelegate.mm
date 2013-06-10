//
//  AppDelegate.h.m
//  ofxRemoteUIClientOSX
//
//  Created by Oriol Ferrer Mesia on 8/28/11.
//  Copyright 2011 uri.cat. All rights reserved.
//

#import "Item.h"
#import "AppDelegate.h"


@implementation AppDelegate

- (void)applicationDidFinishLaunching:(NSNotification *)note {

	client = new ofxRemoteUIClient();
	
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

	[self updateGroupPopup];
	currentGroup = ""; //empty group means show all params
}


-(void)windowBecameMain:(id)a{
	if(connectButton.state == 0){
		[self connect];
	}
	//[window resignKeyWindow];
}


- (void)applicationDidBecomeActive:(NSNotification *)notification{

	if(connectButton.state == 0){
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



-(BOOL)fullParamsUpdate{

	[self cleanUpParams];

	vector<string> paramList = client->getAllParamNamesList();
	vector<string> updatedParamsList = client->getChangedParamsList();

	//NSLog(@"Client holds %d params so far", (int) paramList.size());
	//NSLog(@"Client reports %d params changed since last check", (int)updatedParamsList.size());

	if(paramList.size() > 0){

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
		return YES;
	}else{
		return NO;
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
	if(!waitingForResults){
		client->requestCompleteUpdate();
		//NSLog(@"waitingForResults YES");
		waitingForResults = YES;
	}else{
		//NSLog(@"cant syn yet, waitingo n tohers");
	}
	//delay a bit the screen update so that we have gathered the values
	//[self performSelector:@selector(fullParamsUpdate) withObject:nil afterDelay: REFRESH_RATE * 2];
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

-(void)updateGroupPopup{
	
	NSMutableArray *menuItemNameArray = [NSMutableArray arrayWithCapacity:4];
	[menuItemNameArray addObject: ALL_PARAMS_GROUP ];
	vector<string> allGroupNames = [self getAllGroupsInParams];
	for(int i = 0 ; i < allGroupNames.size(); i++){
		[menuItemNameArray addObject: [NSString stringWithFormat:@"%s",allGroupNames[i].c_str()] ];
	}
    [groups removeAllItems];
    [groups addItemsWithTitles: menuItemNameArray];
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
		client->update(REFRESH_RATE);

	}else{ // let's disconnect
		//NSLog(@"disconnecting");
		[addressField setEnabled:true];
		[portField setEnabled:true];
		connectButton.state = 0;
		connectButton.title = @"Connect";
		[updateFromServerButton setEnabled: false];
		[updateContinuouslyCheckbox setEnabled:false];
		[statusImage setImage:[NSImage imageNamed:@"offline"]];
		[progress stopAnimation:self];
		lagField.stringValue = @"";
		[self cleanUpParams];
		waitingForResults = false;
	}
}


-(void)cleanUpParams{
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

	client->update(REFRESH_RATE);

	if ( connectButton.state == 1 ){ // if connected

		if( waitingForResults ){
			if ( [self fullParamsUpdate] ){
				client->update(REFRESH_RATE); // just to be super sure, we run a second update
				[self partialParamsUpdate];
				waitingForResults = false;
				[self updateGroupPopup];
			}else{
				//NSLog(@"NOT yet...");
				client->requestCompleteUpdate();
			}
		}

		if(updateContinuosly){
			client->requestCompleteUpdate();
			//[self partialParamsUpdate];
			[self performSelector:@selector(partialParamsUpdate) withObject:nil afterDelay: 0];
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
		//printf("client sending: "); p.print();
		client->sendParamUpdate(p, name);
	}
}


@end
