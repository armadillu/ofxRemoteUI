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
	updateContinuosly = false;

	//connect to last used server by default
	NSUserDefaults * df = [NSUserDefaults standardUserDefaults];
	NSLog(@"%@", [df stringForKey:@"lastAddress"]);
	if([df stringForKey:@"lastAddress"]) [addressField setStringValue:[df stringForKey:@"lastAddress"]];
	[self performSelector:@selector(connect) withObject:nil afterDelay:0.0];

	//first load of vars
	[self performSelector:@selector(pressedSync:) withObject:nil afterDelay:0.5];
}

-(NSString*)stringFromString:(string) s{
	return  [NSString stringWithCString:s.c_str() encoding:[NSString defaultCStringEncoding]];
}


-(void)syncLocalParamsToClientParams{ 

	vector<string> paramList = client.getAllParamNamesList();
	NSLog(@"Client holds %d params so far", (int) paramList.size());

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
		if ( it == widgets.end() ){	//not found!

			Item * row = [[Item alloc] initWithParam: p paramName: paramName];
			widgets[paramName] = row;
		}else{
			[widgets[paramName] updateValues:p];
			[widgets[paramName] updateUI]; // TODO only if needed!
		}

	}
	//[tableView reloadData];
}


-(IBAction)pressedSync:(id)sender;{
	client.requestCompleteUpdate();

	//delay a bit the screen update so taht we have gathered the values
	[self performSelector:@selector(syncLocalParamsToClientParams) withObject:nil afterDelay: 3 * REFRESH_RATE];
	[tableView performSelector:@selector(reloadData) withObject:nil afterDelay: 3 * REFRESH_RATE];
}

-(IBAction)pressedContinuously:(NSButton *)sender;{

	if ([sender state]) {
		updateContinuosly = true;
		[updateFromServerButton setEnabled: false];
	}else{
		updateContinuosly = false;
		[updateFromServerButton setEnabled: true];
	}
}


-(IBAction)pressedConnect:(id)sender{
	[self connect];
}



-(void) connect{
	//NSLog(@"connect!");
	NSUserDefaults * df = [NSUserDefaults standardUserDefaults];
	[df setObject: addressField.stringValue forKey:@"lastAddress"];

	if ([[connectButton title] isEqualToString:@"Connect"]){ //we are not connected
		[addressField setEnabled:false];
		connectButton.title = @"Connected";
		connectButton.state = 1;
		NSLog(@"ofxRemoteUIClientOSX Connecting to %@", addressField.stringValue);
		client.setup([addressField.stringValue UTF8String]);
	}else{
		[addressField setEnabled:true];
		connectButton.state = 0;
		connectButton.title = @"Connect";
	}

}

/////// OF ///////////////////////////

-(void)setup{
	//client.setup("127.0.0.1", 0.1);
}


-(void)update{

	client.update(REFRESH_RATE);

	if(updateContinuosly){
		client.requestCompleteUpdate();
		[self syncLocalParamsToClientParams];
		[tableView reloadData];
	}
}

-(void)userChangedParam:(RemoteUIParam)p paramName:(string)name{
	//NSLog(@"usr changed param! %s", name.c_str());
	printf("client sending: ");
	p.print();
	client.sendParamUpdate(p, name);
}


- (NSInteger)numberOfRowsInTableView:(NSTableView *)tableView {
	return widgets.size();
}


- (NSView *)tableView:(NSTableView *)myTableView viewForTableColumn:(NSTableColumn *)tableColumn row:(NSInteger)row {

	map<string, Item*>::iterator it = widgets.begin();
	std::advance(it, row);
	Item* item = it->second;
	//NSLog(@"viewForTableColumn %@", item);
	ItemCellView *result = [myTableView makeViewWithIdentifier:tableColumn.identifier owner:self];
	[item setCellView:result];
	[item updateUI];

	//NSLog(@"item: %@", item);
	//result.detailTextField.stringValue = item.itemKind;
	return result;
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
