#include "Platform.h"

#ifdef Q_OS_MAC
#include <CoreFoundation/CoreFoundation.h>

void Platform::disableAppNapViaInfoDict()
{
    CFBundleRef bundle = CFBundleGetMainBundle();
    if (!bundle) return;

    // CFBundleGetInfoDictionary returns the dictionary the OS already
    // parsed from Info.plist.  Cast it to mutable so we can tweak it.
    CFMutableDictionaryRef infoDict = (CFMutableDictionaryRef) CFBundleGetInfoDictionary(bundle);

    // Inject the key → true
    if (infoDict) {
        CFDictionarySetValue(infoDict,
                             CFSTR("NSAppSleepDisabled"),
                             kCFBooleanTrue);
    }
}
#endif
