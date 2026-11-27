#pragma once

#include <QtCore/QString>

/// What the tile library uses of the application besides its settings (MapTilesSettings). Only MapTilesApp.cc has the
/// header of the application for it.
namespace MapTilesApp {

/// Tells the user \a message in a dialog, which waits for a user interface that is not there yet
void showMessage(const QString& message);

/// The application runs its unit tests: nothing is fetched from a server then
bool runningUnitTests();

}  // namespace MapTilesApp
