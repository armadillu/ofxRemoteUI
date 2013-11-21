#include "testApp.h"

bool hasEnding(std::string const &fullString, std::string const &ending){
    if (fullString.length() >= ending.length()) {
        return (0 == fullString.compare (fullString.length() - ending.length(), ending.length(), ending));
    } else {
        return false;
    }
}

//ofxRemoteUIClient callback entry point
void clientCallback(RemoteUIClientCallBackArg a){

	a.host.c_str();
	cout << "callback" << endl;

	switch (a.action) {

		case SERVER_CONNECTED:{
			cout << "SERVER_CONNECTED" << endl;
			//[me showNotificationWithTitle:@"Connected to Server" description:remoteIP ID:@"ConnectedToServer" priority:-1];
		}break;

		case SERVER_DELETED_PRESET:{
			//[me showNotificationWithTitle:@"Server Deleted Preset OK" description:[NSString stringWithFormat:@"%@ deleted preset named '%s'", remoteIP, a.msg.c_str()] ID:@"ServerDeletedPreset" priority:2];
		}break;

		case SERVER_SAVED_PRESET:{
			//[me showNotificationWithTitle:@"Server Saved Preset OK" description:[NSString stringWithFormat:@"%@ saved preset named '%s'", remoteIP, a.msg.c_str()] ID:@"ServerSavedPreset" priority:2];
		}break;

		case SERVER_DID_SET_PRESET:{
			//[me hideAllWarnings];
			//[me showNotificationWithTitle:@"Server Did Set Preset OK" description:[NSString stringWithFormat:@"%@ did set preset named '%s'", remoteIP, a.msg.c_str()] ID:@"ServerDidSetPreset" priority:-1];
		}break;

		case SERVER_REQUESTED_ALL_PARAMS_UPDATE:
			cout << "SERVER_REQUESTED_ALL_PARAMS_UPDATE" << endl;
			//NSLog(@"## Callback: PARAMS_UPDATED");
			//if(me->needFullParamsUpdate){ //a bit ugly here...
				//[me fullParamsUpdate];
				//me->needFullParamsUpdate = NO;
			//}
			//[me partialParamsUpdate];
			//[me updateGroupPopup];
			break;

		case SERVER_PRESETS_LIST_UPDATED:{
			cout << "SERVER_PRESETS_LIST_UPDATED" << endl;
			//NSLog(@"## Callback: PRESETS_UPDATED");
			//vector<string> list = [me getClient]->getPresetsList();
			//if ( list.size() > 0 ){
				//[me updatePresetsPopup];
			//}
		}break;

		case SERVER_DISCONNECTED:{
			cout << "SERVER_DISCONNECTED" << endl;
			//[me connect];
			//me->client->disconnect();
			//[me showNotificationWithTitle:@"Server Exited, Disconnected!" description:remoteIP ID:@"ServerDisconnected" priority:-1];
			//[me updateGroupPopup];
			//[me updatePresetsPopup];
		}break;

		case SERVER_CONFIRMED_SAVE:{
			//NSString * s = [NSString stringWithFormat:@"%@ - Default XML now holds the current param values", remoteIP];
			//[me showNotificationWithTitle:@"Server Saved OK" description:s ID:@"CurrentParamsSavedToDefaultXML" priority:2];
		}break;

		case SERVER_DID_RESET_TO_XML:{
			//NSString * s = [NSString stringWithFormat:@"%@ - Params are reset to Server-Launch XML values", remoteIP];
			//[me showNotificationWithTitle:@"Server Did Reset To XML OK" description:s ID:@"ServerDidResetToXML" priority:1];
		}break;

		case SERVER_DID_RESET_TO_DEFAULTS:{
			//NSString * s = [NSString stringWithFormat:@"%@ - Params are reset to its Share-Time values (Source Code Defaults)", remoteIP];
			//[me showNotificationWithTitle:@"Server Did Reset To Default OK" description:s ID:@"ServerDidResetToDefault" priority:1];
		}break;

		case SERVER_REPORTS_MISSING_PARAMS_IN_PRESET:{
			//printf("SERVER_REPORTS_MISSING_PARAMS_IN_PRESET\n");
			for(int i = 0; i < a.paramList.size(); i++){
				//ParamUI* t = me->widgets[ a.paramList[i] ];
				//[t flashWarning:[NSNumber numberWithInt:NUM_FLASH_WARNING]];
			}
		}

		case NEIGHBORS_UPDATED:{
			cout << "NEIGHBORS_UPDATED" << endl;
			//[me updateNeighbors];
		}
			break;
		default:
			break;
	}

	if( a.action != SERVER_DISCONNECTED ){ //server disconnection is logged from connect button press
		//[me log:a];
	}
}


//--------------------------------------------------------------
void testApp::setup(){

	ofSetFrameRate(60);
	ofBackground(22);

	client = new ofxRemoteUIClient();
	client->setCallback(clientCallback);

	client->setup("192.168.1.24", 10000);
	client->connect();

    float xInit = OFX_UI_GLOBAL_WIDGET_SPACING;
    float dim = 32;
    float length = 320-xInit;
	//    length*=.25;

    gui = new ofxUIScrollableCanvas(0,0,ofGetWidth(),ofGetHeight());
	gui->setDrawBack(false);
    gui->setScrollAreaToScreen();
    gui->setScrollableDirections(false, true);

    ofAddListener(gui->newGUIEvent, this, &testApp::guiEvent);

}

//--------------------------------------------------------------
void testApp::update(){
	float dt = 1./60.;
	client->update(dt);

}

//--------------------------------------------------------------
void testApp::draw(){

}


void testApp::fullParamsUpdate(){

	vector<string> paramList = client->getAllParamNamesList();
	if(paramList.size() > 0 /*&& updatedParamsList.size() > 0*/){

		gui->removeWidgets();
		int c = 0;


		for(int i = 0; i < paramList.size(); i++){

			string paramName = paramList[i];

			RemoteUIParam p = client->getParamForName(paramName);
			switch(p.type){
				case REMOTEUI_PARAM_FLOAT:
				gui->addSlider(paramName, p.minFloat, p.maxFloat, p.floatVal);
				break;

			case REMOTEUI_PARAM_INT:
				gui->addIntSlider(paramName, p.minInt, p.maxInt, p.intVal);
				break;

			case REMOTEUI_PARAM_BOOL:
				gui->addToggle(paramName, p.boolVal);
				break;

			case REMOTEUI_PARAM_COLOR:
				gui->addIntSlider(paramName + ".red", 0, 255, p.redVal);
				gui->addIntSlider(paramName + ".green", 0, 255, p.greenVal);
				gui->addIntSlider(paramName + ".blue", 0, 255, p.blueVal);
					gui->addIntSlider(paramName + ".alpha", 0, 255, p.alphaVal);
				break;

			case REMOTEUI_PARAM_STRING:
				gui->addTextInput(paramName, p.stringVal)->setAutoClear(false);
				break;

			case REMOTEUI_PARAM_ENUM:{
				vector<string> items;
				for(int i = 0; i < p.enumList.size(); i++){
					items.push_back(p.enumList[i]);
				}
				gui->addDropDownList(paramName, items);
				}break;

			}
		}
		gui->autoSizeToFitWidgets();
		gui->getRect()->setWidth(ofGetWidth());
	}
}

void testApp::guiEvent(ofxUIEventArgs &e){

	string paramName = e.widget->getName();
	int colorComponent = 0;
	if( hasEnding(paramName, ".red") ){
		paramName = paramName.substr(0, paramName.size() - 4);
		colorComponent = 1;
	}
	if( hasEnding(paramName, ".green") ){
		paramName = paramName.substr(0, paramName.size() - 6);
		colorComponent = 2;
	}
	if( hasEnding(paramName, ".blue") ){
		paramName = paramName.substr(0, paramName.size() - 5);
		colorComponent = 3;
	}
	if( hasEnding(paramName, ".alpha") ){
		paramName = paramName.substr(0, paramName.size() - 6);
		colorComponent = 4;
	}

	RemoteUIParam p = client->getParamForName(paramName);
	switch(p.type){
		case REMOTEUI_PARAM_FLOAT:{
			ofxUISlider *slider = (ofxUISlider *) e.widget;
			p.floatVal = slider->getScaledValue();
		}break;

		case REMOTEUI_PARAM_INT:{
			ofxUIIntSlider *slider = (ofxUIIntSlider *) e.widget;
			p.intVal = slider->getValue();
		}break;

		case REMOTEUI_PARAM_BOOL:{
			ofxUIToggle * t = (ofxUIToggle*) e.widget;
			p.boolVal = t->getValue();
		}break;

		case REMOTEUI_PARAM_COLOR:{
			ofxUIIntSlider *slider = (ofxUIIntSlider *) e.widget;
			switch (colorComponent) {
				case 1:
					p.redVal = slider->getValue();
					break;
				case 2:
					p.greenVal = slider->getValue();
					break;
				case 3:
					p.blueVal = slider->getValue();
					break;
				case 4:
					p.alphaVal = slider->getValue();
					break;
				}
			}break;

		case REMOTEUI_PARAM_STRING:{
			ofxUITextInput *inp = (ofxUITextInput*) e.widget;
			p.stringVal = inp->getTextString();
			}break;

		case REMOTEUI_PARAM_ENUM:{
		}break;
	}
	client->sendUntrackedParamUpdate(p, paramName);
}


//--------------------------------------------------------------
void testApp::keyPressed(int key){

	fullParamsUpdate();

}

//--------------------------------------------------------------
void testApp::keyReleased(int key){

}

//--------------------------------------------------------------
void testApp::mouseMoved(int x, int y ){

}

//--------------------------------------------------------------
void testApp::mouseDragged(int x, int y, int button){

}

//--------------------------------------------------------------
void testApp::mousePressed(int x, int y, int button){

}

//--------------------------------------------------------------
void testApp::mouseReleased(int x, int y, int button){

}

//--------------------------------------------------------------
void testApp::windowResized(int w, int h){


	fullParamsUpdate();
	gui->setDimensions(w, h);
	//gui->setScrollAreaToScreen();
    //gui->autoSizeToFitWidgets();
	//gui->centerWidgets();
}

//--------------------------------------------------------------
void testApp::gotMessage(ofMessage msg){

}

//--------------------------------------------------------------
void testApp::dragEvent(ofDragInfo dragInfo){ 

}
