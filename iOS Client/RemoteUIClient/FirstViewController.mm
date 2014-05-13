//
//  FirstViewController.m
//  RemoteUIClient
//
//  Created by Oriol Ferrer Mesià on 11/05/14.
//  Copyright (c) 2014 Oriol Ferrer Mesià. All rights reserved.
//

#import "FirstViewController.h"

FirstViewController * paramsController;



void clientCallback(RemoteUIClientCallBackArg a){

	NSString * remoteIP = [NSString stringWithFormat:@"%s", a.host.c_str()];

	FirstViewController * me = paramsController;

	switch (a.action) {

		case SERVER_CONNECTED:{
			//[me showNotificationWithTitle:@"Connected to Server" description:remoteIP ID:@"ConnectedToServer" priority:-1];
		}break;

		case SERVER_DELETED_PRESET:{
			//[me showNotificationWithTitle:@"Server Deleted Preset OK" description:[NSString stringWithFormat:@"%@ deleted preset named '%s'", remoteIP, a.msg.c_str()] ID:@"ServerDeletedPreset" priority:1];
		}break;

		case SERVER_SAVED_PRESET:{
			//[me showNotificationWithTitle:@"Server Saved Preset OK" description:[NSString stringWithFormat:@"%@ saved preset named '%s'", remoteIP, a.msg.c_str()] ID:@"ServerSavedPreset" priority:1];
		}break;

		case SERVER_DID_SET_PRESET:{
			//[me hideAllWarnings];
			//[me showNotificationWithTitle:@"Server Did Set Preset OK" description:[NSString stringWithFormat:@"%@ did set preset named '%s'", remoteIP, a.msg.c_str()] ID:@"ServerDidSetPreset" priority:-1];
		}break;

		case SERVER_SAVED_GROUP_PRESET:{
			//[me showNotificationWithTitle:@"Server Saved Group Preset OK" description:[NSString stringWithFormat:@"%@ saved group preset named '%s'", remoteIP, a.msg.c_str()] ID:@"ServerSavedPreset" priority:1];
		}break;

		case SERVER_DID_SET_GROUP_PRESET:{
			//[me hideAllWarnings];
			//[me showNotificationWithTitle:@"Server Did Set Group Preset OK" description:[NSString stringWithFormat:@"%@ did set group preset named '%s'", remoteIP, a.msg.c_str()] ID:@"ServerDidSetPreset" priority:-1];
		}break;

		case SERVER_DELETED_GROUP_PRESET:{
			//[me showNotificationWithTitle:@"Server Deleted Group Preset OK" description:[NSString stringWithFormat:@"%@ deleted group preset named '%s'", remoteIP, a.msg.c_str()] ID:@"ServerDeletedPreset" priority:1];
		}break;

		case SERVER_SENT_FULL_PARAMS_UPDATE:
			//NSLog(@"## Callback: PARAMS_UPDATED");
			if(me->needFullParamsUpdate){ //a bit ugly here...
				[me fullParamsUpdate];
				me->needFullParamsUpdate = NO;
			}
			[me partialParamsUpdate];
			//[me updateGroupPopup];

			break;

		case SERVER_PRESETS_LIST_UPDATED:{
			vector<string> presetsList = [me getClient]->getPresetsList();
//			if ( presetsList.size() > 0 ){
				[me updatePresets];
//				[me updateGroupPresetMenus];
//			}
//			for(int i = 0; i < a.paramList.size(); i++){ //notify the missing params
//				ParamUI* t = me->widgets[ a.paramList[i] ];
//				[t flashWarning:[NSNumber numberWithInt:NUM_FLASH_WARNING]];
//			}
		}break;

		case SERVER_DISCONNECTED:{
			//NSLog(@"## Callback: SERVER_DISCONNECTED");
			//
			[me disconnect];
//			[me showNotificationWithTitle:@"Server Exited, Disconnected!" description:remoteIP ID:@"ServerDisconnected" priority:-1];
//			[me updateGroupPopup];
//			[me updatePresetsPopup];
//			[me updateGroupPresetMenus];
		}break;

		case SERVER_CONFIRMED_SAVE:{
			NSString * s = [NSString stringWithFormat:@"%@ - Default XML now holds the current param values", remoteIP];
//			[me showNotificationWithTitle:@"Server Saved OK" description:s ID:@"CurrentParamsSavedToDefaultXML" priority:1];
		}break;

		case SERVER_DID_RESET_TO_XML:{
			NSString * s = [NSString stringWithFormat:@"%@ - Params are reset to Server-Launch XML values", remoteIP];
//			[me showNotificationWithTitle:@"Server Did Reset To XML OK" description:s ID:@"ServerDidResetToXML" priority:0];
		}break;

		case SERVER_DID_RESET_TO_DEFAULTS:{
			NSString * s = [NSString stringWithFormat:@"%@ - Params are reset to its Share-Time values (Source Code Defaults)", remoteIP];
//			[me showNotificationWithTitle:@"Server Did Reset To Default OK" description:s ID:@"ServerDidResetToDefault" priority:0];
		}break;

		case SERVER_REPORTS_MISSING_PARAMS_IN_PRESET:{
			//printf("SERVER_REPORTS_MISSING_PARAMS_IN_PRESET\n");
//			for(int i = 0; i < a.paramList.size(); i++){
//				ParamUI* t = me->widgets[ a.paramList[i] ];
//				[t flashWarning:[NSNumber numberWithInt:NUM_FLASH_WARNING]];
//			}
		}

		case NEIGHBORS_UPDATED:{
			[me updateNeighbors];
		}break;

		case SERVER_SENT_LOG_LINE:{
//			NSString * date = [[NSDate date] descriptionWithCalendarFormat:@"%H:%M:%S" timeZone:nil locale:nil];
//			NSString * logLine = [NSString stringWithFormat:@"%@ >> %s\n", date,  a.msg.c_str() ];
//			[logs performSelectorOnMainThread:@selector(appendToServerLog:) withObject:logLine
//								waitUntilDone:NO];
		}break;

		case NEIGHBOR_JUST_LAUNCHED_SERVER:
//			[me autoConnectToNeighbor:a.host port:a.port];
			break;
		default:
			break;
	}

	if( a.action != SERVER_DISCONNECTED ){ //server disconnection is logged from connect button press
//		[logs log:a];
	}
}

#pragma mark -

@implementation FirstViewController


- (void)viewDidLoad{

    [super viewDidLoad];
	paramsController = self;
	connected = NO;

	//toolbar creation
	toolbar = [[UIToolbar alloc] initWithFrame:CGRectMake(0, self.view.bounds.size.height - TOOLBAR_H, self.view.bounds.size.width, TOOLBAR_H)];
	toolbar.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleTopMargin;
	[self.view addSubview:toolbar];

	connectB = [[UIBarButtonItem alloc] initWithTitle:[NSString stringWithFormat:@"%@(0)", CONNECT_EMOJI]
												style:UIBarButtonItemStyleBordered
											   target:self
											   action:@selector(pressedConnectButton)];

	presetsButton = [[UIBarButtonItem alloc] initWithTitle:[NSString stringWithFormat:@"%@(0)", PRESET_EMOJI]
													 style:UIBarButtonItemStyleBordered
													target:self
													action:@selector(pressedPresetsButton)];

	saveButton = [[UIBarButtonItem alloc] initWithTitle:SAVE_EMOJI
													 style:UIBarButtonItemStyleBordered
													target:self
													action:@selector(pressedSaveButton)];

	addPresetsButton = [[UIBarButtonItem alloc] initWithTitle:ADD_PRESET_EMOJI
												  style:UIBarButtonItemStyleBordered
												 target:self
												 action:@selector(pressedAddPresetButton)];


	toolbar.items = @[connectB, presetsButton, addPresetsButton, saveButton];

	paramViews = [[NSMutableArray alloc] initWithCapacity:50];
	currentNeighbors = [[NSMutableArray alloc] initWithCapacity:1];

	client = new ofxRemoteUIClient();
	client->setCallback(clientCallback);
	client->setVerbose(false);

	bool OK = client->setup("0.0.0.0", 10000); //test

	needFullParamsUpdate = YES; //before connect, always!

	timer = [NSTimer scheduledTimerWithTimeInterval:REFRESH_RATE target:self selector:@selector(update) userInfo:nil repeats:YES];

//	UICollectionViewFlowLayout *flowLayout = [[UICollectionViewFlowLayout alloc] init];
//	[flowLayout setScrollDirection:UICollectionViewScrollDirectionVertical];
//	[self.collectionView setCollectionViewLayout:flowLayout];
}


- (void)viewWillLayoutSubviews{
	CGRect bounds = [[UIScreen mainScreen] bounds]; // portrait bounds
	if (UIInterfaceOrientationIsLandscape([[UIApplication sharedApplication] statusBarOrientation])) {
		bounds.size = CGSizeMake(bounds.size.height, bounds.size.width);
	}
	bounds.size.height -= TOOLBAR_H;
	[self.collectionView setFrame:bounds];
}


-(IBAction)pressedConnectButton{

	vector<Neighbor> ns = client->getNeighbors();
	NSMutableArray *arr = [NSMutableArray arrayWithCapacity:1];
	[currentNeighbors removeAllObjects];

	for(int i = 0; i < ns.size(); i++){
		[currentNeighbors addObject:[NSString stringWithFormat:@"%s:%d",  ns[i].IP.c_str(), ns[i].port]];
		//[arr addObject:[NSString stringWithFormat:@"%s@%s", ns[i].binary.c_str(), ns[i].name.c_str()]];
		[arr addObject:[NSString stringWithFormat:@"%s@%s (%s:%d)", ns[i].binary.c_str(), ns[i].name.c_str(), ns[i].IP.c_str(), ns[i].port]];
	}
	[arr addObject: @"Disconnect"];

    connectSheet = [[UIActionSheet alloc] initWithTitle:@"ofxRemoteUI Nearby Servers"
                                                             delegate:self
                                                    cancelButtonTitle:nil
                                               destructiveButtonTitle:nil
                                                    otherButtonTitles:nil];
    for (NSString *title in arr) {
        [connectSheet addButtonWithTitle:title];
    }

    [connectSheet addButtonWithTitle:@"Cancel"];
    connectSheet.cancelButtonIndex = [arr count] ;
	connectSheet.actionSheetStyle = UIActionSheetStyleDefault;
    [connectSheet showFromToolbar:toolbar];
}


-(IBAction)pressedPresetsButton{

	if(connected){
		NSMutableArray *arr = [NSMutableArray arrayWithCapacity:1];
		int numPresets = 0;
		for(int i = 0; i < presets.size(); i++){
			bool isGroupPreset = presets[i].find_first_of("/") != std::string::npos;
			if ( presets[i] != OFXREMOTEUI_NO_PRESETS && !isGroupPreset){
				[arr addObject:[NSString stringWithFormat:@"%s",presets[i].c_str()]];
				numPresets++;
			}
		}

		if(numPresets > 0){
			presetsSheet = [[UIActionSheet alloc] initWithTitle:@"Preset List"
													   delegate:self
											  cancelButtonTitle:nil
										 destructiveButtonTitle:nil
											  otherButtonTitles:nil];
			for (NSString *title in arr) {
				[presetsSheet addButtonWithTitle:title];
			}

			[presetsSheet addButtonWithTitle:@"Cancel"];
			presetsSheet.cancelButtonIndex = [arr count];
			presetsSheet.actionSheetStyle = UIActionSheetStyleDefault;
			[presetsSheet showFromToolbar:toolbar];
		}
	}
}


-(IBAction)pressedSaveButton{
	if(connected){
		client->saveCurrentStateToDefaultXML();
	}
}


-(IBAction)pressedAddPresetButton{
	if(connected){
		UIAlertView *alert = [[UIAlertView alloc] initWithTitle: @"Input New Preset Name"
														message: @""
													   delegate: self
											  cancelButtonTitle: @"Cancel"
											  otherButtonTitles: @"OK", nil
							  ];

		alert.alertViewStyle = UIAlertViewStylePlainTextInput;
		[alert textFieldAtIndex:0].placeholder = @"New Preset Name";
		[alert show];
	}
}

//moved this to the alert dismiss to avoid jerky fps when fading away the alertview
- (void)alertView:(UIAlertView *)alertView clickedButtonAtIndex:(NSInteger)buttonIndex{

	if ( alertView.alertViewStyle == UIAlertViewStylePlainTextInput && buttonIndex == 1){
		NSString *name = [alertView textFieldAtIndex:0].text;
		NSLog(@"user saved preset named %@", name);
		client->savePresetWithName([name UTF8String]);
	}

}

- (void)actionSheet:(UIActionSheet *)actionSheet clickedButtonAtIndex:(NSInteger)buttonIndex;{

	if (actionSheet == connectSheet){

		if(buttonIndex >= [currentNeighbors count] + 1){
			//cancel
		}else{
			if (buttonIndex == [currentNeighbors count] ){
				[self disconnect];
			}else{
				NSString * server_port = [currentNeighbors objectAtIndex:buttonIndex];
				NSArray * info = [server_port componentsSeparatedByString:@":"];
				port = [info objectAtIndex:1];
				address = [info objectAtIndex:0];
				[self disconnect];
				[self connect];
			}
		}
		connectSheet = nil;
	}

	if (actionSheet == presetsSheet){
		NSString* presetName = [actionSheet buttonTitleAtIndex:buttonIndex];
		NSLog(@"user chose preset: %@", presetName );
		client->setPreset([presetName UTF8String]);
		presetsSheet = nil;
	}
}


//tweak font size on action sheet
- (void)willPresentActionSheet:(UIActionSheet *)actionSheet {
    [actionSheet.subviews enumerateObjectsUsingBlock:^(id _currentView, NSUInteger idx, BOOL *stop) {
        if ([_currentView isKindOfClass:[UIButton class]]) {
//			if ([[UIApplication sharedApplication] statusBarOrientation] == UIInterfaceOrientationPortrait ||
//				[[UIApplication sharedApplication] statusBarOrientation] == UIInterfaceOrientationPortraitUpsideDown
//				){ //make labels smaller if in portrait mode && iphone
//				[((UIButton *)_currentView).titleLabel setFont:[UIFont boldSystemFontOfSize:12.5f]];
//			}
			((UIButton *)_currentView).titleLabel.adjustsFontSizeToFitWidth = true;
			((UIButton *)_currentView).titleLabel.minimumScaleFactor = 0.5;
			((UIButton *)_currentView).titleLabel.baselineAdjustment = UIBaselineAdjustmentAlignCenters;
			//((UIButton *)_currentView).titleLabel.lineBreakMode = NSLineBreakByTruncatingTail;

        }
    }];
}

-(void)updatePresets{

	presets = client->getPresetsList();
	int numPresets = 0;
	for(int i = 0; i < presets.size(); i++){
		bool isGroupPreset = presets[i].find_first_of("/") != std::string::npos;
		if ( presets[i] != OFXREMOTEUI_NO_PRESETS && !isGroupPreset){
			numPresets++;
		}
	}

	presetsButton.title = [NSString stringWithFormat:[NSString stringWithFormat:@"%@(%d)", PRESET_EMOJI, numPresets]];
}


-(void)cleanUpGUIParams{

//	for( map<string,ParamUI*>::iterator ii = widgets.begin(); ii != widgets.end(); ++ii ){
//		string key = (*ii).first;
//		ParamUI* t = widgets[key];
//		//[t release];
//	}
	widgets.clear();
	[paramViews removeAllObjects];
	widgets.clear();
	presets.clear();
	[self updatePresets];
	[self.collectionView reloadData];

	//also remove the spacer bars. Dont ask me why, but dynamic array walking crashes! :?
	//that why this ghetto walk is here
//	NSArray * subviews = [listContainer subviews];
//	for( int i = (int)[subviews count]-1 ; i >= 0 ; i-- ){
//		[[subviews objectAtIndex:i] removeFromSuperview];
//		//[[subviews objectAtIndex:i] release];
//	}
//	[paramViews removeAllObjects];

}


-(void)fullParamsUpdate{

	[self cleanUpGUIParams];

	vector<string> paramList = client->getAllParamNamesList();
	//vector<string> updatedParamsList = client->getChangedParamsList();

	NSLog(@"Client holds %d params so far", (int) paramList.size());
	//NSLog(@"Client reports %d params changed since last check", (int)updatedParamsList.size());

	if(paramList.size() > 0 /*&& updatedParamsList.size() > 0*/){

		int c = 0;

		for(int i = 0; i < paramList.size(); i++){

			string paramName = paramList[i];
			RemoteUIParam p = client->getParamForName(paramName);

			map<string,ParamUI*>::iterator it = widgets.find(paramName);
			if ( it == widgets.end() ){	//not found, this is a new param... lets make an UI item for it

				//ParamUI *paramView = [[ParamUI alloc] initWithParam:p name: [name UTF8String] ID:i];
				//NSLog(@"%@", paramView);

				ParamUI * paramView = [[ParamUI alloc] initWithParam: p name: paramName ID: c client:client];
				c++;
				widgets[paramName] = paramView;

				if (paramView){
					[paramViews addObject:paramView];
				}
			}
		}
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
			RemoteUIParam p = client->getParamForName(paramName);
			ParamUI * item = widgets[paramName];
			[item updateParam:p];
			[item updateUI];
		}
	}

	[self.collectionView reloadData];
}


-(void)updateNeighbors{
	vector<Neighbor> ns = client->getNeighbors();
	connectB.title = [NSString stringWithFormat:[NSString stringWithFormat:@"%@(%d)", CONNECT_EMOJI, (int)ns.size()]];
}



-(void)disconnect{

	//[presetsMenu removeAllItems];
	//[groupsMenu removeAllItems];
	client->disconnect();
	[self cleanUpGUIParams];
	connected = false;
}

-(void) connect{

	bool OK = client->setup([address UTF8String], [port intValue]); //test
	client->connect();
	needFullParamsUpdate = YES; //before connect, always!
	connected = TRUE;

//	NSString * date = [[NSDate date] descriptionWithCalendarFormat:@"%H:%M:%S" timeZone:nil locale:nil];

//	if (!connected){ //we are not connected, let's connect

//		int port = [portField.stringValue intValue];
//		bool OK = client->setup([addressField.stringValue UTF8String], port);
//		if (!OK){//most likely no network inerfaces available!
//			NSLog(@"Can't Setup ofxRemoteUI Client! Most likely no network interfaces available!");
//			[self showNotificationWithTitle:@"Cant Setup ofxRemoteUI Client!"
//								description:@"No Network Interface available?"
//										 ID:@"CantSetupClient"
//								   priority:2];
//			return;
//		}
//
//		[addressField setEnabled:false];
//		[portField setEnabled:false];
//		connectButton.title = DISCONNECT_STRING;
//		connectButton.state = 1;
//		connected = true;
//		printf("ofxRemoteUIClientOSX Connecting to %s\n", [addressField.stringValue UTF8String] );
//		[updateFromServerButton setEnabled: true];
//		[updateContinuouslyCheckbox setEnabled: true];
//		[statusImage setImage:nil];
//		//first load of vars
//		[self pressedSync:nil];
//		[self performSelector:@selector(pressedSync:) withObject:nil afterDelay:REFRESH_RATE];
//		[progress startAnimation:self];
//		connecting = TRUE;
//		needFullParamsUpdate = YES;
//		client->connect();
//		[logs appendToServerLog:[NSString stringWithFormat:@"%@ >> ## CLIENT CONNECTED ###################\n", date]];
//
//	}else{ // let's disconnect
//
//		RemoteUIClientCallBackArg arg;
//		arg.action = SERVER_DISCONNECTED;
//		arg.host = [addressField.stringValue UTF8String];
//		[logs log:arg];
//		arg.host = "offline";
//		[presetsMenu removeAllItems];
//		[groupsMenu removeAllItems];
//		[addressField setEnabled:true];
//		[portField setEnabled:true];
//		[updateFromServerButton setEnabled: false];
//		[updateContinuouslyCheckbox setEnabled:false];
//		if ([statusImage image] != [NSImage imageNamed:@"offline"])
//			[statusImage setImage:[NSImage imageNamed:@"offline"]];
//		[progress stopAnimation:self];
//		connecting = FALSE;
//		[self cleanUpGUIParams];
//		client->disconnect();
//		connectButton.state = 0;
//		connectButton.title = CONNECT_STRING;
//		[self layoutWidgetsWithConfig: [self calcLayoutParams]]; //update scrollbar
//		[logs appendToServerLog:[NSString stringWithFormat:@"%@ >> ## CLIENT DISCONNECTED ###################\n", date]];
//	}
}


-(void)update{

	if (client->isSetup()){
		//NSLog(@"update");
		client->updateAutoDiscovery(REFRESH_RATE);
		client->update(REFRESH_RATE);
	}

}

-(ofxRemoteUIClient *)getClient;{
	return client;
}



- (void)didReceiveMemoryWarning{
    [super didReceiveMemoryWarning];
}


#pragma mark collectionView

- (NSInteger)collectionView:(UICollectionView *)collectionView numberOfItemsInSection:(NSInteger)section{
    return paramViews.count;
}


- (void)didRotateFromInterfaceOrientation:(UIInterfaceOrientation)fromInterfaceOrientation{
    [self.collectionView performBatchUpdates:nil completion:nil];
}

- (CGSize)collectionView:(UICollectionView *)collectionView
                  layout:(UICollectionViewLayout *)collectionViewLayout
  sizeForItemAtIndexPath:(NSIndexPath *)indexPath{

	CGRect bounds = [[UIScreen mainScreen] bounds]; // portrait bounds
	if (UIInterfaceOrientationIsLandscape([[UIApplication sharedApplication] statusBarOrientation])) {
		bounds.size = CGSizeMake(bounds.size.height, bounds.size.width);
	}

	float minW = 240;
	if ( UI_USER_INTERFACE_IDIOM() == UIUserInterfaceIdiomPad ){
		minW = 320;
	}
	int nc = 0;
	float w = FLT_MAX;
	while (w >= minW) {
		nc++;
		w = bounds.size.width / nc ;
	}
	nc--;
	w = (bounds.size.width / nc) - nc + 1;

	float ww = w;
	if (nc == 1){
		ww = bounds.size.width;
	}

	//NSLog(@"sw: %f w: %f nc: %d >> ww: %f",bounds.size.width, ww, nc, ww);
    return CGSizeMake(ww , 50.0f);
}



- (UICollectionViewCell *)collectionView:(UICollectionView *)collectionView cellForItemAtIndexPath:(NSIndexPath *)indexPath{

    static NSString *identifier = @"Cell";

//	CGRect bounds = [[UIScreen mainScreen] bounds]; // portrait bounds
//	if (UIInterfaceOrientationIsLandscape([[UIApplication sharedApplication] statusBarOrientation])) {
//		bounds.size = CGSizeMake(bounds.size.height, bounds.size.width);
//	}
//
//
//	float minW = 240;
//	int nc = 0;
//	float w = FLT_MAX;
//	while (w >= minW) {
//		nc++;
//		w = bounds.size.width / nc ;
//	}nc--;
//	int numR = (int)[paramViews count] / nc;
//
//	//NSLog(@"ww = %d  hh = %d", nc, numR);
//
//	//todo this leaks! a lot!
//	int** map = (int**) malloc( sizeof(int*) * nc);
//	for(int i = 0; i < nc; i++){
//		map[i] = (int*) malloc(sizeof(int) * numR);
//	}
//
//	//all this is to change the flow direction; i want vertical scroll BUT vertical flow/grow too
//	//so im remapping the numbers to what they would be if thhe flow was shifted
//	int x = 0;
//	int y = 0;
//	for(int i = 0; i < [paramViews count]; i++){
//		map[x][y] = i;
//		//printf("map[%d][%d] = %d\n", x, y, i);
//		y++;
//		if (y >= numR){
//			y = 0;
//			x++;
//		}
//	}
//	NSLog(@"section: %d, row: %d, len: %d", indexPath.section, indexPath.row , indexPath.length);
//
//	int xx = indexPath.row % nc;
//	int yy = indexPath.row / nc;
//	int remapedIndex = map[xx][yy];
//	printf("original: %d >> remapedIndex = %d >> map[%d][%d]\n",indexPath.row, remapedIndex,  xx, yy);

	//NSIndexPath *ind = [NSIndexPath indexPathWithIndexes: ((const NSUInteger [])[indexPath getIndexes]) length: indexPath.length];

	//NSLog(@"section: %d, row: %d, len: %d", indexPath.section, indexPath.row , indexPath.length);
	UICollectionViewCell *cell = [collectionView dequeueReusableCellWithReuseIdentifier:identifier forIndexPath:indexPath];

	cell.opaque = YES;
//	UIView * v = [[cell subviews] objectAtIndex:0];
//	int tag = [v tag];


	if ([paramViews count] > 0){

		//[v setTag:1];
		ParamUI * paramView = [paramViews objectAtIndex:indexPath.row];
		//[paramView setNeedsLayout];
		[cell addSubview: [paramView getView]];
		//[cell setAutoresizingMask:UIViewAutoresizingFlexibleWidth];
		//if (![paramView hasBeenSetup]){
			CGRect f = [cell frame];
			f.origin = CGPointMake(0, 0);
			[[paramView getView] setFrame: f ];
			[paramView setup];
			//NSLog(@"making new View ");
		//}
	}
	//NSLog(@"addSubview %@ from %d",[recipeImages objectAtIndex:indexPath.row], indexPath.row);
	return cell;
}

@end
