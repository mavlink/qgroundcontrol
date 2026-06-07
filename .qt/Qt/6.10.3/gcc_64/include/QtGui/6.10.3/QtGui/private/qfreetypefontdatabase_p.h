// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QFREETYPEFONTDATABASE_H
#define QFREETYPEFONTDATABASE_H

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

#include <qpa/qplatformfontdatabase.h>
#include <QtCore/QByteArray>
#include <QtCore/QString>
#include <QtCore/private/qglobal_p.h>

QT_BEGIN_NAMESPACE

struct FontFile
{
    QString fileName;
    int indexValue;
    int instanceIndex = -1;

    // Note: The data may be implicitly shared throughout the
    // font database and platform font database, so be careful
    // to never detach when accessing this member!
    const QByteArray data;
};

class Q_GUI_EXPORT QFreeTypeFontDatabase : public QPlatformFontDatabase
{
public:
    void populateFontDatabase() override;
    QFontEngine *fontEngine(const QFontDef &fontDef, void *handle) override;
    QFontEngine *fontEngine(const QByteArray &fontData, qreal pixelSize, QFont::HintingPreference hintingPreference) override;
    QStringList addApplicationFont(const QByteArray &fontData, const QString &fileName, QFontDatabasePrivate::ApplicationFont *applicationFont = nullptr) override;
    void releaseHandle(void *handle) override;
    bool supportsVariableApplicationFonts() const override;
    bool supportsColrv0Fonts() const override;

    static void addNamedInstancesForFace(void *face, int faceIndex,
                                         const QString &family, const QString &styleName,
                                         QFont::Weight weight, QFont::Stretch stretch,
                                         QFont::Style style, bool fixedPitch, bool isColor,
                                         const QSupportedWritingSystems &writingSystems,
                                         const QByteArray &fileName, const QByteArray &fontData);

    static QStringList addTTFile(const QByteArray &fontData, const QByteArray &file, QFontDatabasePrivate::ApplicationFont *applicationFont = nullptr);
};

QT_END_NAMESPACE

#endif // QFREETYPEFONTDATABASE_H
