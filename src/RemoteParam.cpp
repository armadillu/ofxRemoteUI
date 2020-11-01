//
//  RemoteParam.cpp
//  emptyExample
//
//  Created by Oriol Ferrer Mesià on 24/06/13.
//
//

#include "RemoteParam.h"
#include "ofxRemoteUI.h"

RemoteUIParam::RemoteUIParam(){
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


bool RemoteUIParam::isEqualTo(const RemoteUIParam &p) const{

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
		default: RLOG_ERROR << "weird RemoteUIParam at isEqualTo()!"; break;
	}
	//if(equal) equal = description == p.description; //also compare param description
	return equal;
}

std::string RemoteUIParam::getValueAsString() const{
	std::ostringstream ss;
	char aux[50];
	switch (type) {
		case REMOTEUI_PARAM_FLOAT: ss << floatVal; return ss.str();
		case REMOTEUI_PARAM_ENUM:
			if (intVal >= minInt && intVal <= maxInt && (intVal - minInt) < (int)enumList.size())
				ss << enumList[intVal - minInt];
			else
				ss << "Invalid Enum!";
			return ss.str();
		case REMOTEUI_PARAM_INT: ss << intVal; return ss.str();
		case REMOTEUI_PARAM_BOOL: return boolVal ? "TRUE" : "FALSE";
		case REMOTEUI_PARAM_STRING: return stringVal;
		case REMOTEUI_PARAM_COLOR:{
			sprintf(aux, "RGBA: [%d, %d, %d, %d]", redVal, greenVal, blueVal, alphaVal);
			return std::string(aux);
		}
		case REMOTEUI_PARAM_SPACER: return "";
		default: return "unknown value (BUG!)";
	}
}

std::string RemoteUIParam::getValueAsStringFromPointer(){
	std::ostringstream ss;
	char aux[50];
	switch (type) {
		case REMOTEUI_PARAM_FLOAT: ss << *floatValAddr; return ss.str();
		case REMOTEUI_PARAM_ENUM:{
			int v = *intValAddr;
			if (v >= minInt && v <= maxInt && (v - minInt) < (int)enumList.size())
				ss << enumList[v - minInt];
			else
				ss << "Invalid Enum!";
			return ss.str();
		}
		case REMOTEUI_PARAM_INT: ss << *intValAddr; return ss.str();
		case REMOTEUI_PARAM_BOOL: return *boolValAddr ? "TRUE" : "FALSE";
		case REMOTEUI_PARAM_STRING: return *stringValAddr;
		case REMOTEUI_PARAM_COLOR:{
			sprintf(aux, "RGBA: [%d, %d, %d, %d]", redValAddr[0], redValAddr[1], redValAddr[2], redValAddr[3]);
			return std::string(aux);
		}
		case REMOTEUI_PARAM_SPACER: return "";
		default: return "unknown value (BUG!)";
	}
}

std::string RemoteUIParam::getInfoAsString(){
	char aux[2048];
	switch (type) {
		case REMOTEUI_PARAM_FLOAT: sprintf(aux, "Float: %f [%f, %f]", floatVal, minFloat, maxFloat); break;
		case REMOTEUI_PARAM_INT: sprintf(aux, "Int: %d [%d, %d]", intVal, minInt, maxInt); break;
		case REMOTEUI_PARAM_COLOR: sprintf(aux, "Color: RGBA(%d %d %d %d)", redVal, greenVal, blueVal, alphaVal); break;
		case REMOTEUI_PARAM_ENUM: sprintf(aux, "Enum: %d [%d, %d]", intVal, minInt, maxInt); break;
		case REMOTEUI_PARAM_BOOL: sprintf(aux, "Bool: %s", boolVal ? "TRUE" : "FALSE"); break;
		case REMOTEUI_PARAM_STRING: sprintf(aux, "String: \"%s\"", stringVal.c_str()); break;
		case REMOTEUI_PARAM_SPACER: sprintf(aux, "Group: \"%s\"", group.c_str()); break;
		default: RLOG_ERROR << "weird RemoteUIParam at print()!"; break;
	}
	return std::string(aux);
}

void RemoteUIParam::print(){
	//printf("%s\n", getInfoAsString().c_str());
	RLOG_NOTICE << getInfoAsString();
};

ofColor RemoteUIParam::getColor(){
	return ofColor(redVal, greenVal, blueVal, alphaVal);
}

void RemoteUIParam::setBgColor(const ofColor & c){
	r = c.r; g = c.g; b = c.b; a = c.a;
}

RemoteUIServerValueWatch::RemoteUIServerValueWatch(){
	floatAddress = nullptr; intAddress = nullptr; boolAddress = nullptr;
}

std::string RemoteUIServerValueWatch::getValueAsString(){
	switch(type){
		case REMOTEUI_PARAM_FLOAT: return ofToString(*floatAddress, 4);
		case REMOTEUI_PARAM_INT: return ofToString(*intAddress);
		case REMOTEUI_PARAM_BOOL: return (*boolAddress) ? "true" : "false";
		default: break;
	}
	return "unknown var type?";
}

