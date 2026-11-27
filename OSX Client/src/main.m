//
//  main.m
//  ofxRemoteUIClientOSX
//
//  Created by Oriol Ferrer Mesia on 8/28/11.
//  Copyright 2011 uri.cat. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#include <signal.h>

int main(int argc, char *argv[])
{
	// Match the ImGui client: an unavailable endpoint must not kill the app.
	signal(SIGPIPE, SIG_IGN);
	return NSApplicationMain(argc, (const char **)argv);
}
