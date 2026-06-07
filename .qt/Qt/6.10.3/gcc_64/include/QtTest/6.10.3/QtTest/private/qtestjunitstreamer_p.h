// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QTESTJUNITSTREAMER_P_H
#define QTESTJUNITSTREAMER_P_H

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

#include <QtCore/private/qglobal_p.h>
#include <vector>

QT_BEGIN_NAMESPACE


class QTestElement;
class QTestElementAttribute;
class QJUnitTestLogger;
struct QTestCharBuffer;

class QTestJUnitStreamer
{
    public:
        QTestJUnitStreamer(QJUnitTestLogger *logger);
        ~QTestJUnitStreamer();

        void formatStart(const QTestElement *element, QTestCharBuffer *formatted) const;
        void formatEnd(const QTestElement *element, QTestCharBuffer *formatted) const;
        void formatAfterAttributes(const QTestElement *element, QTestCharBuffer *formatted) const;
        void output(QTestElement *element) const;
        void outputElements(const std::vector<QTestElement*> &) const;
        void outputElementAttributes(const QTestElement *element, const std::vector<QTestElementAttribute*> &attributes) const;

        void outputString(const char *msg) const;

    private:
        [[nodiscard]] bool formatAttributes(const QTestElement *element,
                                            const QTestElementAttribute *attribute,
                                            QTestCharBuffer *formatted) const;
        static void indentForElement(const QTestElement* element, char* buf, int size);

        QJUnitTestLogger *testLogger;
};

QT_END_NAMESPACE

#endif
