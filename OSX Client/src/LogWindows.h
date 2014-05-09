//
//  LogWindows.h
//  ofxRemoteUIClientOSX
//
//  Created by Oriol Ferrer Mesià on 19/04/14.
//
//

#import <Foundation/Foundation.h>
#include "ofxRemoteUIClient.h"
#import "ParamUI.h"


@interface LogWindows : NSObject{

	IBOutlet NSTextView *			logView;
	IBOutlet NSTextView *			serverLogView;
}


-(void)log:(RemoteUIClientCallBackArg) arg;
-(IBAction)clearLog:(id)sender;
-(IBAction)clearServerLog:(id)sender;
-(void)appendToServerLog:(NSString*)line ;
-(void)appendToLog:(NSString*) line;


@end
