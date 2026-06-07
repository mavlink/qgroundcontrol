// Copyright (C) 2014 John Layt <jlayt@kde.org>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QPRINTDEVICE_H
#define QPRINTDEVICE_H

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

#include <QtPrintSupport/private/qtprintsupportglobal_p.h>
#include "private/qprint_p.h"

#include <QtCore/qsharedpointer.h>
#include <QtGui/qpagelayout.h>

QT_BEGIN_NAMESPACE

#ifndef QT_NO_PRINTER

class QPlatformPrintDevice;
class QMarginsF;
class QMimeType;
class QDebug;

class Q_PRINTSUPPORT_EXPORT QPrintDevice
{
public:

    QPrintDevice();
    QPrintDevice(const QString & id);
    QPrintDevice(const QPrintDevice &other);
    ~QPrintDevice();

    QPrintDevice &operator=(const QPrintDevice &other);
    QPrintDevice &operator=(QPrintDevice &&other) { swap(other); return *this; }

    void swap(QPrintDevice &other) { d.swap(other.d); }

    bool operator==(const QPrintDevice &other) const;

    QString id() const;
    QString name() const;
    QString location() const;
    QString makeAndModel() const;

    bool isValid() const;
    bool isDefault() const;
    bool isRemote() const;

    QPrint::DeviceState state() const;

    bool isValidPageLayout(const QPageLayout &layout, int resolution) const;

    bool supportsMultipleCopies() const;
    bool supportsCollateCopies() const;

    QPageSize defaultPageSize() const;
    QList<QPageSize> supportedPageSizes() const;

    QPageSize supportedPageSize(const QPageSize &pageSize) const;
    QPageSize supportedPageSize(QPageSize::PageSizeId pageSizeId) const;
    QPageSize supportedPageSize(const QString &pageName) const;
    QPageSize supportedPageSize(const QSize &pointSize) const;
    QPageSize supportedPageSize(const QSizeF &size, QPageSize::Unit units = QPageSize::Point) const;

    bool supportsCustomPageSizes() const;

    QSize minimumPhysicalPageSize() const;
    QSize maximumPhysicalPageSize() const;

    QMarginsF printableMargins(const QPageSize &pageSize, QPageLayout::Orientation orientation, int resolution) const;

    int defaultResolution() const;
    QList<int> supportedResolutions() const;

    QPrint::InputSlot defaultInputSlot() const;
    QList<QPrint::InputSlot> supportedInputSlots() const;

    QPrint::OutputBin defaultOutputBin() const;
    QList<QPrint::OutputBin> supportedOutputBins() const;

    QPrint::DuplexMode defaultDuplexMode() const;
    QList<QPrint::DuplexMode> supportedDuplexModes() const;

    QPrint::ColorMode defaultColorMode() const;
    QList<QPrint::ColorMode> supportedColorModes() const;

    enum PrintDevicePropertyKey {
        PDPK_CustomBase = 0xff00
    };

    QVariant property(PrintDevicePropertyKey key) const;
    bool setProperty(PrintDevicePropertyKey key, const QVariant &value);
    bool isFeatureAvailable(PrintDevicePropertyKey key, const QVariant &params) const;

#if QT_CONFIG(mimetype)
    QList<QMimeType> supportedMimeTypes() const;
#endif

#  ifndef QT_NO_DEBUG_STREAM
    void format(QDebug debug) const;
#  endif

private:
    friend class QPlatformPrinterSupport;
    friend class QPlatformPrintDevice;
    QPrintDevice(QPlatformPrintDevice *dd);
    QSharedPointer<QPlatformPrintDevice> d;
};

Q_DECLARE_SHARED(QPrintDevice)

#  ifndef QT_NO_DEBUG_STREAM
Q_PRINTSUPPORT_EXPORT QDebug operator<<(QDebug debug, const QPrintDevice &);
#  endif
#endif // QT_NO_PRINTER

QT_END_NAMESPACE

#endif // QPLATFORMPRINTDEVICE_H
