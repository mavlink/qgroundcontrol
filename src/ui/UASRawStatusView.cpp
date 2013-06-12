#include "UASRawStatusView.h"
#include "MAVLinkDecoder.h"
#include "UASInterface.h"
#include "UAS.h"
#include <QTimer>
#include <QScrollBar>
UASRawStatusView::UASRawStatusView(QWidget *parent) : QWidget(parent)
{
    ui.setupUi(this);
    ui.tableWidget->setColumnCount(2);
    ui.tableWidget->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    ui.tableWidget->setShowGrid(false);
    ui.tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
    QTimer *timer = new QTimer(this);
    connect(timer,SIGNAL(timeout()),this,SLOT(updateTableTimerTick()));

    // FIXME reinstate once fixed.

    timer->start(2000);
}
void UASRawStatusView::addSource(MAVLinkDecoder *decoder)
{
    connect(decoder,SIGNAL(valueChanged(int,QString,QString,double,quint64)),this,SLOT(valueChanged(int,QString,QString,double,quint64)));
    connect(decoder,SIGNAL(valueChanged(int,QString,QString,qint8,quint64)),this,SLOT(valueChanged(int,QString,QString,qint8,quint64)));
    connect(decoder,SIGNAL(valueChanged(int,QString,QString,qint16,quint64)),this,SLOT(valueChanged(int,QString,QString,qint16,quint64)));
    connect(decoder,SIGNAL(valueChanged(int,QString,QString,qint32,quint64)),this,SLOT(valueChanged(int,QString,QString,qint32,quint64)));
    connect(decoder,SIGNAL(valueChanged(int,QString,QString,qint64,quint64)),this,SLOT(valueChanged(int,QString,QString,qint64,quint64)));
    connect(decoder,SIGNAL(valueChanged(int,QString,QString,quint8,quint64)),this,SLOT(valueChanged(int,QString,QString,quint8,quint64)));
    connect(decoder,SIGNAL(valueChanged(int,QString,QString,qint16,quint64)),this,SLOT(valueChanged(int,QString,QString,qint16,quint64)));
    connect(decoder,SIGNAL(valueChanged(int,QString,QString,quint32,quint64)),this,SLOT(valueChanged(int,QString,QString,quint32,quint64)));
    connect(decoder,SIGNAL(valueChanged(int,QString,QString,quint64,quint64)),this,SLOT(valueChanged(int,QString,QString,quint64,quint64)));
}
void UASRawStatusView::valueChanged(const int uasId, const QString& name, const QString& unit, const quint8 value, const quint64 msec)
{
    valueChanged(uasId,name,unit,(double)value,msec);
}
void UASRawStatusView::valueChanged(const int uasId, const QString& name, const QString& unit, const qint8 value, const quint64 msec)
{
    valueChanged(uasId,name,unit,(double)value,msec);
}
void UASRawStatusView::valueChanged(const int uasId, const QString& name, const QString& unit, const quint16 value, const quint64 msec)
{
    valueChanged(uasId,name,unit,(double)value,msec);
}
void UASRawStatusView::valueChanged(const int uasId, const QString& name, const QString& unit, const qint16 value, const quint64 msec)
{
    valueChanged(uasId,name,unit,(double)value,msec);
}
void UASRawStatusView::valueChanged(const int uasId, const QString& name, const QString& unit, const quint32 value, const quint64 msec)
{
    valueChanged(uasId,name,unit,(double)value,msec);
}
void UASRawStatusView::valueChanged(const int uasId, const QString& name, const QString& unit, const qint32 value, const quint64 msec)
{
    valueChanged(uasId,name,unit,(double)value,msec);
}
void UASRawStatusView::valueChanged(const int uasId, const QString& name, const QString& unit, const quint64 value, const quint64 msec)
{
    valueChanged(uasId,name,unit,(double)value,msec);
}
void UASRawStatusView::valueChanged(const int uasId, const QString& name, const QString& unit, const qint64 value, const quint64 msec)
{
    valueChanged(uasId,name,unit,(double)value,msec);
}

void UASRawStatusView::valueChanged(const int uasId, const QString& name, const QString& unit, const double value, const quint64 msec)
{
    valueMap[name] = value;
    if (nameToUpdateWidgetMap.contains(name))
    {
        nameToUpdateWidgetMap[name]->setText(QString::number(value));
    }
    else
    {
        m_tableDirty = true;
    }
    return;
}
void UASRawStatusView::resizeEvent(QResizeEvent *event)
{
    m_tableDirty = true;
}

void UASRawStatusView::updateTableTimerTick()
{
    if (m_tableDirty)
    {
        m_tableDirty = false;
        int columncount = 2;
        bool good = false;
        while (!good)
        {
            ui.tableWidget->clear();
            ui.tableWidget->setRowCount(0);
            ui.tableWidget->setColumnCount(columncount);
            ui.tableWidget->horizontalHeader()->hide();
            ui.tableWidget->verticalHeader()->hide();
            int currcolumn = 0;
            int currrow = 0;
            int totalheight = 2 + ui.tableWidget->horizontalScrollBar()->height();
            bool broke = false;
            for (QMap<QString,double>::const_iterator i=valueMap.constBegin();i!=valueMap.constEnd();i++)
            {
                if (ui.tableWidget->rowCount() < currrow+1)
                {
                    ui.tableWidget->setRowCount(currrow+1);
                }
                ui.tableWidget->setItem(currrow,currcolumn,new QTableWidgetItem(i.key().split(".")[1]));
                QTableWidgetItem *item = new QTableWidgetItem(QString::number(i.value()));
                nameToUpdateWidgetMap[i.key()] = item;
                ui.tableWidget->setItem(currrow,currcolumn+1,item);
                ui.tableWidget->resizeRowToContents(currrow);
                totalheight += ui.tableWidget->rowHeight(currrow);
                currrow++;
                if ((totalheight + ui.tableWidget->rowHeight(currrow-1)) > ui.tableWidget->height())
                {
                    currcolumn+=2;
                    totalheight = 2 + ui.tableWidget->horizontalScrollBar()->height();
                    currrow = 0;
                    if (currcolumn >= columncount)
                    {
                        //We're over what we can do. Add a column and continue.
                        columncount+=2;
                        broke = true;
                        break;
                    }
                }
            }
            if (!broke)
            {
                good = true;
            }
        }
        ui.tableWidget->resizeColumnsToContents();
        //ui.tableWidget->columnCount()-2
    }
}

UASRawStatusView::~UASRawStatusView()
{
}
