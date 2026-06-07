// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:insignificant

#ifndef QTQMLGLOBAL_H
#define QTQMLGLOBAL_H


#include <QtCore/qglobal.h>
#include <QtQml/qtqml-config.h>
#if QT_CONFIG(qml_network)
#  include <QtNetwork/qtnetworkglobal.h>
#endif

#  include <QtQml/qtqmlexports.h>

#endif // QTQMLGLOBAL_H
