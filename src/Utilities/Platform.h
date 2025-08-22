/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

namespace Platform {

// Call before constructing Q(Core)Application.
void setupPreApp(bool quietWindowsAsserts);

// Call after Q(Core)Application exists and logging is installed.
void setupPostApp();

} // namespace Platform
