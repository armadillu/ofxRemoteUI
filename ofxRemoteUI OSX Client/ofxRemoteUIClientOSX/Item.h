
#include "ofxRemoteUI.h"
#include "ItemCellView.h"
#import <Foundation/Foundation.h>
#include <string>


@interface Item : NSObject{

	RemoteUIParam param;
	string paramName;
	ItemCellView * cellView;
	NSControl * widget; //
}

-(id)initWithParam: (RemoteUIParam)p paramName:(string)name;
-(void)setCellView:(ItemCellView*)v;
-(void)updateValues:(RemoteUIParam)p;
-(void)updateUI;





-(IBAction)updateFloat:(id)sender;
-(IBAction)updateInt:(id)sender;
-(IBAction)updateBool:(id)sender;
-(IBAction)updateString:(id)sender;
@end
