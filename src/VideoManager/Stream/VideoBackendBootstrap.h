#pragma once

#include <QtCore/QFuture>

/// Owns video backend startup policy that does not belong in the QML facade.
class VideoBackendBootstrap
{
public:
    [[nodiscard]] static bool shouldSkipForUnitTests();
    [[nodiscard]] static QFuture<bool> start(bool backendDisabledForUnitTests);
};
