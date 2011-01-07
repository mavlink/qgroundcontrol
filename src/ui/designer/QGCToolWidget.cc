#include "QGCToolWidget.h"
#include "ui_QGCToolWidget.h"

#include <QMenu>
#include <QList>
#include <QInputDialog>
#include <QDockWidget>
#include <QContextMenuEvent>
#include "QGCParamSlider.h"
#include "QGCActionButton.h"
#include "UASManager.h"

QGCToolWidget::QGCToolWidget(QWidget *parent) :
        QWidget(parent),
        toolLayout(new QVBoxLayout(this)),
        mav(NULL),
        ui(new Ui::QGCToolWidget)
{
    ui->setupUi(this);
    createActions();
    toolLayout->setAlignment(Qt::AlignTop);

    QList<UASInterface*> systems = UASManager::instance()->getUASList();
    foreach (UASInterface* uas, systems)
    {
        UAS* newMav = dynamic_cast<UAS*>(uas);
        if (newMav)
        {
            addUAS(uas);
        }
    }
    connect(UASManager::instance(), SIGNAL(UASCreated(UASInterface*)), this, SLOT(addUAS(UASInterface*)));
}

QGCToolWidget::~QGCToolWidget()
{
    delete ui;
}

void QGCToolWidget::addUAS(UASInterface* uas)
{
    UAS* newMav = dynamic_cast<UAS*>(uas);
    if (newMav)
    {
        // FIXME Convert to list
        if (mav == NULL) mav = newMav;
    }
}

void QGCToolWidget::contextMenuEvent (QContextMenuEvent* event)
{
    QMenu menu(this);
    //menu.addAction(addParamAction);
    menu.addAction(addButtonAction);
    menu.addAction(setTitleAction);
    menu.exec(event->globalPos());
}

void QGCToolWidget::createActions()
{
    addParamAction = new QAction(tr("New &Parameter Slider"), this);
    addParamAction->setStatusTip(tr("Add a parameter setting slider widget to the tool"));
    connect(addParamAction, SIGNAL(triggered()), this, SLOT(addParam()));

    addButtonAction = new QAction(tr("New MAV &Action Button"), this);
    addButtonAction->setStatusTip(tr("Add a new action button to the tool"));
    connect(addButtonAction, SIGNAL(triggered()), this, SLOT(addAction()));

    setTitleAction = new QAction(tr("Set Widget Title"), this);
    setTitleAction->setStatusTip(tr("Set the title caption of this tool widget"));
    connect(setTitleAction, SIGNAL(triggered()), this, SLOT(setTitle()));
}

void QGCToolWidget::addParam()
{
    QGCParamSlider* slider = new QGCParamSlider(this);
    toolLayout->addWidget(slider);
    slider->startEditMode();
}

void QGCToolWidget::addAction()
{
    QGCActionButton* button = new QGCActionButton(this);
    toolLayout->addWidget(button);
    button->startEditMode();
}

void QGCToolWidget::setTitle()
{
    QDockWidget* parent = dynamic_cast<QDockWidget*>(this->parentWidget());
    if (parent)
    {
    bool ok;
        QString text = QInputDialog::getText(this, tr("QInputDialog::getText()"),
                                             tr("Widget title:"), QLineEdit::Normal,
                                             parent->windowTitle(), &ok);
        if (ok && !text.isEmpty())
        parent->setWindowTitle(text);
    }
}
