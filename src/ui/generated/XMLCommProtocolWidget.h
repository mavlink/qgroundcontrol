/********************************************************************************
** Form generated from reading UI file 'XMLCommProtocolWidget.ui'
**
** Created: Sun Jul 11 18:53:34 2010
**      by: Qt User Interface Compiler version 4.6.2
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef XMLCOMMPROTOCOLWIDGET_H
#define XMLCOMMPROTOCOLWIDGET_H

#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>
#include <QtGui/QGridLayout>
#include <QtGui/QHeaderView>
#include <QtGui/QLabel>
#include <QtGui/QPlainTextEdit>
#include <QtGui/QPushButton>
#include <QtGui/QTextEdit>
#include <QtGui/QTreeView>
#include <QtGui/QWidget>

QT_BEGIN_NAMESPACE

class Ui_XMLCommProtocolWidget
{
public:
    QGridLayout *gridLayout;
    QLabel *fileNameLabel;
    QPushButton *selectFileButton;
    QTextEdit *xmlTextView;
    QLabel *outputDirNameLabel;
    QPushButton *selectOutputButton;
    QTreeView *xmlTreeView;
    QLabel *label;
    QPlainTextEdit *compileLog;
    QLabel *validXMLLabel;
    QPushButton *saveButton;
    QPushButton *generateButton;

    void setupUi(QWidget *XMLCommProtocolWidget) {
        if (XMLCommProtocolWidget->objectName().isEmpty())
            XMLCommProtocolWidget->setObjectName(QString::fromUtf8("XMLCommProtocolWidget"));
        XMLCommProtocolWidget->resize(846, 480);
        gridLayout = new QGridLayout(XMLCommProtocolWidget);
        gridLayout->setSpacing(12);
        gridLayout->setObjectName(QString::fromUtf8("gridLayout"));
        gridLayout->setContentsMargins(-1, 6, 6, 6);
        fileNameLabel = new QLabel(XMLCommProtocolWidget);
        fileNameLabel->setObjectName(QString::fromUtf8("fileNameLabel"));
        fileNameLabel->setMaximumSize(QSize(300, 16777215));
        fileNameLabel->setScaledContents(true);
        fileNameLabel->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

        gridLayout->addWidget(fileNameLabel, 0, 0, 1, 1);

        selectFileButton = new QPushButton(XMLCommProtocolWidget);
        selectFileButton->setObjectName(QString::fromUtf8("selectFileButton"));
        QIcon icon;
        icon.addFile(QString::fromUtf8(":/files/images/status/folder-open.svg"), QSize(), QIcon::Normal, QIcon::Off);
        selectFileButton->setIcon(icon);

        gridLayout->addWidget(selectFileButton, 0, 2, 1, 1);

        xmlTextView = new QTextEdit(XMLCommProtocolWidget);
        xmlTextView->setObjectName(QString::fromUtf8("xmlTextView"));
        xmlTextView->setReadOnly(false);

        gridLayout->addWidget(xmlTextView, 0, 3, 6, 1);

        outputDirNameLabel = new QLabel(XMLCommProtocolWidget);
        outputDirNameLabel->setObjectName(QString::fromUtf8("outputDirNameLabel"));
        outputDirNameLabel->setMaximumSize(QSize(400, 16777215));
        outputDirNameLabel->setScaledContents(true);

        gridLayout->addWidget(outputDirNameLabel, 1, 0, 1, 2);

        selectOutputButton = new QPushButton(XMLCommProtocolWidget);
        selectOutputButton->setObjectName(QString::fromUtf8("selectOutputButton"));
        selectOutputButton->setIcon(icon);

        gridLayout->addWidget(selectOutputButton, 1, 2, 1, 1);

        xmlTreeView = new QTreeView(XMLCommProtocolWidget);
        xmlTreeView->setObjectName(QString::fromUtf8("xmlTreeView"));

        gridLayout->addWidget(xmlTreeView, 2, 0, 1, 3);

        label = new QLabel(XMLCommProtocolWidget);
        label->setObjectName(QString::fromUtf8("label"));

        gridLayout->addWidget(label, 3, 0, 1, 2);

        compileLog = new QPlainTextEdit(XMLCommProtocolWidget);
        compileLog->setObjectName(QString::fromUtf8("compileLog"));

        gridLayout->addWidget(compileLog, 4, 0, 1, 3);

        validXMLLabel = new QLabel(XMLCommProtocolWidget);
        validXMLLabel->setObjectName(QString::fromUtf8("validXMLLabel"));

        gridLayout->addWidget(validXMLLabel, 5, 0, 1, 1);

        saveButton = new QPushButton(XMLCommProtocolWidget);
        saveButton->setObjectName(QString::fromUtf8("saveButton"));

        gridLayout->addWidget(saveButton, 5, 1, 1, 1);

        generateButton = new QPushButton(XMLCommProtocolWidget);
        generateButton->setObjectName(QString::fromUtf8("generateButton"));
        QIcon icon1;
        icon1.addFile(QString::fromUtf8(":/files/images/categories/applications-system.svg"), QSize(), QIcon::Normal, QIcon::Off);
        generateButton->setIcon(icon1);

        gridLayout->addWidget(generateButton, 5, 2, 1, 1);

        gridLayout->setRowStretch(2, 100);
        gridLayout->setColumnStretch(0, 1);
        gridLayout->setColumnStretch(1, 1);
        gridLayout->setColumnStretch(2, 1);
        gridLayout->setColumnStretch(3, 100);

        retranslateUi(XMLCommProtocolWidget);

        QMetaObject::connectSlotsByName(XMLCommProtocolWidget);
    } // setupUi

    void retranslateUi(QWidget *XMLCommProtocolWidget) {
        XMLCommProtocolWidget->setWindowTitle(QApplication::translate("XMLCommProtocolWidget", "Form", 0, QApplication::UnicodeUTF8));
        fileNameLabel->setText(QApplication::translate("XMLCommProtocolWidget", "Select input file", 0, QApplication::UnicodeUTF8));
        selectFileButton->setText(QApplication::translate("XMLCommProtocolWidget", "Select input file", 0, QApplication::UnicodeUTF8));
        outputDirNameLabel->setText(QApplication::translate("XMLCommProtocolWidget", "Select output directory", 0, QApplication::UnicodeUTF8));
        selectOutputButton->setText(QApplication::translate("XMLCommProtocolWidget", "Select directory", 0, QApplication::UnicodeUTF8));
        label->setText(QApplication::translate("XMLCommProtocolWidget", "Compile Output", 0, QApplication::UnicodeUTF8));
        validXMLLabel->setText(QApplication::translate("XMLCommProtocolWidget", "No file loaded", 0, QApplication::UnicodeUTF8));
        saveButton->setText(QApplication::translate("XMLCommProtocolWidget", "Save file", 0, QApplication::UnicodeUTF8));
        generateButton->setText(QApplication::translate("XMLCommProtocolWidget", "Save and generate", 0, QApplication::UnicodeUTF8));
    } // retranslateUi

};

namespace Ui
{
class XMLCommProtocolWidget: public Ui_XMLCommProtocolWidget {};
} // namespace Ui

QT_END_NAMESPACE

#endif // XMLCOMMPROTOCOLWIDGET_H
