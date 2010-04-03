#include <QFileDialog>
#include <QTextBrowser>
#include <QMessageBox>

#include "XMLCommProtocolWidget.h"
#include "ui_XMLCommProtocolWidget.h"
#include "MAVLinkXMLParser.h"

#include <QDebug>
#include <iostream>

XMLCommProtocolWidget::XMLCommProtocolWidget(QWidget *parent) :
        QWidget(parent),
        m_ui(new Ui::XMLCommProtocolWidget)
{
    m_ui->setupUi(this);

    connect(m_ui->selectFileButton, SIGNAL(clicked()), this, SLOT(selectXMLFile()));
    connect(m_ui->selectOutputButton, SIGNAL(clicked()), this, SLOT(selectOutputDirectory()));
    connect(m_ui->generateButton, SIGNAL(clicked()), this, SLOT(generate()));
    connect(m_ui->saveButton, SIGNAL(clicked()), this, SLOT(save()));
}

void XMLCommProtocolWidget::selectXMLFile()
{
    qDebug() << "OPENING XML FILE";

    //QString fileName = QFileDialog::getOpenFileName(this, tr("Load Protocol Definition File"), ".", "*.xml");
    QFileDialog dialog;
    dialog.setFileMode(QFileDialog::AnyFile);
    dialog.setFilter(tr("MAVLink XML (*.xml)"));
    dialog.setViewMode(QFileDialog::Detail);
    QStringList fileNames;
    if (dialog.exec())
    {
        fileNames = dialog.selectedFiles();
    }

    if (fileNames.size() > 0)
    {
        m_ui->fileNameLabel->setText(fileNames.first());
        QFile file(fileNames.first());
        if (file.open(QIODevice::ReadOnly | QIODevice::Text))
        {
            const QString instanceText(QString::fromUtf8(file.readAll()));
            setXML(instanceText);
        }
        else
        {
            QMessageBox msgBox;
            msgBox.setText("Could not write XML file. Permission denied");
            msgBox.exec();
        }
    }
}

void XMLCommProtocolWidget::setXML(const QString& xml)
{
    m_ui->xmlTextView->setText(xml);
    QDomDocument doc;

    if (doc.setContent(xml))
    {
        m_ui->validXMLLabel->setText(tr("Valid XML file"));
    }
    else
    {
        m_ui->validXMLLabel->setText(tr("File is NOT valid XML, please fix in editor"));
    }

    if (model != NULL)
    {
        m_ui->xmlTreeView->reset();
        //delete model;
    }
    model = new DomModel(doc, this);
    m_ui->xmlTreeView->setModel(model);
    m_ui->xmlTreeView->repaint();
}

void XMLCommProtocolWidget::selectOutputDirectory()
{
    QFileDialog dialog;
    dialog.setFileMode(QFileDialog::Directory);
    dialog.setFilter(tr("MAVLink XML (*.xml)"));
    dialog.setViewMode(QFileDialog::Detail);
    QStringList fileNames;
    if (dialog.exec())
    {
        fileNames = dialog.selectedFiles();
    }

    if (fileNames.size() > 0)
    {
        m_ui->outputDirNameLabel->setText(fileNames.first());
        //QFile file(fileName);
    }
}

void XMLCommProtocolWidget::generate()
{
    // First save file
    save();
    MAVLinkXMLParser parser(m_ui->fileNameLabel->text().trimmed(), m_ui->outputDirNameLabel->text().trimmed());
    bool result = parser.generate();
    if (result)
    {
        QMessageBox msgBox;
        msgBox.setText(QString("The C code / headers have been generated in folder\n%1").arg(m_ui->outputDirNameLabel->text().trimmed()));
        msgBox.exec();
    }
    else
    {
        QMessageBox::critical(this, tr("Could not write files"), QString("The C code / headers could not be written to folder\n%1").arg(m_ui->outputDirNameLabel->text().trimmed()), QMessageBox::Ok);
    }
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
