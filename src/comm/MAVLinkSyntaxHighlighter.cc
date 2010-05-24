#include "MAVLinkSyntaxHighlighter.h"

MAVLinkSyntaxHighlighter::MAVLinkSyntaxHighlighter(QObject *parent) :
    QSyntaxHighlighter(parent)
{
}


void MAVLinkSyntaxHighlighter::highlightBlock(const QString &text)
 {
     QTextCharFormat myClassFormat;
     myClassFormat.setFontWeight(QFont::Bold);
     myClassFormat.setForeground(Qt::darkMagenta);
     QString pattern = "\"[A-Za-z0-9]+\"";

     QRegExp expression(pattern);
     int index = text.indexOf(expression);
     while (index >= 0) {
         int length = expression.matchedLength();
         setFormat(index, length, myClassFormat);
         index = text.indexOf(expression, index + length);
     }
 }
