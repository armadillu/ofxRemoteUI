#include "ControllerBindings.h"
#include "MidiFighterHighRes.h"
#include "ClientPaths.h"
#include <GLFW/glfw3.h>
#include <algorithm>
#include <cmath>
#include <fstream>
#include <stdexcept>
#include <cctype>

void ControllerBindings::setup() { load(); refreshMidi(); }


void ControllerBindings::shutdown() { for (auto &p : inputs) { p->removeListener(this); p->closePort(); } inputs.clear(); }


void ControllerBindings::newMidiMessage(ofxMidiMessage &m) {
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


void ControllerBindings::beginLearn(const std::string &param) { learning = param; axisBaseline.clear(); }


bool ControllerBindings::importFile(const std::string &path, std::string &error) {
        std::vector<Binding> imported;
        try {
            const auto json = ofLoadJson(path);
            if (!json.is_array()) { error = "Bindings file must contain a JSON array"; return false; }
            for (const auto &item : json) {
                Binding binding;
                binding.server = item.value("server", std::string());
                binding.param = item.at("param").get<std::string>();
                binding.device = item.at("device").get<std::string>();
                binding.kind = item.at("kind").get<std::string>();
                binding.channel = item.value("channel", 0);
                binding.control = item.at("control").get<int>();
                binding.invertRange = item.value("invertRange", false);
                if (binding.param.empty() || binding.device.empty() || binding.kind.empty()) throw std::runtime_error("Binding is missing required fields");
                imported.push_back(std::move(binding));
            }
        } catch (const std::exception &e) {
            error = std::string("Could not parse bindings JSON: ") + e.what();
            return false;
        }
        bindings = std::move(imported);
        save();
        return true;
    }


bool ControllerBindings::matchesFilters(const Binding &binding, const std::string &serverFilter, const std::string &paramFilter) {
        if (!serverFilter.empty() && binding.server != serverFilter) return false;
        if (paramFilter.empty()) return true;
        std::string name = binding.param;
        std::string filter = paramFilter;
        const auto lowercase = [](std::string &value) {
            std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) { return (char)std::tolower(c); });
        };
        lowercase(name);
        lowercase(filter);
        return name.find(filter) != std::string::npos;
    }


bool ControllerBindings::exportFile(const std::string &path, const std::string &serverFilter, const std::string &paramFilter, size_t &exportedCount, std::string &error) const {
        ofJson json = ofJson::array();
        for (const auto &binding : bindings) {
            if (!matchesFilters(binding, serverFilter, paramFilter)) continue;
            json.push_back({{"server", binding.server}, {"param", binding.param}, {"device", binding.device}, {"kind", binding.kind}, {"channel", binding.channel}, {"control", binding.control}, {"invertRange", binding.invertRange}});
        }
        exportedCount = json.size();
        std::ofstream output(path, std::ios::binary);
        if (!output) { error = "Could not create bindings file"; return false; }
        output << json.dump(2) << '\n';
        if (!output) { error = "Could not write bindings file"; return false; }
        return true;
    }


void ControllerBindings::update(float dt, const std::string &server, std::function<void(const std::string &, const Input &)> handler, std::function<bool(const std::string &, const Input &)> canBind) {
        refreshTimer += dt;
        activationHighlightTime = std::max(0.f, activationHighlightTime - dt);
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
                if (difference > (!learning.empty() ? .75f : .01f)) {
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
            for (const auto &b : bindings) if (b.server == server && b.device == event.device && b.kind == event.kind && b.channel == event.channel && b.control == event.control) {
                recentlyActivated = b;
                activationHighlightTime = .45f;
                Input mappedEvent = event;
                if (!event.button && b.invertRange) mappedEvent.value = 1.f - mappedEvent.value;
                handler(b.param, mappedEvent);
            }
        }
    }


void ControllerBindings::remove(size_t i) { if (i < bindings.size()) { bindings.erase(bindings.begin() + i); save(); } }

void ControllerBindings::removeAll() {
    if (bindings.empty()) return;
    bindings.clear();
    save();
}

void ControllerBindings::setRangeInverted(size_t i, bool inverted) {
    if (i >= bindings.size() || bindings[i].invertRange == inverted) return;
    bindings[i].invertRange = inverted;
    save();
}


std::string ControllerBindings::label(const Binding &b) { return b.device + " — " + b.kind + " " + ofToString(b.control) + (b.channel ? " / ch " + ofToString(b.channel) : ""); }


void ControllerBindings::refreshMidi() {
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


void ControllerBindings::save() const {
        ofJson json = ofJson::array();
        for (const auto &b : bindings) json.push_back({{"server", b.server}, {"param", b.param}, {"device", b.device}, {"kind", b.kind}, {"channel", b.channel}, {"control", b.control}, {"invertRange", b.invertRange}});
        ofSavePrettyJson(ClientPaths::settingsFile("controller-bindings.json"), json);
    }


void ControllerBindings::load() {
        const auto path = ClientPaths::existingSettingsFile("controller-bindings.json");
        if (!ofFile::doesFileExist(path)) return;
        try {
            auto json = ofLoadJson(path);
            for (const auto &j : json) bindings.push_back({j.at("server").get<std::string>(), j.at("param").get<std::string>(), j.at("device").get<std::string>(), j.at("kind").get<std::string>(), j.at("channel").get<int>(), j.at("control").get<int>(), j.value("invertRange", false)});
        } catch (const std::exception &e) { ofLogWarning("ControllerBindings") << "Could not load bindings: " << e.what(); }
    }



bool ControllerBindings::wasRecentlyActivated(const Binding &binding) const {
    return activationHighlightTime > 0.f
        && recentlyActivated.server == binding.server
        && recentlyActivated.param == binding.param
        && recentlyActivated.device == binding.device
        && recentlyActivated.kind == binding.kind
        && recentlyActivated.channel == binding.channel
        && recentlyActivated.control == binding.control;
}
