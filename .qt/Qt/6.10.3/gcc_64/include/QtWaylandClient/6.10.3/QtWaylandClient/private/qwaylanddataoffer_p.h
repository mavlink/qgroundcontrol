// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QWAYLANDDATAOFFER_H
#define QWAYLANDDATAOFFER_H

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

#include <QtCore/qhash.h>
#include <QtCore/qstring.h>

#include <QtGui/private/qinternalmimedata_p.h>

#include <QtWaylandClient/private/qtwaylandclientglobal_p.h>
#include <QtWaylandClient/private/qwayland-wayland.h>

QT_REQUIRE_CONFIG(wayland_datadevice);

QT_BEGIN_NAMESPACE

namespace QtWaylandClient {

class QWaylandDisplay;
class QWaylandMimeData;

class QWaylandAbstractDataOffer
{
public:
    virtual void startReceiving(const QString &mimeType, int fd) = 0;
    virtual QMimeData *mimeData() = 0;

    virtual ~QWaylandAbstractDataOffer() = default;
};

class Q_WAYLANDCLIENT_EXPORT QWaylandDataOffer
        : public QtWayland::wl_data_offer // needs to be the first because we do static casts from the user pointer to the wrapper
        , public QWaylandAbstractDataOffer
{
public:
    explicit QWaylandDataOffer(QWaylandDisplay *display, struct ::wl_data_offer *offer);
    ~QWaylandDataOffer() override;
    QMimeData *mimeData() override;
    Qt::DropActions supportedActions() const;

    QString firstFormat() const;

    void startReceiving(const QString &mimeType, int fd) override;

protected:
    void data_offer_offer(const QString &mime_type) override;
    void data_offer_source_actions(uint32_t source_actions) override;
    void data_offer_action(uint32_t dnd_action) override;

private:
    QWaylandDisplay *m_display = nullptr;
    QScopedPointer<QWaylandMimeData> m_mimeData;
    Qt::DropActions m_supportedActions;
};


class QWaylandMimeData : public QInternalMimeData {
public:
    explicit QWaylandMimeData(QWaylandAbstractDataOffer *dataOffer);
    ~QWaylandMimeData() override;

    void appendFormat(const QString &mimeType);

protected:
    bool hasFormat_sys(const QString &mimeType) const override;
    QStringList formats_sys() const override;
    QVariant retrieveData_sys(const QString &mimeType, QMetaType type) const override;

private:
    int readData(int fd, QByteArray &data) const;

    QWaylandAbstractDataOffer *m_dataOffer = nullptr;
    mutable QStringList m_types;
    mutable QHash<QString, QByteArray> m_data;
};

} // namespace QtWaylandClient

QT_END_NAMESPACE
#endif
