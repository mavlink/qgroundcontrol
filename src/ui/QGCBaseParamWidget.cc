#include "QGCBaseParamWidget.h"

#include <QFile>
#include <QVariant>
#include <QTextStream>

#include "QGCUASParamManagerInterface.h"
#include "UASInterface.h"
#include "QGCApplication.h"
#include "QGCFileDialog.h"

QGCBaseParamWidget::QGCBaseParamWidget(QWidget *parent) :
    QWidget(parent),
    paramMgr(NULL),
    mav(NULL),
    updatingParamNameLock("")
{
}

QGCBaseParamWidget* QGCBaseParamWidget::initWithUAS(UASInterface *uas)
{
    setUAS(uas);
    return this;
}

void QGCBaseParamWidget::setUAS(UASInterface* uas)
{
    if (uas != mav) {
        if (mav) {
            //TODO disconnect any connections as needed
            disconnectViewSignalsAndSlots();
            disconnectFromParamManager();
            clearOnboardParamDisplay();
            clearPendingParamDisplay();
        }

        mav = uas;

        if (mav) {
            connectToParamManager();
            connectViewSignalsAndSlots();
            layoutWidget();
        }
    }

}


void QGCBaseParamWidget::connectToParamManager()
{
    paramMgr = mav->getParamManager();
    //TODO route via paramManager instead?
    // Listen to updated param signals from the data model
    connect(paramMgr, SIGNAL(parameterUpdated(int, QString , QVariant )),
            this, SLOT(handleOnboardParamUpdate(int,QString,QVariant)));

    connect(paramMgr, SIGNAL(pendingParamUpdate(int , const QString&, QVariant , bool )),
            this, SLOT(handlePendingParamUpdate(int , const QString& ,  QVariant, bool )));

    // Listen for param list reload finished
    connect(paramMgr, SIGNAL(parameterListUpToDate()),
            this, SLOT(handleOnboardParameterListUpToDate()));
    if (paramMgr->parametersReady()) {
        handleOnboardParameterListUpToDate();
    }

    // Listen to communications status messages so we can display them
    connect(paramMgr, SIGNAL(parameterStatusMsgUpdated(QString,int)),
            this, SLOT(handleParamStatusMsgUpdate(QString , int )));
}


void QGCBaseParamWidget::disconnectFromParamManager()
{
    disconnect(paramMgr, SIGNAL(parameterUpdated(int, QString , QVariant )),
            this, SLOT(handleOnboardParamUpdate(int,QString,QVariant)));

    disconnect(paramMgr, SIGNAL(pendingParamUpdate(int , const QString&, QVariant , bool )),
            this, SLOT(handlePendingParamUpdate(int , const QString& ,  QVariant, bool )));

    disconnect(paramMgr, SIGNAL(parameterListUpToDate()),
            this, SLOT(handleOnboardParameterListUpToDate()));

    // Listen to communications status messages so we can display them
    disconnect(paramMgr, SIGNAL(parameterStatusMsgUpdated(QString,int)),
            this, SLOT(handleParamStatusMsgUpdate(QString , int )));

    paramMgr = NULL;
}



void QGCBaseParamWidget::requestOnboardParamsUpdate()
{
    paramMgr->requestParameterList();
}

void QGCBaseParamWidget::requestOnboardParamUpdate(QString parameterName)
{
    paramMgr->requestParameterUpdate(paramMgr->getDefaultComponentId(), parameterName);
}


void QGCBaseParamWidget::saveParametersToFile()
{
    if (!mav)
        return;
    QString defaultSuffix("txt");
    QString fileName = QGCFileDialog::getSaveFileName(
        this,
        tr("Save Parameters"),
        qgcApp()->savedParameterFilesLocation(),
        tr("Parameter File (*.txt)"),
        0, 0,
        &defaultSuffix,
        true);
    if (!fileName.isEmpty()) {
        QFile file(fileName);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            return;
        }
        QTextStream outstream(&file);
        paramMgr->writeOnboardParamsToStream(outstream,mav->getUASName());
        file.close();
    }
}


void QGCBaseParamWidget::loadParametersFromFile()
{
    if (!mav)
        return;

    QString fileName = QGCFileDialog::getOpenFileName(this, tr("Load File"), qgcApp()->savedParameterFilesLocation(), tr("Parameter file (*.txt)"));
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return;

    QTextStream in(&file);
    paramMgr->readPendingParamsFromStream(in);
    file.close();
}


