//
//  ofStolenUtils.h
//  ofxRemoteUIClientOSX
//
//  Created by Oriol Ferrer Mesià on 29/06/13.
//
//

/*
 THIS IS A SHAMEFUL RIP OF A FEW OPENFRAMEWORKS METHODS
 the whole point is for ofxRemoteUI to work also outside OF if required
 */

#pragma once
#ifndef ofxRemoteUIClientOSX_ofStolenUtils_h
#define ofxRemoteUIClientOSX_ofStolenUtils_h

#include <string>
#include <sstream>
#include <stdio.h>
#include <iostream>
#include <sstream>
#include <iomanip>


int ofToInt(const std::string& intString) ;
float ofToFloat(const std::string& floatString);
float ofClamp(float value, float min, float max) ;


template <class T>
std::string ofToString(const T& value){
	std::ostringstream out;
	out << value;
	return out.str();
}

/// like sprintf "%4f" format, in this example precision=4
template <class T>
std::string ofToString(const T& value, int precision){
	std::ostringstream out;
	out << std::fixed << std::setprecision(precision) << value;
	return out.str();
}

/// like sprintf "% 4d" or "% 4f" format, in this example width=4, fill=' '
template <class T>
std::string ofToString(const T& value, int width, char fill ){
	std::ostringstream out;
	out << std::fixed << std::setfill(fill) << std::setw(width) << value;
	return out.str();
}

/// like sprintf "%04.2d" or "%04.2f" format, in this example precision=2, width=4, fill='0'
template <class T>
std::string ofToString(const T& value, int precision, int width, char fill ){
	std::ostringstream out;
	out << std::fixed << std::setfill(fill) << std::setw(width) << std::setprecision(precision) << value;
	return out.str();
}

#endif
