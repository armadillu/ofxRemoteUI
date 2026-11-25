#pragma once
#include <functional>
namespace NativeMacMenu {
enum Command { Connect = 1, Refresh, Save, RestoreLaunch, RestoreDefaults, OpenLocal, CopyValues, PasteValues, BlinkMidi, BlinkUsb, ToggleAllGroups, HighlightChanged, ActivityPanel, ServerLogPanel, PreferencesPanel, BindingsPanel, ImGuiDemoPanel, StyleEditorPanel, RemoveAllWatches, MetricsDebuggerPanel };
using Action = std::function<void(Command)>;
bool setup(Action handler);
void update(bool connected, bool activity, bool serverLog, bool preferences, bool bindings, bool highlightChanged);
void shutdown();
}
