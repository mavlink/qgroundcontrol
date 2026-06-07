// Copyright (C) 2016 The Qt Company Ltd.
// Copyright (C) 2014 BlackBerry Limited. All rights reserved.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QHTTPPROTOCOLHANDLER_H
#define QHTTPPROTOCOLHANDLER_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of the Network Access API.  This header file may change from
// version to version without notice, or even be removed.
//
// We mean it.
//

#include <QtNetwork/private/qtnetworkglobal_p.h>
#include <private/qabstractprotocolhandler_p.h>

#include <QtCore/qbytearray.h>

QT_REQUIRE_CONFIG(http);

QT_BEGIN_NAMESPACE

class QHttpProtocolHandler : public QAbstractProtocolHandler {
public:
    QHttpProtocolHandler(QHttpNetworkConnectionChannel *channel);

private:
    virtual void _q_receiveReply() override;
    virtual void _q_readyRead() override;
    virtual bool sendRequest() override;

    QByteArray m_header;
};

QT_END_NAMESPACE

#endif
