#pragma once

#include "ofMain.h"
#include "ofxMidi.h"
#include "MidiFighterHighRes.h"
#include <GLFW/glfw3.h>
#include <algorithm>
#include <cmath>
#include <map>
#include <mutex>
#include <memory>
#include <vector>

// MIDI callbacks run on a backend thread; all binding and parameter work stays on the app thread.
class ControllerBindings : public ofxMidiListener {
public:
    struct Binding { std::string server, param, device, kind; int channel = 0, control = 0; };
    struct Input { std::string device, kind; int channel = 0, control = 0; float value = 0; bool button = false; };
    std::vector<Binding> bindings;
    std::string learning;
    bool toggleButtons = false;
    bool colorControlsAlpha = false;

    void setup() { load(); refreshMidi(); }
    void shutdown() { for (auto &p : inputs) { p->removeListener(this); p->closePort(); } inputs.clear(); }
    void newMidiMessage(ofxMidiMessage &m) override {
        Input e; e.device = m.portName; e.channel = m.channel;
        if (m.status == MIDI_SYSEX) {
            MidiFighterHighRes::Message highRes;
            if (!MidiFighterHighRes::decode(m.bytes, highRes)) return;
            e.kind = "MIDI CC"; e.channel = highRes.channel; e.control = highRes.control; e.value = highRes.value;
        }
        else if (m.status == MIDI_CONTROL_CHANGE) { e.kind = "MIDI CC"; e.control = m.control; e.value = m.value / 127.f; }
        else if (m.status == MIDI_NOTE_ON || m.status == MIDI_NOTE_OFF) {
            e.kind = "MIDI Note"; e.control = m.pitch; e.button = true;
            e.value = m.status == MIDI_NOTE_ON && m.velocity > 0 ? 1.f : 0.f;
        } else return;
        std::lock_guard<std::mutex> lock(mutex);
        if (pending.size() < 1024) pending.push_back(std::move(e));
    }
    void beginLearn(const std::string &param) { learning = param; axisBaseline.clear(); }
    template<class Handler, class CanBind> void update(float dt, const std::string &server, Handler handler, CanBind canBind) {
        refreshTimer += dt;
        if (refreshTimer >= 3.f) { refreshTimer = 0; refreshMidi(); }
        std::vector<Input> events;
        { std::lock_guard<std::mutex> lock(mutex); events.swap(pending); }
        for (int id = GLFW_JOYSTICK_1; id <= GLFW_JOYSTICK_LAST; ++id) {
            if (!glfwJoystickPresent(id)) { axes.erase(id); buttons.erase(id); continue; }
            const char *name = glfwGetJoystickName(id), *guid = glfwGetJoystickGUID(id);
            std::string device = std::string(name ? name : "Joystick") + " [" + (guid ? guid : "unknown") + "]";
            int count = 0; const float *a = glfwGetJoystickAxes(id, &count);
			if (!a) count = 0;
            auto &previous = axes[id];
            if (count == 0 || !a) previous.clear();
            else if (previous.size() != (size_t)count) previous.assign(a, a + count);
            for (int i = 0; i < count; ++i) {
                const std::string key = device + "/" + ofToString(i);
                if (!learning.empty() && !axisBaseline.count(key)) axisBaseline[key] = a[i];
                const float difference = !learning.empty() ? std::abs(a[i] - axisBaseline[key]) : std::abs(a[i] - previous[i]);
                if (difference > (!learning.empty() ? .2f : .01f)) {
                    events.push_back({device, "Joystick Axis", 0, i, ofClamp((a[i] + 1.f) * .5f, 0.f, 1.f), false});
                    previous[i] = a[i];
                }
            }
            int n = 0; const unsigned char *b = glfwGetJoystickButtons(id, &n);
			if (!b) n = 0;
            auto &old = buttons[id];
            if (n == 0 || !b) old.clear();
            else if (old.size() != (size_t)n) old.assign(b, b + n);
            for (int i = 0; i < n; ++i) if (b[i] != old[i]) {
                events.push_back({device, "Joystick Button", 0, i, b[i] == GLFW_PRESS ? 1.f : 0.f, true}); old[i] = b[i];
            }
        }
        for (const auto &event : events) {
            if (!learning.empty()) {
                if (event.button && event.value == 0) continue;
                if (!canBind(learning, event)) continue;
                auto match = [&](const Binding &b) { return b.server == server && b.device == event.device && b.kind == event.kind && b.channel == event.channel && b.control == event.control; };
                bindings.erase(std::remove_if(bindings.begin(), bindings.end(), match), bindings.end());
                bindings.push_back({server, learning, event.device, event.kind, event.channel, event.control});
                learning.clear(); save(); continue;
            }
            for (const auto &b : bindings) if (b.server == server && b.device == event.device && b.kind == event.kind && b.channel == event.channel && b.control == event.control) handler(b.param, event);
        }
    }
    void remove(size_t i) { if (i < bindings.size()) { bindings.erase(bindings.begin() + i); save(); } }
    static std::string label(const Binding &b) { return b.device + " — " + b.kind + " " + ofToString(b.control) + (b.channel ? " / ch " + ofToString(b.channel) : ""); }
private:
    std::vector<std::unique_ptr<ofxMidiIn>> inputs;
    std::vector<std::string> portNames;
    std::mutex mutex;
    std::vector<Input> pending;
    std::map<int, std::vector<float>> axes;
    std::map<int, std::vector<unsigned char>> buttons;
    std::map<std::string, float> axisBaseline;
    float refreshTimer = 0;
    void refreshMidi() {
        try {
            ofxMidiIn probe; auto names = probe.getInPortList();
            if (names == portNames) return;
            for (auto &p : inputs) { p->removeListener(this); p->closePort(); }
            inputs.clear(); portNames = names;
            for (unsigned i = 0; i < names.size(); ++i) {
                auto p = std::make_unique<ofxMidiIn>();
                p->ignoreTypes(false, true, true); // High-resolution Twister messages arrive as SysEx.
                if (p->openPort(i)) { p->addListener(this); inputs.push_back(std::move(p)); }
            }
        } catch (const std::exception &e) { ofLogWarning("ControllerBindings") << e.what(); }
    }
    void save() const {
        ofJson json = ofJson::array();
        for (const auto &b : bindings) json.push_back({{"server", b.server}, {"param", b.param}, {"device", b.device}, {"kind", b.kind}, {"channel", b.channel}, {"control", b.control}});
        ofSavePrettyJson(ofToDataPath("controller-bindings.json", true), json);
    }
    void load() {
        const auto path = ofToDataPath("controller-bindings.json", true);
        if (!ofFile::doesFileExist(path)) return;
        try {
            auto json = ofLoadJson(path);
            for (const auto &j : json) bindings.push_back({j.at("server").get<std::string>(), j.at("param").get<std::string>(), j.at("device").get<std::string>(), j.at("kind").get<std::string>(), j.at("channel").get<int>(), j.at("control").get<int>()});
        } catch (const std::exception &e) { ofLogWarning("ControllerBindings") << "Could not load bindings: " << e.what(); }
    }
};
