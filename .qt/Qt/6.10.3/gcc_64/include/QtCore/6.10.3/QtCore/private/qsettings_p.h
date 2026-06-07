// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QSETTINGS_P_H
#define QSETTINGS_P_H

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

#include "QtCore/qdatetime.h"
#include "QtCore/qmap.h"
#include "QtCore/qmutex.h"
#include "QtCore/qiodevice.h"
#include "QtCore/qstack.h"
#include "QtCore/qstringlist.h"

#include <QtCore/qvariant.h>
#include "qsettings.h"

#ifndef QT_NO_QOBJECT
#include "private/qobject_p.h"
#endif

QT_BEGIN_NAMESPACE

#ifndef Q_OS_WIN
#define QT_QSETTINGS_ALWAYS_CASE_SENSITIVE_AND_FORGET_ORIGINAL_KEY_ORDER
#endif

// used in testing framework
#define QSETTINGS_P_H_VERSION 3

#ifdef QT_QSETTINGS_ALWAYS_CASE_SENSITIVE_AND_FORGET_ORIGINAL_KEY_ORDER
static const Qt::CaseSensitivity IniCaseSensitivity = Qt::CaseSensitive;

class QSettingsKey : public QString
{
public:
    inline QSettingsKey(const QString &key, Qt::CaseSensitivity cs, qsizetype /* position */ = -1)
        : QString(key) { Q_ASSERT(cs == Qt::CaseSensitive); Q_UNUSED(cs); }

    inline QString originalCaseKey() const { return *this; }
    inline qsizetype originalKeyPosition() const { return -1; }
};
#else
static const Qt::CaseSensitivity IniCaseSensitivity = Qt::CaseInsensitive;

class QSettingsKey : public QString
{
public:
    inline QSettingsKey(const QString &key, Qt::CaseSensitivity cs, qsizetype position = -1)
        : QString(key), theOriginalKey(key), theOriginalKeyPosition(position)
    {
        if (cs == Qt::CaseInsensitive)
            QString::operator=(toLower());
    }

    inline QString originalCaseKey() const { return theOriginalKey; }
    inline qsizetype originalKeyPosition() const { return theOriginalKeyPosition; }

private:
    QString theOriginalKey;
    qsizetype theOriginalKeyPosition;
};
#endif

Q_DECLARE_TYPEINFO(QSettingsKey, Q_RELOCATABLE_TYPE);

typedef QMap<QSettingsKey, QByteArray> UnparsedSettingsMap;
typedef QMap<QSettingsKey, QVariant> ParsedSettingsMap;

class QSettingsGroup
{
public:
    inline QSettingsGroup()
        : num(-1), maxNum(-1) {}
    inline QSettingsGroup(const QString &s)
        : str(s), num(-1), maxNum(-1) {}
    inline QSettingsGroup(const QString &s, bool guessArraySize)
        : str(s), num(0), maxNum(guessArraySize ? 0 : -1) {}

    inline QString name() const { return str; }
    inline QString toString() const;
    inline bool isArray() const { return num != -1; }
    inline qsizetype arraySizeGuess() const { return maxNum; }
    inline void setArrayIndex(qsizetype i)
    { num = i + 1; if (maxNum != -1 && num > maxNum) maxNum = num; }

    QString str;
    qsizetype num;
    qsizetype maxNum;
};
Q_DECLARE_TYPEINFO(QSettingsGroup, Q_RELOCATABLE_TYPE);

inline QString QSettingsGroup::toString() const
{
    QString result;
    result = str;
    if (num > 0) {
        result += u'/';
        result += QString::number(num);
    }
    return result;
}

class QConfFile
{
public:
    ~QConfFile();

    ParsedSettingsMap mergedKeyMap() const;
    bool isWritable() const;

    static QConfFile *fromName(const QString &name, bool _userPerms);
    Q_AUTOTEST_EXPORT
    static void clearCache();

    QString name;
    QDateTime timeStamp;
    qint64 size;
    UnparsedSettingsMap unparsedIniSections;
    ParsedSettingsMap originalKeys;
    ParsedSettingsMap addedKeys;
    ParsedSettingsMap removedKeys;
    QAtomicInt ref;
    QMutex mutex;
    bool userPerms;

private:
#ifdef Q_DISABLE_COPY
    QConfFile(const QConfFile &);
    QConfFile &operator=(const QConfFile &);
#endif
    QConfFile(const QString &name, bool _userPerms);

    friend class QConfFile_createsItself; // silences compiler warning
};

class Q_AUTOTEST_EXPORT QSettingsPrivate
#ifndef QT_NO_QOBJECT
    : public QObjectPrivate
#endif
{
#ifdef QT_NO_QOBJECT
    QSettings *q_ptr;
#endif
    Q_DECLARE_PUBLIC(QSettings)

public:
    QSettingsPrivate(QSettings::Format format);
    QSettingsPrivate(QSettings::Format format, QSettings::Scope scope,
                     const QString &organization, const QString &application);
    virtual ~QSettingsPrivate();

    virtual void remove(const QString &key) = 0;
    virtual void set(const QString &key, const QVariant &value) = 0;
    virtual std::optional<QVariant> get(const QString &key) const = 0;

    enum ChildSpec { AllKeys, ChildKeys, ChildGroups };
    virtual QStringList children(const QString &prefix, ChildSpec spec) const = 0;

    virtual void clear() = 0;
    virtual void sync() = 0;
    virtual void flush() = 0;
    virtual bool isWritable() const = 0;
    virtual QString fileName() const = 0;

    QVariant value(QAnyStringView key, const QVariant *defaultValue) const;
    QString actualKey(QAnyStringView key) const;
    void beginGroupOrArray(const QSettingsGroup &group);
    void setStatus(QSettings::Status status) const;
    void requestUpdate();
    void update();

    static QString normalizedKey(QAnyStringView key);
    static QSettingsPrivate *create(QSettings::Format format, QSettings::Scope scope,
                                        const QString &organization, const QString &application);
    static QSettingsPrivate *create(const QString &fileName, QSettings::Format format);

    static void processChild(QStringView key, ChildSpec spec, QStringList &result);

    // Variant streaming functions
    static QStringList variantListToStringList(const QVariantList &l);
    static QVariant stringListToVariantList(const QStringList &l);

    // parser functions
    static QString variantToString(const QVariant &v);
    static QVariant stringToVariant(const QString &s);
    static void iniEscapedKey(const QString &key, QByteArray &result);
    static bool iniUnescapedKey(QByteArrayView key, QString &result);
    static void iniEscapedString(const QString &str, QByteArray &result);
    static void iniEscapedStringList(const QStringList &strs, QByteArray &result);
    static bool iniUnescapedStringList(QByteArrayView str, QString &stringResult,
                                       QStringList &stringListResult);
    static QStringList splitArgs(const QString &s, qsizetype idx);

    QSettings::Format format;
    QSettings::Scope scope;
    QString organizationName;
    QString applicationName;

protected:
    QStack<QSettingsGroup> groupStack;
    QString groupPrefix;
    bool fallbacks;
    bool pendingChanges;
    bool atomicSyncOnly = true;
    mutable QSettings::Status status;
};

class QConfFileSettingsPrivate : public QSettingsPrivate
{
public:
    QConfFileSettingsPrivate(QSettings::Format format, QSettings::Scope scope,
                             const QString &organization, const QString &application);
    QConfFileSettingsPrivate(const QString &fileName, QSettings::Format format);
    ~QConfFileSettingsPrivate();

    void remove(const QString &key) override;
    void set(const QString &key, const QVariant &value) override;
    std::optional<QVariant> get(const QString &key) const override;

    QStringList children(const QString &prefix, ChildSpec spec) const override;

    void clear() override;
    void sync() override;
    void flush() override;
    bool isWritable() const override;
    QString fileName() const override;

    bool readIniFile(QByteArrayView data, UnparsedSettingsMap *unparsedIniSections);
    static bool readIniSection(const QSettingsKey &section, QByteArrayView data,
                               ParsedSettingsMap *settingsMap);
    static bool readIniLine(QByteArrayView data, qsizetype &dataPos,
                            qsizetype &lineStart, qsizetype &lineLen,
                            qsizetype &equalsPos);

protected:
    const QList<QConfFile *> &getConfFiles() const { return confFiles; }

private:
    void initFormat();
    virtual void initAccess();
    void syncConfFile(QConfFile *confFile);
    bool writeIniFile(QIODevice &device, const ParsedSettingsMap &map);
#ifdef Q_OS_DARWIN
    bool readPlistFile(const QByteArray &data, ParsedSettingsMap *map) const;
    bool writePlistFile(QIODevice &file, const ParsedSettingsMap &map) const;
#endif
    void ensureAllSectionsParsed(QConfFile *confFile) const;
    void ensureSectionParsed(QConfFile *confFile, const QSettingsKey &key) const;

    QList<QConfFile *> confFiles;
    QSettings::ReadFunc readFunc;
    QSettings::WriteFunc writeFunc;
    QString extension;
    Qt::CaseSensitivity caseSensitivity;
    qsizetype nextPosition;
#ifdef Q_OS_WASM
    friend class QWasmIDBSettingsPrivate;
#endif
};

QT_END_NAMESPACE

#endif // QSETTINGS_P_H
