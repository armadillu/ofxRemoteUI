
#include "ofStolenUtils.h"
#include <string.h>

int ofToInt(const std::string& intString) {
	int x = 0;
	std::istringstream cur(intString);
	cur >> x;
	return x;
}

//----------------------------------------
float ofToFloat(const std::string& floatString) {
	float x = 0;
	std::istringstream cur(floatString);
	cur >> x;
	return x;
}

float ofClamp(float value, float min, float max) {
	return value < min ? min : value > max ? max : value;
}

