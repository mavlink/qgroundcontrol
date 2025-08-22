#pragma once

#include <QtCore/QtSystemDetection>

namespace Platform {
#ifdef Q_OS_MAC
    /// Prevent Apple's app nap from screwing us over
    /// tip: the domain can be cross-checked on the command line with <defaults domains>
    void disableAppNapViaInfoDict();
#endif
}
