
#include "ofxRemoteUI.h"
#include "ItemCellView.h"
#import <Foundation/Foundation.h>
#include <string>


@interface Item : NSObject{

	@public
	RemoteUIParam param;
	string paramName;
	ItemCellView * cellView;
	NSControl * widget; //
}

-(id)initWithParam: (RemoteUIParam)p paramName:(string)name;
-(void)setCellView:(ItemCellView*)v;
-(void)updateValues:(RemoteUIParam)p;
-(void)updateUI;

-(void)disableChanges;
-(void)enableChanges;




-(IBAction)updateFloat:(id)sender;
-(IBAction)updateInt:(id)sender;
-(IBAction)updateBool:(id)sender;
-(IBAction)updateString:(id)sender;
@end
