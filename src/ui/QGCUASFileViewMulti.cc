#include "QGCUASFileViewMulti.h"
#include "ui_QGCUASFileViewMulti.h"
#include "UASInterface.h"
#include "UASManager.h"
#include "QGCUASFileView.h"

QGCUASFileViewMulti::QGCUASFileViewMulti(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::QGCUASFileViewMulti)
{
    ui->setupUi(this);
    setMinimumSize(600, 80);
    connect(UASManager::instance(), &UASManager::UASCreated, this, &QGCUASFileViewMulti::systemCreated);
    connect(UASManager::instance(), SIGNAL(activeUASSet(UASInterface*)), this, SLOT(systemSetActive(UASInterface*)));

    if (UASManager::instance()->getActiveUAS()) {
        systemCreated(UASManager::instance()->getActiveUAS());
        systemSetActive(UASManager::instance()->getActiveUAS());
    }

}

void QGCUASFileViewMulti::systemDeleted(QObject* uas)
{
    Q_ASSERT(uas);
    
    // Do not dynamic cast or de-reference QObject, since object is either in destructor or may have already
    // been destroyed.
    
    UASInterface* mav = static_cast<UASInterface*>(uas);
    QGCUASFileView* list = lists.value(mav, NULL);
    if (list)
    {
        delete list;
        lists.remove(mav);
    }
}

void QGCUASFileViewMulti::systemCreated(UASInterface* uas)
{
    Q_ASSERT(uas);

    QGCUASFileView* list = new QGCUASFileView(ui->stackedWidget, uas->getFileManager());
    lists.insert(uas, list);
    ui->stackedWidget->addWidget(list);
    // Ensure widget is deleted when system is deleted
    connect(uas, SIGNAL(destroyed(QObject*)), this, SLOT(systemDeleted(QObject*)));
}

void QGCUASFileViewMulti::systemSetActive(UASInterface* uas)
{
    QGCUASFileView* list = lists.value(uas, NULL);
    if (list) {
        ui->stackedWidget->setCurrentWidget(list);
    }
}

QGCUASFileViewMulti::~QGCUASFileViewMulti()
{
    delete ui;
}

void QGCUASFileViewMulti::changeEvent(QEvent *e)
{
    QWidget::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        ui->retranslateUi(this);
        break;
    default:
        break;
    }
}
