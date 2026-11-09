#pragma once

#include "ofMain.h"

namespace ClientPaths {

// Mutable settings must not be written into a signed application bundle.
inline std::string settingsFile(const std::string &name) {
#ifdef TARGET_OSX
	const auto directory = of::filesystem::path(ofFilePath::getUserHomeDir()) / "Library" / "Application Support" / "ofxRemoteUI ImGui Client";
	std::error_code error;
	of::filesystem::create_directories(directory, error);
	if (!error) return (directory / name).string();
	ofLogWarning("ClientPaths") << "Could not create settings directory: " << error.message();
#endif
	return ofToDataPath(name, true);
}

inline std::string existingSettingsFile(const std::string &name) {
	const std::string current = settingsFile(name);
	if (ofFile::doesFileExist(current, false)) return current;
#ifdef TARGET_OSX
	// Previous versions stored settings in the data folder next to the app.
	const auto legacy = ofFilePath::getCurrentExeDirFS() / "../../../data" / name;
	if (ofFile::doesFileExist(legacy.string(), false)) return legacy.string();
#endif
	return current;
}
}
