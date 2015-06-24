//
//  RemoteParam.h
//  emptyExample
//
//  Created by Oriol Ferrer Mesià on 24/06/13.
//
//

#ifndef emptyExample_RemoteParam_h
#define emptyExample_RemoteParam_h

#include <stdio.h>
#include <sstream>

enum RemoteUICallClientAction{
	SERVER_CONNECTED, SERVER_DISCONNECTED, SERVER_SENT_FULL_PARAMS_UPDATE, SERVER_PRESETS_LIST_UPDATED,
	SERVER_CONFIRMED_SAVE, SERVER_DID_RESET_TO_DEFAULTS, SERVER_DELETED_PRESET,
	SERVER_SAVED_PRESET, SERVER_DID_RESET_TO_XML, SERVER_DID_SET_PRESET, SERVER_REPORTS_MISSING_PARAMS_IN_PRESET,
	NEIGHBORS_UPDATED, NEIGHBOR_JUST_LAUNCHED_SERVER,
	//group presets
	SERVER_SAVED_GROUP_PRESET, SERVER_DID_SET_GROUP_PRESET, SERVER_DELETED_GROUP_PRESET,
	SERVER_SENT_LOG_LINE
};

enum RemoteUICallServerAction{
	CLIENT_CONNECTED, CLIENT_DISCONNECTED, CLIENT_UPDATED_PARAM,
	CLIENT_DID_SET_PRESET, CLIENT_SAVED_PRESET, CLIENT_DELETED_PRESET,
	CLIENT_SAVED_STATE, CLIENT_DID_RESET_TO_XML, CLIENT_DID_RESET_TO_DEFAULTS,
	//group presets
	CLIENT_DID_SET_GROUP_PRESET, CLIENT_SAVED_GROUP_PRESET, CLIENT_DELETED_GROUP_PRESET
};

enum RemoteUIParamType{
	REMOTEUI_PARAM_UNKNOWN = 0,
	REMOTEUI_PARAM_FLOAT = 100,
	REMOTEUI_PARAM_INT,
	REMOTEUI_PARAM_BOOL,
	REMOTEUI_PARAM_STRING,
	REMOTEUI_PARAM_ENUM,
	REMOTEUI_PARAM_COLOR,
	REMOTEUI_PARAM_SPACER,
};

enum ActionType{
	HELO_ACTION, REQUEST_ACTION, SEND_PARAM_ACTION, CIAO_ACTION,
	TEST_ACTION, PRESET_LIST_ACTION, SET_PRESET_ACTION,
	SAVE_PRESET_ACTION, DELETE_PRESET_ACTION, RESET_TO_XML_ACTION, RESET_TO_DEFAULTS_ACTION,
	SAVE_CURRENT_STATE_ACTION, GET_MISSING_PARAMS_IN_PRESET,
	SET_GROUP_PRESET_ACTION, SAVE_GROUP_PRESET_ACTION, DELETE_GROUP_PRESET_ACTION,
	SEND_LOG_LINE_ACTION
};

enum ArgType{
	FLT_ARG, INT_ARG, BOL_ARG, STR_ARG, ENUM_ARG, COLOR_ARG, SPACER_ARG, NULL_ARG
};

struct DecodedMessage{
	ActionType action;
	ArgType argument;
	string paramName;
};


#ifndef OF_AVAILABLE //if OF is not available, redefine ofColor to myColor
//#warning "Openframeworks is not available!"
#define ofColor myColor

struct myColor{

	myColor(){}
	myColor(int rr, int gg, int bb, int aa){
		r = rr;	g = gg;	b = bb; a = aa;
	}
	myColor(int bright){
		r = g = b = bright; a = 255;
	}
	bool operator==(const myColor& c){
		return r == c.r && g == c.g && b == c.b && a == c.a;
	}
	bool operator!=(const myColor& c){
		return r != c.r || g != c.g || b != c.b || a != c.a;
	}
	union  {
		struct { unsigned char r, g, b, a; };
		unsigned char v[4];
	};
#ifdef CINDER_CINDER //if cinder available, define an easy port to cinderColor
#warning "Compiling for Cinder!"
	cinder::ColorA8u toCinder(){
		return cinder::ColorA8u(r,g,b,a);
	}
#endif
};
#endif

class RemoteUIParam{ //I am lazy and I know it

public:

	RemoteUIParam(){
		type = REMOTEUI_PARAM_UNKNOWN;
		floatValAddr = NULL;
		intValAddr = NULL;
		boolValAddr = NULL;
		stringValAddr = NULL;
		redValAddr = NULL;
		floatVal = minFloat = maxFloat = 0;
		intVal = minInt = maxInt = 0;
		redVal = greenVal = blueVal = alphaVal = 0;
		boolVal = false;
		stringVal = "empty";
		r = g = b = a = 0; //bg color
		group = OFXREMOTEUI_DEFAULT_PARAM_GROUP;
	};


	bool isEqualTo(RemoteUIParam &p){

		bool equal = true;
		switch (type) {
			case REMOTEUI_PARAM_FLOAT:
				if(p.floatVal != floatVal) equal = false;
				if(p.minFloat != minFloat) equal = false;
				if(p.maxFloat != maxFloat) equal = false;
				break;
			case REMOTEUI_PARAM_ENUM:
			case REMOTEUI_PARAM_INT:
				if(p.intVal != intVal) equal = false;
				if(p.minInt != minInt) equal = false;
				if(p.maxInt != maxInt) equal = false;
				break;
			case REMOTEUI_PARAM_BOOL:
				if(p.boolVal != boolVal) equal = false;
				break;
			case REMOTEUI_PARAM_STRING:
				if(p.stringVal != stringVal) equal = false;
				break;
			case REMOTEUI_PARAM_COLOR:
				if (p.redVal != redVal || p.greenVal != greenVal || p.blueVal != blueVal || p.alphaVal != alphaVal ) equal = false;
				break;
			case REMOTEUI_PARAM_SPACER:
				equal = false;
				break;
			default: printf("weird RemoteUIParam at isEqualTo()!\n"); break;
		}
		return equal;
	}

	string getValueAsString(){
		std::ostringstream ss;
		char aux[50];
		switch (type) {
			case REMOTEUI_PARAM_FLOAT: ss << floatVal; return ss.str();
			case REMOTEUI_PARAM_ENUM:
				if (intVal >= 0 && intVal < enumList.size())
					ss << enumList[intVal];
				else
					ss << "Invalid Enum!";
				return ss.str();
			case REMOTEUI_PARAM_INT: ss << intVal; return ss.str();
			case REMOTEUI_PARAM_BOOL: return boolVal ? "TRUE" : "FALSE";
			case REMOTEUI_PARAM_STRING: return stringVal;
			case REMOTEUI_PARAM_COLOR:{
				sprintf(aux, "RGBA: [%d, %d, %d, %d]", redVal, greenVal, blueVal, alphaVal);
				return string(aux);
			}
			case REMOTEUI_PARAM_SPACER: return "";
			default: return "unknown value (BUG!)";
		}
	}

	string getValueAsStringFromPointer(){
		std::ostringstream ss;
		char aux[50];
		switch (type) {
			case REMOTEUI_PARAM_FLOAT: ss << *floatValAddr; return ss.str();
			case REMOTEUI_PARAM_ENUM:{
				int v = *intValAddr;
				if (v >= 0 && v < enumList.size())
					ss << enumList[v];
				else
					ss << "Invalid Enum!";
				return ss.str();
			}
			case REMOTEUI_PARAM_INT: ss << *intValAddr; return ss.str();
			case REMOTEUI_PARAM_BOOL: return *boolValAddr ? "TRUE" : "FALSE";
			case REMOTEUI_PARAM_STRING: return *stringValAddr;
			case REMOTEUI_PARAM_COLOR:{
				sprintf(aux, "RGBA: [%d, %d, %d, %d]", redValAddr[0], redValAddr[1], redValAddr[2], redValAddr[3]);
				return string(aux);
			}
			case REMOTEUI_PARAM_SPACER: return "";
			default: return "unknown value (BUG!)";
		}
	}

	void print(){
		switch (type) {
			case REMOTEUI_PARAM_FLOAT: printf("float: %.2f [%.2f, %.2f]\n", floatVal, minFloat, maxFloat); break;
			case REMOTEUI_PARAM_INT: printf("int: %d [%d, %d]\n", intVal, minInt, maxInt); break;
			case REMOTEUI_PARAM_COLOR: printf("color: RGBA(%d %d %d %d)\n", redVal, greenVal, blueVal, alphaVal); break;
			case REMOTEUI_PARAM_ENUM: printf("enum: %d [%d, %d]\n", intVal, minInt, maxInt); break;
			case REMOTEUI_PARAM_BOOL: printf("bool: %s\n", boolVal ? "TRUE" : "FALSE"); break;
			case REMOTEUI_PARAM_STRING: printf("string: %s\n", stringVal.c_str()); break;
			case REMOTEUI_PARAM_SPACER: printf("Spacer\n"); break;
			default: printf("weird RemoteUIParam at print()!\n"); break;
		}
	};


	void setBgColor(const ofColor & c){
		r = c.r;
		g = c.g;
		b = c.b;
		a = c.a;
	}
	RemoteUIParamType type;

	float * floatValAddr;	//used in server
	float floatVal;			//used in client
	float minFloat;
	float maxFloat;

	int * intValAddr;
	int intVal;
	int minInt;
	int maxInt;

	bool * boolValAddr;
	bool boolVal;

	string * stringValAddr;
	string stringVal;

	unsigned char redVal, greenVal, blueVal, alphaVal;
	unsigned char * redValAddr;

	string group;
	vector<string> enumList; //for enum type

	unsigned char r,g,b,a; // param bg color [0,255]
};


struct RemoteUIClientCallBackArg{
	RemoteUICallClientAction action;
	string msg; //sort of a wildcard; usually its the preset name
	string host;
	string group;
	int port;
	vector<string> paramList; //wildacard, used for missing param list
};

struct RemoteUIServerCallBackArg{
	RemoteUICallServerAction action;
	string paramName;
	RemoteUIParam param; //get a copy o the new value of the param (only makes sense when action==SERVER_SENT_FULL_PARAMS_UPDATE)
	string msg; //sort of a wildcard; usually its the preset name
	string host;
	string group;
};



#ifdef OF_AVAILABLE
	#define RUI_LOG_NOTICE		(ofLogNotice("ofxRemoteUI"))
	#define RUI_LOG_ERROR		(ofLogError("ofxRemoteUI"))
	#define RUI_LOG_WARNING		(ofLogWarning("ofxRemoteUI"))
	#define RUI_LOG_VERBOSE		(ofLogVerbose("ofxRemoteUI"))
#else
	#define RUI_LOG_NOTICE		(cout << endl) //TODO!
	#define RUI_LOG_ERROR		(cout << endl)
	#define RUI_LOG_WARNING		(cout << endl)
	#define RUI_LOG_VERBOSE		(cout << endl)
#endif

#endif //emptyExample_RemoteParam_h
