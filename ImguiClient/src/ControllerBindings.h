#pragma once

#include "ofMain.h"
#include "ofxMidi.h"
#include <mutex>
#include <memory>
#include <map>
#include <vector>
#include <functional>

// MIDI callbacks run on a backend thread; all binding and parameter work stays on the app thread.
class ControllerBindings : public ofxMidiListener {
public:
    struct Binding { std::string server, param, device, kind; int channel = 0, control = 0; };
    struct Input { std::string device, kind; int channel = 0, control = 0; float value = 0; bool button = false; };
    std::vector<Binding> bindings;
    std::string learning;
    bool toggleButtons = false;
    bool colorControlsAlpha = false;

    void setup();
    void shutdown();
    void newMidiMessage(ofxMidiMessage &m) override;
    void beginLearn(const std::string &param);
    bool importFile(const std::string &path, std::string &error);
    static bool matchesFilters(const Binding &binding, const std::string &serverFilter, const std::string &paramFilter);
    bool exportFile(const std::string &path, const std::string &serverFilter, const std::string &paramFilter, size_t &exportedCount, std::string &error) const;
    void update(float dt, const std::string &server, std::function<void(const std::string &, const Input &)> handler, std::function<bool(const std::string &, const Input &)> canBind);
    void remove(size_t i);
    static std::string label(const Binding &b);
private:
    std::vector<std::unique_ptr<ofxMidiIn>> inputs;
    std::vector<std::string> portNames;
    std::mutex mutex;
    std::vector<Input> pending;
    std::map<int, std::vector<float>> axes;
    std::map<int, std::vector<unsigned char>> buttons;
    std::map<std::string, float> axisBaseline;
    float refreshTimer = 0;
    void refreshMidi();
    void save() const;
    void load();
};
