// Volador Branding
// Phase 1

/****************************************************************************
 *
 * (c) 2026 Volador Aerospace. All rights reserved.
 *
 * Volador Ground Control Station (VGCS) - Version & Metadata Constants
 *
 ****************************************************************************/

#pragma once

#include <QtCore/QString>

namespace Volador {

class Version {
public:
    static constexpr const char* APP_NAME             = "Volador Ground Control Station";
    static constexpr const char* APP_SHORT_NAME       = "VGCS";
    static constexpr const char* APP_COMPANY          = "Volador Aerospace";
    static constexpr const char* APP_DESCRIPTION      = "Enterprise Drone Mission Control Platform";
    static constexpr const char* APP_WEBSITE          = "https://volador.in";
    static constexpr const char* APP_COPYRIGHT        = "© 2026 Volador Aerospace. All Rights Reserved.";

    static constexpr int VERSION_MAJOR                = 2;
    static constexpr int VERSION_MINOR                = 0;
    static constexpr int VERSION_PATCH                = 0;
    static constexpr const char* VERSION_SUFFIX       = "alpha.1";
    static constexpr const char* VERSION_STRING       = "2.0.0-alpha.1";

    static QString versionString() { return QStringLiteral("2.0.0-alpha.1"); }
    static QString appName()       { return QStringLiteral("Volador Ground Control Station"); }
    static QString appShortName()  { return QStringLiteral("VGCS"); }
    static QString company()       { return QStringLiteral("Volador Aerospace"); }
    static QString description()   { return QStringLiteral("Enterprise Drone Mission Control Platform"); }
    static QString website()       { return QStringLiteral("https://volador.in"); }
    static QString copyright()     { return QString::fromUtf8("© 2026 Volador Aerospace. All Rights Reserved."); }
};

} // namespace Volador
