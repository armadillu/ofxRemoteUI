//
//  constants.h
//  ofxRemoteUIClientOSX
//
//  Created by Oriol Ferrer Mesià on 30/03/15.
//
//

#ifndef ofxRemoteUIClientOSX_constants_h
#define ofxRemoteUIClientOSX_constants_h

#define REFRESH_RATE			1.0f/15.0f
#define STATUS_REFRESH_RATE		0.333f
#define ROW_HEIGHT				(rowHeight == LARGE_34 ? 34.0f : (rowHeight == TINY_20 ? 20.0f : 26.0f))
#define ROW_WIDTH				220.0f
#define ALL_PARAMS_GROUP		@"*All Params"
#define DIRTY_PRESET_NAME		@"*No Preset"
#define NUM_FLASH_WARNING		5
#define NUM_BOUND_FLASH			15
#define MAIN_WINDOW_NON_LIST_H (82 + 66)

#define DEFAULT_BINDINGS_FOLDER ([NSString stringWithFormat:@"%@/Library/Application Support/ofxRemoteUIClient/",NSHomeDirectory()])
#define DEFAULT_BINDINGS_FILE (@"lastUsedBindings.ctrlrBind")

#define CONNECT_STRING		@"Connect"
#define DISCONNECT_STRING	@"Disconnect"

#endif
