//
//  LogWindows.m
//  ofxRemoteUIClientOSX
//
//  Created by Oriol Ferrer Mesià on 19/04/14.
//
//

#import "LogWindows.h"

@implementation LogWindows

-(id)init{

	NSLog(@"Init LogWindows");

	NSFont * font = [NSFont fontWithName:@"Monaco" size:10];
	[[serverLogView textStorage] setFont:font];
	[[logView  textStorage] setFont:font];
	[self appendToServerLog:@"\n"];
	[self appendToLog:@"\n"];

	return self;
}
-(void)log:(RemoteUIClientCallBackArg) arg{

	if (
		arg.action == SERVER_SENT_FULL_PARAMS_UPDATE ||
		arg.action == SERVER_PRESETS_LIST_UPDATED ||
		arg.action == NEIGHBORS_UPDATED ||
		arg.action == NEIGHBOR_JUST_LAUNCHED_SERVER ||
		arg.action == SERVER_SENT_LOG_LINE
		){
		return; //this stuff is not worth logging
	}

	NSString * action = @"";
	switch (arg.action) {
		case SERVER_CONNECTED: action = @"Connected To Server!";  break;
		case SERVER_DISCONNECTED: action = @"Server Disconnected!"; break;
		case SERVER_SENT_FULL_PARAMS_UPDATE: action = @"Server Requested all Params Update!"; break;
		case SERVER_PRESETS_LIST_UPDATED: action = @"Server Presets lists updated!"; break;
		case SERVER_DELETED_PRESET: action = [NSString stringWithFormat:@"Server Deleted Preset named '%s'", arg.msg.c_str()]; break;
		case SERVER_SAVED_PRESET:  action = [NSString stringWithFormat:@"Server Saved Preset named '%s'", arg.msg.c_str()]; break;
		case SERVER_DID_SET_GROUP_PRESET: action = [NSString stringWithFormat:@"Server did set Group Preset named '%s'", arg.msg.c_str()]; break;
		case SERVER_DELETED_GROUP_PRESET: action = [NSString stringWithFormat:@"Server Deleted Group Preset named '%s'", arg.msg.c_str()]; break;
		case SERVER_SAVED_GROUP_PRESET:  action = [NSString stringWithFormat:@"Server Saved Group Preset named '%s'", arg.msg.c_str()]; break;
		case SERVER_DID_SET_PRESET: action = [NSString stringWithFormat:@"Server did set Preset named '%s'", arg.msg.c_str()]; break;
		case SERVER_CONFIRMED_SAVE: action = @"Server Did Save to Default XML"; break;
		case SERVER_DID_RESET_TO_XML: action = @"Server Did Reset Params to Server-Launch Default XML"; break;
		case SERVER_DID_RESET_TO_DEFAULTS: action = @"Server Did Reset Params to Share-Time (Source Code)"; break;
		case NEIGHBORS_UPDATED: action = @"Server found nearby neighbors"; break;
		case SERVER_REPORTS_MISSING_PARAMS_IN_PRESET: action = @"Server loaded preset, but some params are not specified in it (old preset)"; break;
		default: NSLog(@"log switch missing RemoteUICallClientAction!");
	}

	NSString * date = [[NSDate date] descriptionWithCalendarFormat:@"%H:%M:%S" timeZone:nil locale:nil];
	NSString * logLine = [NSString stringWithFormat:@"[%@@%s] %@\n", date, arg.host.c_str(), action ];

	[self appendToLog:logLine];
}

-(IBAction)clearLog:(id)sender;{
	[[logView textStorage] beginEditing];
    [[[logView textStorage] mutableString] setString:@""];
    [[logView textStorage] endEditing];
}

-(IBAction)clearServerLog:(id)sender{
	[[serverLogView textStorage] beginEditing];
    [[[serverLogView textStorage] mutableString] setString:@""];
    [[serverLogView textStorage] endEditing];
}

-(void)appendToServerLog:(NSString*)line{

	[[serverLogView textStorage] beginEditing];
    [[[serverLogView textStorage] mutableString] appendString:line];
    [[serverLogView textStorage] endEditing];
}

-(void)appendToLog:(NSString*)line{
	[[logView textStorage] beginEditing];
	[[[logView textStorage] mutableString] appendString:line];
	[[logView textStorage] endEditing];
}

@end
