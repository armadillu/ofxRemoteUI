#pragma once

#ifdef TARGET_OSX
#include <objc/message.h>
#include <objc/runtime.h>
#include <functional>

namespace NativeMacMenu {
enum Command { Connect = 1, Disconnect, Refresh, Save, RestoreLaunch, RestoreDefaults, OpenLocal,
               ConnectionPanel, ParametersPanel, PresetsPanel, ActivityPanel, ServerLogPanel, PreferencesPanel, BindingsPanel };
using Action = std::function<void(Command)>;
static Action action;
static id target = nil;
static id roots[2] = {nil, nil};
static id items[BindingsPanel + 1] = {};

template<typename R, typename... A> R send(id object, const char *selector, A... args) {
    return reinterpret_cast<R (*)(id, SEL, A...)>(objc_msgSend)(object, sel_registerName(selector), args...);
}
static id string(const char *s) { return send<id>((id)objc_getClass("NSString"), "stringWithUTF8String:", s); }
static void perform(id, SEL, id sender) {
    auto command = static_cast<Command>(send<long>(sender, "tag"));
    if (action) action(command);
}
static id makeMenu(const char *title, int index, id mainMenu) {
    id root = send<id>(send<id>((id)objc_getClass("NSMenuItem"), "alloc"), "initWithTitle:action:keyEquivalent:", string(title), (SEL)nullptr, string(""));
    id menu = send<id>(send<id>((id)objc_getClass("NSMenu"), "alloc"), "initWithTitle:", string(title));
    send<void>(menu, "setAutoenablesItems:", false);
    send<void>(root, "setSubmenu:", menu);
    send<void>(mainMenu, "addItem:", root);
    send<void>(menu, "release");
    roots[index] = root;
    return menu;
}
static void add(id menu, const char *title, Command command, const char *key = "", bool shift = false) {
    id item = send<id>(send<id>((id)objc_getClass("NSMenuItem"), "alloc"), "initWithTitle:action:keyEquivalent:", string(title), sel_registerName("remoteUIAction:"), string(key));
    send<void>(item, "setTarget:", target);
    send<void>(item, "setTag:", (long)command);
    if (shift) send<void>(item, "setKeyEquivalentModifierMask:", (unsigned long)((1UL << 20) | (1UL << 17)));
    send<void>(menu, "addItem:", item);
    items[command] = item;
    send<void>(item, "release");
}
static void separator(id menu) { send<void>(menu, "addItem:", send<id>((id)objc_getClass("NSMenuItem"), "separatorItem")); }
static bool setup(Action handler) {
    id app = send<id>((id)objc_getClass("NSApplication"), "sharedApplication");
    id mainMenu = send<id>(app, "mainMenu");
    if (!mainMenu) return false;
    Class cls = objc_getClass("RemoteUIImGuiMenuTarget");
    if (!cls) {
        cls = objc_allocateClassPair((Class)objc_getClass("NSObject"), "RemoteUIImGuiMenuTarget", 0);
        class_addMethod(cls, sel_registerName("remoteUIAction:"), (IMP)perform, "v@:@");
        objc_registerClassPair(cls);
    }
    target = send<id>(send<id>((id)cls, "alloc"), "init");
    action = std::move(handler);
    id remote = makeMenu("RemoteUI", 0, mainMenu);
    add(remote, "Connect", Connect, "k"); add(remote, "Disconnect", Disconnect, "k", true);
    separator(remote);
    add(remote, "Refresh Parameters", Refresh, "s", true);
    add(remote, "Save Current Values to Server", Save, "s");
    add(remote, "Restore Launch Values", RestoreLaunch, "r");
    add(remote, "Restore Code Defaults", RestoreDefaults, "r", true);
    separator(remote); add(remote, "Open Local Preset...", OpenLocal, "o");
    id windows = makeMenu("Windows", 1, mainMenu);
    add(windows, "Connection & Discovery", ConnectionPanel, "1");
    add(windows, "Parameters", ParametersPanel, "2");
    add(windows, "Presets & State", PresetsPanel, "3");
    add(windows, "Action Log", ActivityPanel, "4");
    add(windows, "Server Log", ServerLogPanel, "6");
    add(windows, "Controller Bindings", BindingsPanel, "5");
    add(windows, "Preferences", PreferencesPanel, ",");
    return true;
}
static void update(bool connected, bool connection, bool parameters, bool presets, bool activity, bool serverLog, bool preferences, bool bindings) {
    if (!target) return;
    for (int i = Disconnect; i <= RestoreDefaults; ++i) send<void>(items[i], "setEnabled:", connected);
    const bool states[] = {connection, parameters, presets, activity, serverLog, preferences, bindings};
    for (int i = 0; i < 7; ++i) send<void>(items[ConnectionPanel + i], "setState:", (long)(states[i] ? 1 : 0));
}
static void shutdown() {
    if (!target) return;
    action = {};
    id menu = send<id>(send<id>((id)objc_getClass("NSApplication"), "sharedApplication"), "mainMenu");
    for (id &root : roots) { if (root) { send<void>(menu, "removeItem:", root); send<void>(root, "release"); root = nil; } }
    send<void>(target, "release"); target = nil;
}
}
#endif
