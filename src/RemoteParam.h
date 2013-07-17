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

enum RemoteUICallBackArg{
	SERVER_CONNECTED, PARAMS_UPDATED, PRESETS_UPDATED, SERVER_DISCONNECTED,
	SERVER_CONFIRMED_SAVE, SERVER_DID_RESET_TO_DEFAULTS, SERVER_DELETED_PRESET,
	SERVER_SAVED_PRESET, SERVER_DID_RESET_TO_XML, SERVER_DID_SET_PRESET
};

enum RemoteUIParamType{
	REMOTEUI_PARAM_FLOAT = 100,
	REMOTEUI_PARAM_INT,
	REMOTEUI_PARAM_BOOL,
	REMOTEUI_PARAM_STRING,
	REMOTEUI_PARAM_ENUM,
	REMOTEUI_PARAM_COLOR,
};

enum ActionType{
	HELO_ACTION, REQUEST_ACTION, SEND_PARAM_ACTION, CIAO_ACTION,
	TEST_ACTION, PRESET_LIST_ACTION, SET_PRESET_ACTION,
	SAVE_PRESET_ACTION, DELETE_PRESET_ACTION, RESET_TO_XML_ACTION, RESET_TO_DEFAULTS_ACTION,
	SAVE_CURRENT_STATE_ACTION
};

enum ArgType{
	FLT_ARG, INT_ARG, BOL_ARG, STR_ARG, ENUM_ARG, COLOR_ARG, NULL_ARG
};

struct DecodedMessage{
	ActionType action;
	ArgType argument;
	string paramName;
	string paramGroup;
};


class RemoteUIParam{ //I am lazy and I know it

public:

	RemoteUIParam(){
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
		group = DEFAULT_PARAM_GROUP;
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
			default: printf("weird RemoteUIParam at isEqualTo()!\n"); break;
		}
		return equal;
	}


	void print(){
		switch (type) {
			case REMOTEUI_PARAM_FLOAT: printf("float: %.2f [%.2f, %.2f]\n", floatVal, minFloat, maxFloat); break;
			case REMOTEUI_PARAM_INT: printf("int: %d [%d, %d]\n", intVal, minInt, maxInt); break;
			case REMOTEUI_PARAM_COLOR: printf("color: %d %d %d %d\n", redVal, greenVal, blueVal, alphaVal); break;
			case REMOTEUI_PARAM_ENUM: printf("enum: %d [%d, %d]\n", intVal, minInt, maxInt); break;
			case REMOTEUI_PARAM_BOOL: printf("bool: %d\n", boolVal); break;
			case REMOTEUI_PARAM_STRING: printf("string: %s\n", stringVal.c_str()); break;
			default: printf("weird RemoteUIParam at print()!\n"); break;
		}
	};

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

	unsigned char r,g,b,a; // color [0,255]
};

#endif
