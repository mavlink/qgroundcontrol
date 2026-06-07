// Copyright (C) 2022 Giuseppe D'Angelo <dangelog@gmail.com>.
// Copyright (C) 2022 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Giuseppe D'Angelo <giuseppe.dangelo@kdab.com>
// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//! [0]
QString p("a .*|pattern");

// re matches exactly the pattern string p
QRegularExpression re(QRegularExpression::anchoredPattern(p));
//! [0]

//! [1]
QString subject("the quick fox");

int offset = 0;
QRegExp re("(\\w+)");
while ((offset = re.indexIn(subject, offset)) != -1) {
    offset += re.matchedLength();
    // ...
}
//! [1]

//! [2]
QString subject("the quick fox");

QRegularExpression re("(\\w+)");
QRegularExpressionMatchIterator i = re.globalMatch(subject);
while (i.hasNext()) {
    QRegularExpressionMatch match = i.next();
    // ...
}
//! [2]

//! [3]
QRegExp wildcard("*.txt");
wildcard.setPatternSyntax(QRegExp::Wildcard);
//! [3]

//! [4]
auto wildcard = QRegularExpression(QRegularExpression::wildcardToRegularExpression("*.txt"));
//! [4]

//! [5]
const QString fp1("C:/Users/dummy/files/content.txt");
const QString fp2("/home/dummy/files/content.txt");

QRegExp re1("*/files/*");
re1.setPatternSyntax(QRegExp::Wildcard);
re1.exactMatch(fp1); // returns true
re1.exactMatch(fp2); // returns true

// but converted with QRegularExpression::wildcardToRegularExpression()

QRegularExpression re2(QRegularExpression::wildcardToRegularExpression("*/files/*"));
re2.match(fp1).hasMatch(); // returns false
re2.match(fp2).hasMatch(); // returns false
//! [5]

//! [6]
QRegularExpression re3(QRegularExpression::wildcardToRegularExpression(
                           "*/files/*", QRegularExpression::UnanchoredWildcardConversion));
re3.match(fp1).hasMatch(); // returns true
re3.match(fp2).hasMatch(); // returns true
//! [6]
