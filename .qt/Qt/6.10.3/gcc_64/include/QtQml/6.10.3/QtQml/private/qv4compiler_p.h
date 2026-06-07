// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QV4COMPILER_P_H
#define QV4COMPILER_P_H

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

#include <QtCore/qstring.h>
#include <QtCore/qhash.h>
#include <QtCore/qstringlist.h>
#include <private/qv4compilerglobal_p.h>
#include <private/qqmljsastfwd_p.h>
#include <private/qv4compileddata_p.h>
#include <private/qv4staticvalue_p.h>

QT_BEGIN_NAMESPACE

class QQmlPropertyData;

namespace QV4 {

namespace CompiledData {
struct Unit;
struct Lookup;
struct RegExp;
struct JSClassMember;
}

namespace Compiler {

struct Context;
struct Module;
struct Class;
struct TemplateObject;

struct Q_QML_COMPILER_EXPORT StringTableGenerator {
    StringTableGenerator();

    int registerString(const QString &str);
    int getStringId(const QString &string) const;
    bool hasStringId(const QString &string) const { return stringToId.contains(string); }
    QString stringForIndex(int index) const { return strings.at(index); }
    uint stringCount() const { return strings.size() - backingUnitTableSize; }

    uint sizeOfTableAndData() const { return stringDataSize + ((stringCount() * sizeof(uint) + 7) & ~7); }

    void freeze() { frozen = true; }

    void clear();

    void initializeFromBackingUnit(const CompiledData::Unit *unit);

    void serialize(CompiledData::Unit *unit);
    QStringList allStrings() const { return strings.mid(backingUnitTableSize); }

private:
    QHash<QString, int> stringToId;
    QStringList strings;
    uint stringDataSize;
    uint backingUnitTableSize = 0;
    bool frozen = false;
};

struct Q_QML_COMPILER_EXPORT JSUnitGenerator {
    enum LookupMode { LookupForStorage, LookupForCall };

    static void generateUnitChecksum(CompiledData::Unit *unit);

    struct MemberInfo {
        QString name;
        bool isAccessor;
    };

    JSUnitGenerator(Module *module);

    int registerString(const QString &str) { return stringTable.registerString(str); }
    int getStringId(const QString &string) const { return stringTable.getStringId(string); }
    bool hasStringId(const QString &string) const { return stringTable.hasStringId(string); }
    QString stringForIndex(int index) const { return stringTable.stringForIndex(index); }

    int registerGetterLookup(const QString &name, LookupMode mode);
    int registerGetterLookup(int nameIndex, LookupMode mode);
    int registerSetterLookup(const QString &name);
    int registerSetterLookup(int nameIndex);
    int registerGlobalGetterLookup(int nameIndex, LookupMode mode);
    int registerQmlContextPropertyGetterLookup(int nameIndex, LookupMode mode);
    int lookupNameIndex(int index) const { return lookups[index].nameIndex(); }
    QString lookupName(int index) const { return stringForIndex(lookupNameIndex(index)); }

    int registerRegExp(QQmlJS::AST::RegExpLiteral *regexp);

    int registerConstant(ReturnedValue v);
    ReturnedValue constant(int idx) const;

    int registerJSClass(const QStringList &members);
    int jsClassSize(int jsClassId) const;
    QString jsClassMember(int jsClassId, int member) const;

    int registerTranslation(const CompiledData::TranslationData &translation);

    enum GeneratorOption {
        GenerateWithStringTable,
        GenerateWithoutStringTable
    };

    QV4::CompiledData::Unit *generateUnit(GeneratorOption option = GenerateWithStringTable);
    void writeFunction(char *f, Context *irFunction) const;
    void writeClass(char *f, const Class &c);
    void writeTemplateObject(char *f, const TemplateObject &o);
    void writeBlock(char *f, Context *irBlock) const;

    StringTableGenerator stringTable;
    QString codeGeneratorName;

private:
    CompiledData::Unit generateHeader(GeneratorOption option, quint32_le *functionOffsets, uint *jsClassDataOffset);

    Module *module;

    QList<CompiledData::Lookup> lookups;
    QVector<CompiledData::RegExp> regexps;
    QVector<ReturnedValue> constants;
    QByteArray jsClassData;
    QVector<int> jsClassOffsets;
    QVector<CompiledData::TranslationData> translations;
};

}

}

QT_END_NAMESPACE

#endif
