#include "testApp.h"

bool hasEnding(std::string const &fullString, std::string const &ending){
    if (fullString.length() >= ending.length()) {
        return (0 == fullString.compare (fullString.length() - ending.length(), ending.length(), ending));
    } else {
        return false;
    }
}

std::vector<std::string> &split(const std::string &s, char delim, std::vector<std::string> &elems) {
    std::stringstream ss(s);
    std::string item;
    while (std::getline(ss, item, delim)) {
        elems.push_back(item);
    }
    return elems;
}

std::vector<std::string> split(const std::string &s, char delim) {
    std::vector<std::string> elems;
    split(s, delim, elems);
    return elems;
}

//--------------------------------------------------------------*---------------------------------------

void testApp::setup(){

	retinaScale = 1.0;

	ofSetFrameRate(60);
	ofBackground(22);
	ofSetWindowTitle("ofxRemoteUI Client");

	gui = NULL;
	staticUI = presetNameUI = NULL;
	needFullParamUpdate = false;

	client = new ofxRemoteUIClient();
	client->setCallback(clientCallback);
	client->setVerbose(true);

	//client->setup("192.168.1.24", 10000);
	//client->connect();

    float xInit = OFX_UI_GLOBAL_WIDGET_SPACING;
    float dim = 32;
    float length = 320-xInit;
	//    length*=.25;


	allocUI(); //dynamic
	prepareStaticUI();

}


void testApp::preparePresetUI(){

	float h = 80;
	float w = ofGetWidth() * 0.8;
	float widgetH = 40;

	if(presetNameUI) delete presetNameUI;
	presetNameUI = new ofxUICanvas( (ofGetWidth() - w )/2.0, ofGetHeight()/2 -h/2, w , h );
	presetNameUI->setDrawOutline(true);
	presetNameUI->setColorBack(ofxUIColor(64,128));
	presetNameUI->setColorFill(ofxUIColor(255,128));

	presetNameUI->addWidgetDown(new ofxUILabel("TYPE IN NEW PRESET NAME", OFX_UI_FONT_MEDIUM));
	presetNameUI->setWidgetFontSize(OFX_UI_FONT_LARGE);
	ofxUITextInput* ti = presetNameUI->addTextInput("TEXT INPUT", "myPreset", 0);
	ti->setDrawOutline(true);

	presetNameUI->setWidgetFontSize(OFX_UI_FONT_SMALL);
	presetNameUI->addLabelButton("CANCEL", false, 80.0, widgetH);
	presetNameUI->setWidgetPosition(OFX_UI_WIDGET_POSITION_RIGHT);
	presetNameUI->addLabelButton("MAKE NEW PRESET", false, 170.0, widgetH);
	presetNameUI->setWidgetPosition(OFX_UI_WIDGET_POSITION_DOWN);

	presetNameUI->centerWidgetsOnCanvas(true, true);

	ofAddListener(presetNameUI->newGUIEvent, this, &testApp::presetGuiEvent);

}


void testApp::prepareStaticUI(){

	// STATIC UI ///////////////////
	staticUI = new ofxUICanvas( EDGE_SPACE, EDGE_SPACE, retinaScale * ( ofGetWidth() - 2 * EDGE_SPACE), STATIC_UI_H  - 2 * EDGE_SPACE);

#ifdef TARGET_OF_IOS
	staticUI->setRetinaResolution();
	if ( [[UIScreen mainScreen] scale] > 1 ){
		retinaScale = 2.0;
	}
#endif


	staticUI->setFontSize(OFX_UI_FONT_SMALL, retinaScale * 5);
	staticUI->setFontSize(OFX_UI_FONT_MEDIUM, retinaScale * 5);
	staticUI->setFontSize(OFX_UI_FONT_LARGE, retinaScale * 8);


	//	staticUI->setDrawBack(true);
	staticUI->setColorBack(ofxUIColor(0,33,128,128));
	//
	//	staticUI->setDrawFill(true);
	staticUI->setColorFill(ofxUIColor(255,128));
	//
	//	staticUI->setDrawFillHighLight(true);
	//	staticUI->setColorFillHighlight(ofxUIColor(0,128,0,255));
	//
	//	staticUI->setDrawPaddingOutline(true);
	//	staticUI->setColorPaddedOutline(ofxUIColor(255,255,0,255));
	//
	//	staticUI->setDrawPadding(true);
	//	staticUI->setColorPadded(ofxUIColor(128,0,0));
	//
	//	staticUI->setDrawOutline(true);
	//	staticUI->setColorOutline(ofxUIColor(0,255,255));
	//
	//	staticUI->setDrawOutlineHighLight(true);
	//	staticUI->setColorOutlineHighlight(ofxUIColor(255,0,255));
	//

	//	staticUI->addLabel("host");
	//	staticUI->addTextInput("hostField", "");
	//	staticUI->addLabel("port");
	//	staticUI->addTextInput("portField", "");


	ofxUILabelButton * save = staticUI->addLabelButton("SAVE TO XML", false, retinaScale * 100.0, WIDGET_H);
	save->setColorBack(ofxUIColor(255,44,44,128));
	staticUI->setWidgetPosition(OFX_UI_WIDGET_POSITION_RIGHT);
	staticUI->addLabelButton("SYNC", false, retinaScale *  50.0, WIDGET_H)->setColorBack(ofxUIColor(33,255,33,128));;
	staticUI->addLabelButton("MAKE NEW PRESET", false, retinaScale * 120.0, WIDGET_H)->setColorBack(ofxUIColor(200,33,200,128));;
	staticUI->setWidgetPosition(OFX_UI_WIDGET_POSITION_DOWN);

	staticUI->addLabel("serverName", "not connected");

	vector<string> empty;
	ofxUIDropDownList * neigh = staticUI->addDropDownList("NEIGHBORS", empty,  (ofGetWidth() - retinaScale * 2 * EDGE_SPACE - retinaScale * 8));
	neigh->setColorBack(ofxUIColor(128,200));
	neigh->setAllowMultiple(false);
	neigh->setShowCurrentSelected(false);
	neigh->setAutoClose(true);
	neigh->setModal(false);


	//staticUI->setWidgetPosition(OFX_UI_WIDGET_POSITION_RIGHT);
	ofxUIDropDownList * presets = staticUI->addDropDownList("PRESETS", empty,  (ofGetWidth() - retinaScale * 2 * EDGE_SPACE - retinaScale * 8));
	presets->setColorBack(ofxUIColor(0,160,160,255));
	presets->setAllowMultiple(false);
	presets->setShowCurrentSelected(true);
	presets->setAutoClose(true);
	presets->setModal(true);
	staticUI->setWidgetPosition(OFX_UI_WIDGET_POSITION_DOWN);

	ofAddListener(staticUI->newGUIEvent, this, &testApp::staticGuiEvent);
}




void testApp::allocUI(){
	// DYNAMIC UI //////////////////
	if(gui !=NULL) delete gui;
    gui = new ofxUIScrollableCanvas(EDGE_SPACE, STATIC_UI_H - 15 , ofGetWidth() - 2 * EDGE_SPACE, ofGetHeight() - STATIC_UI_H);

#ifdef TARGET_OF_IOS
	//gui->setRetinaResolution();
#endif

	gui->setDrawBack(false);
	gui->setDrawWidgetPadding(false);
	gui->setDrawFill(false);
	gui->setColorFill(ofxUIColor(255));
	gui->setColorBack(ofxUIColor(44));
	gui->setColorPadded(ofxUIColor(22));
	gui->setColorOutline(ofxUIColor(0,0,255));
	gui->setColorOutline(ofxUIColor(255));

	gui->setFontSize(OFX_UI_FONT_SMALL, 5);
	gui->setFontSize(OFX_UI_FONT_MEDIUM, 7);
	gui->setFontSize(OFX_UI_FONT_LARGE, 10);

	ofAddListener(gui->newGUIEvent, this, &testApp::guiEvent);

}

void testApp::savePresetConfirmed(string presetName){
	client->savePresetWithName(presetName);
}

void testApp::presetGuiEvent(ofxUIEventArgs &e){

	string paramName = e.widget->getName();
	bool doneWithScreen = false;

	if(paramName == "CANCEL"){
		//do nothing
		if (e.getButton()->getValue()){
			doneWithScreen = true;
		}
	}else
	if(paramName == "MAKE NEW PRESET"){

		if (e.getButton()->getValue()){
			ofxUITextInput * ti = (ofxUITextInput*)presetNameUI->getWidget("TEXT INPUT");
			savePresetConfirmed(ti->getTextString());
			doneWithScreen = true;
		}
	}

	if(doneWithScreen){
		staticUI->setVisible(true);
		gui->setVisible(true);
		presetNameUI->setVisible(false);
	}
}

void testApp::staticGuiEvent(ofxUIEventArgs &e){

	cout << "staticGuiEvent" << endl;
	string paramName = e.widget->getName();

	//USER CHOSE NEIGHBOR
	if(paramName == "NEIGHBORS"){
		ofxUIDropDownList *ddlist = (ofxUIDropDownList *) e.widget;
		vector<ofxUIWidget*> chosenList = ddlist->getSelected();
		if(chosenList.size() > 0){
			string selected = chosenList[0]->getName();
			cout << selected << endl;
			string server = neighborNames[selected];
			vector<string> comp = split(server, ':');
			string host = comp[0];
			string port = comp[1];

			//			((ofxUILabelToggle*)staticUI->getWidget("hostField"))->getLabel()->setLabel(host);
			//			((ofxUILabelToggle*)staticUI->getWidget("portField"))->getLabel()->setLabel(port);
 			client->disconnect();
			client->setup(host, ofToInt(port));
			needFullParamUpdate = true;
			client->connect();
			gui->removeWidgets();
			ofxUILabel * label = (ofxUILabel *) staticUI->getWidget("serverName");
			label->setLabel(selected);
		}
		if (e.getButton()->getValue()){
			gui->setVisible(false);
			staticUI->getWidget("PRESETS")->setVisible(false); //hide the params (whch are thenext dropdown) to avoid depth issues
		}else{
			gui->setVisible(true);
			staticUI->getWidget("PRESETS")->setVisible(true);
		}
	}else
	if(paramName == "SAVE TO XML"){

		if (e.getButton()->getValue()){ //save ¡n mouse Down
			client->saveCurrentStateToDefaultXML();
		}
	}else
	if(paramName == "SYNC"){

		if (e.getButton()->getValue()){
			client->requestCompleteUpdate(); //both params and presets
		}
	}else
	if(paramName == "MAKE NEW PRESET"){

		if (e.getButton()->getValue()){
			staticUI->setVisible(false);
			gui->setVisible(false);
			preparePresetUI();
		}
	}else
	if(paramName == "PRESETS"){

		if (e.getButton()->getValue()){
			gui->setVisible(false);
		}else{
			gui->setVisible(true); //user chose one of the presets (supposedly)
			ofxUIDropDownList * l = (ofxUIDropDownList *)e.widget;
			vector<ofxUIWidget *> selected = l->getSelected();
			if(selected.size() > 0){
				string selectedPreset = selected[0]->getName();
				needFullParamUpdate = true;
				client->setPreset(selectedPreset);
			}
		}
	}
}

void testApp::guiEvent(ofxUIEventArgs &e){

	string paramName = e.widget->getName();
	int colorComponent = 0;
	if( hasEnding(paramName, ".r") ){
		paramName = paramName.substr(0, paramName.size() - 2);
		colorComponent = 1;
	}
	if( hasEnding(paramName, ".g") ){
		paramName = paramName.substr(0, paramName.size() - 2);
		colorComponent = 2;
	}
	if( hasEnding(paramName, ".b") ){
		paramName = paramName.substr(0, paramName.size() - 2);
		colorComponent = 3;
	}
	if( hasEnding(paramName, ".a") ){
		paramName = paramName.substr(0, paramName.size() - 2);
		colorComponent = 4;
	}

	RemoteUIParam p = client->getParamForName(paramName);

	switch(p.type){
		case REMOTEUI_PARAM_UNKNOWN:
			cout << "wtf! " << endl;
			break;

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
			ofxUIDropDownList *ddlist = (ofxUIDropDownList *) e.widget;
			vector<int> &selected = ddlist->getSelectedIndeces();
			if(selected.size() > 0){
				int selectedIndex = selected[0];
				p.intVal = p.minInt + selectedIndex;
			}

		}break;
	}

	if(p.type != REMOTEUI_PARAM_UNKNOWN){
		client->sendUntrackedParamUpdate(p, paramName);
	}
}


//--------------------------------------------------------------
void testApp::update(){
	float dt = 1./60.;
	client->update(dt);
	client->updateAutoDiscovery(dt);

}

//--------------------------------------------------------------
void testApp::draw(){

}


void testApp::updatePresets(){

	vector<string> ns = client->getPresetsList();
	ofxUIDropDownList * list = (ofxUIDropDownList *)staticUI->getWidget("PRESETS");

	if(list){
		list->clearToggles();
		for(int i = 0; i < ns.size(); i++){
			list->addToggle( ns[i] );
		}

		vector<ofxUILabelToggle*> ts = list->getToggles();

		for(int i = 0; i  < ts.size(); i++){
			ts[i]->setColorBack(ofxUIColor(0,200,200,200));
		}
	}
}


void testApp::updateNeighbors(){

	vector<Neighbor> ns = client->getNeighbors();
	ofxUIDropDownList * list = (ofxUIDropDownList *)staticUI->getWidget("NEIGHBORS");

	if(list){
		neighborNames.clear();
		list->clearToggles();
		for(int i = 0; i < ns.size(); i++){
			string address = ns[i].IP + ":" + ofToString(ns[i].port);
			string name = ns[i].binary + " @ " +  ns[i].name ;
			neighborNames[name] = address; //store for easy access later
			list->addToggle( name );
		}

		vector<ofxUILabelToggle*> ts = list->getToggles();

		for(int i = 0; i  < ts.size(); i++){
			ts[i]->setColorBack(ofxUIColor(50,255));
		}
	}
}


void testApp::fullParamsUpdate(){

	vector<string> paramList = client->getAllParamNamesList();
	if(paramList.size() > 0 /*&& updatedParamsList.size() > 0*/){

		allocUI();
		gui->removeWidgets();
		gui->addSpacer(ofGetWidth(), GROUP_SPACE_H);
		string group = "randomcrapname";

		for(int i = 0; i < paramList.size(); i++){

			string paramName = paramList[i];
			RemoteUIParam p = client->getParamForName(paramName);

			if(p.type == REMOTEUI_PARAM_UNKNOWN){
				cout << "REMOTEUI_PARAM_UNKNOWN" << endl;
			}
			if(group != p.group){
				cout << "new group!" << endl;
				group = p.group;

				if(i != 0) gui->addSpacer(ofGetWidth(), GROUP_SPACE_H);
				ofxUIWidget * w = gui->addWidgetDown(new ofxUILabel(group, OFX_UI_FONT_LARGE));
				w->setColorPadded(GROUP_BG_COLOR);
				w->setDrawPadding(true);
				gui->addSpacer(ofGetWidth(), GROUP_SPACE_H);
			};

			switch(p.type){

				case REMOTEUI_PARAM_UNKNOWN:
					break;
				case REMOTEUI_PARAM_FLOAT:
				gui->addSlider(paramName, p.minFloat, p.maxFloat, p.floatVal, ofGetWidth() - retinaScale * 2 * EDGE_SPACE - retinaScale * 8, WIDGET_H);
				break;

				case REMOTEUI_PARAM_INT:
				gui->addIntSlider(paramName, p.minInt, p.maxInt, p.intVal);
				break;

				case REMOTEUI_PARAM_BOOL:
				gui->addToggle(paramName, p.boolVal);
				break;

				case REMOTEUI_PARAM_COLOR:{
				float h = 20;
				float w = ( ofGetWidth() )/ 6.0  ;
				gui->addWidgetDown(new ofxUILabel(paramName, OFX_UI_FONT_MEDIUM));
				//gui->addWidgetDown(new ofxUIMinimalSlider(w, h, 0, 255, p.redVal, paramName + ".r",OFX_UI_FONT_SMALL));

				gui->addIntSlider(paramName + ".r", 0, 255, p.redVal, w, h);
				gui->setWidgetPosition(OFX_UI_WIDGET_POSITION_RIGHT);
				gui->addIntSlider(paramName + ".g", 0, 255, p.greenVal, w, h);
				gui->addIntSlider(paramName + ".b", 0, 255, p.blueVal, w, h);
				gui->addIntSlider(paramName + ".a", 0, 255, p.alphaVal, w, h);
				gui->setWidgetPosition(OFX_UI_WIDGET_POSITION_DOWN);
				}break;

				case REMOTEUI_PARAM_STRING:

				gui->addWidgetDown(new ofxUILabel(paramName, OFX_UI_FONT_SMALL));
				gui->addTextInput(paramName, p.stringVal)->setAutoClear(false);
				break;

				case REMOTEUI_PARAM_ENUM:{
				vector<string> items;
				for(int i = 0; i < p.enumList.size(); i++){
					items.push_back(p.enumList[i]);
				}
				gui->addWidgetDown(new ofxUILabel(0,0, ofGetWidth()/3, paramName, OFX_UI_FONT_MEDIUM));
				gui->setWidgetPosition(OFX_UI_WIDGET_POSITION_RIGHT);
				ofxUIDropDownList * l = gui->addDropDownList(paramName, items, ofGetWidth()/2, 20.0, ofGetWidth()/2);
				gui->setWidgetPosition(OFX_UI_WIDGET_POSITION_DOWN);

				l->setAllowMultiple(false);
				l->setAutoClose(true);
				l->setShowCurrentSelected(true);
				l->setModal(false);
				l->setColorFill(ofxUIColor(88,0,0));

				vector<ofxUILabelToggle *> ts = l->getToggles();
				for(int i = 0; i < ts.size(); i++){
					if (p.intVal - p.minInt == i ){
						string option = ts[i]->getLabel()->getLabel();
						cout << "first shown: " << option << endl;
						l->activateToggle(option);
					}
					ts[i]->setColorBack(ofxUIColor(32, 255));
				}

				l->checkAndSetTitleLabel();
				//no matter how hard i try, ofxUI dropdown list wont allow to show/set the selected item unless you do it with the mouse.
				}break;

			}
			//gui->addSpacer(ofGetWidth() - 2 * EDGE_SPACE, 4);
		}
		gui->autoSizeToFitWidgets();
		//gui->getRect()->setWidth(ofGetWidth());
		//gui->update();
	}
}



//--------------------------------------------------------------
void testApp::keyPressed(int key){

	//fullParamsUpdate();
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
	gui->setDimensions(w - 2 * EDGE_SPACE, h - STATIC_UI_H);
	staticUI->setDimensions(w - 2 * EDGE_SPACE, STATIC_UI_H);
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



//ofxRemoteUIClient callback entry point
void clientCallback(RemoteUIClientCallBackArg a){

	a.host.c_str();
	cout << "callback" << endl;
	testApp * me = (testApp*)ofGetAppPtr();

	switch (a.action) {

		case SERVER_CONNECTED:{
			cout << "SERVER_CONNECTED" << endl;
			//[me showNotificationWithTitle:@"Connected to Server" description:remoteIP ID:@"ConnectedToServer" priority:-1];
		}break;

		case SERVER_DELETED_PRESET:{
			cout << "SERVER_DELETED_PRESET" << endl;
			//[me showNotificationWithTitle:@"Server Deleted Preset OK" description:[NSString stringWithFormat:@"%@ deleted preset named '%s'", remoteIP, a.msg.c_str()] ID:@"ServerDeletedPreset" priority:2];
		}break;

		case SERVER_SAVED_PRESET:{
			cout << "SERVER_SAVED_PRESET" << endl;
			//[me showNotificationWithTitle:@"Server Saved Preset OK" description:[NSString stringWithFormat:@"%@ saved preset named '%s'", remoteIP, a.msg.c_str()] ID:@"ServerSavedPreset" priority:2];
		}break;

		case SERVER_DID_SET_PRESET:{
			cout << "SERVER_DID_SET_PRESET" << endl;
			//[me hideAllWarnings];
			//[me showNotificationWithTitle:@"Server Did Set Preset OK" description:[NSString stringWithFormat:@"%@ did set preset named '%s'", remoteIP, a.msg.c_str()] ID:@"ServerDidSetPreset" priority:-1];
		}break;

		case SERVER_SENT_FULL_PARAMS_UPDATE:
			cout << "SERVER_SENT_FULL_PARAMS_UPDATE" << endl;
			//NSLog(@"## Callback: PARAMS_UPDATED");
			if(me->needFullParamUpdate){ //a bit ugly here...
				me->fullParamsUpdate();
				me->needFullParamUpdate = false;
			}
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
			me->updatePresets();
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
			cout << "SERVER_CONFIRMED_SAVE" << endl;
			//NSString * s = [NSString stringWithFormat:@"%@ - Default XML now holds the current param values", remoteIP];
			//[me showNotificationWithTitle:@"Server Saved OK" description:s ID:@"CurrentParamsSavedToDefaultXML" priority:2];
		}break;

		case SERVER_DID_RESET_TO_XML:{
			cout << "SERVER_DID_RESET_TO_XML" << endl;
			//NSString * s = [NSString stringWithFormat:@"%@ - Params are reset to Server-Launch XML values", remoteIP];
			//[me showNotificationWithTitle:@"Server Did Reset To XML OK" description:s ID:@"ServerDidResetToXML" priority:1];
		}break;

		case SERVER_DID_RESET_TO_DEFAULTS:{
			cout << "SERVER_DID_RESET_TO_DEFAULTS" << endl;
			//NSString * s = [NSString stringWithFormat:@"%@ - Params are reset to its Share-Time values (Source Code Defaults)", remoteIP];
			//[me showNotificationWithTitle:@"Server Did Reset To Default OK" description:s ID:@"ServerDidResetToDefault" priority:1];
		}break;

		case SERVER_REPORTS_MISSING_PARAMS_IN_PRESET:{
			cout << "SERVER_REPORTS_MISSING_PARAMS_IN_PRESET" << endl;
			for(int i = 0; i < a.paramList.size(); i++){
				//ParamUI* t = me->widgets[ a.paramList[i] ];
				//[t flashWarning:[NSNumber numberWithInt:NUM_FLASH_WARNING]];
			}
		}

		case NEIGHBORS_UPDATED:{
			cout << "NEIGHBORS_UPDATED" << endl;
			me->updateNeighbors();
		}
			break;
		default:
			break;
	}

//	if( a.action != SERVER_DISCONNECTED && a.action != SERVER_SENT_FULL_PARAMS_UPDATE ){ //server disconnection is logged from connect button press
//		//[me log:a];
//		me->allocUI();
//	}
}
