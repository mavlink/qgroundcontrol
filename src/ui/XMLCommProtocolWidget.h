#ifndef XMLCOMMPROTOCOLWIDGET_H
#define XMLCOMMPROTOCOLWIDGET_H

#include <QtGui/QWidget>
#include "DomModel.h"
#include "MAVLinkSyntaxHighlighter.h"

namespace Ui {
    class XMLCommProtocolWidget;
}

/**
 * @brief Tool to generate MAVLink code out of XML protocol definitions
 * @see http://doc.trolltech.com/4.6/itemviews-simpledommodel.html for a XML view tutorial
 */
class XMLCommProtocolWidget : public QWidget {
    Q_OBJECT
public:
    XMLCommProtocolWidget(QWidget *parent = 0);
    ~XMLCommProtocolWidget();

protected slots:
    /** @brief Select input XML protocol definition */
    void selectXMLFile();
    /** @brief Select output directory for generated .h files */
    void selectOutputDirectory();
    /** @brief Set the XML this widget currently operates on */
    void setXML(const QString& xml);
    /** @brief Parse XML file and generate .h files */
    void generate();
    /** @brief Save the edited file */
    void save();

protected:
    MAVLinkSyntaxHighlighter* highlighter;
    DomModel* model;
    void changeEvent(QEvent *e);

private:
    Ui::XMLCommProtocolWidget *m_ui;
};

#endif // XMLCOMMPROTOCOLWIDGET_H
