#if defined(__APPLE__) || defined(TARGET_OSX)
#include "NativeMacMenu.h"

#include <objc/message.h>
#include <objc/runtime.h>

#if __has_include("BuildCommit.h")
#include "BuildCommit.h"
#else
#define OFX_REMOTEUI_BUILD_COMMIT "unknown"
#endif

namespace NativeMacMenu {
static Action action;
static id target = nil;
static id roots[4] = {nil, nil, nil, nil};
static id hostWindowsMenu = nil;
static id aboutItem = nil;
static id previousAboutTarget = nil;
static SEL previousAboutAction = nullptr;
static id items[AboutPanel + 1] = {};

template<typename R, typename... A> R send(id object, const char *selector, A... args) {
    return reinterpret_cast<R (*)(id, SEL, A...)>(objc_msgSend)(object, sel_registerName(selector), args...);
}
static id string(const char *s) { return send<id>((id)objc_getClass("NSString"), "stringWithUTF8String:", s); }
static void showAbout(id, SEL, id) {
    id text = string("By Oriol Ferrer Mesià\nBuild: " __DATE__ " " __TIME__
                     "\nGit commit: " OFX_REMOTEUI_BUILD_COMMIT "\nGitHub project");
    id credits = send<id>(send<id>((id)objc_getClass("NSMutableAttributedString"), "alloc"), "initWithString:", text);
    struct Range { unsigned long location; unsigned long length; };
    id paragraph = send<id>(send<id>((id)objc_getClass("NSMutableParagraphStyle"), "alloc"), "init");
    send<void>(paragraph, "setAlignment:", (unsigned long)2); // NSTextAlignmentCenter
    Range allText = {0, send<unsigned long>(text, "length")};
    send<void>(credits, "addAttribute:value:range:", string("NSParagraphStyle"), paragraph, allText);
    send<void>(paragraph, "release");
    id linkText = string("GitHub project");
    Range range = send<Range>(text, "rangeOfString:", linkText);
    id url = send<id>((id)objc_getClass("NSURL"), "URLWithString:", string("https://github.com/armadillu/ofxRemoteUI"));
    send<void>(credits, "addAttribute:value:range:", string("NSLink"), url, range);
    id options = send<id>((id)objc_getClass("NSDictionary"), "dictionaryWithObject:forKey:", credits, string("Credits"));
    id app = send<id>((id)objc_getClass("NSApplication"), "sharedApplication");
    send<void>(app, "orderFrontStandardAboutPanelWithOptions:", options);
    id window = send<id>(app, "keyWindow");
    if (window) {
        struct Size { double width; double height; };
        send<void>(window, "setContentSize:", Size{400, 300});
        send<void>(window, "center");
    }
    send<void>(credits, "release");
}
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
static id findMenu(id mainMenu, const char *title) {
    id menuItems = send<id>(mainMenu, "itemArray");
    const unsigned long count = send<unsigned long>(menuItems, "count");
    id wanted = string(title);
    for (unsigned long i = 0; i < count; ++i) {
        id root = send<id>(menuItems, "objectAtIndex:", i);
        if (send<bool>(send<id>(root, "title"), "isEqualToString:", wanted)) return send<id>(root, "submenu");
    }
    return nil;
}
bool setup(Action handler) {
    id app = send<id>((id)objc_getClass("NSApplication"), "sharedApplication");
    id mainMenu = send<id>(app, "mainMenu");
    if (!mainMenu) return false;
    hostWindowsMenu = send<id>(app, "windowsMenu");
    if (!hostWindowsMenu) hostWindowsMenu = findMenu(mainMenu, "Window");
    if (!hostWindowsMenu) hostWindowsMenu = findMenu(mainMenu, "Windows");
    if (!hostWindowsMenu) {
        hostWindowsMenu = makeMenu("Window", 3, mainMenu);
        send<void>(app, "setWindowsMenu:", hostWindowsMenu);
    }
    Class cls = objc_getClass("RemoteUIImGuiMenuTarget");
    if (!cls) {
        cls = objc_allocateClassPair((Class)objc_getClass("NSObject"), "RemoteUIImGuiMenuTarget", 0);
        class_addMethod(cls, sel_registerName("remoteUIAction:"), (IMP)perform, "v@:@");
        class_addMethod(cls, sel_registerName("remoteUIAbout:"), (IMP)showAbout, "v@:@");
        objc_registerClassPair(cls);
    }
    target = send<id>(send<id>((id)cls, "alloc"), "init");
    action = std::move(handler);
    // GLFW's first menu is the application menu. Reuse its standard About item.
    id applicationRoot = send<id>(mainMenu, "itemAtIndex:", (long)0);
    id applicationMenu = send<id>(applicationRoot, "submenu");
    id applicationItems = send<id>(applicationMenu, "itemArray");
    const unsigned long applicationItemCount = send<unsigned long>(applicationItems, "count");
    for (unsigned long i = 0; i < applicationItemCount; ++i) {
        id item = send<id>(applicationItems, "objectAtIndex:", i);
        if (send<SEL>(item, "action") == sel_registerName("orderFrontStandardAboutPanel:")) {
            aboutItem = item;
            previousAboutTarget = send<id>(item, "target");
            previousAboutAction = send<SEL>(item, "action");
            send<void>(item, "setTarget:", target);
            send<void>(item, "setAction:", sel_registerName("remoteUIAbout:"));
            break;
        }
    }
    id file = makeMenu("File", 0, mainMenu);
    add(file, "Open Local Preset...", OpenLocal, "o");
    add(file, "Settings", PreferencesPanel, ",");
    id remote = makeMenu("ofxRemoteUI", 1, mainMenu);
    add(remote, "Connect", Connect, "k");
    separator(remote);
    add(remote, "Refresh Parameters", Refresh, "s", true);
    add(remote, "Save Current Values to Server", Save, "s");
    add(remote, "Restore Launch Values", RestoreLaunch, "r");
    add(remote, "Restore Code Defaults", RestoreDefaults, "r", true);
    separator(remote);
    add(remote, "Copy Parameter Values", CopyValues, "c", true);
    add(remote, "Paste Parameter Values", PasteValues, "v", true);
	separator(remote);
    add(remote, "Highlight Changes Since Connection", HighlightChanged);
    add(remote, "Collapse / Expand All Groups", ToggleAllGroups, "g", true);
    add(remote, "Remove All Watches", RemoveAllWatches);
    separator(remote);
    add(remote, "Blink MIDI-Bound Parameters", BlinkMidi);
    add(remote, "Blink USB Controller-Bound Parameters", BlinkUsb);
    separator(hostWindowsMenu);
    add(hostWindowsMenu, "Action Log", ActivityPanel, "1");
    add(hostWindowsMenu, "Server Log", ServerLogPanel, "2");
    add(hostWindowsMenu, "Controller Bindings", BindingsPanel, "3");
    id developer = makeMenu("Developer", 2, mainMenu);
    add(developer, "ImGui Demo", ImGuiDemoPanel);
    add(developer, "ImGui Style Editor", StyleEditorPanel);
    add(developer, "Metrics / Debugger", MetricsDebuggerPanel);
    id menuItems = send<id>(mainMenu, "itemArray");
    const unsigned long menuCount = send<unsigned long>(menuItems, "count");
    long windowIndex = -1;
    for (unsigned long i = 0; i < menuCount; ++i) {
        id root = send<id>(menuItems, "objectAtIndex:", i);
        if (send<id>(root, "submenu") == hostWindowsMenu) { windowIndex = static_cast<long>(i); break; }
    }
    if (windowIndex >= 0) {
        for (int i = 0; i < 3; ++i) send<void>(mainMenu, "removeItem:", roots[i]);
        menuItems = send<id>(mainMenu, "itemArray");
        const unsigned long reorderedCount = send<unsigned long>(menuItems, "count");
        windowIndex = -1;
        for (unsigned long i = 0; i < reorderedCount; ++i) {
            id root = send<id>(menuItems, "objectAtIndex:", i);
            if (send<id>(root, "submenu") == hostWindowsMenu) { windowIndex = static_cast<long>(i); break; }
        }
    }
    if (windowIndex >= 0) {
        send<void>(mainMenu, "insertItem:atIndex:", roots[0], windowIndex);
        send<void>(mainMenu, "insertItem:atIndex:", roots[1], windowIndex + 1);
        send<void>(mainMenu, "insertItem:atIndex:", roots[2], windowIndex + 2);
    }
    return true;
}
void update(bool connected, bool activity, bool serverLog, bool preferences, bool bindings, bool highlightChanged) {
    if (!target) return;
    for (int i = Refresh; i <= RestoreDefaults; ++i) send<void>(items[i], "setEnabled:", connected);
    send<void>(items[CopyValues], "setEnabled:", connected);
    send<void>(items[PasteValues], "setEnabled:", connected);
    send<void>(items[BlinkMidi], "setEnabled:", connected);
    send<void>(items[BlinkUsb], "setEnabled:", connected);
    send<void>(items[ToggleAllGroups], "setEnabled:", connected);
    send<void>(items[RemoveAllWatches], "setEnabled:", connected);
    send<void>(items[HighlightChanged], "setEnabled:", connected);
    send<void>(items[HighlightChanged], "setState:", (long)(highlightChanged ? 1 : 0));
    const bool states[] = {activity, serverLog, preferences, bindings};
    for (int i = 0; i < 4; ++i) send<void>(items[ActivityPanel + i], "setState:", (long)(states[i] ? 1 : 0));
}
void shutdown() {
    if (!target) return;
    action = {};
    if (aboutItem) {
        send<void>(aboutItem, "setTarget:", previousAboutTarget);
        send<void>(aboutItem, "setAction:", previousAboutAction);
        aboutItem = nil;
        previousAboutTarget = nil;
        previousAboutAction = nullptr;
    }
    id menu = send<id>(send<id>((id)objc_getClass("NSApplication"), "sharedApplication"), "mainMenu");
    if (hostWindowsMenu) {
        for (int i = ActivityPanel; i <= ServerLogPanel; ++i) send<void>(hostWindowsMenu, "removeItem:", items[i]);
        send<void>(hostWindowsMenu, "removeItem:", items[BindingsPanel]);
        hostWindowsMenu = nil;
    }
    if (roots[0]) {
        id file = send<id>(roots[0], "submenu");
        send<void>(file, "removeItem:", items[PreferencesPanel]);
    }
    for (id &root : roots) { if (root) { send<void>(menu, "removeItem:", root); send<void>(root, "release"); root = nil; } }
    send<void>(target, "release"); target = nil;
}
}
#endif // TARGET_OSX
