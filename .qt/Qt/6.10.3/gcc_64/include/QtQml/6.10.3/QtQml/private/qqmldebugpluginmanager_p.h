// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QQMLDEBUGPLUGINMANAGER_P_H
#define QQMLDEBUGPLUGINMANAGER_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QDebug>
#include <private/qtqmlglobal_p.h>
#include <private/qfactoryloader_p.h>

QT_BEGIN_NAMESPACE

#if !QT_CONFIG(qml_debug)

#define Q_QML_DEBUG_PLUGIN_LOADER(interfaceName)\
    static interfaceName *load##interfaceName(const QString &key)\
    {\
        qWarning() << "Qml Debugger: QtQml is not configured for debugging. Ignoring request for"\
                   << "debug plugin" << key;\
        return 0;\
    }\
    Q_DECL_UNUSED static QList<QPluginParsedMetaData> metaDataFor##interfaceName()\
    {\
        return {};\
    }

#else // QT_CONFIG(qml_debug)

#define Q_QML_DEBUG_PLUGIN_LOADER(interfaceName)\
    Q_GLOBAL_STATIC_WITH_ARGS(QFactoryLoader, interfaceName##Loader,\
        (interfaceName##Factory_iid, QLatin1String("/qmltooling")))\
    static interfaceName *load##interfaceName(const QString &key)\
    {\
        return qLoadPlugin<interfaceName, interfaceName##Factory>(interfaceName##Loader(), key);\
    }\
    Q_DECL_UNUSED static QList<QPluginParsedMetaData> metaDataFor##interfaceName()\
    {\
        return interfaceName##Loader()->metaData();\
    }

#endif // QT_CONFIG(qml_debug)

QT_END_NAMESPACE
#endif // QQMLDEBUGPLUGINMANAGER_P_H
