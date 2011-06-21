#include "QGCMAVLinkTextEdit.h"
#include <QMessageBox>
#include <QMenu>
#include <QEvent>
#include <QPainter>
#include <QScrollBar>
#include <QTextLayout>

QGCMAVLinkTextEdit::QGCMAVLinkTextEdit(QWidget *parent)
    : QTextEdit(parent)
{
    setViewportMargins(50, 0, 0, 0);
    highlight = new XmlHighlighter(document());
    setLineWrapMode ( QTextEdit::NoWrap );
    setAcceptRichText ( false );
    connect(verticalScrollBar(), SIGNAL(valueChanged(int)), this, SLOT(update()));
    connect(this, SIGNAL(textChanged()), this, SLOT(update()));
}


bool QGCMAVLinkTextEdit::Conform()
{
    QString errorStr;
    int errorLine, errorColumn;
    QDomDocument doc;
    return doc.setContent(text(),false, &errorStr, &errorLine, &errorColumn);
}

QDomDocument QGCMAVLinkTextEdit::xml_document()
{
    QString errorStr;
    int errorLine, errorColumn;
    QDomDocument doc;
    doc.setContent(text(),false, &errorStr, &errorLine, &errorColumn);
    return doc;
}


void QGCMAVLinkTextEdit::setPlainText( const QString txt )
{
    QString errorStr;
    int errorLine, errorColumn;
    QDomDocument doc;
    if (!doc.setContent(txt,false, &errorStr, &errorLine, &errorColumn)) {
        QTextEdit::setPlainText(txt);
    } else {
        QTextEdit::setPlainText(doc.toString(5));
    }
}

bool QGCMAVLinkTextEdit::syntaxcheck()
{
    bool error = false;
    if (text().size() > 0 ) {
        QString errorStr;
        int errorLine, errorColumn;
        QDomDocument doc;
        if (!doc.setContent(text(),false, &errorStr, &errorLine, &errorColumn)) {
            //////return doc.toString(5);
            QMessageBox::information(0, tr("Found xml error"),tr("Check line %1 column %2 on string \"%3\"!")
                                     .arg(errorLine - 1)
                                     .arg(errorColumn - 1)
                                     .arg(errorStr));
            error = true;

            // FIXME Mark line
            if (errorLine >= 0 ) {

            }
        } else {
            QMessageBox::information(0, tr("XML valid."),tr("All tag are valid size %1.").arg(text().size()));
            setPlainText(doc.toString(5));
        }
    } else {
        QMessageBox::information(0, tr("XML not found!"),tr("Null size xml document!"));
        error = true;
    }
}

void QGCMAVLinkTextEdit::contextMenuEvent ( QContextMenuEvent * e )
{
    QMenu *RContext = createOwnStandardContextMenu();
    RContext->exec(QCursor::pos());
    delete RContext;
}

QMenu *QGCMAVLinkTextEdit::createOwnStandardContextMenu()
{
    QMenu *TContext = createStandardContextMenu();
    TContext->addAction(QIcon(QString::fromUtf8(":/img/zoomin.png")),tr( "Zoom In" ), this , SLOT( zoomIn() ) );
    TContext->addAction(QIcon(QString::fromUtf8(":/img/zoomout.png")),tr( "Zoom Out" ), this , SLOT( zoomOut() ) );
    TContext->addAction(tr("Check xml syntax" ), this , SLOT( syntaxcheck() ) );
    return TContext;
}

bool QGCMAVLinkTextEdit::event( QEvent *event )
{
    if (event->type()==QEvent::Paint) {
        QPainter p(this);
        p.fillRect(0, 0, 50, height(), QColor("#636363"));
        QFont workfont(font());
        QPen pen(QColor("#ffffff"),1);
        p.setPen(pen);
        p.setFont (workfont);
        int contentsY = verticalScrollBar()->value();
        qreal pageBottom = contentsY+viewport()->height();
        int m_lineNumber(1);
        const QFontMetrics fm=fontMetrics();
        const int ascent = fontMetrics().ascent() +1;

        for (QTextBlock block=document()->begin(); block.isValid(); block=block.next(), m_lineNumber++) {
            QTextLayout *layout = block.layout();
            const QRectF boundingRect = layout->boundingRect();
            QPointF position = layout->position();
            if ( position.y() +boundingRect.height() < contentsY ) {
                continue;
            }
            if ( position.y() > pageBottom ) {
                break;
            }
            const QString txt = QString::number(m_lineNumber);
            p.drawText(50-fm.width(txt)-2, qRound(position.y())-contentsY+ascent, txt);

        }
        p.setPen(QPen(Qt::NoPen));
    } else if ( event->type() == QEvent::KeyPress ) {
        QKeyEvent *ke = static_cast<QKeyEvent *>(event);
        if ((ke->modifiers() & Qt::ControlModifier) && ke->key() == Qt::Key_Minus)   {
            QTextEdit::zoomOut();
            return true;
        }
        if ((ke->modifiers() & Qt::ControlModifier) && ke->key() == Qt::Key_Plus)   {
            QTextEdit::zoomIn();
            return true;
        }
    }
    return QTextEdit::event(event);
}















static const QColor DEFAULT_SYNTAX_CHAR		= Qt::blue;
static const QColor DEFAULT_ELEMENT_NAME	= Qt::darkRed;
static const QColor DEFAULT_COMMENT			= Qt::darkGreen;
static const QColor DEFAULT_ATTRIBUTE_NAME	= Qt::red;
static const QColor DEFAULT_ATTRIBUTE_VALUE	= Qt::darkGreen;
static const QColor DEFAULT_ERROR			= Qt::darkMagenta;
static const QColor DEFAULT_OTHER			= Qt::black;

// Regular expressions for parsing XML borrowed from:
// http://www.cs.sfu.ca/~cameron/REX.html
static const QString EXPR_COMMENT			= "<!--[^-]*-([^-][^-]*-)*->";
static const QString EXPR_COMMENT_BEGIN		= "<!--";
static const QString EXPR_COMMENT_END		= "[^-]*-([^-][^-]*-)*->";
static const QString EXPR_ATTRIBUTE_VALUE	= "\"[^<\"]*\"|'[^<']*'";
static const QString EXPR_NAME				= "([A-Za-z_:]|[^\\x00-\\x7F])([A-Za-z0-9_:.-]|[^\\x00-\\x7F])*";

XmlHighlighter::XmlHighlighter(QObject* parent)
    : QSyntaxHighlighter(parent)
{
    init();
}

XmlHighlighter::XmlHighlighter(QTextDocument* parent)
    : QSyntaxHighlighter(parent)
{
    init();
}

XmlHighlighter::XmlHighlighter(QTextEdit* parent)
    : QSyntaxHighlighter(parent)
{
    init();
}

XmlHighlighter::~XmlHighlighter()
{
}

void XmlHighlighter::init()
{
    fmtSyntaxChar.setForeground(DEFAULT_SYNTAX_CHAR);
    fmtElementName.setForeground(DEFAULT_ELEMENT_NAME);
    fmtComment.setForeground(DEFAULT_COMMENT);
    fmtAttributeName.setForeground(DEFAULT_ATTRIBUTE_NAME);
    fmtAttributeValue.setForeground(DEFAULT_ATTRIBUTE_VALUE);
    fmtError.setForeground(DEFAULT_ERROR);
    fmtOther.setForeground(DEFAULT_OTHER);
}

void XmlHighlighter::setHighlightColor(HighlightType type, QColor color, bool foreground)
{
    QTextCharFormat format;
    if (foreground)
        format.setForeground(color);
    else
        format.setBackground(color);
    setHighlightFormat(type, format);
}

void XmlHighlighter::setHighlightFormat(HighlightType type, QTextCharFormat format)
{
    switch (type)
    {
                case SyntaxChar:
        fmtSyntaxChar = format;
        break;
                case ElementName:
        fmtElementName = format;
        break;
                case Comment:
        fmtComment = format;
        break;
                case AttributeName:
        fmtAttributeName = format;
        break;
                case AttributeValue:
        fmtAttributeValue = format;
        break;
                case Error:
        fmtError = format;
        break;
                case Other:
        fmtOther = format;
        break;
    }
    rehighlight();
}

void XmlHighlighter::highlightBlock(const QString& text)
{
    int i = 0;
    int pos = 0;
    int brackets = 0;

    state = (previousBlockState() == InElement ? ExpectAttributeOrEndOfElement : NoState);

    if (previousBlockState() == InComment)
    {
        // search for the end of the comment
        QRegExp expression(EXPR_COMMENT_END);
        pos = expression.indexIn(text, i);

        if (pos >= 0)
        {
            // end comment found
            const int iLength = expression.matchedLength();
            setFormat(0, iLength - 3, fmtComment);
            setFormat(iLength - 3, 3, fmtSyntaxChar);
            i += iLength; // skip comment
        }
        else
        {
            // in comment
            setFormat(0, text.length(), fmtComment);
            setCurrentBlockState(InComment);
            return;
        }
    }

    for (; i < text.length(); i++)
    {
        switch (text.at(i).toAscii())
        {
                case '<':
            brackets++;
            if (brackets == 1)
            {
                setFormat(i, 1, fmtSyntaxChar);
                state = ExpectElementNameOrSlash;
            }
            else
            {
                // wrong bracket nesting
                setFormat(i, 1, fmtError);
            }
            break;

                case '>':
            brackets--;
            if (brackets == 0)
            {
                setFormat(i, 1, fmtSyntaxChar);
            }
            else
            {
                // wrong bracket nesting
                setFormat( i, 1, fmtError);
            }
            state = NoState;
            break;

                case '/':
            if (state == ExpectElementNameOrSlash)
            {
                state = ExpectElementName;
                setFormat(i, 1, fmtSyntaxChar);
            }
            else
            {
                if (state == ExpectAttributeOrEndOfElement)
                {
                    setFormat(i, 1, fmtSyntaxChar);
                }
                else
                {
                    processDefaultText(i, text);
                }
            }
            break;

                case '=':
            if (state == ExpectEqual)
            {
                state = ExpectAttributeValue;
                setFormat(i, 1, fmtOther);
            }
            else
            {
                processDefaultText(i, text);
            }
            break;

                case '\'':
                case '\"':
            if (state == ExpectAttributeValue)
            {
                // search attribute value
                QRegExp expression(EXPR_ATTRIBUTE_VALUE);
                pos = expression.indexIn(text, i);

                if (pos == i) // attribute value found ?
                {
                    const int iLength = expression.matchedLength();

                    setFormat(i, 1, fmtOther);
                    setFormat(i + 1, iLength - 2, fmtAttributeValue);
                    setFormat(i + iLength - 1, 1, fmtOther);

                    i += iLength - 1; // skip attribute value
                    state = ExpectAttributeOrEndOfElement;
                }
                else
                {
                    processDefaultText(i, text);
                }
            }
            else
            {
                processDefaultText(i, text);
            }
            break;

                case '!':
            if (state == ExpectElementNameOrSlash)
            {
                // search comment
                QRegExp expression(EXPR_COMMENT);
                pos = expression.indexIn(text, i - 1);

                if (pos == i - 1) // comment found ?
                {
                    const int iLength = expression.matchedLength();

                    setFormat(pos, 4, fmtSyntaxChar);
                    setFormat(pos + 4, iLength - 7, fmtComment);
                    setFormat(iLength - 3, 3, fmtSyntaxChar);
                    i += iLength - 2; // skip comment
                    state = NoState;
                    brackets--;
                }
                else
                {
                    // Try find multiline comment
                    QRegExp expression(EXPR_COMMENT_BEGIN); // search comment start
                    pos = expression.indexIn(text, i - 1);

                    //if (pos == i - 1) // comment found ?
                    if (pos >= i - 1)
                    {
                        setFormat(i, 3, fmtSyntaxChar);
                        setFormat(i + 3, text.length() - i - 3, fmtComment);
                        setCurrentBlockState(InComment);
                        return;
                    }
                    else
                    {
                        processDefaultText(i, text);
                    }
                }
            }
            else
            {
                processDefaultText(i, text);
            }

            break;

                default:
            const int iLength = processDefaultText(i, text);
            if (iLength > 0)
                i += iLength - 1;
            break;
        }
    }

    if (state == ExpectAttributeOrEndOfElement)
    {
        setCurrentBlockState(InElement);
    }
}

int XmlHighlighter::processDefaultText(int i, const QString& text)
{
    // length of matched text
    int iLength = 0;

    switch(state)
    {
    case ExpectElementNameOrSlash:
    case ExpectElementName:
        {
            // search element name
            QRegExp expression(EXPR_NAME);
            const int pos = expression.indexIn(text, i);

            if (pos == i) // found ?
            {
                iLength = expression.matchedLength();

                setFormat(pos, iLength, fmtElementName);
                state = ExpectAttributeOrEndOfElement;
            }
            else
            {
                setFormat(i, 1, fmtOther);
            }
        }
        break;

    case ExpectAttributeOrEndOfElement:
        {
            // search attribute name
            QRegExp expression(EXPR_NAME);
            const int pos = expression.indexIn(text, i);

            if (pos == i) // found ?
            {
                iLength = expression.matchedLength();

                setFormat(pos, iLength, fmtAttributeName);
                state = ExpectEqual;
            }
            else
            {
                setFormat(i, 1, fmtOther);
            }
        }
        break;

    default:
        setFormat(i, 1, fmtOther);
        break;
    }
    return iLength;
}
