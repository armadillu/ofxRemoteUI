#pragma once

#include <vector>

namespace MidiFighterHighRes {
struct Message { int channel; int control; float value; };

// RtMidi supplies the complete SysEx frame, including the three-byte
// manufacturer ID and F7. The payload channel is zero-based; ofxMidi uses 1-16.
inline bool decode(const std::vector<unsigned char> &bytes, Message &message) {
    if (bytes.size() != 10 || bytes[0] != 0xF0 || bytes[1] != 0x00 ||
        bytes[2] != 0x01 || bytes[3] != 0x79 || bytes[4] != 0x06 ||
        bytes[9] != 0xF7 || bytes[5] > 15 || bytes[6] > 127 ||
        bytes[7] > 127 || bytes[8] > 127) return false;
    message.channel = bytes[5] + 1;
    message.control = bytes[6];
    message.value = ((bytes[7] << 7) | bytes[8]) / 16383.f;
    return true;
}
}
