// Copyright (C) 2014 John Layt <jlayt@kde.org>
// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QPLATFORMPRINTDEVICE_H
#define QPLATFORMPRINTDEVICE_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of internal files.  This header file may change from version to version
// without notice, or even be removed.
//
// We mean it.
//

#include <QtPrintSupport/qtprintsupportglobal.h>
#include <private/qprint_p.h>
#include <private/qprintdevice_p.h>

#include <QtCore/qlist.h>
#include <QtCore/qvariant.h>
#if QT_CONFIG(mimetype)
#include <QtCore/qmimetype.h>
#endif
#include <QtGui/qpagelayout.h>


QT_BEGIN_NAMESPACE

#ifndef QT_NO_PRINTER

class Q_PRINTSUPPORT_EXPORT QPlatformPrintDevice
{
    Q_DISABLE_COPY(QPlatformPrintDevice)
public:
    explicit QPlatformPrintDevice(const QString &id = QString());
    virtual ~QPlatformPrintDevice();

    virtual QString id() const;
    virtual QString name() const;
    virtual QString location() const;
    virtual QString makeAndModel() const;

    virtual bool isValid() const;
    virtual bool isDefault() const;
    virtual bool isRemote() const;

    virtual QPrint::DeviceState state() const;

    virtual bool isValidPageLayout(const QPageLayout &layout, int resolution) const;

    virtual bool supportsMultipleCopies() const;
    virtual bool supportsCollateCopies() const;

    virtual QPageSize defaultPageSize() const;
    virtual QList<QPageSize> supportedPageSizes() const;

    virtual QPageSize supportedPageSize(const QPageSize &pageSize) const;
    virtual QPageSize supportedPageSize(QPageSize::PageSizeId pageSizeId) const;
    virtual QPageSize supportedPageSize(const QString &pageName) const;
    virtual QPageSize supportedPageSize(const QSize &pointSize) const;
    virtual QPageSize supportedPageSize(const QSizeF &size, QPageSize::Unit units) const;

    virtual bool supportsCustomPageSizes() const;

    virtual QSize minimumPhysicalPageSize() const;
    virtual QSize maximumPhysicalPageSize() const;

    virtual QMarginsF printableMargins(const QPageSize &pageSize, QPageLayout::Orientation orientation,
                                       int resolution) const;

    virtual int defaultResolution() const;
    virtual QList<int> supportedResolutions() const;

    virtual QPrint::InputSlot defaultInputSlot() const;
    virtual QList<QPrint::InputSlot> supportedInputSlots() const;

    virtual QPrint::OutputBin defaultOutputBin() const;
    virtual QList<QPrint::OutputBin> supportedOutputBins() const;

    virtual QPrint::DuplexMode defaultDuplexMode() const;
    virtual QList<QPrint::DuplexMode> supportedDuplexModes() const;

    virtual QPrint::ColorMode defaultColorMode() const;
    virtual QList<QPrint::ColorMode> supportedColorModes() const;

    virtual QVariant property(QPrintDevice::PrintDevicePropertyKey key) const;
    virtual bool setProperty(QPrintDevice::PrintDevicePropertyKey key, const QVariant &value);
    virtual bool isFeatureAvailable(QPrintDevice::PrintDevicePropertyKey key, const QVariant &params) const;

#if QT_CONFIG(mimetype)
    virtual QList<QMimeType> supportedMimeTypes() const;
#endif

    static QPageSize createPageSize(const QString &key, const QSize &size, const QString &localizedName);
    static QPageSize createPageSize(int windowsId, const QSize &size, const QString &localizedName);

protected:
    virtual void loadPageSizes() const;
    virtual void loadResolutions() const;
    virtual void loadInputSlots() const;
    virtual void loadOutputBins() const;
    virtual void loadDuplexModes() const;
    virtual void loadColorModes() const;
#if QT_CONFIG(mimetype)
    virtual void loadMimeTypes() const;
#endif

    QPageSize supportedPageSizeMatch(const QPageSize &pageSize) const;

    QString m_id;
    QString m_name;
    QString m_location;
    QString m_makeAndModel;

    bool m_isRemote;

    mutable bool m_supportsMultipleCopies;
    mutable bool m_supportsCollateCopies;

    mutable bool m_havePageSizes;
    mutable QList<QPageSize> m_pageSizes;

    mutable bool m_supportsCustomPageSizes;

    mutable QSize m_minimumPhysicalPageSize;
    mutable QSize m_maximumPhysicalPageSize;

    mutable bool m_haveResolutions;
    mutable QList<int> m_resolutions;

    mutable bool m_haveInputSlots;
    mutable QList<QPrint::InputSlot> m_inputSlots;

    mutable bool m_haveOutputBins;
    mutable QList<QPrint::OutputBin> m_outputBins;

    mutable bool m_haveDuplexModes;
    mutable QList<QPrint::DuplexMode> m_duplexModes;

    mutable bool m_haveColorModes;
    mutable QList<QPrint::ColorMode> m_colorModes;

#if QT_CONFIG(mimetype)
    mutable bool m_haveMimeTypes;
    mutable QList<QMimeType> m_mimeTypes;
#endif
};

#endif // QT_NO_PRINTER

QT_END_NAMESPACE

#endif // QPLATFORMPRINTDEVICE_H
