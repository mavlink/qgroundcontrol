#include <QFileDialog>
#include <QTextBrowser>
#include <QMessageBox>
#include <QSettings>

#include "XMLCommProtocolWidget.h"
#include "ui_XMLCommProtocolWidget.h"
#include "MAVLinkXMLParser.h"
#include "MAVLinkSyntaxHighlighter.h"
#include "QGC.h"

#include <QDebug>
#include <iostream>

XMLCommProtocolWidget::XMLCommProtocolWidget(QWidget *parent) :
    QWidget(parent),
    m_ui(new Ui::XMLCommProtocolWidget)
{
    m_ui->setupUi(this);

    // Now set syntax highlighter
    //highlighter = new MAVLinkSyntaxHighlighter(m_ui->xmlTextView->document());

    connect(m_ui->selectFileButton, SIGNAL(clicked()), this, SLOT(selectXMLFile()));
    connect(m_ui->selectOutputButton, SIGNAL(clicked()), this, SLOT(selectOutputDirectory()));
    connect(m_ui->generateButton, SIGNAL(clicked()), this, SLOT(generate()));
    connect(m_ui->saveButton, SIGNAL(clicked()), this, SLOT(save()));
}

void XMLCommProtocolWidget::selectXMLFile()
{
    //QString fileName = QFileDialog::getOpenFileName(this, tr("Load Protocol Definition File"), ".", "*.xml");
    QSettings settings(QGC::COMPANYNAME, QGC::APPNAME);
    const QString mavlinkXML = "MAVLINK_XML_FILE";
    QString dirPath = settings.value(mavlinkXML, QCoreApplication::applicationDirPath() + "../").toString();
    QFileInfo dir(dirPath);
    QFileDialog dialog;
    dialog.setDirectory(dir.absoluteDir());
    dialog.setFileMode(QFileDialog::AnyFile);
    dialog.setFilter(tr("MAVLink XML (*.xml)"));
    dialog.setViewMode(QFileDialog::Detail);
    QStringList fileNames;
    if (dialog.exec()) {
        fileNames = dialog.selectedFiles();
    }

    if (fileNames.size() > 0) {
        m_ui->fileNameLabel->setText(fileNames.first());
        QFile file(fileNames.first());

        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            const QString instanceText(QString::fromUtf8(file.readAll()));
            setXML(instanceText);
            // Store filename for next time
            settings.setValue(mavlinkXML, QFileInfo(file).absoluteFilePath());
            settings.sync();
        } else {
            QMessageBox msgBox;
            msgBox.setText("Could not read XML file. Permission denied");
            msgBox.exec();
        }
    }
}

void XMLCommProtocolWidget::setXML(const QString& xml)
{
    m_ui->xmlTextView->setText(xml);
    QDomDocument doc;

    if (doc.setContent(xml)) {
        m_ui->validXMLLabel->setText(tr("<font color=\"green\">Valid XML file</font>"));
    } else {
        m_ui->validXMLLabel->setText(tr("<font color=\"red\">File is NOT valid XML, please fix in editor</font>"));
    }

    if (model != NULL) {
        m_ui->xmlTreeView->reset();
        //delete model;
    }
    model = new DomModel(doc, this);
    m_ui->xmlTreeView->setModel(model);
    // Expand the tree so that message names are visible
    m_ui->xmlTreeView->expandToDepth(1);
    m_ui->xmlTreeView->hideColumn(2);
    m_ui->xmlTreeView->repaint();
}

void XMLCommProtocolWidget::selectOutputDirectory()
{
    QSettings settings(QGC::COMPANYNAME, QGC::APPNAME);
    const QString mavlinkOutputDir = "MAVLINK_OUTPUT_DIR";
    QString dirPath = settings.value(mavlinkOutputDir, QCoreApplication::applicationDirPath() + "../").toString();
    QFileDialog dialog;
    dialog.setDirectory(dirPath);
    dialog.setFileMode(QFileDialog::Directory);
    dialog.setViewMode(QFileDialog::Detail);
    QStringList fileNames;
    if (dialog.exec()) {
        fileNames = dialog.selectedFiles();
    }

    if (fileNames.size() > 0) {
        m_ui->outputDirNameLabel->setText(fileNames.first());
        // Store directory for next time
        settings.setValue(mavlinkOutputDir, QFileInfo(fileNames.first()).absoluteFilePath());
        settings.sync();
        //QFile file(fileName);
    }
}

void XMLCommProtocolWidget::generate()
{
    // Check if input file is present
    if (!QFileInfo(m_ui->fileNameLabel->text().trimmed()).isFile()) {
        QMessageBox::critical(this, tr("Please select an XML input file first"), tr("You have to select an input XML file before generating C files."), QMessageBox::Ok);
        return;
    }

    // Check if output dir is selected
    if (!QFileInfo(m_ui->outputDirNameLabel->text().trimmed()).isDir()) {
        QMessageBox::critical(this, tr("Please select output directory first"), tr("You have to select an output directory before generating C files."), QMessageBox::Ok);
        return;
    }

    // First save file
    save();
    // Clean log
    m_ui->compileLog->clear();

    // Check XML validity
    if (!m_ui->xmlTextView->syntaxcheck()) return;

    MAVLinkXMLParser* parser = new MAVLinkXMLParser(m_ui->fileNameLabel->text().trimmed(), m_ui->outputDirNameLabel->text().trimmed());
    connect(parser, SIGNAL(parseState(QString)), m_ui->compileLog, SLOT(appendHtml(QString)));
    bool result = parser->generate();
    if (result) {
        QMessageBox msgBox;
        msgBox.setText(QString("The C code / headers have been generated in folder\n%1").arg(m_ui->outputDirNameLabel->text().trimmed()));
        msgBox.exec();
    } else {
        QMessageBox::critical(this, tr("C code generation failed, please see the compile log for further information"), QString("The C code / headers could not be written to folder\n%1").arg(m_ui->outputDirNameLabel->text().trimmed()), QMessageBox::Ok);
    }
    delete parser;
}

void XMLCommProtocolWidget::save()
{
    QFile file(m_ui->fileNameLabel->text().trimmed());
    setXML(m_ui->xmlTextView->document()->toPlainText().toUtf8());
    file.open(QIODevice::WriteOnly | QIODevice::Text);
    file.write(m_ui->xmlTextView->document()->toPlainText().toUtf8());
}

XMLCommProtocolWidget::~XMLCommProtocolWidget()
{
    delete model;
    delete m_ui;
}

void XMLCommProtocolWidget::changeEvent(QEvent *e)
{
    QWidget::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        m_ui->retranslateUi(this);
        break;
    default:
        break;
    }
}
