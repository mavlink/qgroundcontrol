// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "std_catch_conversions.h"

#include <ostream>

#include <QChar>
#include <QString>

inline std::ostream& operator<<(std::ostream& os, const QChar& character) {
    return os << QString{character}.toStdString();
}

inline std::ostream& operator<<(std::ostream& os, const QString& string) {
    return os << string.toStdString();
}
