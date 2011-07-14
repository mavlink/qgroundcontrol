
// Based on: Syntax highlighting from:
// http://code.google.com/p/fop-miniscribus/
// (GPL v2) thanks!

#ifndef QGCMAVLINKTEXTEDIT_H
#define QGCMAVLINKTEXTEDIT_H

#include <QSyntaxHighlighter>
#include <QTextCharFormat>
#include <QColor>
#include <QDomDocument>
#include <QTextEdit>

//class QGCMAVLinkTextEdit : public QTextEdit
//{
//public:
//    QGCMAVLinkTextEdit();
//};

class XmlHighlighter : public QSyntaxHighlighter
{
public:
    XmlHighlighter(QObject* parent);
    XmlHighlighter(QTextDocument* parent);
    XmlHighlighter(QTextEdit* parent);
    ~XmlHighlighter();

    enum HighlightType
    {
        SyntaxChar,
        ElementName,
        Comment,
        AttributeName,
        AttributeValue,
        Error,
        Other
    };

    void setHighlightColor(HighlightType type, QColor color, bool foreground = true);
    void setHighlightFormat(HighlightType type, QTextCharFormat format);

protected:
    void highlightBlock(const QString& rstrText);
    int  processDefaultText(int i, const QString& rstrText);

private:
    void init();

    QTextCharFormat fmtSyntaxChar;
    QTextCharFormat fmtElementName;
    QTextCharFormat fmtComment;
    QTextCharFormat fmtAttributeName;
    QTextCharFormat fmtAttributeValue;
    QTextCharFormat fmtError;
    QTextCharFormat fmtOther;

    enum ParsingState
    {
        NoState = 0,
        ExpectElementNameOrSlash,
        ExpectElementName,
        ExpectAttributeOrEndOfElement,
        ExpectEqual,
        ExpectAttributeValue
    };

    enum BlockState
    {
        NoBlock = -1,
        InComment,
        InElement
    };

    ParsingState state;
};





class QGCMAVLinkTextEdit : public QTextEdit
{
    Q_OBJECT
    //
public:
    QGCMAVLinkTextEdit( QWidget * parent = 0 );
    bool Conform();
    QDomDocument xml_document();
    inline QString text() const
    {
        return QTextEdit::toPlainText();
    }
    QMenu *createOwnStandardContextMenu();
protected:
    void contextMenuEvent ( QContextMenuEvent * e );
    bool event( QEvent *event );
private:
    XmlHighlighter *highlight;
signals:
public slots:
    bool syntaxcheck();
    void setPlainText( const QString txt );
};

#endif // QGCMAVLINKTEXTEDIT_H
