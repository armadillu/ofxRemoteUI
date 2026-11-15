#pragma once

#include "ofxRemoteUIClient.h"
#include <string>

namespace ofAppImGuiHelpers {

bool sameParameterValue(const RemoteUIParam &a, const RemoteUIParam &b);

bool copyParameterValue(RemoteUIParam &target, const RemoteUIParam &source);

float parameterFontScale(float rowHeight);

bool framedCheckbox(const char *label, bool *value);

void prepareSecondaryWindow();

void applyWindowColor(const float color[4]);

const char *paramTypeName(RemoteUIParamType type);

bool containsCaseInsensitive(const std::string &text, const std::string &query);

void pushPopupWindowPadding();
}
