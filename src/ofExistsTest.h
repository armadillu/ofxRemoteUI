//
//  ofExistsTest.h
//  SensorData
//
//  Created by Oriol Ferrer Mesià on 01/11/2020.
//

#pragma once


#if defined(__has_include) /*llvm only - query about header files being available or not*/
	#if __has_include("ofMain.h")
		#define OF_AVAILABLE
	#endif
#endif


#ifdef OF_VERSION_MINOR
	#define OF_AVAILABLE
#endif

