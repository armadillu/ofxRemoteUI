#pragma once

#ifdef TARGET_OSX
#include <objc/message.h>
#include <objc/runtime.h>
#include <string>
#include <vector>

// GLFW installs the application delegate during initialization. Add document
// callbacks to its class before GLFW calls finishLaunching so Finder's initial
// open-document event is accepted as well as events for a running client.
namespace NativeMacFileOpen {
inline std::vector<std::string> pending;

template<typename R, typename... A> R send(id object, const char *selector, A... args) {
    return reinterpret_cast<R (*)(id, SEL, A...)>(objc_msgSend)(object, sel_registerName(selector), args...);
}
inline void queuePath(id path) {
    const char *utf8 = path ? send<const char *>(path, "UTF8String") : nullptr;
    if (utf8) pending.emplace_back(utf8);
}
inline void openFiles(id, SEL, id application, id paths) {
    const unsigned long count = paths ? send<unsigned long>(paths, "count") : 0;
    for (unsigned long i = 0; i < count; ++i) queuePath(send<id>(paths, "objectAtIndex:", i));
    // NSApplicationDelegateReplySuccess. AppKit otherwise reports open failure.
    send<void>(application, "replyToOpenOrPrint:", (unsigned long)0);
}
inline signed char openFile(id, SEL, id, id path) {
    queuePath(path);
    return 1;
}
inline void setup() {
    Class delegate = objc_getClass("GLFWApplicationDelegate");
    if (!delegate) return;
    class_addMethod(delegate, sel_registerName("application:openFiles:"), (IMP)openFiles, "v@:@@");
    class_addMethod(delegate, sel_registerName("application:openFile:"), (IMP)openFile, "c@:@@");
}
inline std::vector<std::string> takePending() { std::vector<std::string> result; result.swap(pending); return result; }
inline void shutdown() { pending.clear(); }
}
#endif
