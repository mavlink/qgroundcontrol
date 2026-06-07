// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QFONTCONFIGDATABASE_H
#define QFONTCONFIGDATABASE_H

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
#include <QtGui/private/qfreetypefontdatabase_p.h>

QT_BEGIN_NAMESPACE

class QFontEngineFT;

class Q_GUI_EXPORT QFontconfigDatabase : public QFreeTypeFontDatabase
{
public:
    ~QFontconfigDatabase() override;
    void populateFontDatabase() override;
    void invalidate() override;
    bool supportsVariableApplicationFonts() const override;
    QFontEngineMulti *fontEngineMulti(QFontEngine *fontEngine,
                                      QFontDatabasePrivate::ExtendedScript script) override;
    QFontEngine *fontEngine(const QFontDef &fontDef, void *handle) override;
    QFontEngine *fontEngine(const QByteArray &fontData, qreal pixelSize, QFont::HintingPreference hintingPreference) override;
    QStringList fallbacksForFamily(const QString &family,
                                   QFont::Style style,
                                   QFont::StyleHint styleHint,
                                   QFontDatabasePrivate::ExtendedScript script) const override;
    QStringList addApplicationFont(const QByteArray &fontData, const QString &fileName, QFontDatabasePrivate::ApplicationFont *applicationFont = nullptr) override;
    QString resolveFontFamilyAlias(const QString &family) const override;
    QFont defaultFont() const override;

private:
    void setupFontEngine(QFontEngineFT *engine, const QFontDef &fontDef) const;
};

QT_END_NAMESPACE

#endif // QFONTCONFIGDATABASE_H
