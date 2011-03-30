#ifndef MAVLINKSYNTAXHIGHLIGHTER_H
#define MAVLINKSYNTAXHIGHLIGHTER_H

#include <QSyntaxHighlighter>

class MAVLinkSyntaxHighlighter : public QSyntaxHighlighter
{
    Q_OBJECT
public:
    explicit MAVLinkSyntaxHighlighter(QObject *parent = 0);

signals:

public slots:
    void highlightBlock(const QString &text);

};

#endif // MAVLINKSYNTAXHIGHLIGHTER_H
